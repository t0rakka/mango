# mango
A multi-platform development framework for graphics programmers

#### Library Features

##### Core Services
- ThreadPool with lock-free Serial- and ConcurrentQueue front-end
- Memory mapped file I/O
- Compressed and Encrypted containers (zip, rar, cbz, cbr, custom)
- Zero-overhead endianess adaptors
- Filesystem observer
- Compression Tools with unified interface: miniz, lz4, lzo, zstd, bzip2, lzfse

##### Image Format Support
- Low-level toolkit for building image loading pipelines
- Single-pass memory to target surface decoding architecture
- Supports: tga, iff, bmp, jpg, png, dds, pcx, hdr, pkm, gif, ktx, pvr, astc
- Pixelformat conversion with universal format layout mechanism
- Compressed Texture support for ALL formats in DirectX, OpenGL and Vulkan

##### SIMD Abstraction
- Intel: SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2
- ARM NEON
- PPC Altivec (deprecated)
- SPU (deprecated)

##### Short Vector Math
- Super efficient implementation
- Specialized float4 and matrix4x4 for SIMD
- GLSL compatible syntax

#### Code Samples

https://github.com/t0rakka/mango-examples
