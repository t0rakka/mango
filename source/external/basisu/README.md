# Basis Universal (vendored transcoder)

Upstream: [BinomialLLC/basis_universal](https://github.com/BinomialLLC/basis_universal)  
License: Apache 2.0 (see transcoder file headers)

Mango ships **only** the `transcoder/` sources (no encoder, no bundled zstd tree). Package managers (vcpkg, Conan, distro packages) do not track a stable basisu recipe, so vendoring is the reliable integration path.

## Pinned version

Check `BASISD_VERSION_STRING` in `transcoder/basisu_transcoder_internal.h` (e.g. `02.50`).

## Upgrade recipe

```bash
git clone --depth 1 https://github.com/BinomialLLC/basis_universal.git /tmp/basisu
rsync -a --delete /tmp/basisu/transcoder/ source/external/basisu/transcoder/
```

Then rebuild `mango-image`, run `ktx2_memory_test` / `ktx2_blocks --all` on sample KTX2 files, and fix any API renames in `source/mango/image/image_ktx2.cpp`.

## Mango build flags

- `BASISD_SUPPORT_KTX2=0` — KTX2 parsing and Zstd level decompress live in `image_ktx2.cpp`.
- `BASISD_SUPPORT_KTX2_ZSTD=0` — no dependency on basisu’s `../zstd/zstd.h`; mango links libzstd for `zstd::decompress`.
- `basisu_transcoder.cpp` is compiled with `-fno-strict-aliasing` on GCC/Clang (upstream requirement).

Optional future: set `BASISD_SUPPORT_KTX2=1` and link zstd into basisu if you want the upstream `ktx2_transcoder` class in-process.
