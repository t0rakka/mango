[← Installation guide](setup.md)

# Linux

Debian-family and Arch-family distros share the same configure/build steps below; only the package manager lines differ.

<h2><img src="logo-linux.png" alt="" width="80"/> Debian, Ubuntu, Mint (apt)</h2>

Raspberry Pi OS and other apt-based distros use the same packages.

### Dependencies

    sudo apt-get install cmake ninja-build pkg-config g++ libfmt-dev zlib1g-dev libdeflate-dev libzstd-dev liblcms2-dev libjxl-dev libopenjp2-7-dev libwebp-dev libavif-dev libheif-dev libraw-dev libisal-dev liblz4-dev libbz2-dev mesa-common-dev libgl1-mesa-dev libegl-dev glslang-dev libfreetype-dev libharfbuzz-dev libsimdjson-dev libjpeg-dev libpng-dev libx11-dev libxext-dev libxrandr-dev libx11-xcb-dev libxcb1-dev libxcb-xkb-dev libxcb-keysyms1-dev libxcb-icccm4-dev libxcb-randr0-dev libxcb-sync-dev libxcb-glx0-dev libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev libwayland-bin wayland-protocols libdecor-0-dev libvulkan-dev

**Ubuntu 24.04 (noble):** distro `libjxl-dev` may be too old — see [JPEG XL on the hub page](setup.md#jpeg-xl-libjxl).

<h2 id="arch-pacman"><img src="logo-archlinux.png" alt="" width="80"/> Arch (pacman)</h2>

Manjaro, EndeavourOS, and other pacman-based distros use the same packages.

### Dependencies

    sudo pacman -S cmake ninja pkgconf gcc fmt zlib libdeflate zstd lcms2 libjxl openjpeg2 libwebp libavif libheif libraw isa-l lz4 bzip2 jxrlib mesa glu glslang freetype2 harfbuzz simdjson libjpeg-turbo libpng vulkan-headers vulkan-icd-loader libx11 libxext libxrandr libxcb xcb-util-keysyms xcb-util-wm libxkbcommon libxkbcommon-x11 wayland wayland-protocols libdecor

## Building

    cmake -S . -B build -G "Ninja"
    cd build
    ninja
    sudo ninja install

Or use the repo preset: `cmake --preset default` then `cmake --build build`.

Omit `-G "Ninja"` if you prefer Make. Tune mango with CMake options in `CMakeLists.txt` before building.

### Wayland & window backends

The apt/pacman lines include Xlib, Xcb, and Wayland. Backends that are found are compiled in; the active one is chosen at runtime. Wayland is increasingly the default session (e.g. Linux Mint / Cinnamon), so Wayland packages are recommended: `libwayland-dev` + `libwayland-bin` + `wayland-protocols` (+ `libdecor-0-dev`) on apt, or `wayland` + `wayland-protocols` (+ `libdecor`) on pacman. Without them, CMake disables `ENABLE_WAYLAND` and builds X11/Xcb only.

`libdecor-0-dev` / `libdecor` is optional at configure time but needed for window chrome on GNOME Wayland (title bar, move, resize). Hyprland and KWin draw decorations themselves. A successful Wayland+libdecor configure prints `Wayland (libdecor)` under `[WindowSystem]`; `Wayland` alone means libdecor was not found. `libwayland-bin` provides `wayland-scanner`, which the Wayland backend needs at build time.
