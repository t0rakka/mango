# ---------------------------------------------------------------------------
# mango makefile
#
# Copyright (C) 2012-2021 Twilight 3D Finland Oy Ltd.
# ---------------------------------------------------------------------------

# NOTE: These build scripts are for custom tinkering and hacking, please use
#       the cmake build system if you can.

# ---------------------------------------------------------------------------
# configuration
# ---------------------------------------------------------------------------

# build
ifeq ($(build),)
    OPTIONS = -DNDEBUG
else ifeq ($(build), release)
    OPTIONS = -DNDEBUG
else ifeq ($(build), debug)
    OPTIONS = -O0 -g
else
    $(error Incorrect option: build=$(build))
endif

# simd
ifeq ($(simd),)
    OPTIONS_X86   =
else ifeq ($(simd), sse2)
    OPTIONS_X86   = -msse2
else ifeq ($(simd), sse4)
    OPTIONS_X86   = -msse4
else ifeq ($(simd), avx)
    OPTIONS_X86   = -mavx
else ifeq ($(simd), avx2)
    OPTIONS_X86   = -mavx2
else ifeq ($(simd), avx512)
    OPTIONS_X86   = -mavx512dq -mavx512vl -mavx512bw
else
    $(error Incorrect option: simd=$(simd))
endif

# arch
ifeq ($(arch),)
    ARCH = $(shell uname -m)
else ifeq ($(arch), ppc64le)
    ARCH = ppc64le
else ifeq ($(arch), mips64)
    ARCH = mips64
else ifeq ($(arch), aarch64)
    ARCH = aarch64
else
    $(error Incorrect option: arch=$(arch))
endif

OS = $(shell uname)

# language versions
CC_STD = -std=c11
CPP_STD = -std=c++17

# common compiler options (LLVM/CLANG/GCC)
OPTIONS     += -c -Wall -O3 -ffast-math
OPTIONS_GCC = -ftree-vectorize

OPTIONS_X86 += -maes -mpclmul

# linker options after objects (gcc 4.9 workaround)
LINK_POST =

#NEON = -mfpu=neon
NEON = -mfpu=neon-fp16 -mfp16-format=ieee

# ---------------------------------------------------------------------------
# sources
# ---------------------------------------------------------------------------

LIBNAME_MANGO  = mango
LIBNAME_OPENGL = mango-opengl

INCLUDE_BASE = ../../include
SOURCE_BASE  = ../../source
OBJECTS_PATH  = objects

INCLUDE_WEBP = ../../source/external/libwebp
SOURCE_WEBP = external/libwebp/src/dec \
              external/libwebp/src/enc \
              external/libwebp/src/dsp \
              external/libwebp/src/demux \
              external/libwebp/src/mux \
              external/libwebp/src/utils

SOURCE_DIRS_MANGO  = mango/core \
                     mango/core/unix \
                     mango/filesystem \
                     mango/filesystem/unix \
                     mango/image \
                     mango/math \
                     mango/jpeg \
                     external/zstd/common \
                     external/zstd/compress \
                     external/zstd/decompress \
                     external/zpng \
                     external/lzfse \
                     external/bc \
                     external/google \
                     external/unrar \
                     external/libdeflate/lib \
                     external/libdeflate/lib/x86 \
                     external/libdeflate/lib/arm \
                     external/lz4 \
                     external/lzo \
                     external/bzip2 \
                     external/aes \
                     external/lzma \
                     $(SOURCE_WEBP)
SOURCE_DIRS_OPENGL = mango/opengl

# ---------------------------------------------------------------------------
# Linux
# ---------------------------------------------------------------------------

