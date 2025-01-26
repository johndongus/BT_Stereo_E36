
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <vector>
#include <cstring>
#include "SSD1306/SSD1306.h"
#include "UI/UI.h"
#include "IO/IO.h"
// Define GPIO Pins
#define DC_PIN    25 // GPIO25 (Physical pin 22)
#define RESET_PIN 24 // GPIO24 (Physical pin 18)
#define CS_PIN    8  // GPIO8 (Physical pin 24)


volatile sig_atomic_t stop = 0;


void handle_signal(int signum) {
    stop = 1;
}


const uint8_t smileyBitmap[16 * 16] = {
    // Row 0
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 1
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    // Row 2
    1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    // Row 3
    1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,
    // Row 4
    1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,
    // Row 5
    1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    // Row 6
    1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,
    // Row 7
    1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,
    // Row 8
    1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    // Row 9
    1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,
    // Row 10
    1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,
    // Row 11
    1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    // Row 12
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    // Row 13
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    // Row 14
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 15
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int main() {




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


    signal(SIGINT, handle_signal);

    IOHandler ioHandler;
    
    if (!ioHandler.begin()) {
        std::cerr << "Failed to initialize IOHandler.\n";
        return 1;
    }
       std::cout << "IOHandler initialized. Current volume: " << ioHandler.getVolume() << "\n";

  
    SPIDevice spiDevice(CS_PIN, 1000000, 0, 0, 0); // baudrate=1MHz for stability
    if (!spiDevice.begin()) {
        std::cerr << "Failed to initialize SPI Device.\n";
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        unlink("/tmp/stereo-shit.lock");
        return 1;
    }


    SSD1306_SPI display(128, 64, spiDevice, DC_PIN, RESET_PIN);
    if (!display.initDisplay()) {
        std::cerr << "Failed to initialize SSD1306 Display.\n";
        spiDevice.end();
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        unlink("/tmp/stereo-shit.lock");
        return 1;
    }
    display.setContrast(254);

    std::cout << "Display initialized. Drawing content...\n";

    // Clear the buffer
    display.fillRect(0, 0, display.width, display.height, false);

    // 1. Draw Scaled Text
    std::string scaledText = "SSD1306";
    float textScale = 2.0f; 
    uint8_t textWidth = 5 * scaledText.length() * textScale; 
    uint8_t textX = (display.width - textWidth) / 2;
    uint8_t textY = 5; // 5 pixels from the top
    display.drawText(textX, textY, scaledText, textScale);

    // 2. Draw Rectangle
    uint8_t rectWidth = 60;
    uint8_t rectHeight = 20;
    uint8_t rectX = (display.width - rectWidth) / 2;
    uint8_t rectY = textY + 10; // Below the text
    display.drawRect(rectX, rectY, rectWidth, rectHeight);

    // 3. Draw Filled Rectangle
    uint8_t filledRectWidth = 40;
    uint8_t filledRectHeight = 15;
    uint8_t filledRectX = (display.width - filledRectWidth) / 2;
    uint8_t filledRectY = rectY + rectHeight + 5; 
    display.fillRect(filledRectX, filledRectY, filledRectWidth, filledRectHeight, true);

    // 4. Draw Circle
    uint8_t circleRadius = 10;
    uint8_t circleX = 20;
    uint8_t circleY = display.height - 20;
    display.drawCircle(circleX, circleY, circleRadius);

    // 5. Draw Line
    uint8_t lineStartX = 0;
    uint8_t lineStartY = display.height - 1;
    uint8_t lineEndX = display.width - 1;
    uint8_t lineEndY = 0;
    display.drawLine(lineStartX, lineStartY, lineEndX, lineEndY);

    // 6. Draw Bitmap
    uint8_t bitmapX = display.width - 20;
    uint8_t bitmapY = display.height - 20; 
    display.drawBitmap(bitmapX, bitmapY, smileyBitmap, 16, 16); 

    
    display.show();
    std::cout << "Content displayed. Press Ctrl+C to exit.\n";


    while (!stop) {
        sleep(1); // Sleep to reduce CPU usage
            std::cout << "Volume: " << ioHandler.getVolume() << "\n";
    }



    std::cout << "Exiting gracefully.\n";
    ioHandler.end();
    spiDevice.end();  
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    unlink("/tmp/stereo-shit.lock");

    return 0;
}
