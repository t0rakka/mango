
# MANGO Installation Guide


## Background

A long time ago MANGO was self-contained source tree where all external libraries were vendored, which means the source code was included. You clone the repo and compile. This not the way anymore because it means a lot of maintenance work encumbers the developer. The external libraries will be installed by package managers and this varies between platforms, the methods we document below are not the only ones but should get you going.

The external libraries are divided into three categories: REQUIRED libraries MUST be installed. OPTIONAL libraries can be disabled by the build script options and they are also disabled if the libraries cannot be found. The EXAMPLE libraries must be installed if examples are going to be compiled (image codec benchmarks specifically).


## Linux

There are different package managers but our examples use apt-get (Ubuntu, Mint, etc.).

### REQUIRED Libraries

    sudo apt-get install mesa-common-dev libgl1-mesa-dev zlib1g-dev libdeflate-dev libzstd-dev liblz4-dev liblcms2-dev

### OPTIONAL Libraries

    sudo apt-get install libjxl-dev libopenjp2-7-dev libwebp-dev libavif-dev libheif-dev libisal-dev

### EXAMPLE Libraries

    sudo apt-get install libjpeg-dev libpng-dev

### Building

Building on Linux is fairly straightforward; generate build system scripts, run them, install.

    cmake -S . -B build
    cd build
    ninja
    sudo ninja install

You're ready to go. The default generator is "make" as cmake users are familiar, but ninja is a very good alternative. Here is example how to configure the build to use ninja and as extra bonus enable all optional Intel ISA instructions, it is just cmake option that enables other options.

    cmake -S . -B build -G "Ninja" -DINTEL_DELUXE=ON


## macOS

### REQUIRED Libraries

    brew install zlib libdeflate zstd

### OPTIONAL Libraries

    brew install jpeg-xl openjpeg webp libavif libheif isa-l lz4 lcms2

### EXAMPLE Libraries

    brew install libjpeg-turbo libpng

### Building

On macOS the building is exactly same as it is on Linux.


## Windows

Windows is different and long story short the vcpkg is the supported method to get the libraries. If you are new to vcpkg just install it using Microsoft's official installation instructions. It is recommended that VCPKG_DEFAULT_TRIPLET environment variable is set, while not necessary this way you avoid having to type it all the time everywhere.

Here are the environment variables that are needed, the x64-windows is just example, x86 32-bit triplet also works.

    VCPKG_DEFAULT_TRIPLET   x64-windows
    VCPKG_ROOT              <vcpkg_root>
    path: <vcpkg_root>\installed\x64-windows\bin
          <vcpkg_root>

### REQUIRED Libraries

    vcpkg install pkgconf zlib libdeflate zstd lz4 lcms

### OPTIONAL Libraries

    vcpkg install libjxl openjpeg libwebp libavif libheif isal

### EXAMPLE Libraries

    vcpkg install libjpeg-turbo libpng

### Building

Here's the part where VCPKG_ROOT environment variable comes in handy; you don't need to manually type the path where VCPKG is installed on your system. The first cmake call generates the Visual Studio solution files into the build\ directory. Once you go into the directory you can open the mango.sln file or keep using the command line to compile and install the library.

    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DINTEL_DELUXE=ON
    cd build
    cmake --build . --config Release
    cmake --build . --config Release --target install

If you are not sure where the MANGO was installed, you can write "cmake .." in the build directory and the target directory will be printed into the console.

Windows build generator is so-called "multi generator" where there are different configurations and it is recommended you use --config parameter to choose one you want to build. It works a bit differently than UNIX builds where the selection mechanism is different.

The INTEL_DELUXE option configures the build to use all available special instructions the library supports. The ISA extensions can also be configured individually, for example, 32-bit build often doesn't want everything included but cherry pick only instructions the developer wants to use.
