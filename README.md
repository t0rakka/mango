# mango
A multi-platform low-level development framework for graphics programmers

#### About mango
Pssst... hey you; looking for a really good time? You have come to the right place! What you will find here is a collection of essential graphics programming building blocks which seamlessly work together to create a fabric that brings the fun back into the programming. Despite the fact that the code is written in C++ which is indeed quite uncool and not very trendy at all.

Let's make it crystal clear from the beginning what this library is not: this is not a game or rendering engine. If you want to develop games or cool applications - look elsewhere. There are tons of engines these days with very liberal licenses with "pay as you go" licensing model and the engines are really, really good with excellent tools, ecosystems and communities with tons of support to go with them. That's it, you can stop reading now; thanks for coming by.

However, if you are wondering why we are still looking at progress bars and waiting for computers when they should be waiting for us instead you should keep reading. The reason for this is very simple: there is a lot of software written using sloppy programming paradigms and ways of doing things. The hardware is capable of so much more. It's easy to have the false sense that the computer is doing heavy work because it's taking so long - we are after all waiting for something to happen. Most likely most of the time is spent waiting for data. It's not because the programming language used was bad, poorly performing or anything like that. One can write fast software in almost any language.

The reason is very simple. A large body of programmers simply DO NOT CARE. In fact, they think it is a virtue to not optimize prematurely or waste time doing things "inefficiently" (as-in, programmer time). I have good news for you; I have wasted my time so that you do not have to waste yours.

The library, right here, is end result of over 20 years of evolution. I know, this guy has just re-invented the same wheel multiple times and this is the roundest variation yet, right? It's okay to look at it that way as it is not very far from the truth. But the difference here is that this time around we are not trying to fit a round peg into a square hole. We have taken radically different approach; the library consists of very low-level functional components which can be combined to do more complicated things.

Code speaks more than 1000 words.

    Bitmap bitmap("hello.zip/image.jpg", Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));
    u32* image = bitmap.address<u32>(0, 0);

Looks fairly typical image loading library code. We give a filename and can get a pointer to the loaded image. This is the HIGH-LEVEL API, convenience to get started quickly.

What is actually happening here is that first the URI is parsed: hello.zip + image.jpg, breaking it into components, in this case container (zip file) and filename. The container is memory mapped. If the zip is uncompressed, the offset of the file within the container is directly mapped. If the data is compressed, we use our virtual memory mechanism to decompress and return the data to the Bitmap object's constructor. The constructor finds a ImageDecoder for the format, in this case JPEG and parses the header from the mapped memory. Then the storage is allocated in the requested format (the pixel format is completely configurable, not just enumeration like in most libraries). Then the decoder is invoked and it decodes *and* does pixel format conversion when writing into the storage.

The decoder is using lock-free work queue to distribute the decoding to available CPU resources and eventually the data is available. This variant of the code has a synchronization point before the constructor returns so that the data is in consistent state ; the call is "blocking" until the decoding operation is complete.

The fun begins when you as a programmer realize that you can construct your own custom loading pipeline using the same exact components the above convenience constructor did.

Let's assume we have a simple task: load image file and create OpenGL texture from it. Sounds simple? It is, actually, but let's take a look at how this is almost universally done with other existing libraries:
- Open a file
- Read the file contents into a buffer
- Interpret the buffer and decode the image into another (image) buffer
- Copy the image buffer contents to the GL
- GL will make a copy so that glTexImage2D call can return immediately and will transfer the buffer contents at it's leisure

Let's look at the copies being done: Filesystem pages (4k) -> filesystem buffer -> client's buffer -> client's image -> GPU driver internal buffer -> GPU internal storage (data is copied or transformed 5 times!)

We on the other hand have many alternatives:

1. If we have APPLE_client_storage extension (we are on macOS or iOS), we can simply memory map a compressed texture file and directly copy it into the GPU internal storage. We have the low-level APIs to parse and dispatch or decompress compressed image file formats. The cost is as follows: Filesystem pages (4k) -> GPU internal storage (1 memory copy)

2. We can decode the image file from the memory map directly to the GPU mapped memory using the low-level APIs. This is possible because the pixel format conversion is done in the decoding. The cost looks like this: Filesystem pages (4k) -> GPU driver internal buffer -> GPU internal storage (2 memory copies)

This was just one example how we do things differently and give you, the programmer more control how things should work.

#### Library Features

##### Core Services
- ThreadPool with lock-free Serial- and ConcurrentQueue front-end
- Memory mapped file I/O
- Compressed and Encrypted containers (zip, rar, cbz, cbr, custom)
- Zero-overhead endianess adaptors
- Filesystem observer
- Compression Tools with unified interface: miniz, lz4, lzo, zstd, bzip2, lzfse
- Hashing and checksum algorithms with enhanced CPU instruction support: SHA-1, SHA-256, MD5, CRC32, CRC32c

