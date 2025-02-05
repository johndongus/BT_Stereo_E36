// SSD1309_SPI.h
#ifndef SSD1309_SPI_H
#define SSD1309_SPI_H

#include "SSD1309_Base.h"
#include "SPIDevice.h"
#include "GPIOPin.h"
#include <memory>

class SSD1309_SPI : public SSD1309_Base {
public:
    /**
     * @param width: Display width in pixels
     * @param height: Display height in pixels
     * @param spiDevice: Reference to an initialized SPIDevice object
     * @param dcPin: GPIO pin number for DC (Data/Command)
     * @param resetPin: GPIO pin number for Reset (use 255 if not connected)
     */
    SSD1309_SPI(uint8_t width, uint8_t height, SPIDevice& spiDevice, 
               uint8_t dcPin, uint8_t resetPin = 255);

    ~SSD1309_SPI();

    bool initDisplay() override;
    void writeCommand(uint8_t cmd) override;
    void writeData(const std::vector<uint8_t>& data) override;
    void resetDisplay();
    void clearDisplay();
private:
    SPIDevice& spiDevice;
    GPIOPin dc;
    GPIOPin reset;
    bool hasReset;

    void resetSequence();
};

#endif // SSD1309_SPI_H
