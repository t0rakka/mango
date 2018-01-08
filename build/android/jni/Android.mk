# -------------------------------------------------------------
# mango Android.mk
#
# Copyright (C) 2012-2016 Twilight 3D Finland Oy Ltd.
# -------------------------------------------------------------

# -------------------------------------------------------------
# sources
# -------------------------------------------------------------

LOCAL_PATH := $(call my-dir)/../../..
MANGO_INCLUDE := $(LOCAL_PATH)/include
MANGO_SOURCE := $(LOCAL_PATH)/source

SOURCE_DIRS := mango/core \
               mango/core/unix \
               mango/filesystem \
               mango/filesystem/unix \
               mango/image  \
               mango/math \
               mango/opengl \
               mango/jpeg \
               external/zstd/common \
               external/zstd/compress \
               external/zstd/decompress \
               external/lzfse \
               external/unrar \
               external/bc \
               external/google \
               external/miniz \
               external/lz4 \
               external/lzo \
               external/bzip2

SOURCES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(MANGO_SOURCE)/$(dir)/*.cpp) $(wildcard $(MANGO_SOURCE)/$(dir)/*.c))
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
LOCAL_C_INCLUDES := $(MANGO_INCLUDE)
LOCAL_EXPORT_C_INCLUDES := $(MANGO_INCLUDE)
LOCAL_CPP_FEATURES += exceptions
LOCAL_CPPFLAGS := $(OPTIONS) -Wno-extern-c-compat 
LOCAL_STATIC_LIBRARIES := cpufeatures

ifeq ($(LOCAL_MODULE),mango_static)
    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_LDLIBS := -lGLESv2
    include $(BUILD_SHARED_LIBRARY)
endif

$(call import-module,android/cpufeatures)
