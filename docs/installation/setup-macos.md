[← Installation guide](setup.md)

<h1><img src="logo-apple.png" alt="logo" width="80"/> macOS (Homebrew)</h1>

## Dependencies

    brew install fmt zlib libdeflate zstd lcms2 jpeg-xl openjpeg webp libavif libheif libraw isa-l lz4 bzip2 freetype harfbuzz simdjson libjpeg-turbo libpng

For **JPEG XR** (`.jxr`), Homebrew no longer ships libjxr — see [manual builds on the hub page](setup.md#jpeg-xr-libjxr).

## Building

Same as Linux:

    cmake --preset default
    cmake --build build
    sudo cmake --install build
