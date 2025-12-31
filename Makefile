ifndef OS_NAME
$(error OS_NAME is not set. Must be one of: windows, macos, linux)
endif

ifndef ARCH
$(error ARCH is not set. Must be one of: x86_64, arm64)
endif

include scripts/config.mk
include scripts/os.mk

SRC_DIR = src
DIST_DIR = dist

ifeq ($(DEBUG),1)
	BUILD_DIR = build/debug
else
	BUILD_DIR = build/release
endif

SRC_DIR := $(shell pwd)/$(SRC_DIR)
BUILD_DIR := $(shell pwd)/$(BUILD_DIR)
DIST_DIR := $(shell pwd)/$(DIST_DIR)
BIN_DIR = $(DIST_DIR)/bin

# FILES
C_SRCS = $(shell find $(SRC_DIR) -name '*.c' | sed "s|$(SRC_DIR)/||")
C_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.c.o, $(C_SRCS))

CPP_SRCS = $(shell find $(SRC_DIR) -name '*.cpp' | sed "s|$(SRC_DIR)/||")
CPP_OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.cpp.o, $(CPP_SRCS))

CFLAGS	 += -I$(SRC_DIR)
CXXFLAGS += -I$(SRC_DIR)

DEPS := $(C_OBJS:.o=.d) $(CPP_OBJS:.o=.d)
TARGET = $(BIN_DIR)/lct$(EXE_EXT)

.PHONE: all clean

all: lct

lct: $(TARGET)

# Compile C files
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@echo "Compiling " $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@echo "Compiling  " $<
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MMD -MF $(@:.o=.d) -c $< -o $@

# link files
$(TARGET): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(wildcard $(LIB_DIR)/*.a)
	@echo "Linking " $@
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
ifeq (${DEBUG},0)
	@strip $(STRIPFLAGS) $@
endif

-include $(DEPS)

clean:
	@rm -rf $(BUILD_DIR)
