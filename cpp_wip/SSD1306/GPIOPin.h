#ifndef GPIOPIN_H
#define GPIOPIN_H

#include <bcm2835.h>
#include <cstdint>

class GPIOPin {
public:
    GPIOPin(uint8_t pin);
    ~GPIOPin();

    bool begin();
    void setModeOutput();
    void setModeInput();
    void write(bool high);
    bool read();

private:
    uint8_t pin;
};

#endif // GPIOPIN_H
