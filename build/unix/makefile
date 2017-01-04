# ---------------------------------------------------------------------------
# mango makefile
#
# Copyright (C) 2012-2017 Twilight 3D Finland Oy Ltd.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# configuration
# ---------------------------------------------------------------------------

LIBNAME_MANGO  = mango
LIBNAME_OPENGL = mango-opengl
LIBNAME_VULKAN = mango-vulkan

INCLUDE_BASE = ../../include
SOURCE_BASE  = ../../source
OBJECTS_PATH  = objects

SOURCE_DIRS_MANGO  = mango/core \
                     mango/core/unix \
                     mango/filesystem \
                     mango/filesystem/unix \
                     mango/image \
                     mango/math \
                     zstd/common \
                     zstd/compress \
                     zstd/decompress \
                     lzfse \
                     jpeg bc google unrar miniz lz4 lzo bzip2
SOURCE_DIRS_OPENGL = mango/opengl
SOURCE_DIRS_VULKAN = mango/vulkan

# language versions
CC_STD = -std=c11
CPP_STD = -std=c++14

# common compiler options (LLVM/CLANG/GCC)
OPTIONS       = -c -Wall -O3 -ffast-math
OPTIONS_GCC   = -ftree-vectorize
#OPTIONS_X86   = -msse4
OPTIONS_X86   = -mavx

# linker options after objects (gcc 4.9 workaround)
LINK_POST     =

#NEON          = -mfpu=neon
NEON          = -mfpu=neon-fp16 -mfp16-format=ieee

OS   = $(shell uname)
ARCH = $(shell uname -m)

# ---------------------------------------------------------------------------
# Linux
# ---------------------------------------------------------------------------

ifeq (Linux, $(OS))

  SOURCE_DIRS_OPENGL += mango/gui/xlib
  SOURCE_DIRS_OPENGL += mango/opengl/glx

  SOURCE_DIRS_VULKAN += mango/gui/xcb

  LIBRARY_MANGO  = lib$(LIBNAME_MANGO).so
  LIBRARY_OPENGL = lib$(LIBNAME_OPENGL).so
  LIBRARY_VULKAN = lib$(LIBNAME_VULKAN).so
  CLEAN    = rm -fr *.so $(OBJECTS_PATH)
  INSTALL  = cp *.so /usr/lib ; ldconfig ; rm -rf /usr/include/mango ; cp -r $(INCLUDE_BASE)/mango/ /usr/include/mango/
  LINK_POST += -lpthread -ldl

  # Intel x86 64 bit GCC
  ifeq (x86_64, $(ARCH))
    PLATFORM = x86-64
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -fPIC $(CC_STD)
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -fPIC -ansi $(CPP_STD)
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
    LINK_VULKAN = g++ -s -shared -Wl,-soname,$(LIBRARY_VULKAN) -o $(LIBRARY_VULKAN)
  endif

  # Intel x86 32 bit GCC
  ifeq (i686, $(ARCH))
    PLATFORM = x86
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -g $(CC_STD)
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -g -ansi $(CPP_STD)
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
    LINK_VULKAN = g++ -s -shared -Wl,-soname,$(LIBRARY_VULKAN) -o $(LIBRARY_VULKAN)
  endif

  # ARM GCC (Tested on Raspberry Pi)
  ifeq (armv6l, $(ARCH))
    PLATFORM = armv6l
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) -g -mfpu=vfp -mfloat-abi=hard
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) -g -mfpu=vfp -mfloat-abi=hard -ansi -std=c++0x
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
    LINK_VULKAN = g++ -s -shared -Wl,-soname,$(LIBRARY_VULKAN) -o $(LIBRARY_VULKAN)
  endif

  # ARM GCC (Tested on ODROID-U3, Raspberry Pi 3)
  ifeq (armv7l, $(ARCH))
    PLATFORM = armv7l
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) -g -marm -mcpu=cortex-a9 -mtune=cortex-a9 $(NEON) -mfloat-abi=hard -funsafe-math-optimizations -fPIC $(CC_STD)
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) -g -marm -mcpu=cortex-a9 -mtune=cortex-a9 $(NEON) -mfloat-abi=hard -funsafe-math-optimizations -fPIC -ansi $(CPP_STD)
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
    LINK_VULKAN = g++ -s -shared -Wl,-soname,$(LIBRARY_VULKAN) -o $(LIBRARY_VULKAN)
  endif

endif

# ---------------------------------------------------------------------------
# Mac OS X
# ---------------------------------------------------------------------------

