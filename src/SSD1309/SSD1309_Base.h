// SSD1309_Base.h
#ifndef SSD1309_BASE_H
#define SSD1309_BASE_H

#include <cstdint>
#include <vector>
#include <string>

// Forward declaration
class SPIDevice;

class SSD1309_Base {
public:
    SSD1309_Base(uint8_t width, uint8_t height, bool externalVCC = false);
    virtual ~SSD1309_Base();

    virtual bool initDisplay() = 0;
    virtual void writeCommand(uint8_t cmd) = 0;
    virtual void writeData(const std::vector<uint8_t>& data) = 0;

    void powerOn();
    void powerOff();
    void setContrast(uint8_t contrast);
    void invertDisplay(bool invert);
    void rotateDisplay(bool rotate);
    void show();

    // Functions for Text Rendering
    void drawChar(uint8_t x, uint8_t y, char c);
    void drawString(uint8_t x, uint8_t y, const std::string& str);
    uint8_t width;
    uint8_t height;
    std::vector<uint8_t> buffer;
    
  
    void drawRect(uint8_t x, uint8_t y, uint8_t rectWidth, uint8_t rectHeight);
    void fillRect(uint8_t x, uint8_t y, uint8_t rectWidth, uint8_t rectHeight, bool fill = true);
    void drawCircle(uint8_t centerX, uint8_t centerY, uint8_t radius);
    void drawLine(int x0, int y0, int x1, int y1);
    void drawText(uint8_t x, uint8_t y, const std::string& text, float scale = 1.0f);
    void drawBitmap(uint8_t x, uint8_t y, const uint8_t* bitmap, uint8_t bitmapWidth, uint8_t bitmapHeight);

    
protected:
    bool isRotated = false;
    bool externalVCC;
    bool powerState;
    void setPixel(uint8_t x, uint8_t y, bool value);
    void setPixelInt(int x, int y, bool value);
    void sendCommandSequence(const std::vector<uint8_t>& cmds, bool delay = 0);
};

// SSD1309 Commands
const uint8_t SET_CONTRAST = 0x81;
const uint8_t SET_ENTIRE_ON = 0xA4;
const uint8_t SET_NORM_INV = 0xA6;
const uint8_t SET_DISP = 0xAE;
const uint8_t SET_MEM_ADDR = 0x20;
const uint8_t SET_COL_ADDR = 0x21;
const uint8_t SET_PAGE_ADDR = 0x22;
const uint8_t SET_DISP_START_LINE = 0x40;
const uint8_t SET_SEG_REMAP = 0xA0;
const uint8_t SET_MUX_RATIO = 0xA8;
const uint8_t SET_IREF_SELECT = 0xAD;
const uint8_t SET_COM_OUT_DIR = 0xC0;
const uint8_t SET_DISP_OFFSET = 0xD3;
const uint8_t SET_COM_PIN_CFG = 0xDA;
const uint8_t SET_DISP_CLK_DIV = 0xD5;
const uint8_t SET_PRECHARGE = 0xD9;
const uint8_t SET_VCOM_DESEL = 0xDB;
const uint8_t SET_CHARGE_PUMP = 0x8D;

#endif // SSD1309_BASE_H
