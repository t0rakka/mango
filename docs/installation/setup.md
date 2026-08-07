<h1><img src="../mango-logo.png" alt="logo" width="80"/> MANGO Installation Guide</h1>


## Background

A long time ago MANGO was self-contained source tree where all external libraries were included for maximum comfort. You clone the repo and compile. This is not the way anymore because it means a lot of maintenance work encumbers the developer. The external libraries will be installed by package managers and this varies between platforms, the methods we document below are not the only ones but should get you going.

The external libraries are divided into three categories: REQUIRED libraries MUST be installed. OPTIONAL libraries can be disabled by the build script options and they are also disabled if the libraries cannot be found. The EXAMPLE libraries must be installed if examples are going to be compiled (image codec benchmarks specifically).


<h2><img src="logo-linux.png" alt="logo" width="80"/> Debian / Ubuntu (apt)</h2>


Debian, Ubuntu, Mint, Raspberry Pi OS, and other apt-based distros.

### Dependencies

    sudo apt-get install libfmt-dev zlib1g-dev libdeflate-dev libzstd-dev liblcms2-dev libjxl-dev libopenjp2-7-dev libwebp-dev libavif-dev libheif-dev libraw-dev libisal-dev liblz4-dev libbz2-dev libjxr-dev mesa-common-dev libgl1-mesa-dev glslang-dev libfreetype-dev libharfbuzz-dev libsimdjson-dev libjpeg-dev libpng-dev


<h2><img src="logo-archlinux.png" alt="logo" width="80"/> Arch / pacman</h2>


Arch Linux and other pacman-based distros.

### Dependencies

    sudo pacman -S fmt z libdeflate zstd lcms2 libjxl openjpeg2 libwebp libavif libheif libraw isa-l lz4 bzip2 jxrlib mesa glslang freetype2 harfbuzz simdjson libjpeg-turbo libpng


### Building (Linux)

    cmake -S . -B build -G "Ninja"
    cd build
    ninja
    sudo ninja install

Above uses ninja as build system, cmake users know what time it is. If you want to use the default (make) just omit the -G "Ninja" parameter. Configure the cmake options before building to tune the library size and dependencies to your taste.


<h2><img src="logo-apple.png" alt="logo" width="80"/> macOS</h2>


### Dependencies

    brew install fmt zlib libdeflate zstd lcms2 jpeg-xl openjpeg webp libavif libheif libraw isa-l lz4 bzip2 jxrlib freetype harfbuzz simdjson libjpeg-turbo libpng

### Building

On macOS the building is exactly same as it is on Linux.


<h2><img src="logo-windows.png" alt="logo" width="80"/> Windows</h2>


Install [vcpkg](https://vcpkg.io/en/getting-started.html), then set:

    VCPKG_ROOT              <vcpkg_root>
    VCPKG_DEFAULT_TRIPLET   x64-windows

### Dependencies

**Classic mode** (`cmake --preset vcpkg`) — install packages once into the global vcpkg tree; configure stays fast. Re-run when `vcpkg.json` changes:

    vcpkg install pkgconf fmt zlib libdeflate zstd isal lz4 bzip2 simdjson lcms libjxl openjpeg libwebp libavif libheif libraw jxrlib vulkan-headers vulkan-loader glslang freetype harfbuzz libjpeg-turbo libpng blend2d

**Manifest mode** (`cmake --preset vcpkg-manifest`) — skip the command above; vcpkg builds deps from `vcpkg.json` into `build/vcpkg_installed` on configure. Convenient but slow on every configure.

### Building

Presets are toolchain selectors (always Ninja + Release):

    cmake --preset default           # system / find_package deps
    cmake --preset vcpkg             # classic vcpkg (pre-installed packages)
    cmake --preset vcpkg-manifest    # manifest vcpkg (builds deps on configure)
    cmake --preset emscripten        # EMSDK only
    cmake --preset emscripten-vcpkg  # EMSDK + VCPKG_ROOT (wasm deps)

    cmake --build --preset vcpkg
    cmake --install build

Override build type on the configure line when needed, e.g. `cmake --preset vcpkg -DCMAKE_BUILD_TYPE=Debug`.

Fine-grained control of what mango itself compiles is via CMake options in `CMakeLists.txt`.

If you are not sure where mango was installed, re-run configure in the build directory and the install prefix is printed to the console.

#### INTEL_DELUXE

The INTEL_DELUXE option uses all the latest ISA extensions except AVX-512. When targeting older processors it is recommended to cherry pick the extensions.

#### WSL

If you are compiling on Windows WSL there is an issue with cmake find_package() not working correctly when VCPKG packages are installed. This is because the WSL adds windows PATH to it's path on startup. This can be mitigated by adding the following lines into /etc/wsl.conf file:

    [interop]
    appendWindowsPath = false

Don't forget to restart the WSL; "wsl --shutdown" for the setting to take effect.


<h2><img src="logo-emscripten.png" alt="logo" width="80"/> Emscripten</h2>


The Emscripten build is still work-in-progress but is partially working. Activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) so `EMSDK` is set.

### EMSDK only

    cmake --preset emscripten
    cmake --build --preset emscripten

No vcpkg. Optional codecs that are not available for wasm stay disabled via `find_package`. Fine on Linux/macOS when you only need a core wasm build.

### EMSDK + vcpkg (recommended on Windows)

Needs `EMSDK` and `VCPKG_ROOT`. Manifest mode installs wasm deps (`wasm32-emscripten`):

    cmake --preset emscripten-vcpkg
    cmake --build --preset emscripten-vcpkg

The target is currently node; the cmake configuration enables native filesystem access for testing.
