<h1><img src="../mango-logo.png" alt="logo" width="80"/> MANGO Installation Guide</h1>

<p align="center">
  <img src="../installation-confused.png" alt="Hold on — the manual is upside down until you pick the right preset" width="360"/>
</p>

<p align="center"><em>Lots of knobs, but you only need the ones you turn on.</em></p>

## Choose your platform

Pick one guide — package lists differ, but configure/build flow is the same everywhere.

| | |
|---|---|
| <img src="logo-linux.png" alt="" width="44"/> **[Linux](setup-linux.md)** | **Debian, Ubuntu, Mint, Arch, etc.** |
| <img src="logo-apple.png" alt="" width="44"/> **[macOS](setup-macos.md)** | `brew` dependencies and build. |
| <img src="logo-windows.png" alt="" width="44"/> **[Windows](setup-windows.md)** | vcpkg classic vs manifest, CMake presets, WSL notes. |
| <img src="logo-emscripten.png" alt="" width="44"/> **[Emscripten](setup-emscripten.md)** | EMSDK-only or EMSDK + vcpkg for wasm deps. |

Quick path (any OS with deps installed):

```bash
git clone https://github.com/t0rakka/mango.git
cd mango
cmake --preset default
cmake --build build
```

Presets are described on [setup-windows.md](setup-windows.md#building) and [setup-emscripten.md](setup-emscripten.md); Linux and macOS usually start with `default`.

---

## How dependencies work

Mango no longer vendors every third-party library. At **configure** time CMake uses `find_package`, pkg-config, and similar probes. What you get falls into three buckets:

| Category | Behaviour |
|----------|-----------|
| **Required** | Must be present (e.g. zlib, fmt). Configure fails if missing. |
| **Optional** | Image codecs, GL/Vulkan helpers, etc. If not found, CMake **disables that feature** and continues — no extra `-D` needed. |
| **Examples / tools** | Extra packages only if you turn on `BUILD_EXAMPLES` / benchmarks that need them. |

You tune the **mango** side with `CMakeLists.txt` options (`BUILD_VULKAN`, `IMAGE_FORMAT_JXL`, `BUILD_MINIMAL`, `LIBRARY_DISABLE`, …). Optional libraries you do not install simply stay off.

**Building a dependency yourself** is fine: install to `/usr/local`, a custom prefix, or point CMake at it with `CMAKE_PREFIX_PATH` (and ensure pkg-config can see it if the dependency provides `.pc` files). Re-run configure; if mango’s probe succeeds, the feature turns on.

If you are unsure what was detected, read the configure summary printed to the console (library names and disabled formats).

---

## Manual builds & awkward packages

Most optional codecs are available from distro packages or vcpkg. A few are **missing or too old** in common repos — mango still supports them if **you** supply a new enough build.

### JPEG XL (libjxl)

Mango needs **libjxl ≥ 0.9**. Some distros ship older headers (notably **Ubuntu 24.04 (noble)** `libjxl-dev`). Options:

- Install a newer libjxl (PPA, rolling distro, or [build from source](https://github.com/libjxl/libjxl)).
- Or disable at configure time: `-DIMAGE_FORMAT_JXL=OFF`.

This is the usual pattern: the format is optional; the fix is upgrade the library or turn the format off.

### JPEG XR (libjxr)

`.jxr` / HD Photo support is enabled when **libjxr** is found. Many systems have no package (Homebrew dropped it; Debian often lacks it). Arch includes `jxrlib` in the [Linux pacman line](setup-linux.md#arch-pacman); Windows can use vcpkg `jxrlib`.

**Build libjxr from source** (any platform where CMake should pick it up after install):

```bash
git clone https://github.com/cgohlke/jxrlib.git
cmake -S jxrlib -B jxrlib-build -DCMAKE_BUILD_TYPE=Release
cmake --build jxrlib-build
sudo cmake --install jxrlib-build
```

Re-configure mango; pkg-config (`libjxr`) or installed headers/libs are detected automatically.

---

## Background

A long time ago MANGO was a self-contained tree with every dependency in-tree. That shifted to package managers and optional features so maintenance stays sane. The platform pages above are starting points, not the only way to install dependencies — as long as CMake can find a library, mango can use it.
