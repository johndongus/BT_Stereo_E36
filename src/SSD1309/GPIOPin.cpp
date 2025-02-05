#include "GPIOPin.h"

GPIOPin::GPIOPin(uint8_t pin) : pin(pin) {}

GPIOPin::~GPIOPin() {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
}

bool GPIOPin::begin() {
    return true;
}

void GPIOPin::setModeOutput() {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
}

void GPIOPin::setModeInput() {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
}

void GPIOPin::write(bool high) {
    bcm2835_gpio_write(pin, high ? HIGH : LOW);
}

bool GPIOPin::read() {
    return bcm2835_gpio_lev(pin);
}
