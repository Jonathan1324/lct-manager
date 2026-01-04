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

SRC_DIR := $(shell $(PWD))/$(SRC_DIR)
BUILD_DIR := $(shell $(PWD))/$(BUILD_DIR)
BIN_DIR = $(BUILD_DIR)

.PHONE: all clean

all: lct

lct:
	@"$(MAKE)" -C $(SRC_DIR)  				\
		DEBUG=$(DEBUG)						\
											\
		CC=$(CC) CFLAGS="$(CFLAGS)" 		\
		CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)"	\
		AR=$(AR) ARFLAGS=$(ARFLAGS)			\
		LDFLAGS="$(LDFLAGS)"				\
		STRIP="$(STRIP)"					\
		STRIPFLAGS="$(STRIPFLAGS)"			\
		RANLIB=$(RANLIB)					\
		SRC_DIR=$(SRC_DIR)  				\
		LIB=core							\
		BUILD_DIR=$(BUILD_DIR)  			\
		BIN_DIR=$(BIN_DIR)					\
		EXE_EXT=$(EXE_EXT)					\
		LICENSE_DIR="$(LICENSE_DIR)"		\
		TPL_TXT="$(LICENSE_DIR)/lasm.txt"	\
		$(ARGS_OS)

clean:
	@$(RM) $(BUILD_DIR)
