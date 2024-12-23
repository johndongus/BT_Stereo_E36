import os
import sys
import dbus
import dbus.service
import dbus.mainloop.glib
from gi.repository import GLib
from subprocess import run, CalledProcessError, PIPE
import time
import serial
import serial.tools.list_ports
from enum import Enum
import logging

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')


class PlaybackStatus(Enum):
    PLAYING = "playing"
    PAUSED = "paused"
    STOPPED = "stopped"


class BluetoothAgent(dbus.service.Object):
    AGENT_INTERFACE = "org.bluez.Agent1"

    def __init__(self, bus, path, pairing_callback=None):
        super().__init__(bus, path)
        self.pairing_callback = pairing_callback

    @dbus.service.method(AGENT_INTERFACE, in_signature="os", out_signature="")
    def AuthorizeService(self, device, uuid):
        return

    @dbus.service.method(AGENT_INTERFACE, in_signature="", out_signature="")
    def Release(self):
        return

    @dbus.service.method(AGENT_INTERFACE, in_signature="", out_signature="")
    def Cancel(self):
        return

    @dbus.service.method(AGENT_INTERFACE, in_signature="o", out_signature="")
    def RequestAuthorization(self, device):
        return

    @dbus.service.method(AGENT_INTERFACE, in_signature="o", out_signature="u")
    def RequestPasskey(self, device):
        return dbus.UInt32(1234)

    @dbus.service.method(AGENT_INTERFACE, in_signature="o", out_signature="s")
    def RequestPinCode(self, device):
        return "1234"

    @dbus.service.method(AGENT_INTERFACE, in_signature="ouq", out_signature="")
    def DisplayPasskey(self, device, passkey, entered):
        return

    @dbus.service.method(AGENT_INTERFACE, in_signature="ou", out_signature="")
    def RequestConfirmation(self, device, passkey):
        return


