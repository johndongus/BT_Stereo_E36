// font5x7.h
#ifndef FONT5X7_H
#define FONT5X7_H

#include <cstdint>

// Font5x7: 5x7 pixel font for SSD1306 displays
// Each character is represented by 5 bytes, each byte representing a column of 8 pixels (only 7 used)

const uint8_t FONT5x7[][5] = {
    // Space ' ' (ASCII 32)
    {0x00, 0x00, 0x00, 0x00, 0x00},
    
    // '!' (ASCII 33)
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    
    // '"' (ASCII 34)
    {0x00, 0x07, 0x00, 0x07, 0x00},
    
    // '#' (ASCII 35)
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    
    // '$' (ASCII 36)
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    
    // '%' (ASCII 37)
    {0x23, 0x13, 0x08, 0x64, 0x62},
    
    // '&' (ASCII 38)
    {0x36, 0x49, 0x55, 0x22, 0x50},
    
    // ''' (ASCII 39)
    {0x00, 0x05, 0x03, 0x00, 0x00},
    
    // '(' (ASCII 40)
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    
    // ')' (ASCII 41)
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    
    // '*' (ASCII 42)
    {0x14, 0x08, 0x3E, 0x08, 0x14},
    
    // '+' (ASCII 43)
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    
    // ',' (ASCII 44)
    {0x00, 0x50, 0x30, 0x00, 0x00},
    
    // '-' (ASCII 45)
    {0x08, 0x08, 0x08, 0x08, 0x08},
    
    // '.' (ASCII 46)
    {0x00, 0x60, 0x60, 0x00, 0x00},
    
    // '/' (ASCII 47)
    {0x20, 0x10, 0x08, 0x04, 0x02},
    
    // '0' (ASCII 48)
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    
    // '1' (ASCII 49)
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    
    // '2' (ASCII 50)
    {0x42, 0x61, 0x51, 0x49, 0x46},
    
    // '3' (ASCII 51)
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    
    // '4' (ASCII 52)
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    
    // '5' (ASCII 53)
    {0x27, 0x45, 0x45, 0x45, 0x39},
    
    // '6' (ASCII 54)
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    
    // '7' (ASCII 55)
    {0x01, 0x71, 0x09, 0x05, 0x03},
    
    // '8' (ASCII 56)
    {0x36, 0x49, 0x49, 0x49, 0x36},
    
    // '9' (ASCII 57)
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    
    // ':' (ASCII 58)
    {0x00, 0x36, 0x36, 0x00, 0x00},
    
    // ';' (ASCII 59)
    {0x00, 0x56, 0x36, 0x00, 0x00},
    
    // '<' (ASCII 60)
    {0x08, 0x14, 0x22, 0x41, 0x00},
    
    // '=' (ASCII 61)
    {0x14, 0x14, 0x14, 0x14, 0x14},
    
    // '>' (ASCII 62)
    {0x00, 0x41, 0x22, 0x14, 0x08},
    
    // '?' (ASCII 63)
    {0x02, 0x01, 0x51, 0x09, 0x06},
    
    // '@' (ASCII 64)
    {0x32, 0x49, 0x79, 0x41, 0x3E},
    
    // 'A' (ASCII 65)
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    
    // 'B' (ASCII 66)
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    
    // 'C' (ASCII 67)
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    
    // 'D' (ASCII 68)
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    
    // 'E' (ASCII 69)
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    
    // 'F' (ASCII 70)
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    
    // 'G' (ASCII 71)
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    
    // 'H' (ASCII 72)
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    
    // 'I' (ASCII 73)
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    
    // 'J' (ASCII 74)
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    
    // 'K' (ASCII 75)
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    
    // 'L' (ASCII 76)
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    
    // 'M' (ASCII 77)
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    
    // 'N' (ASCII 78)
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    
    // 'O' (ASCII 79)
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    
    // 'P' (ASCII 80)
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    
    // 'Q' (ASCII 81)
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    
    // 'R' (ASCII 82)
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    
    // 'S' (ASCII 83)
    {0x46, 0x49, 0x49, 0x49, 0x31},
    
    // 'T' (ASCII 84)
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    
    // 'U' (ASCII 85)
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    
    // 'V' (ASCII 86)
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    
    // 'W' (ASCII 87)
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    
    // 'X' (ASCII 88)
    {0x63, 0x14, 0x08, 0x14, 0x63},
    
    // 'Y' (ASCII 89)
    {0x07, 0x08, 0x70, 0x08, 0x07},
    
    // 'Z' (ASCII 90)
    {0x61, 0x51, 0x49, 0x45, 0x43},
    
    // '[' (ASCII 91)
    {0x00, 0x7F, 0x41, 0x41, 0x00},
    
    // '\' (ASCII 92)
    {0x02, 0x04, 0x08, 0x10, 0x20},
    
    // ']' (ASCII 93)
    {0x00, 0x41, 0x41, 0x7F, 0x00},
    
    // '^' (ASCII 94)
    {0x04, 0x02, 0x01, 0x02, 0x04},
    
    // '_' (ASCII 95)
    {0x40, 0x40, 0x40, 0x40, 0x40},
    
    // '`' (ASCII 96)
    {0x00, 0x01, 0x02, 0x04, 0x00},
    
    // 'a' (ASCII 97)
    {0x20, 0x54, 0x54, 0x54, 0x78},
    
    // 'b' (ASCII 98)
    {0x7F, 0x48, 0x44, 0x44, 0x38},
    
    // 'c' (ASCII 99)
    {0x38, 0x44, 0x44, 0x44, 0x20},
    
    // 'd' (ASCII 100)
    {0x38, 0x44, 0x44, 0x48, 0x7F},
    
    // 'e' (ASCII 101)
    {0x38, 0x54, 0x54, 0x54, 0x18},
    
    // 'f' (ASCII 102)
    {0x08, 0x7E, 0x09, 0x01, 0x02},
    
    // 'g' (ASCII 103)
    {0x0C, 0x52, 0x52, 0x52, 0x3E},
    
    // 'h' (ASCII 104)
    {0x7F, 0x08, 0x04, 0x04, 0x78},
    
    // 'i' (ASCII 105)
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    
    // 'j' (ASCII 106)
    {0x20, 0x40, 0x44, 0x3D, 0x00},
    
    // 'k' (ASCII 107)
    {0x7F, 0x10, 0x28, 0x44, 0x00},
    
    // 'l' (ASCII 108)
    {0x00, 0x41, 0x7F, 0x40, 0x00},
    
    // 'm' (ASCII 109)
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    
    // 'n' (ASCII 110)
    {0x7C, 0x08, 0x04, 0x04, 0x78},
    
    // 'o' (ASCII 111)
    {0x38, 0x44, 0x44, 0x44, 0x38},
    
    // 'p' (ASCII 112)
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    
    // 'q' (ASCII 113)
    {0x08, 0x14, 0x14, 0x18, 0x7C},
    
    // 'r' (ASCII 114)
    {0x7C, 0x08, 0x04, 0x04, 0x08},
    
    // 's' (ASCII 115)
    {0x48, 0x54, 0x54, 0x54, 0x20},
    
    // 't' (ASCII 116)
    {0x04, 0x3F, 0x44, 0x40, 0x20},
    
    // 'u' (ASCII 117)
    {0x3C, 0x40, 0x40, 0x20, 0x7C},
    
    // 'v' (ASCII 118)
    {0x1C, 0x20, 0x40, 0x20, 0x1C},
    
    // 'w' (ASCII 119)
    {0x3C, 0x40, 0x30, 0x40, 0x3C},
    
    // 'x' (ASCII 120)
    {0x44, 0x28, 0x10, 0x28, 0x44},
    
    // 'y' (ASCII 121)
    {0x0C, 0x50, 0x50, 0x50, 0x3C},
    
    // 'z' (ASCII 122)
    {0x44, 0x64, 0x54, 0x4C, 0x44},
    
    // '{' (ASCII 123)
    {0x00, 0x08, 0x36, 0x41, 0x00},
    
    // '|' (ASCII 124)
    {0x00, 0x00, 0x7F, 0x00, 0x00},
    
    // '}' (ASCII 125)
    {0x00, 0x41, 0x36, 0x08, 0x00},
    
    // '~' (ASCII 126)
    {0x08, 0x04, 0x08, 0x10, 0x08},
};

#endif // FONT5X7_H