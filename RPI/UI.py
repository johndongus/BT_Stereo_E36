import time
import board
import busio
import digitalio
from SSD1306 import SSD1306_SPI
import logging
import threading
from gpiozero import RotaryEncoder, Button, PWMLED
import random

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

class UIManager:
    def __init__(self):

        self.SCREEN_WIDTH = 128
        self.SCREEN_HEIGHT = 64
        self.DC_PIN = board.D25
        self.RESET_PIN = board.D24
        self.CS_PIN = board.D8
        self.last_rotation_time_menu = 0
        self.rotation_delay_menu = 200  # in milliseconds
        self.CLK_PIN = 17
        self.DT_PIN = 27
        self.SW_PIN = 22
        self.LED_PIN = 12

        self.should_blink = False
        self.blink_start_time = 0  # Timestamp to track blink start
        self.blink_duration = 400  # Total blink duration in milliseconds (0.4 seconds)

        self.spi = board.SPI()
        self.dc = digitalio.DigitalInOut(self.DC_PIN)
        self.reset = digitalio.DigitalInOut(self.RESET_PIN)
        self.cs = digitalio.DigitalInOut(self.CS_PIN)
        self.oled = SSD1306_SPI(
            self.SCREEN_WIDTH, self.SCREEN_HEIGHT, self.spi, self.dc, self.reset, self.cs
        )
        self.oled.rotate(3)
        self.oled.fill(0)
        self.oled.show()

        # Brightness / Contrast
        self.brightness = 128
        self.oled.contrast(self.brightness)
        self.last_volume_change_time = 0.0  # Changed to float
        self.volume_change_delay = 0.0  # Increased delay to 50 milliseconds

        # BT MANAGER
        self.bt_manager = None

        self.volume = 50

        self.holdThreshold = 0.5  # Time in seconds to consider a hold
        self.doubleClickThreshold = 0.3  # Time in seconds to detect double click

        # Initialize PWMLED
        self.led_pwm = PWMLED(self.LED_PIN)
        self.volume = 70  
        self.led_pwm.value = self.volume / 100 

        # Initialize RotaryEncoder
        self.encoder = RotaryEncoder(self.CLK_PIN, self.DT_PIN)
        self.encoder.when_rotated_clockwise = self.on_rotated_clockwise
        self.encoder.when_rotated_counter_clockwise = self.on_rotated_counter_clockwise

        # Initialize Button for Clicks
        self.button = Button(self.SW_PIN, hold_time=self.holdThreshold)
        self.button.when_pressed = self.on_button_pressed
        self.button.when_released = self.on_button_released
        self.button.when_held = self.on_button_held
        self.buttonPressed = False
        self.clickCount = 0
        self.last_click_time = 0

        self.highlight_current = True

        self.current_mode = "normal"  # "normal", "menu", "brightness_adjustment"
        self.displayOn = True

        # Menu Data
        self.menu_stack = []
        self.current_menu = None
        self.menu_index = 0

        # Define Menus
        self.main_menu_entries = [
            ("Display", self.opendisplay_menu),
            ("Sub2", self.dummy_submenu_action),
            ("Controls", self.open_controls_menu),  # Added Controls option
        ]
        self.display_menu_entries = [
            ("Brightness", self.enter_brightness_adjustment),
        ]
        self.controls_menu_entries = [  # Defined Controls submenu
            ("Action", self.controls_action),
        ]

        # Song/Artist/Scrolling
        self.songName = ""
        self.currentPosition = 0
        self.isPlaying = True

        # Layout
        self.PROGRESS_BAR_Y = 0
        self.SCROLL_SONG_Y = 3
        self.SCROLL_ARTIST_Y = 24
        self.PLAY_PAUSE_Y = 36
        self.VOLUME_BAR_Y = 50
        self.VOLUME_BAR_HEIGHT = 5
        self.VOLUME_BAR_LEFT = 18
        self.VOLUME_BAR_RIGHT_PAD = 24

        # Scrolling
        self.songScrollOffset = 0
        self.songLastScrollUpdate = 0
        self.songScrollInterval = 100
        self.songScrollStep = 2
        self.songTextWidth = 0
        self.songScrollActive = False
        self.artistScrollOffset = 0
        self.artistLastScrollUpdate = 0
        self.artistScrollInterval = 100
        self.artistScrollStep = 2
        self.artistTextWidth = 0
        self.artistScrollActive = False

        # Fast-Forward / Rewind Animation
        self.fastForwardAnimationActive = False
        self.rewindAnimationActive = False
        self.animationStartTime = 0
        self.animationDuration = 600
        self.rotation_count = 0
        self.rotation_threshold = 3  # Number of rotations required to trigger skip

        self.volume_lock = threading.Lock()




    def on_rotated_clockwise(self):
        with self.volume_lock:
            current_time = self.current_millis()
            if self.current_mode == "menu":
                if current_time - self.last_rotation_time_menu < self.rotation_delay_menu:
                    return  # Ignore rotation to reduce sensitivity
                self.last_rotation_time_menu = current_time

            if self.current_mode == "brightness_adjustment":
                # Adjust brightness
                self.brightness = min(255, self.brightness + 5)
                self.display_brightness()
            elif self.current_mode == "menu":
                # Scroll menu
                self.menu_index = (self.menu_index + 1) % len(self.current_menu)
                self.display_menu()
            else:
                # Adjust volume
                self.volume = min(100, self.volume + 1)
                self.led_pwm.value = self.volume / 100
                self.display_ui(self.current_millis())

            logging.debug(f"UIManager: Rotated clockwise. Mode: {self.current_mode}, Brightness: {self.brightness}, Volume: {self.volume}, Menu Index: {self.menu_index}.")


    def on_rotated_counter_clockwise(self):
        with self.volume_lock:
            current_time = self.current_millis()
            if self.current_mode == "menu":
                if current_time - self.last_rotation_time_menu < self.rotation_delay_menu:
                    return  # Ignore rotation to reduce sensitivity
                self.last_rotation_time_menu = current_time

            if self.current_mode == "brightness_adjustment":
                # Adjust brightness
                self.brightness = max(0, self.brightness - 5)
                self.display_brightness()
            elif self.current_mode == "menu":
                # Scroll menu
                self.menu_index = (self.menu_index - 1) % len(self.current_menu)
                self.display_menu()
            else:
                # Adjust volume
                self.volume = max(0, self.volume - 1)
                self.led_pwm.value = self.volume / 100
                self.display_ui(self.current_millis())

            logging.debug(f"UIManager: Rotated counter-clockwise. Mode: {self.current_mode}, Brightness: {self.brightness}, Volume: {self.volume}, Menu Index: {self.menu_index}.")

    # Callback for button press
    def on_button_pressed(self):
        self.buttonPressed = True
        self.buttonPressTime = time.monotonic()
        logging.debug("UIManager: Button pressed.")

    # Callback for button release
    def on_button_released(self):
        press_duration = time.monotonic() - self.buttonPressTime
        self.buttonPressed = False
        logging.debug(f"UIManager: Button released after {press_duration:.2f} seconds.")

        if press_duration >= self.holdThreshold:
            self.toggle_play_pause()
        else:
            current_time = time.monotonic()
            if self.clickCount == 0:
                self.clickCount += 1
                self.last_click_time = current_time
                # Schedule single-click handler with a tighter threshold
                self.click_timer = threading.Timer(self.doubleClickThreshold * 0.8, self.handle_single_click)
                self.click_timer.start()
            elif current_time - self.last_click_time <= self.doubleClickThreshold:
                if hasattr(self, 'click_timer') and self.click_timer.is_alive():
                    self.click_timer.cancel()  # Cancel single click if double click is detected
                self.on_button_double_clicked()
                self.clickCount = 0
            else:
                self.clickCount = 1
                self.last_click_time = current_time


    def on_button_held(self):
        logging.debug("UIManager: Button held.")
        self.rotation_count = 0
        self.encoder.when_rotated_clockwise = self.fast_forward_action
        self.encoder.when_rotated_counter_clockwise = self.rewind_action

    def on_button_double_clicked(self):
        logging.debug("UIManager: Double click detected.")
        if self.current_mode == "normal":
            self.enter_menu()
        elif self.current_mode == "menu":
            # "Back" or exit
            if len(self.menu_stack) > 1:
                self.menu_stack.pop()
                self.current_menu = self.menu_stack[-1]
                self.menu_index = 0
            else:
                self.reset_to_normal_mode()
        elif self.current_mode == "brightness_adjustment":
            self.current_mode = "menu"
            self.current_menu = self.menu_stack.pop() if self.menu_stack else self.main_menu_entries

    def handle_single_click(self):
        if self.clickCount == 1:
            logging.debug("UIManager: Single click detected.")
            if self.current_mode == "menu" and self.current_menu:
                self.handle_menu_selection()
            self.clickCount = 0  # Reset click count after handling single click


    def opendisplay_menu(self):
        self.menu_stack.append(self.current_menu)
        self.current_menu = self.display_menu_entries
        self.menu_index = 0
        self.display_ui(self.current_millis())

    def dummy_submenu_action(self):
        print("Sub2 triggered")

    def enter_brightness_adjustment(self):
        self.current_mode = "brightness_adjustment"
        self.display_brightness()

    def handle_menu_selection(self):
        label, func = self.current_menu[self.menu_index]
        func()

        # Start the blink effect immediately
        self.should_blink = True
        self.blink_start_time = self.current_millis()
        self.highlight_current=False
        # Step 1: Render the unhighlighted state
        self.display_menu()
        time.sleep(self.blink_duration / 100.0)  # Half of the blink duration (convert ms to seconds)
        self.highlight_current=True
        # Step 2: Render the highlighted state
        self.display_menu()
        time.sleep(self.blink_duration / 100.0)  # Half of the blink duration

        # Step 3: End the blink effect
        self.should_blink = False
        self.display_ui(self.current_millis())  # Update the UI after the blink


    def enter_menu(self):
        self.current_mode = "menu"
        self.current_menu = self.main_menu_entries
        self.menu_index = 0
        self.menu_stack.clear()
        self.menu_stack.append(self.main_menu_entries)
        self.display_ui(self.current_millis())

    def reset_to_normal_mode(self):
        self.current_mode = "normal"
        self.current_menu = None
        self.menu_stack.clear()
        self.menu_index = 0
        self.display_ui(self.current_millis())


    def current_millis(self):
        return int(time.monotonic() * 1000)

    def calculate_text_width(self, text, size=1):
        return len(text) * 6 * size

    def prepare_scrolling_text(self, text, size=1):
        w = self.calculate_text_width(text, size)
        active = w > self.SCREEN_WIDTH
        return w, active

    def draw_play_pause_button(self, playing, x, y):
        if playing:
            self.oled.text(">", x, y, 1)
        else:
            self.oled.text("||", x, y, 1)

    def display_menu(self):
        if not self.current_menu or len(self.current_menu) == 0:  # Handle None or empty menu
            logging.debug("UIManager: Menu is None or empty, cannot display.")
            return

        self.oled.fill(0)

        menu_length = len(self.current_menu)
        visible_items = self.SCREEN_HEIGHT // 12
        start_index = max(0, self.menu_index - (visible_items // 2))
        start_index = min(start_index, max(0, menu_length - visible_items))
        end_index = min(menu_length, start_index + visible_items)

        # Calculate whether the current item should be highlighted
        self.highlight_current = True
        if self.should_blink:
            elapsed_time = self.current_millis() - self.blink_start_time
            if elapsed_time < self.blink_duration // 2:
                self.highlight_current = False  # Temporarily unhighlight
            elif elapsed_time >= self.blink_duration:
                self.should_blink = False  # End the blink effect

        # Render the menu items
        for i, (label, _) in enumerate(self.current_menu[start_index:end_index]):
            yPos = i * 12
            if i + start_index == self.menu_index and self.highlight_current:
                self.oled.fill_rect(0, yPos, self.SCREEN_WIDTH, 12, 1)
                self.oled.text(label, 2, yPos, 0)
            else:
                self.oled.text(label, 2, yPos, 1)
        self.oled.show()


    def open_controls_menu(self):
        self.menu_stack.append(self.current_menu)
        self.current_menu = self.controls_menu_entries
        self.menu_index = 0
        self.display_menu()
    
    def controls_action(self):
        logging.info("CLICKED!")  # Changed from print to logging
        logging.debug("UIManager: Controls action triggered.")
        # Removed self.blink_highlight() from here

    def blink_highlight(self):
        self.highlight_current=False
        self.display_menu()
        time.sleep(1.2) 
        self.highlight_current=True
        self.display_menu()
        time.sleep(0.2) 

        blink_count = 3
        for _ in range(blink_count):
            # Highlight the current menu item
            self.oled.fill(0)
            menu_length = len(self.current_menu)
            visible_items = self.SCREEN_HEIGHT // 12  
            start_index = max(0, self.menu_index - (visible_items // 2))
            start_index = min(start_index, max(0, menu_length - visible_items))
            end_index = min(menu_length, start_index + visible_items)

            for i, (label, _) in enumerate(self.current_menu[start_index:end_index]):
                yPos = i * 12
                if i + start_index == self.menu_index:
                    self.oled.fill_rect(0, yPos, self.SCREEN_WIDTH, 12, 1)
                    self.oled.text(label, 2, yPos, 0)
                else:
                    self.oled.text(label, 2, yPos, 1)
            self.oled.show()
            time.sleep(0.2)  # Duration for the highlight to be visible

            # Remove highlight
            self.display_menu()
            time.sleep(0.2)  # Duration for the highlight to be removed



    def display_brightness(self):
        self.oled.fill(0)
        self.oled.text("Brightness", 0, 0, 1)

        bar_total_width = self.SCREEN_WIDTH - 20
        bar_filled = int((self.brightness / 255.0) * bar_total_width)
        self.oled.fill_rect(10, 20, bar_filled, 10, 1)
        self.oled.rect(10, 20, bar_total_width, 10, 1)
        self.oled.text(f"{self.brightness}%", (self.SCREEN_WIDTH // 2) - 10, 40, 1)

        self.oled.contrast(self.brightness)
        self.oled.show()

    def update_scroll_offsets(self, t_ms):
        if (
            self.songScrollActive
            and (t_ms - self.songLastScrollUpdate > self.songScrollInterval)
        ):
            self.songLastScrollUpdate = t_ms
            self.songScrollOffset += self.songScrollStep
            if self.songScrollOffset > self.songTextWidth:
                self.songScrollOffset = 0

        if (
            self.artistScrollActive
            and (t_ms - self.artistLastScrollUpdate > self.artistScrollInterval)
        ):
            self.artistLastScrollUpdate = t_ms
            self.artistScrollOffset += self.artistScrollStep
            if self.artistScrollOffset > self.artistTextWidth:
                self.artistScrollOffset = 0

    def display_ui(self, t_ms):
        if not self.displayOn:
            return
        if self.fastForwardAnimationActive or self.rewindAnimationActive:
            anim_time = t_ms - self.animationStartTime
            if anim_time < self.animationDuration:
                self.oled.fill(0)
                xCenter = (self.SCREEN_WIDTH // 2) - 12
                yCenter = (self.SCREEN_HEIGHT // 2) - 8
                text = ">>" if self.fastForwardAnimationActive else "<<"
                self.oled.text(text, xCenter, yCenter, 1)
                self.oled.show()
                return
            else:
                self.fastForwardAnimationActive = False
                self.rewindAnimationActive = False

        # Brightness Screen
        if self.current_mode == "brightness_adjustment":
            self.display_brightness()
            return

        # Menu Screen
        if self.current_mode == "menu":
            self.display_menu()
            return

        # Normal Playback Screen
        self.oled.fill(0)
        # Progress Bar
        seek_width = int((self.currentPosition / 100.0) * self.SCREEN_WIDTH)
        self.oled.fill_rect(0, 0, seek_width, 2, 1)

        # Separate Artist / Song
        sep_idx = self.songName.find(" - ")
        artist = self.songName[:sep_idx] if sep_idx != -1 else ""
        song = self.songName[sep_idx + 3 :] if sep_idx != -1 else self.songName

        # Scrolling logic
        self.songTextWidth, self.songScrollActive = self.prepare_scrolling_text(
            song, size=2
        )
        self.artistTextWidth, self.artistScrollActive = self.prepare_scrolling_text(
            artist, size=1
        )
        self.update_scroll_offsets(t_ms)

        # Song Title
        if self.songScrollActive:
            self.oled.text(
                song, self.SCREEN_WIDTH - self.songScrollOffset, 3, 1, size=2
            )
        else:
            x = (self.SCREEN_WIDTH - self.songTextWidth) // 2
            self.oled.text(song, x, 3, 1, size=2)

        # Artist Name
        if artist:
            if self.artistScrollActive:
                self.oled.text(
                    artist, self.SCREEN_WIDTH - self.artistScrollOffset, 24, 1
                )
            else:
                x = (self.SCREEN_WIDTH - self.artistTextWidth) // 2
                self.oled.text(artist, x, 24, 1)

        # Play/Pause
        play_pause_x = (self.SCREEN_WIDTH // 2) - 3
        self.draw_play_pause_button(self.isPlaying, play_pause_x, 36)

        # Volume Bar
        available_width = self.SCREEN_WIDTH - (18 + 24)
        vol_width = int((self.volume / 100.0) * available_width)
        self.oled.fill_rect(18, 50, vol_width, 5, 1)
        for v in range(0, 101, 10):
            tick_pos = 18 + int((v / 100.0) * available_width)
            self.oled.vline(tick_pos, 50, 5, 1)
        self.oled.text(str(self.volume), 0, 50, 1)


        # Progress Bar
        seek_width = int((self.currentPosition / 100.0) * self.SCREEN_WIDTH)
        self.oled.fill_rect(0, 0, seek_width, 2, 1)

        # Bluetooth Indicator
        if self.bt_manager and self.bt_manager.is_bt_connected:
            self.oled.text("BT", self.SCREEN_WIDTH - 14, self.SCREEN_HEIGHT - 14, 1)

        self.oled.show()


    def toggle_play_pause(self):
        self.isPlaying = not self.isPlaying
        if self.bt_manager:
            command = "play" if self.isPlaying else "pause"
            self.bt_manager.control_playback(command)
            logging.debug(f"UIManager: Sent '{command}' command to BluetoothManager.")
        else:
            logging.warning("UIManager: BluetoothManager reference not set.")


    def fast_forward_action(self):
        if not self.fastForwardAnimationActive:
            self.fastForwardAnimationActive = True
            self.animationStartTime = self.current_millis()
            if self.bt_manager:
                self.bt_manager.control_playback("next")
                logging.debug("UIManager: Sent 'next' command to BluetoothManager.")
            else:
                logging.warning("UIManager: BluetoothManager reference not set.")

    def rewind_action(self):
        if not self.rewindAnimationActive:
            self.rewindAnimationActive = True
            self.animationStartTime = self.current_millis()
            if self.bt_manager:
                self.bt_manager.control_playback("prev")
                logging.debug("UIManager: Sent 'prev' command to BluetoothManager.")
            else:
                logging.warning("UIManager: BluetoothManager reference not set.")



    def boot_animation(self):
        # Clear the display
        self.oled.fill(0)
        self.oled.show()

        # Define animation parameters
        delay = 0.05
        max_radius = 32  # Maximum radius for circles, half of screen width

        # Draw expanding circles from the center
        for radius in range(1, max_radius):
            self.oled.fill(0)  # Clear screen for each new frame
            x = self.SCREEN_WIDTH // 2
            y = self.SCREEN_HEIGHT // 2
            
            # Draw a circle
            self.oled.circle(x, y, radius, 1)
            
            # Add some random dots for a noise effect
            for _ in range(5):  # 5 dots per frame
                dot_x = random.randint(0, self.SCREEN_WIDTH - 1)
                dot_y = random.randint(0, self.SCREEN_HEIGHT - 1)
                self.oled.pixel(dot_x, dot_y, 1)

            self.oled.show()
            time.sleep(delay)

        # Draw a static pattern to simulate boot completion
        self.oled.fill(0)
        for i in range(0, self.SCREEN_WIDTH, 4):
            for j in range(0, self.SCREEN_HEIGHT, 4):
                self.oled.pixel(i, j, 1)
        self.oled.show()
        time.sleep(0.5)

        # Clear the screen after the animation
        self.oled.fill(0)
        self.oled.show()

    def run(self):
        self.boot_animation()
        try:
            while True:
                t_ms = self.current_millis()
                self.display_ui(t_ms)
                time.sleep(0.005) 
        except KeyboardInterrupt:
            pass
        finally:
            self.led_pwm.close()
