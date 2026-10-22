[← Installation guide](setup.md)

<h1><img src="logo-emscripten.png" alt="logo" width="80"/> Emscripten (wasm, vcpkg)</h1>

Work in progress, but partially working. Activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) so `EMSDK` is set.

## EMSDK only

    cmake --preset emscripten
    cmake --build --preset emscripten

No vcpkg. Optional codecs that are not available for wasm stay disabled via `find_package`. Fine on Linux/macOS when you only need a core wasm build.

## EMSDK + vcpkg (recommended on Windows)

Needs `EMSDK` and `VCPKG_ROOT`. Manifest mode installs wasm deps (`wasm32-emscripten`):

    cmake --preset emscripten-vcpkg
    cmake --build --preset emscripten-vcpkg

The target is currently node; the cmake configuration enables native filesystem access for testing.

Preset overview also appears on [setup-windows.md](setup-windows.md#building).
