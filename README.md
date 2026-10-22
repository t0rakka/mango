<img src="docs/mango-logo.png" alt="mango" width="72" align="left"/>

# MANGO

**C++20 library — building blocks for your imagination.**

[![CI](https://github.com/t0rakka/mango/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/t0rakka/mango/actions/workflows/ci.yml)

#### [Installation](docs/installation/setup.md)

#### [Examples](docs/examples/examples.md)

#### [Discord](https://discord.gg/E2xuXbK9Kf)

<br clear="left"/>

---

## Why MANGO?

MANGO is a **C++20 foundation framework** — platform abstraction and shared infrastructure, not a game engine. We don't prescribe your architecture; we supply **interlocking building blocks** so you can wire up a viewer, a pipeline, or the low-level half of something bigger. Your design, your rules — we just make the boring parts solid so coding the interesting parts feels good again.

The services are the kind you reach for in every real project: **filesystem abstraction** (`game.zip/levels/…` opens like a folder), a **unified compression API** (including streaming), **image decoding and encoding** with **color management**, plus **vector math** and a modern **SIMD** layer that stays friendly to hand-tuned intrinsics. Need windows, GL/Vulkan helpers, or mesh import? Those are **optional modules** on the same foundations — enable them in CMake and they slot in without a second ecosystem.

**Customize the build** to match the job. Run the full stack or a tight subset: `BUILD_MINIMAL`, turn off Vulkan, pick only the `IMAGE_FORMAT_*` codecs you need. HEIC, camera RAW, and friends are deliberately optional — their dependencies are heavy. A desktop image viewer might enable everything; an engine texture path might stay on PNG, JPEG, and KTX2. If a library isn't installed, configure skips that format and keeps going. Details: [installation guide](docs/installation/setup.md).

---

## Quick start

<p align="center">
  <a href="docs/installation/setup.md">
    <img src="docs/getting-started-cheatsheet.png" alt="MANGO getting started — clone, configure, build" width="900"/>
  </a>
</p>

```bash
git clone https://github.com/t0rakka/mango.git
cd mango
cmake --preset default
cmake --build build
```

```cpp
#include <mango/mango.hpp>
using namespace mango;
using namespace mango::filesystem;

int main()
{
    File file("archive.zip/textures/albedo.png");
    // ...
}
```

---

## Filesystem in one glance

Containers look like directories. `Path` keeps container indexes alive so reading many small files stays cheap.

```cpp
File file("test.iso/data/image.png");

Path root("test.iso/data/");
File same(root, "image.png");
```

ZIP, RAR, 7z, ISO, custom HBS, nested archives → [filesystem examples](docs/examples/filesystem.md).

---

## Images

<p align="center">
  <img src="docs/decoding.jpg" alt="Image decoding" width="420"/>
</p>

MANGO is a serious image toolkit: inspect headers, decode to flexible `Format` layouts, async decode, block compression, ICC, and optional GPU-oriented KTX2 / Basis transcode paths.

**<mark>NEW!</mark>** MANGO now decodes **<mark>HDR PNG</mark>** (PNG 3rd edition signalling: `cICP`, `mDCV`, `cLLI`, 16-bit linear) and **<mark>UltraHDR JPEG</mark>** (ISO 21496-1 / Adobe `hdrgm` gain-map JPEG) — scene-linear fp16 output with HDR metadata preserved.

**Benchmarks and format notes:** [docs/examples/image.md](docs/examples/image.md#library-comparisons)

---

## More

- **Encryption** — unified API with AES-NI / ARM Crypto where available (see core headers).
- **Vulkan text** — optional `mango::vulkan::FontRenderer` (vector outlines, hinting, ligatures); try the Vulkan font example.
