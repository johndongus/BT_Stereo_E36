# Makefile

# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -O2 -I./SSD1306 -I./IO -I./UI -I./BT
LDFLAGS = -lbcm2835 -lpigpio -lrt -pthread

# Directories
BUILD_DIR = build
SSD1306_DIR = SSD1306
IO_DIR = IO
BT_DIR = BT
UI_DIR = UI

# Source Files using Wildcards
SSD1306_SRCS = $(wildcard $(SSD1306_DIR)/*.cpp)
IO_SRCS = $(wildcard $(IO_DIR)/*.cpp)
BT_SRCS = $(wildcard $(BT_DIR)/*.cpp)
UI_SRCS = $(wildcard $(UI_DIR)/*.cpp)

SRCS = main.cpp $(SSD1306_SRCS) $(IO_SRCS) $(BT_SRCS) $(UI_SRCS)

# Object Files using Wildcards
SSD1306_OBJS = $(patsubst $(SSD1306_DIR)/%.cpp, $(BUILD_DIR)/SSD1306_%.o, $(SSD1306_SRCS))
IO_OBJS = $(patsubst $(IO_DIR)/%.cpp, $(BUILD_DIR)/IO_%.o, $(IO_SRCS))
BT_OBJS = $(patsubst $(BT_DIR)/%.cpp, $(BUILD_DIR)/BT_%.o, $(BT_SRCS))
UI_OBJS = $(patsubst $(UI_DIR)/%.cpp, $(BUILD_DIR)/UI_%.o, $(UI_SRCS))

# main.cpp to build/main.o
MAIN_OBJ = $(BUILD_DIR)/main.o

# Combine all object files
OBJS = $(MAIN_OBJ) $(SSD1306_OBJS) $(IO_OBJS) $(BT_OBJS) $(UI_OBJS)

# Target Executable
TARGET = $(BUILD_DIR)/ssd1306_spi

# Default Rule
all: $(BUILD_DIR) $(TARGET)

# Create Build Directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile main.cpp
$(MAIN_OBJ): main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile SSD1306/*.cpp to build/SSD1306_*.o
$(BUILD_DIR)/SSD1306_%.o: $(SSD1306_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile IO/*.cpp to build/IO_*.o
$(BUILD_DIR)/IO_%.o: $(IO_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile BT/*.cpp to build/BT_*.o
$(BUILD_DIR)/BT_%.o: $(BT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile UI/*.cpp to build/UI_*.o
$(BUILD_DIR)/UI_%.o: $(UI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link all object files into the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Clean Build Directory
clean:
	rm -rf $(BUILD_DIR)
