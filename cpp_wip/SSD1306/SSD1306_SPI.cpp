// SSD1306_SPI.cpp
#include "SSD1306_SPI.h"
#include <iostream>

// Constructor
SSD1306_SPI::SSD1306_SPI(uint8_t width, uint8_t height, SPIDevice& spiDevice, 
                         uint8_t dcPin, uint8_t resetPin)
    : SSD1306_Base(width, height, false), spiDevice(spiDevice), 
      dc(dcPin), reset(resetPin), hasReset(resetPin != 255) {}

SSD1306_SPI::~SSD1306_SPI() {
    // Destructor
}

bool SSD1306_SPI::initDisplay() {
    // Initialize DC and Reset pins
    dc.begin();
    dc.setModeOutput();
    dc.write(false); // Command mode

    if (hasReset) {
        reset.begin();
        reset.setModeOutput();
        reset.write(false);
        resetSequence();
    }

     std::vector<uint8_t> init_sequence = {
        SET_DISP | 0x00,        // Display off
        SET_MEM_ADDR, 0x00,     // Horizontal addressing mode
        SET_DISP_START_LINE | 0x00,
        SET_SEG_REMAP | 0x01,   // Column address 0 is mapped to SEG127 (Mirrored horizontally)
        SET_MUX_RATIO, height - 1,
        SET_COM_OUT_DIR | 0x08, // Scan from COM[N-1] to COM0 (Mirrored vertically)
        SET_DISP_OFFSET, 0x00,
        SET_COM_PIN_CFG, 0x12,  // Alternative COM pin configuration
        SET_DISP_CLK_DIV, 0x80, // Set display clock divide ratio/oscillator frequency
        SET_PRECHARGE, 0xF1,    // Set pre-charge period
        SET_VCOM_DESEL, 0x40,    // Set VCOMH deselect level
        SET_CONTRAST, 0xCF,      // Set contrast control
        SET_ENTIRE_ON,           // Disable entire display on
        SET_NORM_INV,            // Set normal display (not inverted)
        SET_CHARGE_PUMP, 0x14,   // Enable charge pump
        SET_DISP | 0x01          // Display on
    };
    sendCommandSequence(init_sequence, 1);

    // Clear buffer
    std::fill(buffer.begin(), buffer.end(), 0);
    show();

    return true;
}

void SSD1306_SPI::writeCommand(uint8_t cmd) {
    dc.write(false); // Command mode
    spiDevice.writeCommand(cmd);
}

void SSD1306_SPI::writeData(const std::vector<uint8_t>& data) {
    dc.write(true); // Data mode
    spiDevice.write(data);
}

void SSD1306_SPI::resetSequence() {
    reset.write(false);
    bcm2835_delay(1); // 1 ms
    reset.write(true);
    bcm2835_delay(10); // 10 ms
    reset.write(false);
    bcm2835_delay(1);
    reset.write(true);
    bcm2835_delay(10); // 10 ms
}
