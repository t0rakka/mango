#
# select toolchain
#
NDK_TOOLCHAIN_VERSION := clang

#
# select abi: armeabi-v7a / arm64-v8a / x86 / x86_64 / riscv64 / all
#
APP_ABI := all

#
# select platform
#
APP_PLATFORM := android-21

#
# select std: c++_static / c++_shared
#
APP_STL := c++_shared
APP_CPPFLAGS += -std=c++17
