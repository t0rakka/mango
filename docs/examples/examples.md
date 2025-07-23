<h1><img src="../mango-logo.png" alt="logo" width="80"/> MANGO Code Examples</h1>


## mango::filesystem

The mango::filesystem is abstraction for containers, where a container is a file that contains files and folders. This allows to access .zip files and other containers as if they were regular folders.

This is what accessing a memory mapped file looks like:

    File file("test.txt");

When a file is inside a container:

    File file("foo.zip/test.txt);

What happens is that the path is parsed into two components: "foo.zip/" and "test.txt", a container and a file stored in the container. When a container is detected in the path it creates a data provider, which can be indexed like a regular folder so the "test.txt" file now visible to the File interface. If the file is stored (no compression) a range is memory mapped and is visible to the caller as a block of memory. If the file is compressed the decompressed buffer is made visible to the caller.

The container data provider is recursive so it is possible to have a zip file inside iso file, and so on.

When a lot of files are accessed from the same container it is possible to create a permanent interface called Path. This is different from std::filesystem::path which is just a way to express filenames, this design is older than std::fs which was introduced in C++17 so apologies for the confusing name. Here is how one can create a permanent mapping to a container and use it as a root for files:

    Path path("foo.zip/");
    File file(path, "test.txt);

Create the path enumerates the contents of the zip file only once and creating files using the path is more light-weight operation; the Path is used as new root for parsing the paths and accessing files. Paths are fully recursive, internally the data provider for a path is another path.

    Path pathA("foo.zip/");
    Path pathB(pathA, "bar/");
    File file(pathB, "test.txt");

In fact, when user creates a File object and there is no container in the path, there is still the OS native data provider path that is created internally.

    File file("test.txt");

    Path path("./");
    File file(path, "test.txt");

These two work identically internally; a File always has a Path as a data provider. In above examples the data provider will be OS native filesystem implementation.

The Path doesn't even have to be a file; it can be a block of memory. This feature was created from user request how to support Windows Resource Files (.rc).

    ConstMemory memory(data, size);
    Path path(memory, ".zip"); // let's say it is a zip file in memory
    File file(path, "test/flower.jpg");

We do support AES decryption for zip containers and our custom block compression container must be mentioned; the "mgx" container format is optimized for using multi core CPUs for fast compression and decompression. Larger files are compressed as blocks so that compression and decompression can occur in parallel. Small files are combined into larger macroblocks to keep the compression efficient; when one file in a macroblock is accessed the whole block is decompressed and the results are kept in LRU cache so that accessing multiple small files does not decompress the whole block every time avoiding wasting CPU.

Here is compression benchmark for silesia test suite:

    > time zip test.zip -r silesia/
    3.98 user 0.05s system 98% cpu 4.041 total

    > snitch silesia/ deflate.zlib 4
    Compressed: 202.1 MB --> 66.3 MB (32.8%) in 0.12 seconds (deflate.zlib-4, 1710 MB/s)

zip took 4 seconds to compress 202 MB of data, while our custom compressor took 0.07 seconds, which is over 30 times faster. The compression method and level were chosen to result in same compressed data size (~ 66 MB). Here are some timings with different compression methods and levels:

    Compressed: 202.1 MB --> 104.3 MB (51.6%) in 0.04 seconds (lz4-5, 5174 MB/s)
    Compressed: 202.1 MB --> 62.2 MB (30.8%) in 0.07 seconds (zstd-2, 2915 MB/s)
    Compressed: 202.1 MB --> 49.0 MB (24.3%) in 3.05 seconds (lzma2-10, 67 MB/s)

What about larger amount of data? 11027 files consuming 7.5 GB:

    Compressed: 7584.1 MB --> 5637.8 MB (74.3%) in 3.54 seconds (zstd-2, 2195 MB/s)
    vs.
    > zip foo.zip -r ~/data/  165.47s user 2.60s system 98% cpu 2:50.00 total

Compression took 3 minutes with zip and 3.5 seconds with our compression utility. The access to the data is also substantially faster as it utilizes CPU much better.