ifeq (Darwin, $(OS))

  SOURCE_DIRS_OPENGL += mango/opengl/cocoa

  # hack to create /usr/local (Xcode requires them and they don't exist by default)
  LOCAL    = mkdir -p /usr/local/lib ; mkdir -p /usr/local/include

  LIBRARY_MANGO  = lib$(LIBNAME_MANGO).dylib
  LIBRARY_OPENGL = lib$(LIBNAME_OPENGL).dylib
  LIBRARY_VULKAN =
  PLATFORM = x86-64
  CC  = clang $(OPTIONS) $(CC_STD) -msse4 -mmacosx-version-min=10.7
  CPP = clang++ $(OPTIONS) $(CPP_STD) -msse4 -mmacosx-version-min=10.7 $(STD) -stdlib=libc++
  LINK_MANGO  = libtool -dynamic -o $(LIBRARY_MANGO) -undefined dynamic_lookup -macosx_version_min 10.7
  LINK_OPENGL = libtool -dynamic -o $(LIBRARY_OPENGL) -undefined dynamic_lookup -macosx_version_min 10.7
  LINK_VULKAN = libtool -dynamic -o $(LIBRARY_VULKAN) -undefined dynamic_lookup -macosx_version_min 10.7
  INSTALL  = $(LOCAL) ; cp *.dylib /usr/local/lib ; cp -r $(INCLUDE_BASE)/mango/ /usr/local/include/mango/
  CLEAN    = rm -fr $(OBJECTS_PATH) *.dylib so_locations

endif

# ---------------------------------------------------------------------------
# objects
# ---------------------------------------------------------------------------

# mango

OBJECTS_MANGO += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.cpp,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_MANGO), $(wildcard $(SOURCE_BASE)/$(dir)/*.cpp)))))

OBJECTS_MANGO += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.c,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_MANGO), $(wildcard $(SOURCE_BASE)/$(dir)/*.c)))))

OBJECTS_MANGO += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.mm,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_MANGO), $(wildcard $(SOURCE_BASE)/$(dir)/*.mm)))))

# opengl

OBJECTS_OPENGL += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.cpp,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_OPENGL), $(wildcard $(SOURCE_BASE)/$(dir)/*.cpp)))))

OBJECTS_OPENGL += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.c,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_OPENGL), $(wildcard $(SOURCE_BASE)/$(dir)/*.c)))))

OBJECTS_OPENGL += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.mm,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_OPENGL), $(wildcard $(SOURCE_BASE)/$(dir)/*.mm)))))

# vulkan

OBJECTS_VULKAN += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.cpp,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_VULKAN), $(wildcard $(SOURCE_BASE)/$(dir)/*.cpp)))))

OBJECTS_VULKAN += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.c,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_VULKAN), $(wildcard $(SOURCE_BASE)/$(dir)/*.c)))))

OBJECTS_VULKAN += $(addprefix $(OBJECTS_PATH)/, $(patsubst %.mm,%.o, \
    $(abspath $(foreach dir, $(SOURCE_DIRS_VULKAN), $(wildcard $(SOURCE_BASE)/$(dir)/*.mm)))))

# ---------------------------------------------------------------------------
# rules
# ---------------------------------------------------------------------------

all: $(LIBRARY_MANGO) $(LIBRARY_OPENGL) $(LIBRARY_VULKAN)

$(OBJECTS_PATH)/%.o: %.mm
	@echo [Compile OBJC] $<
	@-mkdir -p $(@D)
	@$(CPP) -I$(INCLUDE_BASE) $< -o $@

$(OBJECTS_PATH)/%.o: %.cpp
	@echo [Compile C++] $<
	@-mkdir -p $(@D)
	@$(CPP) -I$(INCLUDE_BASE) $< -o $@

$(OBJECTS_PATH)/%.o: %.c
	@echo [Compile C] $<
	@-mkdir -p $(@D)
	@$(CC) $< -o $@

$(LIBRARY_MANGO): $(OBJECTS_MANGO)
	@echo [Link $(PLATFORM)] $(LIBRARY_MANGO)
	@$(LINK_MANGO) $(OBJECTS_MANGO) $(LINK_POST)

$(LIBRARY_OPENGL): $(OBJECTS_OPENGL)
	@echo [Link $(PLATFORM)] $(LIBRARY_OPENGL)
	@$(LINK_OPENGL) $(OBJECTS_OPENGL) $(LINK_POST)

$(LIBRARY_VULKAN): $(OBJECTS_VULKAN)
	@echo [Link $(PLATFORM)] $(LIBRARY_VULKAN)
	@$(LINK_VULKAN) $(OBJECTS_VULKAN) $(LINK_POST)

install:
	@echo [Install]
	@$(INSTALL)

clean:
	@echo [Clean]
	@$(CLEAN)
