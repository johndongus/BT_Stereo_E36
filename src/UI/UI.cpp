// UI.cpp

#include "UI.h"
#include "SSD1309_SPI.h" 
#include <algorithm>     
#include <iostream>
#include <string>
#include <chrono>        
#include <thread>

namespace GUI {
    UIManager ui; // Ui class define

        void UIManager::setTemporaryMessage(const std::string& msg, int duration_ms) {
            std::lock_guard<std::mutex> lock(temp_msg_mutex);
            temp_message = msg;
            temp_message_end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
        }
    namespace Windows {

        static const int SPACE_BETWEEN = 20;  // Space between scrolls (pixels)
        static const float SCROLL_SPEED = 0.5f; 

        static float songScrollX = 0.0f;
        static float artistScrollX = 0.0f;
        static bool isSongScrolling = false;
        static bool isArtistScrolling = false;

      
        void InitializeMenus() {
            ListBox subMenu;
            subMenu.title = "Sub Menu";
            subMenu.items = {
                {"SubItem1", [](){
                     std::cout << "SubItem1 selected!\n";
                 }},
                {"SubItem2", [](){
                     std::cout << "SubItem2 selected!\n";
                 }}
            };

            ui.settingsMenu.title = "Settings";
            ui.settingsMenu.items = {
                {"Volume", [&]() {
                    std::cout << "Volume settings selected.\n";
                }},
                {"Brightness", [&]() {
                    std::cout << "Brightness settings selected.\n";
                }},
                {"WiFi", [&]() {
                    std::cout << "WiFi settings selected.\n";
                }}
            };

            ui.mainMenu.title = "Main Menu";
            ui.mainMenu.items = {
                {"Settings", [&]() {
                    ui.pushListBox(ui.settingsMenu);
                }},
                {"Go to SubMenu", [&]() {
                     ui.pushListBox(subMenu);
                }},
                {"Play/Pause", [&]() {
                     std::cout << "Play/Pause selected.\n";
                }},
                {"Exit", [&]() {
                     std::cout << "Exit selected.\n";
                     if (ui.exitFlag) {
                         *(ui.exitFlag) = true; 
                     }
                }}
            };
        }

        void Draw_ListBox(SSD1309_SPI* display, const ListBox& listBox) {
            if (!listBox.title.empty()) {
                int titleWidth = listBox.title.length() * 6;
                int titleX = (display->width - titleWidth) / 2;
                display->drawString(titleX, 0, listBox.title);
            }

            int startY = listBox.title.empty() ? 0 : 14; 
            int lineHeight = 12;

            for (size_t i = 0; i < listBox.items.size(); i++) {
                int y = startY + i * lineHeight;

                if (static_cast<int>(i) == listBox.selectedIndex) {
                    display->drawRect(0, y - 2, display->width, lineHeight);
                }

                display->drawString(2, y, listBox.items[i].label);
            }
        }

        void Draw_VolumeBar(SSD1309_SPI* display, int x, int y, int volume) {
            volume = std::clamp(volume, 0, 100);

            int width = 108; 
            int height = 8;

            display->drawRect(x, y, width, height);
            int filled_width = static_cast<int>((volume / 100.0) * (width - 2));
            display->fillRect(x + 1, y + 1, filled_width, height - 2, true);
        }

    void Draw_MediaScreen(SSD1309_SPI* display, const MediaInfo& mediaInfo, int song_progress) {
        // Draw the seek bar
        int seekBarHeight = 2;
        int progressWidth = (song_progress * display->width) / 100;
        display->fillRect(0, 0, display->width, seekBarHeight, false); 
        if (progressWidth > 0) {
            display->fillRect(0, 0, progressWidth, seekBarHeight, true);
        }

        std::string songName = mediaInfo.title;
        std::string artistName = mediaInfo.artist;

        int songTextWidth = songName.length() * 12;   
        int artistTextWidth = artistName.length() * 6; 

        isSongScrolling = songTextWidth > display->width;
        isArtistScrolling = artistTextWidth > display->width;

        if (isSongScrolling) {
            songScrollX -= SCROLL_SPEED;
            if (songScrollX < -songTextWidth) {
                songScrollX = display->width + SPACE_BETWEEN;
            }
        } else {
            songScrollX = (display->width - songTextWidth) / 2.0f;
        }

        if (isArtistScrolling) {
            artistScrollX -= SCROLL_SPEED;
            if (artistScrollX < -artistTextWidth) {
                artistScrollX = display->width + SPACE_BETWEEN;
            }
        } else {
            artistScrollX = (display->width - artistTextWidth) / 2.0f;
        }

        int songY = seekBarHeight + 4;
        int artistY = songY + 20;
        int indicatorY = artistY + 15;
        int songHeight = 24;    // 12 * 2
        int artistHeight = 12;
        int indicatorHeight = 12;

        display->fillRect(0, songY, display->width, songHeight, false);
        display->fillRect(0, artistY, display->width, artistHeight, false);
        display->fillRect(0, indicatorY, display->width, indicatorHeight, false);

        display->drawText(static_cast<int>(songScrollX), songY, songName, 2.0f);

        display->drawString(static_cast<int>(artistScrollX), artistY, artistName);

        std::string indicator = (mediaInfo.status == "playing") ? "||" : ">";
        int indicatorWidth = indicator.length() * 6;
        int indicatorX = (display->width - indicatorWidth) / 2.0f;
        display->drawText(indicatorX, indicatorY, indicator, 1.0f);
    }


