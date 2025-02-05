#include "IO.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <pwd.h>  

IOHandler::IOHandler(int clk_pin, int dt_pin, int sw_pin, int led_pin)
    : CLK_PIN(clk_pin), DT_PIN(dt_pin), SW_PIN(sw_pin), LED_PIN(led_pin),
      volume(50), pwm_duty(128),
      last_clk(1), last_dt(1),
      button_pressed(false), click_count(0),
      hold_detected(false), hold_time(500),
      poll_running(false)
{
}

IOHandler::~IOHandler() {
    end();
}

bool IOHandler::begin() {
    try {
        chip = std::make_unique<gpiod::chip>("/dev/gpiochip0");
    } catch (const std::exception &e) {
        std::cerr << "Failed to open GPIO chip: " << e.what() << "\n";
        return false;
    }

    try {
        clk_line = std::make_shared<gpiod::line>(chip->get_line(CLK_PIN));
        dt_line  = std::make_shared<gpiod::line>(chip->get_line(DT_PIN));
        sw_line  = std::make_shared<gpiod::line>(chip->get_line(SW_PIN));
        led_line = std::make_shared<gpiod::line>(chip->get_line(LED_PIN));

        clk_line->request({ "IOHandler", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_UP });
        dt_line->request({ "IOHandler", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_UP });
        sw_line->request({ "IOHandler", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_UP });
        
        led_line->request({ "IOHandler", gpiod::line_request::DIRECTION_OUTPUT, 0 }, 0);
    } catch (const std::exception &e) {
        std::cerr << "Failed to request GPIO lines: " << e.what() << "\n";
        return false;
    }

    updateLED(volume.load());

    poll_running.store(true);
    poll_thread = std::thread(&IOHandler::pollInputs, this);
    button_thread = std::thread(&IOHandler::processButton, this);

    return true;
}

void IOHandler::end() {
    poll_running.store(false);
    if (poll_thread.joinable())
        poll_thread.join();
    if (button_thread.joinable())
        button_thread.join();

    std::cout << "IOHandler terminated.\n";
}

int IOHandler::getVolume() const {
    return volume.load();
}



void IOHandler::setVolume(int newVolume) {
    newVolume = std::max(0, std::min(newVolume, 100));
    volume.store(newVolume);
    std::cout << "Volume set to " << newVolume << "\n";
    updateLED(newVolume);

    std::ostringstream cmd;
    const char *sudoUser = std::getenv("SUDO_USER");
    if (sudoUser) {
        struct passwd *pw = getpwnam(sudoUser);
        if (pw) {
            uid_t uid = pw->pw_uid;
            cmd << "sudo -H -u " << sudoUser
                << " env XDG_RUNTIME_DIR=/run/user/" << uid
                << " wpctl set-volume @DEFAULT_SINK@ " << newVolume << "%";
        } else {
            cmd << "wpctl set-volume @DEFAULT_SINK@ " << newVolume << "%";
        }
    } else {
        cmd << "wpctl set-volume @DEFAULT_SINK@ " << newVolume << "%";
    }

    int ret = system(cmd.str().c_str());
    if(ret != 0) {
        std::cerr << "Error: wpctl command failed with error code " << ret << "\n";
    } else {
        std::cout << "Volume set to " << newVolume << "% using wpctl" << std::endl;
    }
}



void IOHandler::set_click_callback(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(click_mutex);
    click_callback = cb;
}

void IOHandler::set_rotate_hold_callback(std::function<void(bool)> cb) {
    std::lock_guard<std::mutex> lock(click_mutex);
    rotate_hold_callback = cb;
}

void IOHandler::set_rotate_callback(std::function<void(bool)> cb) {
    std::lock_guard<std::mutex> lock(click_mutex);
    rotate_callback = cb;
}

void IOHandler::set_doubleclick_callback(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(click_mutex);
    doubleclick_callback = cb;
}

void IOHandler::updateLED(int newVolume) {
    int duty = static_cast<int>((newVolume / 100.0) * 255);
    duty = std::max(0, std::min(duty, 255));
    pwm_duty.store(duty);
    updateLEDHardware(duty);
    std::cout << "LED brightness set to " << duty << "\n";
}

void IOHandler::updateLEDHardware(int duty) {
    int value = (duty > 128) ? 1 : 0;
    try {
        led_line->set_value(value);
    } catch (const std::exception &e) {
        std::cerr << "Failed to set LED value: " << e.what() << "\n";
    }
}

///
/// Encoder polling this is fucked
///
void IOHandler::pollInputs() {
    const std::chrono::milliseconds pollInterval(1);

    int a = clk_line->get_value();
    int b = dt_line->get_value();
    uint8_t oldState = (a << 1) | b;
    int delta = 0;
    auto lastStepTime = std::chrono::steady_clock::now();

    static const int8_t stateTable[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

    while (poll_running.load()) {
        int newA = clk_line->get_value();
        int newB = dt_line->get_value();
        uint8_t newState = (newA << 1) | newB;
        uint8_t index = (oldState << 2) | newState;
        int8_t step = stateTable[index];
        if ((step > 0 && delta < 0) || (step < 0 && delta > 0))
            delta = 0;
        delta += step;

        auto now = std::chrono::steady_clock::now();
        if (delta >= 2 && (now - lastStepTime) > std::chrono::milliseconds(3)) {
            lastStepTime = now;
            delta = 0;
            std::lock_guard<std::mutex> lock(click_mutex);
            if (rotate_callback)
                rotate_callback(true);  // forward
        } else if (delta <= -2 && (now - lastStepTime) > std::chrono::milliseconds(3)) {
            lastStepTime = now;
            delta = 0;
            std::lock_guard<std::mutex> lock(click_mutex);
            if (rotate_callback)
                rotate_callback(false); // backward
        }
        oldState = newState;
        std::this_thread::sleep_for(pollInterval);
    }
}


void IOHandler::processButton() {
    //probably should add these to config, probably should also make configs
    const auto debounceTime = std::chrono::milliseconds(50);
    const auto doubleClickTime = std::chrono::milliseconds(300);
    const auto holdTimeThreshold = std::chrono::milliseconds(500);

    bool wasPressed = false;
    int clickCount = 0;
    auto pressTime = std::chrono::steady_clock::now();
    auto lastReleaseTime = std::chrono::steady_clock::now();

    while (poll_running.load()) {
        int sw = sw_line->get_value();
        auto now = std::chrono::steady_clock::now();

        if (sw == 0 && !wasPressed) {
            wasPressed = true;
            pressTime = now;
        } else if (sw == 1 && wasPressed) {
            wasPressed = false;
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - pressTime);
            if (duration >= holdTimeThreshold) {
                std::lock_guard<std::mutex> lock(click_mutex);
                if (rotate_hold_callback)
                    rotate_hold_callback(true);  
                clickCount = 0;
            } else if (duration >= debounceTime) {
                clickCount++;
                lastReleaseTime = now;
            }
        }
        if (clickCount > 0 && (now - lastReleaseTime) >= doubleClickTime) {
            std::lock_guard<std::mutex> lock(click_mutex);
            if (clickCount == 1) {
                if (click_callback)
                    click_callback();
            } else if (clickCount >= 2) {
                if (doubleclick_callback)
                    doubleclick_callback();
            }
            clickCount = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