class BluetoothManager:
    AGENT_PATH = "/test/agent"

    def __init__(self, capability='KeyboardDisplay', baudrate=9600, reset_delay=5):
        self.capability = capability
        self.past_devices = []
        self.connected_device = None
        self.media_player_path = None
        self.baudrate = baudrate
        self.serial_conn = None
        self.current_volume = 100  # Volume level (0-100)
        self.is_playing = False
        self.is_bt_connected = False
        self.current_song = "Unknown Artist - Unknown Title"
        self.current_pos = 0
        self.duration = None  # Duration in microseconds
        self.command_cooldown = False  # To prevent rapid command processing
        self.reset_delay = reset_delay  # Delay before attempting to reset/reconnect

    def add_to_past_devices(self, device):
        if device not in self.past_devices:
            self.past_devices.append(device)

    def setup_bluetooth(self):
        try:
            run(["bluetoothctl", "power", "on"], check=True)
            run(["bluetoothctl", "pairable", "on"], check=True)
            run(["bluetoothctl", "discoverable", "on"], check=True)
            logging.info("Bluetooth powered on, pairable, and discoverable.")

            dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
            bus = dbus.SystemBus()
            manager = dbus.Interface(bus.get_object("org.bluez", "/org/bluez"), "org.bluez.AgentManager1")
            agent = BluetoothAgent(bus, self.AGENT_PATH)
            manager.RegisterAgent(self.AGENT_PATH, self.capability)
            manager.RequestDefaultAgent(self.AGENT_PATH)
            logging.info("Bluetooth agent registered and set as default.")
        except CalledProcessError as e:
            logging.error(f"Bluetooth setup failed: {e}")
            self.schedule_reset()
        except Exception as e:
            logging.error(f"Unexpected error during Bluetooth setup: {e}")
            self.schedule_reset()

    def trust_device(self, device_address):
        try:
            run(["bluetoothctl", "trust", device_address], check=True)
            logging.info(f"Trusted device: {device_address}")
        except CalledProcessError as e:
            logging.error(f"Failed to trust device {device_address}: {e}")

    def setup_serial(self):
        ports = serial.tools.list_ports.comports()
        if not ports:
            logging.error("No serial ports found. Please connect the Arduino.")
            return False
        self.serial_port = ports[0].device
        try:
            self.serial_conn = serial.Serial(self.serial_port, self.baudrate, timeout=1)
            logging.info(f"Connected to serial port {self.serial_port} at {self.baudrate} baud.")
            #self.send_to_serial("RESET")  # Send RESET command upon successful connection
            logging.info("Sent RESET command to Arduino.")
            return True
        except serial.SerialException as e:
            logging.error(f"Failed to connect to serial port {self.serial_port}: {e}")
            return False

    def send_to_serial(self, message):
        if self.serial_conn and self.serial_conn.is_open:
            try:
                self.serial_conn.write((message + "\n").encode('utf-8'))
                logging.debug(f"Sent to Arduino: {message}")
            except serial.SerialException as e:
                logging.error(f"Failed to send to Arduino: {e}")
                self.handle_serial_disconnect()

    def list_managed_objects(self):
        try:
            bus = dbus.SystemBus()
            manager = dbus.Interface(bus.get_object("org.bluez", "/"), "org.freedesktop.DBus.ObjectManager")
            objects = manager.GetManagedObjects()
            for path, interfaces in objects.items():
                logging.debug(f"Object Path: {path}")
                for interface in interfaces:
                    logging.debug(f"  Interface: {interface}")
        except Exception as e:
            logging.error(f"Error listing managed objects: {e}")



    def poll_call_info(self):
        logging.debug("poll_call_info: Starting call info polling.")
        try:
            bus = dbus.SystemBus()
            ofono_manager = dbus.Interface(bus.get_object('org.ofono', '/'), 'org.ofono.Manager')
            modems = ofono_manager.GetModems()

            if not modems:
                logging.warning("poll_call_info: No modems detected.")
                return True

            for modem_path, properties in modems:
                self.connected_device_name = properties.get('Name', 'Unknown Device')  # Store the device name
                device_name = properties.get('Name', 'Unknown Device')
                logging.debug(f"poll_call_info: Processing modem: {modem_path}, Device Name: {device_name}")

                if 'org.ofono.VoiceCallManager' in properties.get('Interfaces', []):
                    logging.debug(f"poll_call_info: Found VoiceCallManager for modem {modem_path}")
                    voice_call_manager = dbus.Interface(
                        bus.get_object('org.ofono', modem_path),
                        'org.ofono.VoiceCallManager'
                    )

                    try:
                        calls = voice_call_manager.GetCalls()
                        logging.debug(f"poll_call_info: Calls retrieved: {calls}")
                    except dbus.exceptions.DBusException as e:
                        logging.error(f"poll_call_info: Error retrieving calls: {e}")
                        continue

                    if not calls:
                        logging.info("poll_call_info: No active calls.")
                        return True

                    for call_path, call_props in calls:
                        state = call_props.get('State', 'unknown')
                        number = call_props.get('LineIdentification', 'Unknown Number')
                        contact_name = call_props.get('Name', '').strip() or device_name
                        for key, value in call_props.items():
                            logging.debug(f"poll_call_info: {key} = {value}")
                        logging.debug(f"poll_call_info: Call state: {state}, Number: {number}, Name: {contact_name}")

                        if state in ['incoming', 'active']:
                            call_info = f"{contact_name} ({device_name}) - {number}"
                            self.send_to_serial("SONG:" + call_info)
                            logging.info(f"poll_call_info: Call info sent: {call_info}")
        except dbus.exceptions.DBusException as e:
            logging.error(f"poll_call_info: Error polling call info: {e}")
        logging.debug("poll_call_info: Completed call info polling.")
        return True






    def poll_media_info(self):
        try:
            bus = dbus.SystemBus()
            manager = dbus.Interface(bus.get_object("org.bluez", "/"), "org.freedesktop.DBus.ObjectManager")
            objects = manager.GetManagedObjects()
            for path, interfaces in objects.items():
                if 'org.bluez.MediaPlayer1' in interfaces:
                    props = interfaces['org.bluez.MediaPlayer1']
                    track = props.get('Track', {})
                    logging.debug(f"Track properties: {track}")  # Comprehensive logging
                    artist = track.get('Artist', ["Unknown Artist"])
                    title = track.get('Title', "Unknown Title")
                    cs = f"{artist} - {title}"
                    if cs != self.current_song:
                        self.current_song = cs
                        self.send_to_serial("SONG:" + self.current_song)
                        logging.info(f"Updated song: {self.current_song}")

                    status = props.get('Status', 'stopped').lower()
                    playing = (status == "playing")
                    if playing != self.is_playing:
                        self.is_playing = playing
                        st = "PLAY" if playing else "PAUSE" if status == "paused" else "STOP"
                        self.send_to_serial("STATE:" + st)
                        logging.info(f"Updated state: {st}")

                    # Retrieve Duration from Track.Duration if available
                    if 'Duration' in track:
                        self.duration = track['Duration']  # Duration in microseconds
                        logging.info(f"Track duration set to: {self.duration} microseconds")
                        # Optionally, send duration to Arduino if needed
                        # self.send_to_serial("DUR:" + str(int(self.duration)))
                    else:
                        logging.warning("Track Duration not available. Cannot calculate seek percentage.")
                        self.duration = None

                    # Set media_player_path if not already set
                    if not self.media_player_path:
                        self.media_player_path = path
                        logging.info(f"Media player path set to: {self.media_player_path}")

                if 'org.bluez.Device1' in interfaces:
                    props = interfaces['org.bluez.Device1']
                    connected = props.get('Connected', False)
                    if connected != self.is_bt_connected:
                        self.is_bt_connected = connected
                        btstate = "ON" if connected else "OFF"
                        self.send_to_serial("BT:" + btstate)
                        logging.info(f"Updated BT connection state: {btstate}")
        except Exception as e:
            logging.error(f"Error polling media info: {e}")
            self.schedule_reset()
        return True  # Keep the timeout active

    def start_media_monitor(self):
        def on_properties_changed(interface, changed, invalidated, path):
            if interface not in ["org.bluez.MediaPlayer1", "org.bluez.Device1"]:
                logging.debug(f"Ignored properties changed from interface: {interface}")
                return

            logging.debug(f"Properties changed: {interface}, {changed}, {invalidated}, {path}")
            if interface == "org.bluez.Device1" and "CallMetadata" in changed:
                call_metadata = changed["CallMetadata"]
                contact_name = call_metadata.get("Name", "Unknown Contact")
                phone_number = call_metadata.get("Number", "Unknown Number")
                call_info = f"{contact_name} - {phone_number}"
                self.send_to_serial("SONG:" + call_info)
                logging.info(f"Call info updated from signal: {call_info}")
            if interface == "org.bluez.MediaPlayer1":
                track = changed.get("Track", {})
                if track:
                    artist = track.get("Artist", ["Unknown Artist"])[0]
                    title = track.get("Title", "Unknown Title")
                    cs = f"{artist} - {title}"
                    if cs != self.current_song:
                        self.current_song = cs
                        self.send_to_serial("SONG:" + self.current_song)
                        logging.info(f"Updated song from signal: {self.current_song}")

                status = changed.get("Status", 'stopped').lower()
                playing = (status == "playing")
                if playing != self.is_playing:
                    self.is_playing = playing
                    st = "PLAY" if playing else "PAUSE" if status == "paused" else "STOP"
                    self.send_to_serial("STATE:" + st)
                    logging.info(f"Updated state from signal: {st}")

                # Retrieve Duration from Track.Duration if available
                if 'Duration' in track and self.duration is None:
                    self.duration = track['Duration']  # Duration in microseconds
                    logging.info(f"Track duration set to: {self.duration} microseconds")
                    # Optionally, send duration to Arduino if needed
                    # self.send_to_serial("DUR:" + str(int(self.duration)))
                elif 'Position' in changed and self.duration:
                    position = changed['Position']
                    logging.debug(f"Position updated: {position} microseconds")
                    self.send_seek_percentage(position)
                


                # Set media_player_path if not already set
                if not self.media_player_path:
                    self.media_player_path = path
                    logging.info(f"Media player path set to: {self.media_player_path}")

            elif interface == "org.bluez.Device1":
                connected = changed.get("Connected", False)
                btstate = "ON" if connected else "OFF"
                self.is_bt_connected = connected
                self.send_to_serial("BT:" + btstate)
                logging.info(f"Updated BT connection state: {btstate}")

        bus = dbus.SystemBus()
        bus.add_signal_receiver(
            on_properties_changed,
            signal_name="PropertiesChanged",
            dbus_interface="org.freedesktop.DBus.Properties",
            path_keyword="path",
        )
        logging.info("Started monitoring Bluetooth properties.")
    def send_device_name(self):
        if self.connected_device_name:
            self.send_to_serial("DEVICE_NAME:" + self.connected_device_name)
            logging.info(f"Sent device name: {self.connected_device_name}")
        else:
            logging.warning("No connected device name to send.")
    def control_playback(self, command):
        if not self.media_player_path:
            logging.error("No media player available.")
            return

        bus = dbus.SystemBus()
        player = bus.get_object("org.bluez", self.media_player_path)
        media_player = dbus.Interface(player, "org.bluez.MediaPlayer1")

        try:
            if command == "play":
                media_player.Play()
                logging.info("Executed play command.")
            elif command == "pause":
                media_player.Pause()
                logging.info("Executed pause command.")
                # Add a slight delay to prevent fighting the screen
                if not self.command_cooldown:
                    self.command_cooldown = True
                    GLib.timeout_add(200, self.reset_command_cooldown)
            elif command == "next":
                media_player.Next()
                logging.info("Executed next command.")
            elif command == "previous":
                media_player.Previous()
                logging.info("Executed previous command.")
            elif command == "ff":
                media_player.FastForward()
                logging.info("Executed fast-forward command.")
            elif command == "rw":
                media_player.Rewind()
                logging.info("Executed rewind command.")
            elif data == "DEVICE_NAME":
                self.send_device_name()
            else:
                logging.error(f"Unknown command: {command}")
        except dbus.exceptions.DBusException as e:
            logging.error(f"Error executing {command} command: {e}")
            self.schedule_reset()

    def reset_command_cooldown(self):
        self.command_cooldown = False
        return False  # Remove the timeout

    def handle_serial_input(self, data):
        data = data.strip()
        logging.debug(f"Handling serial input: {data}")
        if data == "PLAY":
            self.control_playback("play")
        elif data == "PAUSE":
            self.control_playback("pause")
        elif data == "NEXT":
            self.control_playback("next")
        elif data == "PREV":
            self.control_playback("previous")
        elif data == "FF":
            self.control_playback("ff")
        elif data == "RW":
            self.control_playback("rw")
        elif data == "RESTART_SERVICE":
            os.system("systemctl restart bt-pulseaudio.service")
        elif data.startswith("VOL+") or data.startswith("VOL-"):
            try:
                volume_level = int(data[4:])
                if 0 <= volume_level <= 100:
                    self.set_volume(volume_level)
                    logging.debug(f"Volume level parsed: {volume_level}")
                else:
                    logging.error("Volume level out of range (0-100).")
            except ValueError:
                logging.error("Invalid volume command received.")
        elif data.startswith("RESET"):
            logging.info("Received RESET confirmation from Arduino.")
        else:
            logging.warning(f"Unrecognized command received: {data}")

    def read_serial_callback(self, source, condition):
        try:
            if self.serial_conn.in_waiting > 0:
                data = self.serial_conn.readline().decode('utf-8').strip()
                if data:
                    logging.debug(f"Received from Arduino: {data}")
                    self.handle_serial_input(data)
        except serial.SerialException as e:
            logging.error(f"SerialException: {e}")
            self.handle_serial_disconnect()
        except Exception as e:
            logging.error(f"Error reading from serial: {e}")
            self.handle_serial_disconnect()
        return True  # Keep the callback active

    def send_seek_percentage(self, position):
        """
        Calculate and send the seek percentage as POS:<int>.
        """
        if self.duration and self.duration > 0:
            seek_percentage = int((position / self.duration) * 100)
            seek_percentage = max(0, min(seek_percentage, 100))  # Clamp between 0 and 100
            seek_percentage_str = f"{seek_percentage}"
            self.send_to_serial("POS:" + seek_percentage_str)
            logging.info(f"Seek percentage: {seek_percentage_str}%")
        else:
            logging.warning("Duration not set. Cannot calculate seek percentage.")

    def set_volume(self, level):
 
        if 0 <= level <= 100:
            scaled_volume = level * 2  # Maps 0-100 to 0-100
            scaled_volume_str = f"{scaled_volume}"  # Format as percentage string

            try:
                # Use amixer to set the volume
                #run(["amixer", "set", "PCM", scaled_volume_str], check=True, stdout=PIPE)
                #pamixer
                #run(["pamixer", "--set-volume", scaled_volume_str], check=True, stdout=PIPE)
                #pactl set-sink-volume @DEFAULT_SINK@ 150%
                run(["pactl", "set-sink-volume", "@DEFAULT_SINK@", scaled_volume_str + "%"], check=True, stdout=PIPE)
                # Send the original volume level to the serial device
                self.send_to_serial("VOL:" + str(level))
                # Log the volume change
                logging.info(f"Volume set to {level}% (pamixer volume: {scaled_volume_str})")
            except CalledProcessError as e:
                logging.error(f"Failed to set volume: {e}")
                self.schedule_reset()
        else:
            logging.error("Volume level out of range (0-100).")

    def handle_serial_disconnect(self):
        """
        Handle serial disconnection by closing the connection and scheduling a program reset.
        """
        if self.serial_conn and self.serial_conn.is_open:
            try:
                self.serial_conn.close()
                logging.info("Serial connection closed due to disconnection.")
            except Exception as e:
                logging.error(f"Error closing serial connection: {e}")
        else:
            logging.debug("Serial connection already closed.")

        self.serial_conn = None
        self.schedule_reset()

    def schedule_reset(self):
        """
        Schedule a program reset after a short delay to allow logs to flush.
        """
        logging.info(f"Scheduling program reset in {self.reset_delay} seconds...")
        GLib.timeout_add_seconds(self.reset_delay, self.reset_program)

    def reset_program(self):
        """
        Reset the Python program by re-executing the script.
        """
        logging.info("Resetting the program...")
        try:
            # Flush all logs before resetting
            for handler in logging.root.handlers[:]:
                handler.flush()
        except Exception as e:
            logging.error(f"Error flushing logs: {e}")

        # Restart the script
        try:
            os.execv(sys.executable, [sys.executable] + sys.argv)
        except Exception as e:
            logging.critical(f"Failed to reset the program: {e}")
            sys.exit(1)  # Exit if unable to reset

    def start_seek_monitor(self):
        """
        Optionally, implement a periodic check for the current position to send seek percentage.
        """
        def callback():
            if self.media_player_path and self.duration:
                try:
                    bus = dbus.SystemBus()
                    player = bus.get_object("org.bluez", self.media_player_path)
                    media_player = dbus.Interface(player, "org.bluez.MediaPlayer1")
                    position = media_player.Get("org.bluez.MediaPlayer1", "Position")
                    self.send_seek_percentage(position)
                except dbus.exceptions.DBusException as e:
                    logging.error(f"Error retrieving position for seek percentage: {e}")
                    self.schedule_reset()
            return True  # Continue calling

        # Poll every 1 second
        GLib.timeout_add_seconds(1, callback)

    def start(self):
        while True:
            self.setup_bluetooth()
            if not self.setup_serial():
                logging.error("Serial setup failed. Retrying in 5 seconds...")
                time.sleep(self.reset_delay)
                continue  # Retry setup_serial

            if self.connected_device:
                self.trust_device(self.connected_device)
            self.start_media_monitor()

            # List all managed objects for debugging
            self.list_managed_objects()

            loop = GLib.MainLoop()

            # Poll media info every 10 seconds
            GLib.timeout_add_seconds(2, self.poll_media_info)

            GLib.timeout_add_seconds(5, self.poll_call_info)  # Poll for call information every 10 seconds

            # Optionally, start a seek monitor
            # self.start_seek_monitor()

            # Add an IO watch for serial data
            try:
                fd = self.serial_conn.fileno()
                GLib.io_add_watch(fd, GLib.IO_IN, self.read_serial_callback)
                logging.info("Serial IO watch added.")
            except Exception as e:
                logging.error(f"Failed to add IO watch for serial: {e}")
                self.schedule_reset()

            logging.info("Bluetooth manager started and running.")
            try:
                loop.run()
            except Exception as e:
                logging.error(f"Main loop encountered an error: {e}")
                self.schedule_reset()


if __name__ == "__main__":
    #// run pulseaudio --start
    os.system("pulseaudio --start")
    bt_manager = BluetoothManager()
    bt_manager.start()
