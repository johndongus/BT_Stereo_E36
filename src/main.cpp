// main.cpp
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <vector>
#include <cstring>
#include <thread>
#include <atomic>
#include "SSD1309/SSD1309.h"
#include "UI/UI.h"
#include "IO/IO.h"
#include "BT/BT.h"

// Define GPIO Pins
#define DC_PIN    25 // GPIO25 (Physical pin 22)
#define RESET_PIN 5 // GPIO5 (Physical pin 29)
#define CS_PIN    8  // GPIO8 (Physical pin 24)

volatile sig_atomic_t stop = 0;

void handle_signal(int signum) {
    stop = 1;
}

int main() {
    // Define and initialize the exit flag
    std::atomic<bool> shouldExit(false);
    GUI::ui.exitFlag = &shouldExit;

    int lock_fd = open("/tmp/stereo-shit.lock", O_CREAT | O_RDWR, 0666);
    if (lock_fd < 0) {
        std::cerr << "Unable to create lock file.\n";
        return 1;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        std::cerr << "Another instance is running.\n";
        close(lock_fd);
        return 1;
    }

    // Register signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGTSTP, handle_signal);

    IOHandler ioHandler;
    if (!ioHandler.begin()) {
        std::cerr << "Failed to initialize IOHandler.\n";
        return 1;
    }

    SPIDevice spiDevice(CS_PIN, 1000000, 0, 0, 0);
    if (!spiDevice.begin()) {
        std::cerr << "Failed to initialize SPI Device.\n";
        ioHandler.end();
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        unlink("/tmp/stereo-shit.lock");
        return 1;
    }

    SSD1309_SPI display(128, 64, spiDevice, DC_PIN, RESET_PIN);
    if (!display.initDisplay()) {
        std::cerr << "Failed to initialize SSD1309 Display.\n";
        spiDevice.end();
        ioHandler.end();
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        unlink("/tmp/stereo-shit.lock");
        return 1;
    }

    // Instantiate BluetoothMedia and set up
    BluetoothMedia btMedia;
    btMedia.setup_bluetooth();
    btMedia.start_media_monitor();
    btMedia.trust_connected_device();

    // single click
    ioHandler.set_click_callback([&]() {
        if (GUI::ui.currentMode == GUI::ScreenMode::LISTBOX) {
            // We are in listbox mode
            if (GUI::ui.currentList.selectedIndex >= 0
                && GUI::ui.currentList.selectedIndex < static_cast<int>(GUI::ui.currentList.items.size()))
            {
                auto& item = GUI::ui.currentList.items[GUI::ui.currentList.selectedIndex];
                // If subItems is non-empty => push sub-menu
                if (!item.subItems.empty()) {
                    GUI::ListBox subBox;
                    subBox.title = item.label;
                    subBox.items = item.subItems;
                    subBox.selectedIndex = 0;
                    GUI::ui.pushListBox(subBox);
                }
                else if (item.onSelect) {
                    // Just run its callback
                    item.onSelect();
                }
            }
        }
        else if (GUI::ui.currentMode == GUI::ScreenMode::MEDIA) {
            // If we're in media mode => play/pause
            btMedia.play_pause();
        }
    });

    // Non Hold Rotate
    ioHandler.set_rotate_callback([&](bool forward) {
            if (GUI::ui.currentMode == GUI::ScreenMode::MEDIA) {
                // Adjust volume
                int currentVolume = ioHandler.getVolume();
                if (forward) {
                    currentVolume += 1;
                    if (currentVolume > 100) currentVolume = 100;
                }
                else {
                    currentVolume -= 1;
                    if (currentVolume < 0) currentVolume = 0;
                }
                ioHandler.setVolume(currentVolume);
            }
            else if (GUI::ui.currentMode == GUI::ScreenMode::LISTBOX) {
                // Scroll through menu items
                int newIndex = GUI::ui.currentList.selectedIndex + (forward ? 1 : -1);
                newIndex = std::max(0, std::min(newIndex, static_cast<int>(GUI::ui.currentList.items.size()) - 1));
                GUI::ui.currentList.selectedIndex = newIndex;
            }
        });

    // Track changing callback
    ioHandler.set_rotate_hold_callback([&](bool forward) {
        if (forward) {
            btMedia.next_track();
            GUI::Windows::showTemporaryMessage(&display, ">>", 1000); // Display ">>" for 1 second
        } else {
            btMedia.previous_track();
            GUI::Windows::showTemporaryMessage(&display, "<<", 1000); // Display "<<" for 1 second
        }
    });

    // double click shit
    ioHandler.set_doubleclick_callback([&]() {
        if (GUI::ui.currentMode == GUI::ScreenMode::LISTBOX) {
            // If in a submenu or settings, go back
            if (!GUI::ui.menuStack.empty()) {
                GUI::ui.popListBox();
            }
            else {
                GUI::ui.showMedia();
            }
        }
        else {
            GUI::ui.pushListBox(GUI::ui.mainMenu);
        }
    });


    GUI::Windows::InitializeMenus();
    GUI::ui.showMedia();

    while (!stop && !GUI::ui.exitFlag->load()) {
        btMedia.check_and_set_media_player_path();
    
            GUI::Windows::DrawStereo(&display, btMedia, ioHandler);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Shutting down application..." << std::endl;
    spiDevice.end();
    ioHandler.end();
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    unlink("/tmp/stereo-shit.lock");
    return 0;
}
