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

##### Vector Math performance
Let's take a look at example function:

    float4 test(float4 a, float4 b, float4 c)
    {
  	    return a.wwww * b.xxyy + (c.xxzz - a).zzzz * b.w;
    }

gcc 5.4 will generate the following instructions for ARM64:
https://godbolt.org/g/AoyqXi
    
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

##### JPEG decoder performance
Sample workload: 672 MB of JPEG data in 410 individual files. 6196 KB of RGB image data when decompressed.
Processing time: 3.2 seconds on i7-3770K CPU running 64 bit Ubuntu 16.04
This means data rate of of nearly 2 GB/s for raw uncompressed image data.

The decoder can be up to three times the speed of jpeglib-turbo on "best conditions" and on worst case roughly same performance. What defines "best conditions" is a very simple observation: the Huffman decoding is the bottleneck; every bit has to come through the serial decompressor. The JPEG standard has a feature which was originally inteded for error correction: Restart Interval (RST marker). If a file has these markers we can find them at order of magnitude faster rate. When we find a marker we start a new Huffman decoder as the bitstream is literally restarted at the marker. This allows us to run as many times faster we have I/O bandwidth and hardware concurrency.

##### Blitter
The pixelformat conversion code, "the blitter" used to be JIT-compiled using a realtime x86 assembler called realtime but this functionality has been deprecated for this public github release and replaced with generic template based implementation. The reason is that ARM has gained importance in the past decade dramatically and x86-only code just don't cut it anymore. This means the blitter is not as cool and super as it used to be but it still gets the job done. The blitter is based on a very simple principle: scaling bit-masks:

    uint32 src = 0x000000ff; // unorm component's source bitmask
    uint32 dst = 0x00ffff00; // we want to convert to this mask
    double scale = double(dst) / src; // double because fp32 can only handle 24 bits of unorm w/o precision loss

It's magic, but multiplying any value in the src format by this scale will yield the correct normalized value in the dst format. The largest cost here is the conversion between integer and floating-point but it can be handled efficiently. Our SIMD implementation is using AoS layout so each pixel is individually processed. This is wasteful but so is going full SoA as it requires more loads, stores and cache evictions. The best balance would be to process in batches using local SoA so that's definitely in the roadmap. The most common format conversions have custom loops so they are running on the reasonably fast path as it is. This is a disclaimer. :)

There is also a promising 100% integer scale-and-bias algorithm which can handle every component size with correct rounding and w/o precision loss at very high rates but it is still in experimental stage. The best part is no compromise between precision and performance. More about that in the future.

#### TODO
Features currently being worked on:
- pvr/pvr2 decompressor (uses the new full-surface decoding interface, the compressor used to only support blocks)
- lightweight OpenEXR decompressor / compressor
- Improved blitter
- Rewrite some of the Intel specific SIMD code to be more portable (ARM NEON specificly)
- Remove exceptions
