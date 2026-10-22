<img src="docs/mango-logo.png" alt="mango" width="72" align="left"/>

# MANGO

**C++20 foundation for tools, pipelines, and graphics apps** — virtual paths, compression, images, and math.  
Enable the pieces you need; skip the rest.

[![CI](https://github.com/t0rakka/mango/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/t0rakka/mango/actions/workflows/ci.yml)
· [Installation](docs/installation/setup.md)
· [Examples](docs/examples/examples.md)
· [Discord](https://discord.gg/E2xuXbK9Kf)

<br clear="left"/>

---

## Why MANGO?

| | |
|---|---|
| **Paths that behave like folders** | Open `game.zip/assets/level.png`, `disc.iso/data/…`, nested archives — same `File` / `Path` API as the native filesystem. Fast indexing, block-oriented caching (see [filesystem examples](docs/examples/filesystem.md)). |
| **Compression as plumbing** | One interface for deflate, zstd, lz4, and more — including **streaming** compression and decompression, not a separate API per codec. |
| **Images when you need them** | Broad decode/encode (built-in PNG/JPEG, optional AVIF, HEIF, JXL, KTX2, camera RAW, …). HDR-aware where it matters (HDR PNG, UltraHDR JPEG, GPU block paths). Details: [mango::image](docs/examples/image.md). |
| **Math & SIMD** | Short-vector C++20 math and geometry; portable SIMD types that compose cleanly with native intrinsics. Thread pool via `ConcurrentQueue`. |

**Optional modules** — OpenGL and Vulkan windowing, framebuffers, GPU text, Import3D (glTF, OBJ, …): same types and patterns as the core; enable at CMake time and they plug in without a second toolkit.

---

## Build what you actually need

CMake presets and per-format switches let you ship a **small core** or a **full stack**.

| Profile | Typical CMake | You get |
|---------|----------------|---------|
| **Default** | `cmake --preset default` | Core + image (+ optional codecs found on the system) + window/GL/Vulkan when dependencies are available |
| **Minimal** | `-DBUILD_MINIMAL=ON` | Essentials only — no examples, tools, tests, OpenGL, Vulkan, or Import3D |
| **Image-focused** | `BUILD_VULKAN=OFF`, trim `IMAGE_FORMAT_*` | Decoder/encoder library without the graphics modules |
| **Everything** | `-DBUILD_ALL=ON` | Turn all optional build targets on |

Optional image formats and third-party libraries are **auto-disabled** if not found; see the [installation guide](docs/installation/setup.md) for platform packages and presets (`default`, `vcpkg`, `vcpkg-manifest`, …).

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
