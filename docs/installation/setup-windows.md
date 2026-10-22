[← Installation guide](setup.md)

<h1><img src="logo-windows.png" alt="logo" width="80"/> Windows (vcpkg)</h1>

Install [vcpkg](https://vcpkg.io/en/getting-started.html), then set:

    VCPKG_ROOT              <vcpkg_root>
    VCPKG_DEFAULT_TRIPLET   x64-windows

## Dependencies

**Classic mode** (`cmake --preset vcpkg`) — install packages once into the global vcpkg tree; configure stays fast. Re-run when `vcpkg.json` changes:

    vcpkg install pkgconf fmt zlib libdeflate zstd isal lz4 bzip2 simdjson lcms libjxl openjpeg libwebp libavif libheif libraw jxrlib vulkan-headers vulkan-loader glslang freetype harfbuzz libjpeg-turbo libpng blend2d

**Manifest mode** (`cmake --preset vcpkg-manifest`) — skip the command above; vcpkg builds deps from `vcpkg.json` into `build/vcpkg_installed` on configure. Convenient but slow on every configure.

## Building

Presets are toolchain selectors (Ninja + Release by default):

    cmake --preset default           # system / find_package deps
    cmake --preset vcpkg             # classic vcpkg (pre-installed packages)
    cmake --preset vcpkg-manifest    # manifest vcpkg (builds deps on configure)
    cmake --preset emscripten        # EMSDK only (see setup-emscripten.md)
    cmake --preset emscripten-vcpkg  # EMSDK + VCPKG_ROOT (wasm deps)

    cmake --build --preset vcpkg
    cmake --install build

Override build type when needed, e.g. `cmake --preset vcpkg -DCMAKE_BUILD_TYPE=Debug`.

Fine-grained control of what mango compiles is via CMake options in `CMakeLists.txt`. Re-run configure in the build directory to see the install prefix.

### INTEL_DELUXE

`INTEL_DELUXE` enables recent Intel ISA extensions (not AVX-512). On older CPUs, cherry-pick extensions instead.

### WSL

On WSL, `find_package()` can break when Windows `PATH` is appended to Linux. In `/etc/wsl.conf`:

    [interop]
    appendWindowsPath = false

Restart WSL (`wsl --shutdown`) after changing this.
