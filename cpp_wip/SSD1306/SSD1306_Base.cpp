// SSD1306_Base.cpp
#include "SSD1306_Base.h"
#include "font5x7.h" 
#include <cstring>
#include <iostream> 


SSD1306_Base::SSD1306_Base(uint8_t width, uint8_t height, bool externalVCC)
    : width(width), height(height), externalVCC(externalVCC), powerState(false),
      buffer((width * height) / 8, 0), isRotated(false) {} // Initialize isRotated

SSD1306_Base::~SSD1306_Base() {}

void SSD1306_Base::powerOn() {
    if (powerState) return;
    initDisplay();
    powerState = true;
}

void SSD1306_Base::powerOff() {
    if (!powerState) return;
    writeCommand(SET_DISP | 0x00);
    powerState = false;
}

void SSD1306_Base::setContrast(uint8_t contrast) {
    std::vector<uint8_t> cmds = { SET_CONTRAST, contrast };
    sendCommandSequence(cmds);
}

void SSD1306_Base::invertDisplay(bool invert) {
    std::vector<uint8_t> cmds = { SET_NORM_INV | (invert ? 1 : 0) };
    sendCommandSequence(cmds);
}

void SSD1306_Base::show() {
    std::vector<uint8_t> cmds = {
        SET_COL_ADDR,
        0x00,    
        static_cast<uint8_t>(width - 1),
        SET_PAGE_ADDR,
        0x00,             
        static_cast<uint8_t>((height / 8) - 1)
    };
    sendCommandSequence(cmds);


    writeData(buffer);


}
#include <chrono>
#include <thread>
void SSD1306_Base::sendCommandSequence(const std::vector<uint8_t>& cmds, bool delay) {
    for (auto cmd : cmds) {
        writeCommand(cmd);
    }
}

void SSD1306_Base::drawChar(uint8_t x, uint8_t y, char c) {
    if (c < ' ' || c > '~') { 
        c = ' ';
    }
    int charIndex = c - ' '; 
    if (charIndex < 0 || charIndex >= sizeof(FONT5x7)/sizeof(FONT5x7[0])) {
        charIndex = 0; 
    }
    for (uint8_t i = 0; i < 5; ++i) { 
        uint8_t line = FONT5x7[charIndex][i];
        for (uint8_t bit = 0; bit < 7; ++bit) { 
            uint8_t pixel = (line >> bit) & 0x01;
            uint8_t pixelX = x + i;
            uint8_t pixelY = y + bit;
            if (pixelX < width && pixelY < height) {
                uint16_t index = pixelX + (pixelY / 8) * width;
                if (pixel) {
                    buffer[index] |= (1 << (pixelY % 8));
                } else {
                    buffer[index] &= ~(1 << (pixelY % 8));
                }
            }
        }
    }

    uint8_t spaceX = x + 5;
    for (uint8_t bit = 0; bit < 7; ++bit) {
        uint8_t pixel = 0;
        uint8_t pixelX = spaceX;
        uint8_t pixelY = y + bit;
        if (pixelX < width && pixelY < height) {
            uint16_t index = pixelX + (pixelY / 8) * width;
            buffer[index] &= ~(1 << (pixelY % 8));
        }
    }
}


void SSD1306_Base::drawString(uint8_t x, uint8_t y, const std::string& str) {
    uint8_t cursorX = x;
    uint8_t cursorY = y;
    
    for (char c : str) {
        if (isRotated) {
            drawChar(width - cursorX - 5, height - cursorY - 7, c);
        } else {
            drawChar(cursorX, cursorY, c);
        }
        cursorX += 6;
        if (cursorX + 5 >= width) { 
            break;
        }
    }
}

void SSD1306_Base::setPixel(uint8_t x, uint8_t y, bool value) {
    if (x >= width || y >= height)  return;
    if (isRotated) {
        uint8_t rotatedX = width - x - 1;
        uint8_t rotatedY = height - y - 1;
        x = rotatedX;
        y = rotatedY;
    }

    uint16_t index = x + (y / 8) * width;
    if (value) {
        buffer[index] |= (1 << (y % 8));
    } else {
        buffer[index] &= ~(1 << (y % 8));
    }
}

