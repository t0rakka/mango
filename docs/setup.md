<h1><img src="mango-logo.png" alt="logo" width="80"/> MANGO Installation Guide</h1>


## Background

A long time ago MANGO was self-contained source tree where all external libraries were included for maximum comfort. You clone the repo and compile. This is not the way anymore because it means a lot of maintenance work encumbers the developer. The external libraries will be installed by package managers and this varies between platforms, the methods we document below are not the only ones but should get you going.

The external libraries are divided into three categories: REQUIRED libraries MUST be installed. OPTIONAL libraries can be disabled by the build script options and they are also disabled if the libraries cannot be found. The EXAMPLE libraries must be installed if examples are going to be compiled (image codec benchmarks specifically).


<h2><img src="logo-linux.png" alt="logo" width="80"/> Linux</h2>


There are different package managers but our examples use apt-get (Ubuntu, Mint, etc.).

### REQUIRED Libraries

    sudo apt-get install mesa-common-dev libgl1-mesa-dev zlib1g-dev libdeflate-dev libzstd-dev liblz4-dev liblcms2-dev

### OPTIONAL Libraries

    sudo apt-get install libjxl-dev libopenjp2-7-dev libwebp-dev libavif-dev libheif-dev libisal-dev

### EXAMPLE Libraries

    sudo apt-get install libjpeg-dev libpng-dev

### Building

Building on Linux is fairly straightforward; generate build system scripts, run them, install.

    cmake -S . -B build -G "Ninja"
    cd build
    ninja
    sudo ninja install

Above uses ninja as build system, cmake users know what time it is. If you want to use the default (make) just omit the -G "Ninja" parameter. You're ready to go. 


<h2><img src="logo-apple.png" alt="logo" width="80"/> macOS</h2>


### REQUIRED Libraries

    brew install zlib libdeflate zstd

### OPTIONAL Libraries

    brew install jpeg-xl openjpeg webp libavif libheif isa-l lz4 lcms2

### EXAMPLE Libraries

    brew install libjpeg-turbo libpng

### Building

On macOS the building is exactly same as it is on Linux.


<h2><img src="logo-windows.png" alt="logo" width="80"/> Windows</h2>


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
    cmake --build . --config Release --parallel
    cmake --install .

If you are not sure where the MANGO was installed, you can write "cmake .." in the build directory and the target directory will be printed into the console.

Windows build generator is so-called "multi generator" where there are different configurations and it is recommended you use --config parameter to choose one you want to build. It works a bit differently than UNIX builds where the selection mechanism is different.

#### INTEL_DELUXE

The INTEL_DELUXE option uses all the latest ISA extensions except AVX-512. It does no harm giving it on ARM or other platform build, but does break backward compatiblity with older processors as it might use instructions in generated code that the CPU cannot support.

#### Examples

The examples won't either find the mango.dll, or use a previously installed one unless the install build command is executed before the examples. Windows aims to please by being different; just get used to it.

### WSL

If you are compiling on Windows WSL there is an issue with cmake find_package() not working correctly when VCPKG packages are installed. This is because the WSL adds windows PATH to it's path on startup. This can be mitigated by adding the following lines into /etc/wsl.conf file:

    [interop]
    appendWindowsPath = false

Don't forget to restart the WSL; "wsl --shutdown" for the setting to take effect.
