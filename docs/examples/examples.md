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