void SSD1306_Base::setPixelInt(int x, int y, bool value) {
    if (x < 0 || y < 0 || x >= width || y >= height)  return;
    setPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y), value);
}

void SSD1306_Base::drawRect(uint8_t x, uint8_t y, uint8_t rectWidth, uint8_t rectHeight) {
    for (uint8_t i = 0; i < rectWidth; ++i) {
        setPixel(x + i, y, true); // 
        setPixel(x + i, y + rectHeight - 1, true); 
    }

    for (uint8_t i = 0; i < rectHeight; ++i) {
        setPixel(x, y + i, true);
        setPixel(x + rectWidth - 1, y + i, true);
    }
}


void SSD1306_Base::drawCircle(uint8_t centerX, uint8_t centerY, uint8_t radius) {
    int x = radius;
    int y = 0;
    int decisionOver2 = 1 - x; 

    while (y <= x) {
        setPixelInt(centerX + x, centerY + y, true);
        setPixelInt(centerX + y, centerY + x, true);
        setPixelInt(centerX - x, centerY + y, true);
        setPixelInt(centerX - y, centerY + x, true);
        setPixelInt(centerX - x, centerY - y, true);
        setPixelInt(centerX - y, centerY - x, true);
        setPixelInt(centerX + x, centerY - y, true);
        setPixelInt(centerX + y, centerY - x, true);
        y++;
        if (decisionOver2 <= 0) {
            decisionOver2 += 2 * y + 1;
        } else {
            x--;
            decisionOver2 += 2 * (y - x) + 1;
        }
    }
}


void SSD1306_Base::drawLine(int x0, int y0, int x1, int y1) {
    bool steep = false;
    if (abs(y1 - y0) > abs(x1 - x0)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = abs(y1 - y0);
    int error = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        if (steep) {
            setPixelInt(y, x, true);
        } else {
            setPixelInt(x, y, true);
        }

        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

void SSD1306_Base::drawText(uint8_t x, uint8_t y, const std::string& text, float scale) {
    if (scale < 1.0f) {
        scale = 1.0f; // Minimum scale
    }

    uint8_t cursorX = x;
    uint8_t cursorY = y;

    for (char c : text) {
        if (c < ' ' || c > '~') { 
            c = ' ';
        }
        int charIndex = c - ' '; 
        if (charIndex < 0 || charIndex >= sizeof(FONT5x7)/sizeof(FONT5x7[0])) {
            charIndex = 0; 
        }

        for (uint8_t i = 0; i < 5; ++i) {
            uint8_t line = FONT5x7[charIndex][i];
            for (uint8_t bit = 0; bit < 7; ++bit) {
                uint8_t pixel = (line >> bit) & 0x01;
                for (float sx = 0; sx < scale; ++sx) {
                    for (float sy = 0; sy < scale; ++sy) {
                        uint8_t pixelX = cursorX + i * scale + static_cast<uint8_t>(sx);
                        uint8_t pixelY = cursorY + bit * scale + static_cast<uint8_t>(sy);
                        if (pixelX < width && pixelY < height) {
                            setPixel(pixelX, pixelY, pixel);
                        }
                    }
                }
            }
        }

        cursorX += static_cast<uint8_t>(6 * scale);
        if (cursorX + static_cast<uint8_t>(5 * scale) >= width) { 
            break;
        }
    }
}

void SSD1306_Base::fillRect(uint8_t x, uint8_t y, uint8_t rectWidth, uint8_t rectHeight, bool fill) {
    for (uint8_t i = x; i < x + rectWidth && i < width; ++i) {
        for (uint8_t j = y; j < y + rectHeight && j < height; ++j) {
            setPixel(i, j, fill);
        }
    }
}

void SSD1306_Base::drawBitmap(uint8_t x, uint8_t y, const uint8_t* bitmap, uint8_t bitmapWidth, uint8_t bitmapHeight) {
    for (uint8_t j = 0; j < bitmapHeight; ++j) {
        for (uint8_t i = 0; i < bitmapWidth; ++i) {
            uint8_t byte = bitmap[j * bitmapWidth + i];
            if (byte) {
                setPixel(x + i, y + j, true);
            } else {
                setPixel(x + i, y + j, false);
            }
        }
    }
}