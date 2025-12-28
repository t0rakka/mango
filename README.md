<h1><img src="docs/mango-logo.png" alt="logo" width="80"/> MANGO - C++20 Graphics Library for the NÖRDS.</h1>

Library for C++ masochists. It does short vector math, SIMD, encodes and decodes images, virtual filesystem and other tricks.

"We wasted our time so that you don't have to waste yours."


## [Installation Guide](docs/installation/setup.md)
## [Code Examples](docs/examples/examples.md)


## We load images!
<h1><img src="docs/decoding.jpg"/></h1>

## PNG library comparison
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


## JPEG library comparison

```
CPU: Intel Core i9 8950HK @ 2.90GHz
image: 5184 x 3456 (12652 KB)
-----------------------------------------------------
           decode(ms)   encode(ms)   size(KB)        
-----------------------------------------------------
libjpeg:     197.8 ms      70.1 ms       6677
stb:         300.7 ms    1090.9 ms      16784
toojpeg:          N/A     292.6 ms       5370
wuffs:       185.9 ms          N/A          0
mango:        23.3 ms      18.5 ms       6925
```

```
CPU: Intel Core i9 8950HK @ 2.90GHz
image: 2560 x 1600 (683 KB)
-----------------------------------------------------
           decode(ms)   encode(ms)   size(KB)        
-----------------------------------------------------
libjpeg:      32.9 ms      16.3 ms        678
stb:          54.8 ms     118.9 ms       1607
toojpeg:          N/A      64.2 ms        618
wuffs:        29.4 ms          N/A          0
mango:         3.2 ms       4.3 ms        699
```


## Testimonials

"I'm skeptical of those benchmarks."
-- randy408


## [Discord](https://discord.gg/E2xuXbK9Kf)
