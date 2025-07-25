<h1><img src="../mango-logo.png" alt="logo" width="80"/> MANGO Code Examples</h1>


## mango::filesystem

The mango::filesystem provides an abstraction for containers, where a container is a file that contains files and folders. This allows you to access .zip files and other containers as if they were regular folders.

### Basic File Access

Accessing a memory-mapped file:

```cpp
File file("test.txt");
```

Accessing a file inside a container:

```cpp
File file("foo.zip/test.txt");
```

### How Container Access Works

When a container is detected in the path, the system:
1. Parses the path into two components: "foo.zip/" (container) and "test.txt" (file)
2. Creates a data provider that indexes the container like a regular folder
3. Makes the "test.txt" file visible through the File interface
4. For uncompressed files: memory-maps a range and exposes it as a block of memory
5. For compressed files: decompresses the data and exposes the buffer to the caller

The container data provider is recursive, so you can have nested containers (e.g., a zip file inside an ISO file).

### Optimized Container Access with Path

When accessing many files from the same container, you can create a permanent interface called `Path`. This is different from `std::filesystem::path` (which just expresses filenames) - our `Path` is a data provider abstraction. This design predates C++17's `std::filesystem`, hence the potentially confusing name.

```cpp
Path path("foo.zip/");
File file(path, "test.txt");
```

Creating a `Path` enumerates the container contents only once, making subsequent file access lightweight. The `Path` serves as a new root for parsing paths and accessing files. Paths are fully recursive - internally, the data provider for a path is another path.

```cpp
Path pathA("foo.zip/");
Path pathB(pathA, "bar/");
File file(pathB, "test.txt");
```

### Native Filesystem Integration

Even when creating a `File` object without a container in the path, there's still an OS native data provider path created internally:

```cpp
File file("test.txt");

// Equivalent to:
Path path("./");
File file(path, "test.txt");
```

Both work identically internally - a `File` always has a `Path` as its data provider. In these examples, the data provider is the OS native filesystem implementation.

### Memory-Based Containers

The `Path` doesn't have to be a file; it can be a block of memory. This feature was created to support Windows Resource Files (.rc):

```cpp
ConstMemory memory(data, size);
Path path(memory, ".zip"); // Treats memory as a zip file
File file(path, "test/flower.jpg");
```

### Advanced Features

- **AES Decryption**: Supported for zip containers
- **Custom Container Format**: The "mgx" format is optimized for multi-core CPUs
  - Larger files are compressed as blocks for parallel processing
  - Small files are combined into macroblocks for efficiency
  - LRU caching prevents redundant decompression of macroblocks

### Performance Benchmarks

#### Silesia Test Suite (202.1 MB)

**Traditional zip:**
```bash
> time zip test.zip -r silesia/
3.98 user 0.05s system 98% cpu 4.041 total
```

**Mango snitch:**
```bash
> snitch silesia/ deflate.zlib 4
Compressed: 202.1 MB --> 66.3 MB (32.8%) in 0.12 seconds (deflate.zlib-4, 1710 MB/s)
```

Mango is over 30x faster while achieving the same compression ratio (~66 MB).

#### Different Compression Methods

```bash
Compressed: 202.1 MB --> 104.3 MB (51.6%) in 0.04 seconds (lz4-5, 5174 MB/s)
Compressed: 202.1 MB --> 62.2 MB (30.8%) in 0.07 seconds (zstd-2, 2915 MB/s)
Compressed: 202.1 MB --> 49.0 MB (24.3%) in 3.05 seconds (lzma2-10, 67 MB/s)
```

#### Large Dataset (11,027 files, 7.5 GB)

**Mango:**
```bash
Compressed: 7584.1 MB --> 5637.8 MB (74.3%) in 3.54 seconds (zstd-2, 2195 MB/s)
```

**Traditional zip:**
```bash
> zip foo.zip -r ~/data/  165.47s user 2.60s system 98% cpu 2:50.00 total
```

Mango compresses in 3.5 seconds vs. 3 minutes with zip, while also providing faster data access through better CPU utilization.

## mango::image

