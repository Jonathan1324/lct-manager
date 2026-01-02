C_SRCS = $(shell $(LIST) $(SRC_DIR) "*.c")
C_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.c.o, $(C_SRCS))

CPP_SRCS = $(shell $(LIST) $(SRC_DIR) "*.cpp")
CPP_OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.cpp.o, $(CPP_SRCS))

DEPS := $(C_OBJS:.o=.d) $(CPP_OBJS:.o=.d)

TARGET = $(BUILD_DIR)/lct$(EXE_EXT)

CFLAGS	 += -I$(SRC_DIR)
CXXFLAGS += -I$(SRC_DIR)

.PHONY: all

all: $(TARGET) record

# Compile C files
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@echo "Compiling " $<
	@$(MKDIR) $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@echo "Compiling  " $<
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -MMD -MF $(@:.o=.d) -c $< -o $@

# link files
$(TARGET): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(wildcard $(LIB_DIR)/*.a)
	@echo "Linking " $@
	@$(MKDIR) $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
ifeq (${DEBUG},0)
	@$(STRIP) $(STRIPFLAGS) $@
endif

-include $(DEPS)

record:
	@$(APPEND) $(TARGET) $(BIN_DIR)/binaries.txt
