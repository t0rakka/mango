#
# select toolchain: clang / 4.9
#
NDK_TOOLCHAIN_VERSION := clang

#
# select abi: armeabi / armeabi-v7a / arm64-v8a / x86 / x86_64 / mips / mips64 / all
# broken:     -------                                            ----            ---
#
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

#
# select platform
#
APP_PLATFORM := android-21

#
# select stl: c++_static / c++_shared
#
APP_STL := c++_shared
APP_CPPFLAGS += -std=c++17