The mango::image system provides comprehensive image loading, decoding, and manipulation capabilities. It supports a wide range of formats from simple bitmaps to complex block-compressed textures, with advanced features like asynchronous decoding and format conversion.

### Image Format System

At the heart of the image system is the flexible `Format` class that describes pixel layouts, color spaces, and data types. Unlike simple pixel format enums, mango's `Format` class provides precise control over:

- **Bit depth**: 8, 16, 32, or 64 bits per pixel
- **Data type**: UNORM (unsigned normalized), SNORM (signed normalized), UINT, SINT, FLOAT16, FLOAT32, FLOAT64
- **Component order**: RGBA, BGRA, RGB, BGR, and many other arrangements
- **Component sizes**: Precise bit allocation for each color channel
- **Special flags**: Luminance, indexed, linear color space, premultiplied alpha

```cpp
// Standard 32-bit RGBA format
Format rgba32(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

// 16-bit BGR format (5-6-5)
Format bgr16(16, Format::UNORM, Format::BGR, 5, 6, 5, 0);

// 64-bit float RGBA
Format rgba64f(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16);

// Luminance with alpha
LuminanceFormat la16(16, Format::UNORM, 8, 8);
```

### Indexed/Palette Support

Mango provides first-class support for indexed color formats through the `IndexedFormat` class and `Palette` system:

```cpp
// Create indexed format (8-bit indices)
IndexedFormat indexed8(8);

// Create palette with 256 colors
Palette palette(256);
palette[0] = Color(255, 0, 0, 255);    // Red
palette[1] = Color(0, 255, 0, 255);    // Green
palette[2] = Color(0, 0, 255, 255);    // Blue
// ... more colors

// Create indexed surface
Surface indexed_surface(width, height, indexed8, stride, data);
indexed_surface.palette = &palette;
```

The system automatically handles palette resolution when converting between indexed and RGBA formats:

```cpp
// Convert indexed to RGBA
Surface rgba_surface(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
resolve(rgba_surface, indexed_surface); // Automatically applies palette
```

### Low-Level Decoding

The foundation of mango's image system is the `ImageDecoder` class, which provides direct access to decoding facilities:

```cpp
// Create decoder from memory
ConstMemory memory = file;
ImageDecoder decoder(memory, ".png");

if (decoder.isDecoder())
{
    // Get image header without decoding
    ImageHeader header = decoder.header();
    printLine("Image: {}x{}", header.width, header.height);
    printLine("Format: {} bits", header.format.bits);
    
    // Allocate target surface
    const size_t stride = header.width * header.format.bytes();
    std::vector<u8> buffer(header.height * stride);
    Surface surface(header.width, header.height, header.format, stride, buffer.data());
    
    // Decode with options
    ImageDecodeOptions options;
    options.simd = true;        // Enable SIMD acceleration
    options.multithread = true; // Enable multi-threading
    
    ImageDecodeStatus status = decoder.decode(surface, options);
    if (status.success)
    {
        printLine("Decoded successfully");
    }
}
```

### Format Conversion and Target Surfaces

Mango provides sophisticated format conversion through the `DecodeTargetBitmap` class, which handles cases where the target format differs from the source:

```cpp
// Target surface in different format
Surface target(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));

// Create decode target that handles format conversion
DecodeTargetBitmap decode_target(target, header.width, header.height, header.format);

// Decode directly to target (with automatic conversion)
ImageDecodeStatus status = decoder.decode(decode_target, options);

// Resolve any temporary buffers
decode_target.resolve();
```

### Block-Compressed Texture Formats

Mango supports a wide range of block-compressed texture formats through the `TextureCompression` system:

```cpp
// Supported formats include:
// - DXT1, DXT3, DXT5 (BC1, BC2, BC3)
// - BC4, BC5, BC6H, BC7
// - ASTC (various block sizes)
// - ETC1, ETC2
// - PVRTC

// Decompress block-compressed texture
TextureCompression compression(TextureCompression::BC1);
Surface decompressed(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

TextureCompression::Status status = compression.decompress(decompressed, compressed_data);
if (status.success)
{
    printLine("Decompressed {}x{} texture", width, height);
}
```