ifeq (Linux, $(OS))

  SOURCE_DIRS_OPENGL += mango/window/xlib
  SOURCE_DIRS_OPENGL += mango/opengl/glx

  LIBRARY_MANGO  = lib$(LIBNAME_MANGO).so
  LIBRARY_OPENGL = lib$(LIBNAME_OPENGL).so

  CLEAN    = rm -fr *.so $(OBJECTS_PATH)
  INSTALL  = cp *.so /usr/local/lib ; ldconfig ; rm -rf /usr/local/include/mango ; cp -r $(INCLUDE_BASE)/mango/ /usr/local/include/mango/
  LINK_POST += -lpthread -ldl

  ###############################################################################
  #
  # Intel x86-64
  #
  ###############################################################################

  ifeq (x86_64, $(ARCH))
    PLATFORM = x86-64
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -fPIC $(CC_STD)
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -fPIC -ansi $(CPP_STD)
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

  ###############################################################################
  #
  # Intel x86 32 bit
  #
  ###############################################################################

  ifeq (i686, $(ARCH))
    PLATFORM = x86
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) $(CC_STD)
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) $(OPTIONS_X86) -ansi $(CPP_STD)
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

  ###############################################################################
  #
  # ARM (Tested on Raspberry Pi)
  #
  ###############################################################################

  ifeq (armv6l, $(ARCH))
    PLATFORM = armv6l
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) -mfpu=vfp -mfloat-abi=hard
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) -mfpu=vfp -mfloat-abi=hard -ansi -std=c++0x
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

  ###############################################################################
  #
  # ARM (Tested on ODROID-U3, Raspberry Pi 3)
  #
  ###############################################################################

  ifeq (armv7l, $(ARCH))
    PLATFORM = armv7l
    CC  = gcc $(OPTIONS) $(OPTIONS_GCC) -marm -mcpu=cortex-a9 -mtune=cortex-a9 $(NEON) -mfloat-abi=hard -funsafe-math-optimizations -fPIC $(CC_STD)
    CPP = g++ $(OPTIONS) $(OPTIONS_GCC) -marm -mcpu=cortex-a9 -mtune=cortex-a9 $(NEON) -mfloat-abi=hard -funsafe-math-optimizations -fpermissive -Wno-psabi -fPIC -ansi $(CPP_STD)
    LINK_MANGO  = g++ -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = g++ -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

  ###############################################################################
  #
  # PowerPC
  #
  ###############################################################################

  # NOTE: cross-compiler

  ifeq (ppc64le, $(ARCH))
    PLATFORM = ppc64le
    CC  = powerpc64le-linux-gnu-gcc-5 $(OPTIONS) $(OPTIONS_GCC) -mcpu=power8 -funsafe-math-optimizations -fPIC $(CC_STD)
    CPP = powerpc64le-linux-gnu-g++-5 $(OPTIONS) $(OPTIONS_GCC) -mcpu=power8 -funsafe-math-optimizations -fpermissive -fPIC -ansi $(CPP_STD)
    LINK_MANGO  = powerpc64le-linux-gnu-g++-5 -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = powerpc64le-linux-gnu-g++-5 -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

  ###############################################################################
  #
  # MIPS
  #
  ###############################################################################

  # NOTE: cross-compiler

  ifeq (mips64, $(ARCH))
    PLATFORM = mips64
    CC  = mips64-linux-gnuabi64-gcc-5 $(OPTIONS) $(OPTIONS_GCC) -funsafe-math-optimizations -fPIC $(CC_STD)
    CPP = mips64-linux-gnuabi64-g++-5 $(OPTIONS) $(OPTIONS_GCC) -funsafe-math-optimizations -fpermissive -fPIC -ansi $(CPP_STD)
    CC += -mips64r5 -mfp64
    CPP += -mips64r5 -mfp64
    LINK_MANGO  = mips64-linux-gnuabi64-g++-5 -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = mips64-linux-gnuabi64-g++-5 -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

  ###############################################################################
  #
  # ARM64
  #
  ###############################################################################

  # NOTE: cross-compiler

  ifeq (aarch64, $(ARCH))
    PLATFORM = arm64
    CC  = aarch64-linux-gnu-gcc-5 $(OPTIONS) $(OPTIONS_GCC) -funsafe-math-optimizations -fPIC $(CC_STD)
    CPP = aarch64-linux-gnu-g++-5 $(OPTIONS) $(OPTIONS_GCC) -funsafe-math-optimizations -fpermissive -fPIC -ansi $(CPP_STD)
    LINK_MANGO  = aarch64-linux-gnu-g++-5 -s -shared -Wl,-soname,$(LIBRARY_MANGO) -o $(LIBRARY_MANGO)
    LINK_OPENGL = aarch64-linux-gnu-g++-5 -s -shared -Wl,-soname,$(LIBRARY_OPENGL) -o $(LIBRARY_OPENGL)
  endif

endif

# ---------------------------------------------------------------------------
# macOS
# ---------------------------------------------------------------------------

ifeq (Darwin, $(OS))

  SOURCE_DIRS_OPENGL += mango/opengl/cocoa
  SOURCE_DIRS_OPENGL += mango/window/cocoa

  # hack to create /usr/local (Xcode requires them and they don't exist by default)
  LOCAL    = mkdir -p /usr/local/lib ; mkdir -p /usr/local/include

  LIBRARY_MANGO  = lib$(LIBNAME_MANGO).dylib
  LIBRARY_OPENGL = lib$(LIBNAME_OPENGL).dylib
  PLATFORM = x86-64
  CC  = cc  $(OPTIONS) $(OPTIONS_X86) $(CC_STD) -mmacosx-version-min=10.13
  CPP = c++ $(OPTIONS) $(OPTIONS_X86) $(CPP_STD) -mmacosx-version-min=10.13 $(STD) -stdlib=libc++
  LINK_MANGO  = ld -o $(LIBRARY_MANGO)  -dylib -undefined dynamic_lookup -macosx_version_min 10.13
  LINK_OPENGL = ld -o $(LIBRARY_OPENGL) -dylib -undefined dynamic_lookup -macosx_version_min 10.13
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

# ---------------------------------------------------------------------------
# rules
# ---------------------------------------------------------------------------

all: $(LIBRARY_MANGO) $(LIBRARY_OPENGL)

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
	@$(CC) -I$(INCLUDE_WEBP) $< -o $@

$(LIBRARY_MANGO): $(OBJECTS_MANGO)
	@echo [Link $(PLATFORM)] $(LIBRARY_MANGO)
	@$(LINK_MANGO) $(OBJECTS_MANGO) $(LINK_POST)

$(LIBRARY_OPENGL): $(OBJECTS_OPENGL)
	@echo [Link $(PLATFORM)] $(LIBRARY_OPENGL)
	@$(LINK_OPENGL) $(OBJECTS_OPENGL) $(LINK_POST)

install:
	@echo [Install]
	@$(INSTALL)

clean:
	@echo [Clean]
	@$(CLEAN)
