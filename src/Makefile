CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -Wextra -pedantic -I./SSD1309 -I./IO -I./UI -I./BT `pkg-config --cflags glib-2.0 gio-2.0`
LDFLAGS = -fsanitize=address -lbcm2835  -lgpiodcxx -lrt -pthread `pkg-config --libs glib-2.0 gio-2.0`
 
BUILD_DIR = build
SSD1309_DIR = SSD1309
IO_DIR = IO
BT_DIR = BT
UI_DIR = UI

SSD1309_SRCS = $(wildcard $(SSD1309_DIR)/*.cpp)
IO_SRCS = $(wildcard $(IO_DIR)/*.cpp)
BT_SRCS = $(wildcard $(BT_DIR)/*.cpp)
UI_SRCS = $(wildcard $(UI_DIR)/*.cpp)

SRCS = main.cpp $(SSD1309_SRCS) $(IO_SRCS) $(BT_SRCS) $(UI_SRCS)


SSD1309_OBJS = $(patsubst $(SSD1309_DIR)/%.cpp, $(BUILD_DIR)/SSD1309_%.o, $(SSD1309_SRCS))
IO_OBJS = $(patsubst $(IO_DIR)/%.cpp, $(BUILD_DIR)/IO_%.o, $(IO_SRCS))
BT_OBJS = $(patsubst $(BT_DIR)/%.cpp, $(BUILD_DIR)/BT_%.o, $(BT_SRCS))
UI_OBJS = $(patsubst $(UI_DIR)/%.cpp, $(BUILD_DIR)/UI_%.o, $(UI_SRCS))


MAIN_OBJ = $(BUILD_DIR)/main.o
OBJS = $(MAIN_OBJ) $(SSD1309_OBJS) $(IO_OBJS) $(BT_OBJS) $(UI_OBJS)

TARGET = $(BUILD_DIR)/FrogStereo

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(MAIN_OBJ): main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/SSD1309_%.o: $(SSD1309_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/IO_%.o: $(IO_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/BT_%.o: $(BT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/UI_%.o: $(UI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

debug:
	sudo rm -rf /var/run/pigpio.pid 
	sudo rm -rf /tmp/stereo-shit.lock
	sudo killall -9 FrogStereo
	sudo killall -9 gdb
	sudo gdb --args ./build/FrogStereo
