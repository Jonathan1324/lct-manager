CC = gcc
CXX = g++
AR = ar
RANLIB = ranlib
STRIP = strip

ARFLAGS = r

# Debug-Flag: 1 = Debug, 0 = Release
DEBUG ?= 0

VERSION ?= commit

# Basis-Flags
INCLUDE_FLAGS = -I$(abspath src/libs/core) -I$(abspath src/libs/rust)
COMMON_WARNINGS = -Wall -Wextra
OPT_FLAGS = -O2
DEBUG_FLAGS = -g
RELEASE_FLAGS = -DNDEBUG
SECURITY_FLAGS = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIC

CFLAGS = $(INCLUDE_FLAGS) $(COMMON_WARNINGS)
CXXFLAGS = $(INCLUDE_FLAGS) -std=c++17 $(COMMON_WARNINGS)
LDFLAGS =
RUSTLIBFLAGS = --crate-type staticlib
RUSTFLAGS =
STRIPFLAGS =

STATIC_FLAGS = -static -static-libgcc -static-libstdc++

ifeq ($(OS_NAME),linux)
	LDFLAGS += -Wl,--gc-sections
	CFLAGS += -ffunction-sections -fdata-sections
	CXXFLAGS += -ffunction-sections -fdata-sections
	STRIPFLAGS += --strip-unneeded
endif
ifeq ($(OS_NAME),macos)
    LDFLAGS += -lpthread -lm -lc++
	STRIPFLAGS += -x -S
endif

ifeq ($(DEBUG),0)
	CFLAGS += -DVERSION=\"$(VERSION)\"
	CXXFLAGS += -DVERSION=\"$(VERSION)\"

	ifneq ($(OS_NAME),macos)
		LDFLAGS += $(STATIC_FLAGS)
	endif
endif

ifeq (${DEBUG},1)
	CFLAGS += $(DEBUG_FLAGS)
	CXXFLAGS += $(DEBUG_FLAGS)
	RUSTFLAGS += -C opt-level=0
else
	CFLAGS += $(OPT_FLAGS) $(RELEASE_FLAGS)
	CXXFLAGS += $(OPT_FLAGS) $(RELEASE_FLAGS) $(SECURITY_FLAGS)
	RUSTFLAGS += -C opt-level=3
endif

ifeq ($(OS_NAME),macos)
	CC = clang
	CXX = clang++
	
	CFLAGS += -arch $(ARCH)
	CXXFLAGS += -arch $(ARCH)
else
#	ARM
#	ifneq (,$(filter arm64 aarch64,$(UNAME_M)))
#		CFLAGS += -march=armv8-a
#		CXXFLAGS += -march=armv8-a
#	else ifneq (,$(filter armv7l armhf,$(UNAME_M)))
#		CFLAGS += -march=armv7-a -mfloat-abi=hard -mfpu=neon
#		CXXFLAGS += -march=armv7-a -mfloat-abi=hard -mfpu=neon
#	else ifneq (,$(filter armv6l,$(UNAME_M)))
#		CFLAGS += -march=armv6 -mfloat-abi=hard -mfpu=vfp
#		CXXFLAGS += -march=armv6 -mfloat-abi=hard -mfpu=vfp

#	x86
# 	else ifneq (,$(filter x86_64,$(UNAME_M)))
#		CFLAGS += -m64
#		CXXFLAGS += -m64
#	else ifneq (,$(filter i686 i386,$(UNAME_M)))
#		CFLAGS += -m32
#		CXXFLAGS += -m32

#	Unknown
#	else
#		$(warning Unknown architecture: $(UNAME_M))
#	endif
endif
