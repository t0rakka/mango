<h1><img src="docs/mango-logo.png" alt="logo" width="80"/> MANGO - C++20 Graphics Library for the NÖRDS.</h1>

Library for C++ masochists. It does short vector math, SIMD, encodes and decodes images, virtual filesystem and other tricks.

"We wasted our time so that you don't have to waste yours."


## [Installation Guide](docs/installation/setup.md)
## [Code Examples](docs/examples/examples.md)


## We load images!
<h1><img src="docs/decoding.jpg" width="384"/></h1>

### PNG library comparison
```
CPU: 11th Gen Intel(R) Core(TM) i7-11800H @ 2.30GHz
Image: 2560 x 1600 (3531 KB)
---------------------------------------------------
           decode(ms)  encode(ms)   size(KB)
---------------------------------------------------
libpng:         57.3       820.5       2843
lodepng:       105.0       722.6       2624
stb:            48.6       562.4       3864
spng:           37.7       215.9       3232
fpng:            N/A        46.0       3715
wuffs:          23.9         N/A          0
mango:           3.2        57.2       3531
```

```
CPU: 11th Gen Intel(R) Core(TM) i7-11800H @ 2.30GHz
image: 4833 x 5875 (33598 KB)
---------------------------------------------------
           decode(ms)  encode(ms)   size(KB)
---------------------------------------------------
libpng:        379.5     14051.3      28180
lodepng:       900.9      4953.5      26318
stb:           407.2      5249.0      45144
spng:          296.6      2424.7      34083
fpng:            N/A       341.0      36958
wuffs:         165.9         N/A          0
mango:          53.7       287.0      33598
```


### JPEG library comparison

```
CPU: Intel Core i9 8950HK @ 2.90GHz
image: 5184 x 3456 (12652 KB)
-----------------------------------------------------
            decode(ms)    encode(ms)   size(KB)  
-----------------------------------------------------
libjpeg:        197.8          70.1       6677
stb:            300.7        1090.9      16784
toojpeg:          N/A         292.6       5370
wuffs:          185.9           N/A          0
mango:           23.3       18.5 ms       6925
```

```
CPU: Intel Core i9 8950HK @ 2.90GHz
image: 2560 x 1600 (683 KB)
-----------------------------------------------------
            decode(ms)    encode(ms)   size(KB)   
-----------------------------------------------------
libjpeg:         32.9          16.3        678
stb:             54.8         118.9       1607
toojpeg:          N/A          64.2        618
wuffs:           29.4           N/A          0
mango:            3.2           4.3        699
```


## We multitask!
<h1><img src="docs/simd.jpg"  width="384"/></h1>

### SIMD abstraction

Portable vector programming abstraction that works seamlessly with native intrinsics.

    float32x4 test(float32x4 a, float32x4 b, float32x4 c)
    {
        return a.wwww * b.xxyy + (c.xxzz - a).zzzz * b.w;
    }

ARM64 GCC 15.2.0 generates the following instructions:

    trn1    v2.4s, v2.4s, v2.4s
    dup     v3.4s, v1.s[3]
    zip1    v1.4s, v1.4s, v1.4s
    fsub    v2.4s, v2.4s, v0.4s
    fmul    v2.4s, v3.4s, v2.s[2]
    fmla    v2.4s, v1.4s, v0.4s[3]
    orr     v0.16b, v2.16b, v2.16b

x86_64 clang 18.1.0 (SSE2):

    shufps    xmm2, xmm2, 160
    subps     xmm2, xmm0
    shufps    xmm0, xmm0, 255
    movaps    xmm3, xmm1
    unpcklps  xmm3, xmm3
    mulps     xmm0, xmm3
    shufps    xmm2, xmm2, 170
    shufps    xmm1, xmm1, 255
    mulps     xmm1, xmm2
    addps     xmm0, xmm1


### Easy to use Thread Pool
```
ConcurrentQueue queue;
queue.enqueue([]
{
    // your work here
});
```


## Filesystem Abstraction
<h1><img src="docs/filesystem.jpg"  width="384"/></h1>

The filesystem abstraction hides away details such as container format, compression and encryption. There is a custom container format which is designed for modern multicore systems where large files are broken into blocks that can be processed in parallel, and small files are combined into blocks for higher compression ratio. The access to small files is through LRU cache so decompression cost is per block not per file (accessing two small files would otherwise cause decopression twice).

```
// accessing file from .iso container
File file("test.iso/data/image.png");
ConstMemory memory = file;

// same as above but using parent path
Path path("test.iso/data/");
File file(path, "image.png");
```

The parent Path is the virtual filesystem mechanism where the path provides filesystem. If no path is given the default is native filesystem. It is also possible to provide memory as path which allows to use Windows Resource Files using the same interface as any other file (this is just example use case for the path backed by raw memory).


<h1><img src="docs/compress.jpg"  width="384"/></h1>

Unified interface for large number of supported compression methods.


<h1><img src="docs/encrypt.jpg"  width="384"/></h1>

Unified encryption API with hardware acceleration for Intel AES-NI and AMD Crypto instructions.


## Testimonials

"I'm skeptical of those benchmarks."
-- randy408


## [Discord](https://discord.gg/E2xuXbK9Kf)
