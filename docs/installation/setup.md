<h1><img src="../mango-logo.png" alt="logo" width="80"/> MANGO Installation Guide</h1>


## Background

A long time ago MANGO was self-contained source tree where all external libraries were included for maximum comfort. You clone the repo and compile. This is not the way anymore because it means a lot of maintenance work encumbers the developer. The external libraries will be installed by package managers and this varies between platforms, the methods we document below are not the only ones but should get you going.

The external libraries are divided into three categories: REQUIRED libraries MUST be installed. OPTIONAL libraries can be disabled by the build script options and they are also disabled if the libraries cannot be found. The EXAMPLE libraries must be installed if examples are going to be compiled (image codec benchmarks specifically).


<h2><img src="logo-linux.png" alt="logo" width="80"/> Linux</h2>


There are different package managers but here we use apt-get (Debian, Ubuntu, Mint, etc.)

### Dependencies

    sudo apt-get install libfmt-dev zlib1g-dev libdeflate-dev libzstd-dev liblcms2-dev libjxl-dev libopenjp2-7-dev libwebp-dev libavif-dev libheif-dev libraw-dev libisal-dev liblz4-dev libbz2-dev libjxr-dev mesa-common-dev libgl1-mesa-dev glslang-dev libfreetype-dev libharfbuzz-dev libsimdjson-dev libjpeg-dev libpng-dev

### Building

Building on Linux is fairly straightforward; generate build system scripts, run them, install:

    cmake -S . -B build -G "Ninja"
    cd build
    ninja
    sudo ninja install

Above uses ninja as build system, cmake users know what time it is. If you want to use the default (make) just omit the -G "Ninja" parameter. Configure the cmake options before building to tune the library size and dependencies to your taste.


<h2><img src="logo-archlinux.png" alt="logo" width="80"/> Arch Linux</h2>


### Dependencies

    sudo pacman -S fmt z libdeflate zstd lcms2 libjxl openjpeg2 libwebp libavif libheif libraw isa-l lz4 bzip2 jxrlib mesa glslang freetype2 harfbuzz simdjson libjpeg-turbo libpng

### Building

On Arch Linux the building is exactly same as it is on Linux Debian/Ubuntu/Mint.


<h2><img src="logo-apple.png" alt="logo" width="80"/> macOS</h2>


### Dependencies

    brew install fmt zlib libdeflate zstd lcms2 jpeg-xl openjpeg webp libavif libheif libraw isa-l lz4 bzip2 jxrlib freetype harfbuzz simdjson libjpeg-turbo libpng

### Building

On macOS the building is exactly same as it is on Linux.


<h2><img src="logo-emscripten.png" alt="logo" width="80"/> Emscripten</h2>


The Emscripten build is still work-in-progress but is partially working. The recommended way is to install dependencies using vcpkg, the preset builds examples so the libraries usually in examples are included in the required libraries below. The target is currently node and the cmake configuration builds specifically for node with native filesystem access enabled for testing purposes.

### Dependencies

    vcpkg install --triplet wasm32-emscripten fmt zlib libdeflate zstd lcms libjpeg-turbo libpng


<h2><img src="logo-windows.png" alt="logo" width="80"/> Windows</h2>


Install [vcpkg](https://vcpkg.io/en/getting-started.html), then set:

    VCPKG_ROOT              <vcpkg_root>
    VCPKG_DEFAULT_TRIPLET   x64-windows

Dependencies come from `vcpkg.json` (manifest mode). The Windows presets enable all features (`base`, `image`, `vulkan`, `examples`) so configure installs everything needed.

### Building

    cmake --preset windows
    cmake --build --preset windows-release
    cmake --install build

Ninja alternatives: `windows-ninja-release` / `windows-ninja-debug`.

First configure can take a while while vcpkg builds dependencies into the build tree. Fine-grained control of what mango itself compiles is still via CMake options in `CMakeLists.txt`.

If you are not sure where mango was installed, re-run configure in the build directory and the install prefix is printed to the console.

#### INTEL_DELUXE

The INTEL_DELUXE option uses all the latest ISA extensions except AVX-512. When targeting older processors it is recommended to cherry pick the extensions.

#### WSL

If you are compiling on Windows WSL there is an issue with cmake find_package() not working correctly when VCPKG packages are installed. This is because the WSL adds windows PATH to it's path on startup. This can be mitigated by adding the following lines into /etc/wsl.conf file:

    [interop]
    appendWindowsPath = false

Don't forget to restart the WSL; "wsl --shutdown" for the setting to take effect.
