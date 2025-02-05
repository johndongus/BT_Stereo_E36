#ifndef IO_H
#define IO_H

#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include <gpiod.hpp>  

/**
 * @brief IOHandler class manages the rotary knob and LED dimming.
 *
 * Features:
 * - Detects single-click, double-click, and hold actions on the knob button.
 * - Detects rotation direction to adjust volume or skip tracks.
 * - Controls LED brightness (via a simple on/off method) based on the volume level.
 * - Supports callbacks for specific events.
 *
 * This version uses libgpiod instead of pigpio.
 */
class IOHandler {
public:
    /**
     * @brief Construct a new IOHandler object.
     * @param clk_pin GPIO pin number for CLK (Broadcom numbering; default: 17)
     * @param dt_pin GPIO pin number for DT (default: 27)
     * @param sw_pin GPIO pin number for SW (button; default: 22)
     * @param led_pin GPIO pin number for LED (default: 20)
     */
    IOHandler(int clk_pin = 17, int dt_pin = 27, int sw_pin = 22, int led_pin = 20);
    
    /**
     * @brief Destroy the IOHandler object.
     */
    ~IOHandler();

    /**
     * @brief Initialize the IOHandler, open the GPIO chip, request the lines,
     * and start the polling thread.
     * @return true if initialization was successful.
     * @return false otherwise.
     */
    bool begin();

    /**
     * @brief Clean up resources and stop the polling thread.
     */
    void end();

    /**
     * @brief Get the current volume level (0–100).
     */
    int getVolume() const;

    /**
     * @brief Set the current volume (0–100) and update LED and system volume.
     */
    void setVolume(int newVolume);

    /**
     * @brief Register a callback for single-click events.
     */
    void set_click_callback(std::function<void()> cb);

    /**
     * @brief Register a callback for hold + rotate events.
     * @param cb The callback function to register (true for forward, false for backward).
     */
    void set_rotate_hold_callback(std::function<void(bool)> cb);

    /**
     * @brief Register a callback for rotate events.
     * @param cb The callback function to register (true for forward, false for backward).
     */
    void set_rotate_callback(std::function<void(bool)> cb);

    /**
     * @brief Register a callback for double-click events.
     */
    void set_doubleclick_callback(std::function<void()> cb);

    /**
     * @brief Update the LED brightness based on the given volume.
     * @param volume Volume (0–100).
     */
    void updateLED(int volume);

private:
    // GPIO pin numbers (using Broadcom numbering)
    int CLK_PIN;
    int DT_PIN;
    int SW_PIN;
    int LED_PIN;

    // Volume (0–100)
    std::atomic<int> volume;

    // PWM duty cycle (0–255) for LED brightness.
    std::atomic<int> pwm_duty;

    // Rotary encoder state
    int last_clk;
    int last_dt;
    std::mutex encoder_mutex;

    // Button state and click/hold detection
    std::atomic<bool> button_pressed;
    std::mutex button_mutex;
    int click_count;
    std::chrono::steady_clock::time_point last_click_time;
    std::mutex click_mutex;
    std::atomic<bool> hold_detected;
    std::chrono::milliseconds hold_time;
    std::thread hold_thread;
    std::atomic<bool> hold_thread_running;

    // Callback functions
    std::function<void()> click_callback;
    std::function<void(bool)> rotate_hold_callback;
    std::function<void(bool)> rotate_callback;
    std::function<void()> doubleclick_callback;

    // libgpiod objects
    std::unique_ptr<gpiod::chip> chip;
    std::shared_ptr<gpiod::line> clk_line;
    std::shared_ptr<gpiod::line> dt_line;
    std::shared_ptr<gpiod::line> sw_line;
    std::shared_ptr<gpiod::line> led_line;

    // Polling thread for reading inputs
    std::atomic<bool> poll_running;
    std::thread poll_thread;
    std::thread button_thread;
    std::chrono::steady_clock::time_point lastClickTime;
void processButton();
    /**
     * @brief Poll the GPIO inputs (encoder and button) at a fixed interval.
     */
    void pollInputs();

    /**
     * @brief Update the LED hardware output.
     * For this simple example, if duty > 128 we set the LED “on”, else “off”.
     */
    void updateLEDHardware(int duty);

    // Disable copy constructor and assignment operator.
    IOHandler(const IOHandler&) = delete;
    IOHandler& operator=(const IOHandler&) = delete;
};

#endif // IO_H
