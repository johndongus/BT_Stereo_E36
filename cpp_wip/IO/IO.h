#ifndef IO_H
#define IO_H

#include <pigpio.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

/**
 * @brief IOHandler class manages the rotary knob and LED dimming.
 * 
 * Features:
 * - Detects single-click, double-click, and hold actions on the knob button.
 * - Detects rotation direction to adjust volume.
 * - Controls LED brightness using PWM based on the volume level.
 */
class IOHandler {
public:
    /**
     * @brief Construct a new IO Handler object
     * 
     * @param clk_pin GPIO pin number for CLK (default: 17)
     * @param dt_pin GPIO pin number for DT (default: 27)
     * @param sw_pin GPIO pin number for SW (button) (default: 22)
     * @param led_pin GPIO pin number for LED (default: 12)
     */
    IOHandler(int clk_pin = 17, int dt_pin = 27, int sw_pin = 22, int led_pin = 12);
    
    /**
     * @brief Destroy the IO Handler object
     */
    ~IOHandler();

    /**
     * @brief Initialize the IOHandler, set up GPIO and callbacks
     * 
     * @return true Initialization successful
     * @return false Initialization failed
     */
    bool begin();

    /**
     * @brief Clean up resources, remove callbacks, and terminate GPIO
     */
    void end();

    /**
     * @brief Get the current volume level (0-100)
     * 
     * @return int Current volume
     */
    int getVolume();

private:
    // GPIO pin numbers
    int CLK_PIN;    // Rotary encoder CLK pin
    int DT_PIN;     // Rotary encoder DT pin
    int SW_PIN;     // Rotary encoder SW (button) pin
    int LED_PIN;    // LED pin for PWM control

    // Volume level (0-100)
    std::atomic<int> volume;

    // Callback handles
    int clk_callback_handle;
    int dt_callback_handle;
    int sw_callback_handle;

    // Rotary encoder state
    std::atomic<int> last_clk;
    std::atomic<int> last_dt;
    std::mutex encoder_mutex;

    // Button state
    std::atomic<bool> button_pressed;
    std::mutex button_mutex;

    // Click detection
    int click_count;
    std::chrono::steady_clock::time_point last_click_time;
    std::mutex click_mutex;

    // Hold detection
    std::atomic<bool> hold_detected;
    std::chrono::milliseconds hold_time;
    std::thread hold_thread;
    std::atomic<bool> hold_thread_running;

    // PWM duty cycle (0-255)
    std::atomic<int> pwm_duty;

    // Private methods for handling GPIO events
    static void clk_callback_func(int gpio, int level, uint32_t tick, void* user);
    static void dt_callback_func(int gpio, int level, uint32_t tick, void* user);
    static void sw_callback_func(int gpio, int level, uint32_t tick, void* user);

    /**
     * @brief Handle rotary encoder rotation events
     * 
     * @param level Current level of the GPIO pin
     */
    void handleEncoder(int level);

    /**
     * @brief Handle button press and release events
     * 
     * @param level Current level of the GPIO pin
     */
    void handleButton(int level);

    /**
     * @brief Thread function to process button events (clicks and holds)
     */
    void processButton();

    /**
     * @brief Update the LED brightness based on the current volume
     */
    void updateLED();

    // Disable copy constructor and assignment operator
    IOHandler(const IOHandler&) = delete;
    IOHandler& operator=(const IOHandler&) = delete;
};

#endif // IO_H
