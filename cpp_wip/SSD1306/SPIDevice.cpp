// SPIDevice.cpp
#include "SPIDevice.h"
#include <iostream>

SPIDevice::SPIDevice(uint8_t csPin, uint32_t baudrate, 
                     uint8_t polarity, uint8_t phase, int extraClocks)
    : csPin(csPin), baudrate(baudrate), 
      polarity(polarity), phase(phase), extraClocks(extraClocks) {}

SPIDevice::~SPIDevice() {
    end();
}

bool SPIDevice::begin() {
    if (!bcm2835_init()) {
        std::cerr << "bcm2835_init failed. Are you running as root?\n";
        return false;
    }

    if (!bcm2835_spi_begin()) {
        std::cerr << "bcm2835_spi_begin failed.\n";
        return false;
    }

    // SPI settings
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);   
    bcm2835_spi_setDataMode((polarity << 1) | (phase));
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);    // baudrate=8MHz
    bcm2835_spi_set_speed_hz(baudrate);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE); //CS manual
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);


    bcm2835_gpio_fsel(csPin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(csPin, HIGH);

    return true;
}

void SPIDevice::end() {
    bcm2835_spi_end();
    bcm2835_close();
}

void SPIDevice::setCS(bool level) {
    bcm2835_gpio_write(csPin, level ? HIGH : LOW);
}

void SPIDevice::write(const std::vector<uint8_t>& data) {
    setCS(false); // Activate CS
    bcm2835_spi_transfern((char*)data.data(), data.size());
    setCS(true); // Deactivate CS

    if (extraClocks > 0) {
        int clocks = extraClocks / 8;
        uint8_t dummy = 0xFF;
        std::vector<uint8_t> dummyData(clocks, dummy);
        setCS(false);
        bcm2835_spi_transfern((char*)dummyData.data(), dummyData.size());
        setCS(true);
    }
}

void SPIDevice::writeCommand(uint8_t cmd) {
    std::vector<uint8_t> data = { cmd };
    write(data);
}
