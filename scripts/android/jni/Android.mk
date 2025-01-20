# -------------------------------------------------------------
# mango Android.mk
#
# Copyright (C) 2012-2024 Twilight 3D Finland Oy Ltd.
# -------------------------------------------------------------

# -------------------------------------------------------------
# sources
# -------------------------------------------------------------

LOCAL_PATH := $(call my-dir)/../../..
MANGO_INCLUDE := $(LOCAL_PATH)/include
MANGO_SOURCE := $(LOCAL_PATH)/source

INCLUDE_FASTGLTF := $(LOCAL_PATH)/source/external/fastgltf/include $(LOCAL_PATH)/source/external/simdjson

SOURCE_DIRS := mango/core \
               mango/core/unix \
               mango/filesystem \
               mango/filesystem/unix \
               mango/image  \
               mango/math \
               mango/opengl \
               mango/import3d \
               mango/jpeg \
               external/zstd/common \
               external/zstd/compress \
               external/zstd/decompress \
               external/zpng \
               external/lzfse \
               external/unrar \
               external/bc \
               external/astc \
               external/google \
               external/libdeflate/lib \
               external/libdeflate/lib/x86 \
               external/libdeflate/lib/arm \
               external/zlib \
               external/lcms \
               external/lz4 \
               external/bzip2 \
               external/aes \
               external/lzma \
               external/basisu/transcoder \
               external/fmt/src \
               external/simdjson \
               external/fastgltf/src \
               external/mikktspace

SOURCES := $(foreach dir,$(SOURCE_DIRS), \
                         $(wildcard $(MANGO_SOURCE)/$(dir)/*.cpp) \
                         $(wildcard $(MANGO_SOURCE)/$(dir)/*.cc) \
                         $(wildcard $(MANGO_SOURCE)/$(dir)/*.c) \
                         $(wildcard $(MANGO_SOURCE)/$(dir)/*.S) \
)
mango_sources := $(SOURCES:$(LOCAL_PATH)/%=%)

# -------------------------------------------------------------
# compiler options
# -------------------------------------------------------------

OPTIONS := -Wall -O3 -ffast-math

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    OPTIONS += "-mfpu=neon"
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_ARM_NEON := true
    OPTIONS += "-D__ARM_NEON__"
    # OPTIONS += "-march=armv8.1-a+crypto"
endif

ifeq ($(TARGET_ARCH_ABI),mips64)
    # OPTIONS += "-mmsa"
endif

ifeq ($(TARGET_ARCH_ABI),x86_64)
    # OPTIONS += "-mbmi"
    # OPTIONS += "-mbmi2"
    # OPTIONS += "-mlzcnt"
    # OPTIONS += "-msha"
    # OPTIONS += "-maes"
endif

# -------------------------------------------------------------
# Mango library modules
# -------------------------------------------------------------

include $(CLEAR_VARS)

#
# select static / shared library
#

#LOCAL_MODULE := mango_static
LOCAL_MODULE := mango_shared

LOCAL_SRC_FILES := $(mango_sources)
LOCAL_C_INCLUDES := $(MANGO_INCLUDE) $(INCLUDE_FASTGLTF)
LOCAL_EXPORT_C_INCLUDES := $(MANGO_INCLUDE)
LOCAL_CPP_FEATURES += exceptions
LOCAL_CPPFLAGS := $(OPTIONS) -Wno-extern-c-compat -Wno-\#pragma-messages -Wno-unused-variable
LOCAL_STATIC_LIBRARIES := cpufeatures

ifeq ($(LOCAL_MODULE),mango_static)
    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_LDLIBS := -lGLESv2
    include $(BUILD_SHARED_LIBRARY)
endif

$(call import-module,android/cpufeatures)