##### Image Format Support
- Low-level toolkit for building image loading pipelines
- Single-pass memory to target surface decoding architecture
- Supports: tga, iff, bmp, jpg, png, dds, pcx, hdr, pkm, gif, ktx, pvr, astc
- Pixelformat conversion with universal format layout mechanism
- Compressed Texture support for ALL formats in DirectX, OpenGL and Vulkan

##### SIMD Abstraction
- x86/x86-64: SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX512
- ARM: NEON, NEON64 (aarch64)
- PowerPC: Altivec (VSX >= v2.06)
- MIPS: MSA
- FP16 conversion instruction support where available
- Functional API ; SIMD vector objects are never modified in-place

##### Short Vector Math
- Super efficient implementation
- Specialized float4, double4 and matrix4x4 for SIMD
- GLSL compatible syntax
- Unique ScalarAccessor design for reading/writing lanes and shuffling SIMD-backed vectors

##### Vector Math performance
Let's take a look at example function:

    float32x4 test(float32x4 a, float32x4 b, float32x4 c)
    {
        return a.wwww * b.xxyy + (c.xxzz - a).zzzz * b.w;
    }

gcc 5.4 will generate the following instructions for ARM64:
https://godbolt.org/g/AoyqXi
    
        trn1    v2.4s, v2.4s, v2.4s
        dup     v3.4s, v1.s[3]
        zip1    v1.4s, v1.4s, v1.4s
        fsub    v2.4s, v2.4s, v0.4s
        fmul    v2.4s, v3.4s, v2.s[2]
        fmla    v2.4s, v1.4s, v0.4s[3]
        orr     v0.16b, v2.16b, v2.16b

clang 3.9 for X86_64 / SSE2:

        shufps   xmm2, xmm2, 160
        subps    xmm2, xmm0
        shufps   xmm0, xmm0, 255
        movaps   xmm3, xmm1
        unpcklps xmm3, xmm3
        mulps    xmm0, xmm3
        shufps   xmm2, xmm2, 170
        shufps   xmm1, xmm1, 255
        mulps    xmm1, xmm2
        addps    xmm0, xmm1

It is very easy to shoot yourself into the foot with intrinsics. The most basic error is to use unions of different types; this will dramatically reduce the quality of generated code. We have crafted the "accessor" proxy type to generate the overloads of different shuffling and scalar component access. This has been tested with Microsoft C++, GNU G++ and clang to produce superior code to any other alternative implementation. Nearly every method, operator and function has been extensively optimized to reduce register pressure and spilling with generated code. Most of the time the design attempts to use non-mutable transformations to the data; no in-place modification so that compiler has more options to generate better code.

##### JPEG decoder performance
Sample workload: 672 MB of JPEG data in 410 individual files. 6196 MB of RGB image data when decompressed.
Processing time: 3.2 seconds on i7-3770K CPU running 64 bit Ubuntu 16.04
This means data rate of of nearly 2 GB/s for raw uncompressed image data.

Let's do a CPU upgrade. Intel Core i9 with 10 cores, same workload. 1.1 seconds (5.6 GB/s). A single-threaded library would have improved the performance only marginally because of higher IPC and operating frequency, wasting most of the hardware investment. The only way to maximize throughput with code designed to be single-threaded is to decode multiple images simultaneously but this does not reduce latency which is something our design certainly does.

The decoder can be up to three times the speed of jpeglib-turbo on "best conditions" and on worst case roughly same performance. What defines "best conditions" is a very simple observation: the Huffman decoding is the bottleneck; every bit has to come through the serial decompressor. The JPEG standard has a feature which was originally inteded for error correction: Restart Interval (RST marker). If a file has these markers we can find them at order of magnitude faster rate. When we find a marker we start a new Huffman decoder as the bitstream is literally restarted at the marker. This allows us to run as many times faster we have I/O bandwidth and hardware concurrency.

##### Blitter
The pixelformat conversion code, "the blitter" used to be JIT-compiled using a realtime x86 assembler called realtime but this functionality has been deprecated for this public github release and replaced with generic template based implementation. The reason is that ARM has gained importance in the past decade dramatically and x86-only code just don't cut it anymore. This means the blitter is not as cool and super as it used to be but it still gets the job done. The blitter is based on a very simple principle: scaling bit-masks:

    u32 src = 0x000000ff; // unorm component's source bitmask
    u32 dst = 0x00ffff00; // we want to convert to this mask
    double scale = double(dst) / src; // double because float can only handle 24 bits of unorm w/o precision loss

It's magic, but multiplying any value in the src format by this scale will yield the correct normalized value in the dst format. The largest cost here is the conversion between integer and floating-point but it can be handled efficiently. Our SIMD implementation is using AoS layout so each pixel is individually processed. This approach is very wasteful and round-trip to floating-point format increases processing overhead. The work for improved blitter is on-going and it is built around two cornerstone principles: 

1. The innerloops process multiple pixels per iteration (up to supported SIMD register width)
2. The value scaling is 100% integer based algorithm which is more efficient than the current floating point approach

#### TODO
Features currently being worked on or considered for future roadmap:
- - OpenEXR decompressor / compressor
