#include "IO.h"
#include <iostream>

// Constructor
IOHandler::IOHandler(int clk_pin, int dt_pin, int sw_pin, int led_pin)
    : CLK_PIN(clk_pin), DT_PIN(dt_pin), SW_PIN(sw_pin), LED_PIN(led_pin),
      volume(50), clk_callback_handle(-1), dt_callback_handle(-1), sw_callback_handle(-1),
      last_clk(1), last_dt(1), button_pressed(false),
      click_count(0), hold_detected(false),
      hold_time(std::chrono::milliseconds(500)), // 500ms hold threshold
      hold_thread_running(false), pwm_duty(128) // Initialize PWM to mid brightness
{
}

// Destructor
IOHandler::~IOHandler() {
    end();
}

// Initialize GPIO and set up callbacks
bool IOHandler::begin() {
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed.\n";
        return false;
    }

    // Set GPIO modes
    gpioSetMode(CLK_PIN, PI_INPUT);
    gpioSetMode(DT_PIN, PI_INPUT);
    gpioSetMode(SW_PIN, PI_INPUT);
    gpioSetMode(LED_PIN, PI_OUTPUT);

    // Set pull-up resistors
    gpioSetPullUpDown(CLK_PIN, PI_PUD_UP);
    gpioSetPullUpDown(DT_PIN, PI_PUD_UP);
    gpioSetPullUpDown(SW_PIN, PI_PUD_UP);

    // Initialize last states
    last_clk = gpioRead(CLK_PIN);
    last_dt = gpioRead(DT_PIN);

    // Set up callbacks for rotary encoder
    clk_callback_handle = gpioSetAlertFuncEx(CLK_PIN, clk_callback_func, this);
    if (clk_callback_handle < 0) {
        std::cerr << "Failed to set CLK callback.\n";
        gpioTerminate();
        return false;
    }

    dt_callback_handle = gpioSetAlertFuncEx(DT_PIN, dt_callback_func, this);
    if (dt_callback_handle < 0) {
        std::cerr << "Failed to set DT callback.\n";
        gpioSetAlertFunc(CLK_PIN, nullptr);
        gpioTerminate();
        return false;
    }

    // Set up callback for button (SW_PIN)
    sw_callback_handle = gpioSetAlertFuncEx(SW_PIN, sw_callback_func, this);
    if (sw_callback_handle < 0) {
        std::cerr << "Failed to set SW callback.\n";
        gpioSetAlertFunc(CLK_PIN, nullptr);
        gpioSetAlertFunc(DT_PIN, nullptr);
        gpioTerminate();
        return false;
    }

    // Initialize LED brightness based on volume
    updateLED();

    // Start the button processing thread
    hold_thread_running = true;
    hold_thread = std::thread(&IOHandler::processButton, this);

    return true;
}

// Clean up GPIO and callbacks
void IOHandler::end() {
    if (hold_thread_running) {
        hold_thread_running = false;
        if (hold_thread.joinable()) {
            hold_thread.join();
        }
    }

    // Remove callbacks
    if (clk_callback_handle >= 0) {
        gpioSetAlertFunc(CLK_PIN, nullptr);
    }
    if (dt_callback_handle >= 0) {
        gpioSetAlertFunc(DT_PIN, nullptr);
    }
    if (sw_callback_handle >= 0) {
        gpioSetAlertFunc(SW_PIN, nullptr);
    }

    // Turn off LED
    gpioPWM(LED_PIN, 0);

    // Terminate pigpio
    gpioTerminate();
}

// Get current volume
int IOHandler::getVolume() {
    return volume.load();
}

// Static callback functions
void IOHandler::clk_callback_func(int gpio, int level, uint32_t tick, void* user) {
    IOHandler* handler = static_cast<IOHandler*>(user);
    if (handler) {
        handler->handleEncoder(level);
    }
}

void IOHandler::dt_callback_func(int gpio, int level, uint32_t tick, void* user) {
    IOHandler* handler = static_cast<IOHandler*>(user);
    if (handler) {
        handler->handleEncoder(level);
    }
}

