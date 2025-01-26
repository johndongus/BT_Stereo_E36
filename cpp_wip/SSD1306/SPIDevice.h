// SPIDevice.h
#ifndef SPIDEVICE_H
#define SPIDEVICE_H

#include <bcm2835.h>
#include <cstdint>
#include <vector>

class SPIDevice {
public:
    SPIDevice(uint8_t csPin, uint32_t baudrate = 8000000, 
              uint8_t polarity = 0, uint8_t phase = 0, int extraClocks = 0);
    ~SPIDevice();

    bool begin();
    void end();
    void write(const std::vector<uint8_t>& data);
    void writeCommand(uint8_t cmd);

private:
    uint8_t csPin;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    int extraClocks;

    void setCS(bool level);
};

#endif // SPIDEVICE_H