### Asynchronous Decoding

For large images or real-time applications, mango provides asynchronous decoding with progress callbacks:

```cpp
// Create decoder
ImageDecoder decoder(file, filename);

if (decoder.isAsyncDecoder())
{
    // Allocate target bitmap
    ImageHeader header = decoder.header();
    Bitmap bitmap(header);
    
    // Progress callback
    auto callback = [](const ImageDecodeRect& rect) {
        float percent = rect.progress * 100.0f;
        printLine("Decoding: {:.1f}% - region ({}, {}) {}x{}", 
                  percent, rect.x, rect.y, rect.width, rect.height);
    };
    
    // Launch async decoding
    ImageDecodeFuture future = decoder.launch(callback, bitmap);
    
    // Do other work while decoding...
    
    // Wait for completion
    ImageDecodeStatus status = future.get();
    if (status.success)
    {
        printLine("Async decoding completed");
    }
    
    // Optionally cancel if needed
    // decoder.cancel();
}
```

### SIMD and Multi-threading Configuration

Mango automatically detects and utilizes SIMD instructions and multi-core CPUs:

```cpp
ImageDecodeOptions options;

// Enable SIMD acceleration (SSE, AVX, NEON, etc.)
options.simd = true;

// Enable multi-threading (uses ThreadPool)
options.multithread = true;

// Decode with optimized settings
ImageDecodeStatus status = decoder.decode(surface, options);
```

The system automatically:
- Detects available SIMD instruction sets
- Configures optimal inner loops for the target CPU
- Distributes work across available CPU cores
- Handles thread synchronization and memory management

### Convenient Bitmap Class

For most use cases, the `Bitmap` class provides a convenient high-level interface that handles all the complexity:

```cpp
// Simple loading - automatic format detection
Bitmap bitmap("image.png");

// Load with specific format conversion
Bitmap bitmap_rgba("image.jpg", Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

// Load from memory
ConstMemory memory = file;
Bitmap bitmap_mem(memory, ".png");

// Load with options
ImageDecodeOptions options;
options.simd = true;
options.multithread = true;
Bitmap bitmap_opt("large_image.tiff", options);
```

The `Bitmap` class automatically:
- Creates the appropriate `ImageDecoder`
- Allocates memory for the decoded image
- Handles format conversion if needed
- Manages palette data for indexed images
- Provides RAII cleanup

### Advanced Surface Types

Mango provides specialized surface types for different use cases:

```cpp
// TemporaryBitmap - automatic format conversion
Surface source(width, height, some_format);
TemporaryBitmap temp(source, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
// temp is now in RGBA format, conversion only happens if needed

// LuminanceBitmap - grayscale conversion
LuminanceBitmap gray(source, true); // true = include alpha

// QuantizedBitmap - color quantization for indexed formats
QuantizedBitmap indexed(source, 0.8f, true); // quality, dithering
```

### Color Management and ICC Profiles

Mango supports ICC color profiles for accurate color reproduction:

```cpp
// Load ICC profile
filesystem::File icc_file("DisplayP3-v2-micro.icc");
ConstMemory icc_data = icc_file;

// Encode with ICC profile
ImageEncodeOptions encode_options;
encode_options.icc = icc_data;
bitmap.save("output.jpg", encode_options);

// Read ICC profile from image
ImageDecoder decoder("image.png");
ConstMemory icc_profile = decoder.icc();
if (icc_profile.size > 0)
{
    printLine("Image contains ICC profile: {} bytes", icc_profile.size);
}
```

### Performance Features

Mango's image system is designed for high performance:

- **Zero-copy decoding**: When possible, decoders write directly to the target surface
- **SIMD acceleration**: Optimized inner loops for all major CPU architectures
- **Multi-threading**: Automatic work distribution across CPU cores
- **Memory mapping**: Efficient file access for large images
- **Format specialization**: Dedicated code paths for common formats
- **Block compression**: Hardware-accelerated texture decompression

The system automatically chooses the fastest available path for each operation, whether it's direct memory access, SIMD-optimized conversion, or multi-threaded processing.