void IOHandler::sw_callback_func(int gpio, int level, uint32_t tick, void* user) {
    IOHandler* handler = static_cast<IOHandler*>(user);
    if (handler) {
        handler->handleButton(level);
    }
}

// Handle rotary encoder rotation events
void IOHandler::handleEncoder(int level) {
    std::lock_guard<std::mutex> lock(encoder_mutex);

    int clk = gpioRead(CLK_PIN);
    int dt = gpioRead(DT_PIN);

    // Determine rotation direction based on the changes
    if (last_clk == 1 && clk == 0) { // Falling edge on CLK
        if (dt == 1) {
            // Clockwise rotation
            volume += 1;
            if (volume > 100) volume = 100;
            std::cout << "Volume increased to " << volume.load() << "\n";
        } else {
            // Counter-clockwise rotation
            volume -= 1;
            if (volume < 0) volume = 0;
            std::cout << "Volume decreased to " << volume.load() << "\n";
        }
        updateLED();
    }

    last_clk = clk;
    last_dt = dt;
}

// Handle button press and release events
void IOHandler::handleButton(int level) {
    if (level == 0) { // Button pressed (assuming active low)
        std::lock_guard<std::mutex> lock(button_mutex);
        button_pressed = true;

        // Update click count and timing
        std::lock_guard<std::mutex> click_lock(click_mutex);
        auto now = std::chrono::steady_clock::now();
        if (click_count == 1 && std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_time).count() < 500) {
            // Double click detected
            click_count = 2;
        } else {
            // Single click detected (temporarily)
            click_count = 1;
        }
        last_click_time = now;
    } else { // Button released
        std::lock_guard<std::mutex> lock(button_mutex);
        button_pressed = false;
    }
}

// Thread function to process button events (clicks and holds)
void IOHandler::processButton() {
    while (hold_thread_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        bool pressed;
        {
            std::lock_guard<std::mutex> lock(button_mutex);
            pressed = button_pressed.load();
        }

        if (pressed && !hold_detected.load()) {
            // Start hold timer
            auto hold_start = std::chrono::steady_clock::now();
            while (pressed && hold_thread_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                {
                    std::lock_guard<std::mutex> lock(button_mutex);
                    pressed = button_pressed.load();
                }
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - hold_start).count() >= hold_time.count()) {
                    // Hold detected
                    hold_detected = true;
                    std::cout << "Button held down.\n";
                    // Perform hold action here (e.g., reset volume)
                    volume = 50; // Reset volume to 50 as a placeholder
                    updateLED();
                    break;
                }
            }
            if (!pressed) {
                if (hold_detected.load()) {
                    // Hold was detected and handled
                    hold_detected = false;
                }
            }
        }

        // Check for click events
        std::lock_guard<std::mutex> click_lock(click_mutex);
        if (click_count > 0) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_time).count();
            if (duration > 500) { // Double click timeout
                if (click_count == 1) {
                    // Single click detected
                    std::cout << "Single click detected.\n";
                    // Perform single click action here (e.g., toggle LED brightness)
                    if (pwm_duty > 0) {
                        pwm_duty = 0;
                        std::cout << "LED turned off.\n";
                    } else {
                        pwm_duty = 128; // Mid brightness
                        std::cout << "LED set to mid brightness.\n";
                    }
                    gpioPWM(LED_PIN, pwm_duty.load());
                } else if (click_count == 2) {
                    // Double click detected
                    std::cout << "Double click detected.\n";
                    // Perform double click action here (e.g., set volume to max)
                    volume = 100;
                    std::cout << "Volume set to " << volume.load() << "\n";
                    updateLED();
                }
                click_count = 0;
            }
        }
    }
}

// Update LED brightness based on volume
void IOHandler::updateLED() {
    // Map volume (0-100) to PWM duty cycle (0-255)
    int duty = static_cast<int>((volume.load() / 100.0) * 255);
    if (duty < 0) duty = 0;
    if (duty > 255) duty = 255;
    pwm_duty = duty;
    gpioPWM(LED_PIN, pwm_duty.load());
}
