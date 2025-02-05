// UI.h
#pragma once

#include "../SSD1309/SSD1309_SPI.h"
#include "../BT/BT.h"
#include "../IO/IO.h"
#include <string>
#include <functional>
#include <vector>
#include <stack>
#include <atomic>

namespace GUI {

    struct ListItem;

    struct ListItem {
        std::string label;
        std::function<void()> onSelect;
        std::vector<ListItem> subItems;

        ListItem(const std::string& lbl,
                 std::function<void()> callback = nullptr,
                 std::vector<ListItem> subs = {})
            : label(lbl), onSelect(callback), subItems(std::move(subs))
        {}
    };

    struct ListBox {
        std::vector<ListItem> items;
        int selectedIndex = 0;
        std::string title; 
    };

    enum class ScreenMode {
        MEDIA,
        CALL,
        LISTBOX
    };

    struct UIManager {
        ScreenMode currentMode = ScreenMode::MEDIA;
        ListBox currentList;
        ListBox settingsMenu; 
        ListBox mainMenu;
        ListBox pairingMenu;
        ListBox trustedDevicesMenu;
        std::unordered_map<std::string, ListBox> deviceMenus;
        std::stack<ListBox> menuStack;
        std::mutex temp_msg_mutex;
        std::string temp_message;
        std::chrono::steady_clock::time_point temp_message_end_time;

        // Function to set a temporary message
        void setTemporaryMessage(const std::string& msg, int duration_ms);
        std::atomic<bool>* exitFlag = nullptr;

        void pushListBox(const ListBox& newList) {
            menuStack.push(currentList);
            currentMode = ScreenMode::LISTBOX;
            currentList = newList;
        }

        void popListBox() {
            if (!menuStack.empty()) {
                currentList = menuStack.top();
                menuStack.pop();
                if (menuStack.empty()) {
                    currentMode = ScreenMode::MEDIA;
                } else {
                    currentMode = ScreenMode::LISTBOX;
                }
            } else {
                showMedia();
            }
        }

        void showMedia() {
            while (!menuStack.empty()) {
                menuStack.pop();
            }
            currentMode = ScreenMode::MEDIA;
        }
    };

    extern UIManager ui;

    namespace Windows{
        void DrawStereo(SSD1309_SPI* display, BluetoothMedia& btMedia, IOHandler& ioHandler);
        void InitializeMenus(BluetoothMedia& btMedia);
        void Draw_ListBox(SSD1309_SPI* display, const ListBox& listBox);
        void Draw_MediaScreen(SSD1309_SPI* display, const MediaInfo& mediaInfo, int song_progress);
        void Draw_CallScreen(SSD1309_SPI* display, const std::string& caller_number);
        void Draw_VolumeBar(SSD1309_SPI* display, int x, int y, int volume);
        void Draw_SettingsScreen(SSD1309_SPI* display, int base_x, int base_y);

        void showTemporaryMessage(SSD1309_SPI* display, const std::string& msg, int duration_ms);
    }
}
