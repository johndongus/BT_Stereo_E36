# BT_Stereo_E36

![Current Image](https://i.imgur.com/qe3aNkL.jpeg)

---

## 📦 Parts List

| **Part**                              | **Description**                                     |
|---------------------------------------|-----------------------------------------------------|
| **RPI Zero 2 W**                      | Compact and powerful single-board computer          |
| **Innomaker Raspberry Pi HiFi DAC MINI HAT PCM5122** | High-quality digital-to-analog converter for audio |
| **Arduino Nano**                      | Microcontroller for controlling peripherals         |
| **2.42" OLED 1309**                   | OLED display for menu and song information         |
| **LED Light**                         | Indicator light for system status                  |
| **Rotary Encoder with Push Button**   | For volume control and navigation                  |

---

## ✨ Features

- **Bluetooth with Qualcomm aptX Support**  
- **Volume Knob**  
- **Menu System**  
- **Song Information Display**  
- **Song Seek Progress**  
- **Volume Slider**  
- **LED Indicator Light**  

---

## 🛠️ TODO Features

- **Phone Support**  
  Display caller name and number.
  
- **Microphone Input**  
  Enable voice input and hands-free calling.
  
- **Fast Forward and Rewind**  
  Implement song skipping and replay functionality.
  
- **Equalizer Settings**  
  Expand on existing menu for custom audio adjustments. Including finding a novel way to do equalization with pulseaudio, purely through command line. 
  Note on this, `pactl load-module module-ladspa-sink sink_name=ladspa_out sink_master=@DEFAULT_SINK@ plugin=caps label=Eq10 control=0,0,0,0,0,0,0,0,0,0`
  
- **Brightness Fix for OLED 1309**  
  Resolve brightness control issues with the display. Seems im not the only one with this issue on this paticular board.
  `https://forum.pjrc.com/index.php?threads/how-does-one-control-the-brightness-of-an-oled-display-ssd1309.29265/`

---

Current Stereo Trim is for a BMW E36, but device can be used as a indepedent stereo, will make custom faceplates upon request in the repo. 🚗🎶
