# mango
A multi-platform development framework for graphics programmers

#### About mango
mango is a continuation of series of tools whose history goes all the way back to the early 90's. The origins of the library is shrouded in mystery and only The Chosen Few can remember the accident that did lead into the catalysmic event that took place in February 1994. 

The code has been revised multiple times and the latest revision started as an observation about the state of the hardware and where things are headed in 2011. It was obvious that concurrency is the way to go and memory bandwidth remains as a performance barrier more than ever.

The design started from some prototype code that had one goal in mind: memory map a file and transform it's contents to the GPU local memory as efficiently as possible. 

The most direct approach is to simply mmap a compressed texture and let the GPU consume the data using virtual memory mapping. This is possible with the APPLE_client_storage OpenGL extension. The mango allows to connect the dots very easily; if such direct mapping is not available the next-best thing is to allocate GPU managed memory (OpenGL, DirectX, Vulkan) and map this memory to be visible in the client. Then mmap a file with image data in it. 

Once again, mango connects the dots: the GPU mapped memory must be exposed in a format that mango can understand then create a decoder object for the file's mmap in correct fileformat. This establishes a bridge between GPU and the file with image data in it. The "win" here is that there are no intermediate buffers and multiple copies of the data in-flight as happens with more traditional method of loading an image. 

The traditional approach goes like this:
- Open a file
- Read the file contents into a buffer
- Interpret the buffer and decode the image into another buffer (the Bitmap)
- Copy the image buffer contents to the GL
- GL will make a copy so that glTexImage2D call can return immediately and will transfer the buffer contents at it's leisure

Let's look at the copies being done: Filesystem pages (4k) -> filesystem buffer -> client's buffer -> client's image -> GPU driver internal buffer -> GPU internal storage (5 memory copies)

On the other hand our approach
- APPLE_client_storage: Filesystem pages (4k) -> GPU internal storage (1 memory copy)
- Filesystem pages (4k) -> GPU driver internal buffer -> GPU internal storage (2 memory copies)

After prototyping and benchmarking above methods the mango API practically wrote itself.

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
- fp16 support

##### Short Vector Math
- Super efficient implementation
- Specialized float4 and matrix4x4 for SIMD
- GLSL compatible syntax

#### Code Samples

https://github.com/t0rakka/mango-examples

Let's take a look what the compiler will do with our vector math code:

    float4 test(float4 a, float4 b, float4 c)
    {
  	    return a.wwww * b.xxyy + (c.xxzz - a).zzzz * b.w;
    }

gcc 5.4 will generate the following instructions for ARM64:

    // https://godbolt.org/g/AoyqXi
    test1(float4, float4, float4):
        trn1    v2.4s, v2.4s, v2.4s
        dup     v3.4s, v1.s[3]
        zip1    v1.4s, v1.4s, v1.4s
        fsub    v2.4s, v2.4s, v0.4s
        fmul    v2.4s, v3.4s, v2.s[2]
        fmla    v2.4s, v1.4s, v0.4s[3]
        orr     v0.16b, v2.16b, v2.16b
        ret

It is very easy to shoot yourself into the foot with intrinsics. The most basic error is to use unions of different types; this will dramatically reduce the quality of generated code. We have crafted the "accessor" proxy type to generate the overloads of different shuffling and scalar component access. This has been tested with Microsoft C++, GNU G++ and clang to produce superior code to any other alternative implementation. Nearly every method, operator and function has been extensively optimized to reduce register pressure and spilling with generated code. Most of the time the design attempts to use non-mutable transformations to the data; no in-place modification so that compiler has more options to generate better code.
