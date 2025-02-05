#!/bin/bash
set -e

# This script must be run as root.
if [[ $EUID -ne 0 ]]; then
    echo "Please run as root (sudo ./install.sh)"
    exit 1
fi

# Get the current (login) username.
USERNAME=$(logname)
echo "Using username: $USERNAME"

#############################
# Update & Upgrade the System
#############################
echo "Updating and upgrading the system..."
apt-get update -y
apt-get upgrade -y

#############################
# Install Required Packages
#############################
echo "Installing required packages..."
apt-get install -y bluez bluetooth pi-bluetooth pulseaudio pulseaudio-module-bluetooth ofono git cmake g++ pkg-config libbcm2835-dev libgpiod-dev bcm2835

#############################
# Modify /etc/dbus-1/system.d/ofono.conf
#############################
OFONO_CONF="/etc/dbus-1/system.d/ofono.conf"
if [ -f "$OFONO_CONF" ]; then
    echo "Backing up $OFONO_CONF..."
    cp "$OFONO_CONF" "$OFONO_CONF.bak"
    # Insert policy block if not already present.
    if ! grep -q "<policy user=\"$USERNAME\">" "$OFONO_CONF"; then
        echo "Adding policy for user $USERNAME in $OFONO_CONF..."
        sed -i '/<\/busconfig>/i \
  <policy user="'"$USERNAME"'">\
    <allow own="org.ofono"/>\
    <allow send_destination="org.ofono"/>\
    <allow send_interface="org.ofono.SimToolkitAgent"/>\
    <allow send_interface="org.ofono.PushNotificationAgent"/>\
    <allow send_interface="org.ofono.SmartMessagingAgent"/>\
    <allow send_interface="org.ofono.PositioningRequestAgent"/>\
    <allow send_interface="org.ofono.HandsfreeAudioAgent"/>\
    <allow send_interface="org.ofono.NetworkMonitorAgent"/>\
    <allow send_interface="org.ofono.intel.LteCoexistenceAgent"/>\
  </policy>' "$OFONO_CONF"
    else
        echo "Policy for $USERNAME already present in $OFONO_CONF."
    fi
else
    echo "File $OFONO_CONF not found; skipping ofono configuration."
fi

#############################
# Modify /etc/ssh/sshd_config .. I had to do this to make SSH stable
#############################
#SSHD_CONFIG="/etc/ssh/sshd_config"
#if ! grep -q "IPQoS cs0 cs0" "$SSHD_CONFIG"; then
    echo "Appending 'IPQoS cs0 cs0' to $SSHD_CONFIG..."
    echo "IPQoS cs0 cs0" >> "$SSHD_CONFIG"
#else
    echo "'IPQoS cs0 cs0' is already present in $SSHD_CONFIG."
#fi

#############################
# Modify /etc/pulse/default.pa
#############################
PULSE_DEFAULT="/etc/pulse/default.pa"
if [ -f "$PULSE_DEFAULT" ]; then
    if ! grep -q "module-bluetooth-policy" "$PULSE_DEFAULT"; then
        echo "Appending bluetooth policy module load to $PULSE_DEFAULT..."
        cat <<EOF >> "$PULSE_DEFAULT"

.ifexists module-bluetooth-policy.so
load-module module-bluetooth-policy
.endif
EOF
    else
        echo "Bluetooth policy module already configured in $PULSE_DEFAULT."
    fi
    if ! grep -q "module-bluetooth-discover" "$PULSE_DEFAULT"; then
        echo "Appending bluetooth discover module load to $PULSE_DEFAULT..."
        cat <<EOF >> "$PULSE_DEFAULT"

.ifexists module-bluetooth-discover.so
load-module module-bluetooth-discover autodetect_mtu=yes
.endif
EOF
    else
        echo "Bluetooth discover module already configured in $PULSE_DEFAULT."
    fi
else
    echo "File $PULSE_DEFAULT not found; skipping PulseAudio configuration."
fi

#############################
# Modify /boot/firmware/config.txt
#############################
CONFIG_TXT="/boot/firmware/config.txt"
if [ -f "$CONFIG_TXT" ]; then
    echo "Configuring $CONFIG_TXT..."
    # Uncomment required dtparam lines.
    sed -i 's/^#\(dtparam=i2c_arm=on\)/\1/' "$CONFIG_TXT"
    sed -i 's/^#\(dtparam=i2s=on\)/\1/' "$CONFIG_TXT"
    sed -i 's/^#\(dtparam=spi=on\)/\1/' "$CONFIG_TXT"
    sed -i 's/^#\(dtparam=audio=off\)/\1/' "$CONFIG_TXT"
    # Append overlay and additional lines if not already present.
    if ! grep -q "dtoverlay=allo-boss-dac-pcm512x-audio" "$CONFIG_TXT"; then
        echo "Appending allo-boss overlay and other settings to $CONFIG_TXT..."
        cat <<EOF >> "$CONFIG_TXT"

dtoverlay=allo-boss-dac-pcm512x-audio
hdmi_blanking=1
console=serial0,115200 console=tty1 root=PARTUUID=ac658bf3-02 rw quiet loglevel=3
EOF
    else
        echo "Allo-Boss DAC overlay already present in $CONFIG_TXT."
    fi
else
    echo "File $CONFIG_TXT not found; skipping config.txt configuration."
fi

#############################
# Modify /etc/bluetooth/main.conf
#############################
BT_MAIN="/etc/bluetooth/main.conf"
if [ -f "$BT_MAIN" ]; then
    if ! grep -q "Name = E36_002" "$BT_MAIN"; then
        echo "Appending bluetooth configuration to $BT_MAIN..."
        cat <<EOF >> "$BT_MAIN"

Name = E36_002
Enable=Source,Sink,Media,Socket
PreferredCodec=aptx

AutoEnable=true
EOF
    else
        echo "Bluetooth configuration already present in $BT_MAIN."
    fi
else
    echo "File $BT_MAIN not found; skipping bluetooth main.conf configuration."
fi

#############################
# Create /etc/systemd/system/bt.service
#############################
BT_SERVICE="/etc/systemd/system/bt.service"
echo "Creating $BT_SERVICE..."
cat <<EOF > "$BT_SERVICE"
[Unit]
Description=FrogStereo Service
After=network.target bluetooth.target

[Service]
User=$USERNAME
Group=$USERNAME
WorkingDirectory=/home/$USERNAME/BT_Stereo_E36/src/build
ExecStart=/home/$USERNAME/BT_Stereo_E36/src/build/FrogStereo
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF


#############################
# Setup bluetoothctl configuration
#############################
echo "Configuring Bluetooth..."
bluetoothctl <<EOF
power on
discoverable on
pairable on
agent on
default-agent
EOF

#############################
# Enable and start the bt.service
#############################
echo "Reloading systemd and enabling bt.service..."
systemctl daemon-reload
systemctl enable bt.service
systemctl start bt.service

echo "bt.service status:"
systemctl status bt.service --no-pager












#############################
# Clone
#############################
REPO_DIR="/home/$USERNAME/BT_Stereo_E36"
if [ -d "$REPO_DIR" ]; then
    echo "Repository already cloned at $REPO_DIR."
else
    echo "Cloning repository from https://github.com/johndongus/BT_Stereo_E36..."
    git clone https://github.com/johndongus/BT_Stereo_E36 "$REPO_DIR"
fi


#############################
# Build
#############################
echo "Building repository in ${REPO_DIR}/src ..."
cd "${REPO_DIR}/src"
make -j4









echo "Installation complete!"
echo "Please Reboot the device!"