    void Draw_CallScreen(SSD1309_SPI* display, const std::string& caller_number) {
        display->drawString(0, 0, "Incoming Call");
        display->drawString(0, 20, caller_number.empty() ? "Unknown Number" : caller_number);
    }

  void DrawStereo(SSD1309_SPI* display, BluetoothMedia& btMedia, IOHandler& ioHandler) {
            static bool initialized = false;
            static ScreenMode lastMode = ScreenMode::MEDIA;
            static MediaInfo lastMediaInfo;
            static int lastSongProgress = -1;
            static int lastVolume = -1;
            static ListBox lastListBox;

            if (!initialized) {
                InitializeMenus();
                initialized = true;

                display->clearDisplay();
                display->drawString(0, 0, "Stereo Ready!");
                display->show();
                std::cout << "Initial splash screen drawn.\n";

                lastMode = ui.currentMode;
                lastMediaInfo = btMedia.get_media_info();
                lastSongProgress = btMedia.get_song_progress();
                lastVolume = ioHandler.getVolume();
                lastListBox = ui.currentList;

                songScrollX = display->width + SPACE_BETWEEN;
                artistScrollX = display->width + SPACE_BETWEEN;

                return;
            }

            bool needUpdate = false;

      
            MediaInfo currentMedia = btMedia.get_media_info();
            bool mediaChanged = (currentMedia.title != lastMediaInfo.title ||
                                 currentMedia.artist != lastMediaInfo.artist ||
                                 currentMedia.status != lastMediaInfo.status);

            if (mediaChanged) {
                needUpdate = true;
                lastMediaInfo = currentMedia;
                songScrollX = display->width + SPACE_BETWEEN;
                artistScrollX = display->width + SPACE_BETWEEN;

                std::cout << "Media info changed.\n";
            }

            if (btMedia.g_call_in_progress && lastMode != ScreenMode::CALL) {
                needUpdate = true;
                lastMode = ScreenMode::CALL;
                std::cout << "Switched to CALL mode.\n";
            }
            else if (!btMedia.g_call_in_progress) {
                int currentSongProgress = btMedia.get_song_progress();
                int currentVolume = ioHandler.getVolume();

                if (ui.currentMode != lastMode) {
                    needUpdate = true;
                    lastMode = ui.currentMode;
                    std::cout << "Mode changed to " << static_cast<int>(lastMode) << "\n";
                }

                if (ui.currentMode == ScreenMode::MEDIA) {
                    if (mediaChanged ||
                        currentSongProgress != lastSongProgress ||
                        currentVolume != lastVolume) 
                    {
                        needUpdate = true;
                        lastMediaInfo = currentMedia;
                        lastSongProgress = currentSongProgress;
                        lastVolume = currentVolume;


                        if (mediaChanged) {
                            songScrollX = display->width + SPACE_BETWEEN;
                            artistScrollX = display->width + SPACE_BETWEEN;
                        }
                    }
                }
                else if (ui.currentMode == ScreenMode::LISTBOX) {
                    if (ui.currentList.title != lastListBox.title ||
                        ui.currentList.selectedIndex != lastListBox.selectedIndex ||
                        ui.currentList.items.size() != lastListBox.items.size()) 
                    {
                        needUpdate = true;
                        lastListBox = ui.currentList;
                        std::cout << "ListBox updated.\n";
                    }
                }
            }

            if (ui.currentMode == ScreenMode::MEDIA) {
                needUpdate = true;
            }

            bool show_temp_message = false;
            std::string current_temp_message;
            {
                std::lock_guard<std::mutex> lock(ui.temp_msg_mutex);
                if (!ui.temp_message.empty()) {
                    auto now = std::chrono::steady_clock::now();
                    if (now < ui.temp_message_end_time) {
                        show_temp_message = true;
                        current_temp_message = ui.temp_message;
                    }
                    else {
                        ui.temp_message.clear();
                    }
                }
            }

            if (show_temp_message) {
                std::cout << "Displaying temporary message: " << current_temp_message << "\n";

                display->clearDisplay();
                display->fillRect(0, 0, display->width, display->height, false);

                int msgWidth = current_temp_message.length() * 12; 
                int msgX = (display->width - msgWidth) / 2;
                int msgY = (display->height - 16) / 2; 

                display->drawText(msgX, msgY, current_temp_message, 2.0f);
                display->show();
                std::cout << "Temporary message shown.\n";

                return; 
            }

             


            if (needUpdate) {
                if (!btMedia.is_media_player_connected()) {
                            lastMediaInfo = MediaInfo{" ", "BT Not Connected", 0, 0, "paused"};
                }
                if (ui.currentMode == ScreenMode::MEDIA) {
                    Draw_MediaScreen(display, lastMediaInfo, lastSongProgress);
                    Draw_VolumeBar(display, 10, 56, lastVolume); 
                }
                else {
                    display->clearDisplay();
                    std::cout << "Cleared display for active screen.\n";

                    if (btMedia.g_call_in_progress) {
                        Draw_CallScreen(display, btMedia.g_current_caller_number);
                    }
                    else {
                        switch (ui.currentMode) {
                        case ScreenMode::MEDIA:
                            Draw_MediaScreen(display, lastMediaInfo, lastSongProgress);
                            Draw_VolumeBar(display, 10, 56, lastVolume);
                            break;

                        case ScreenMode::LISTBOX:
                            Draw_ListBox(display, ui.currentList);
                            break;
                        }
                    }
                }
                display->show();
            
            }
        }
        void showTemporaryMessage(SSD1309_SPI* display, const std::string& msg, int duration_ms) {
            ui.setTemporaryMessage(msg, duration_ms);
        }

    } // namespace Windows
} // namespace GUI