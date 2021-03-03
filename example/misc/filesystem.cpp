/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/filesystem/filesystem.hpp>
#include <mango/core/pointer.hpp>
#include <mango/image/surface.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

void example1()
{
    Path path("data/");

    // Iterate all objects in the folder
    for (auto info : path)
    {
        printf("filename: %s ", info.name.c_str());
        if (info.isDirectory())
            printf("(folder)\n");
        else
            printf("(%d bytes)\n", int(info.size));
    }
}

void example2()
{
    // This creates a memory map view of the file into
    // the memory of current process
    File file("test.jpg");

    // This is redundant but let's read the file into a buffer
    u64 size = file.size();
    std::vector<char> buffer(size);
    std::memcpy(buffer.data(), file.data(), size);

    // On the other hand, it is also a block of memory.. so endianess
    // aware pointer will also be a possible use case:
    LittleEndianConstPointer p = file.data();
    u32 value2 = p.read32();
    p += 8; // skip 8 bytes
    float value3 = p.read32f();

    MANGO_UNREFERENCED(value2);
    MANGO_UNREFERENCED(value3);
}

void example3()
{
    // Now we will take a look at how we handle containers

    // Notice how we use a compressed zip file as part of the path -
    // this will "just work" as the "File Mapper" system will recursively
    // memory map the files as the path is parsed. If the compressed file
    // is non-compressed, it won't be decompressed but will be memory mapped
    // directly into the original file, in this case "data.zip". The memory
    // map will simply be offset to where the file is stored within the zip.
    File file("data.zip/foo/test.jpg");
}

void example4()
{
    // Let's say, that our application has it's data in a container -
    // that means the zip file will be opened and parsed all the time
    // which would be very inefficient. We don't cache anything behind the
    // scenes and it won't run fast by magic or wishful thinking. We have
    // engineered a way around the inefficiencies: sub-pathing.
    // here's how it's done..

    // Create a path object.. this will create a permanent mapping into the
    // compressed container.
    Path path("data.zip/");

    // Open multiple files with the path..
    File file1(path, "test1.jpg");
    File file2(path, "test2.jpg");
}

void example5(const std::string& pathname)
{
    // Remember the first example? Iterating all files in a path?
    // If we want to recursively find out what's in the sub-folders,
    // we could just modify the pathname like this:

    Path path(pathname);
    for (auto i : path)
    {
        if (i.isDirectory())
        {
            // recurse into subdirectories
            example5(pathname + i.name);
        }
    }
}

void example6(const Path& parent)
{
    // The previous example works just fine there is no question about that.
    // But why should we keep repeating the same work when we can simply
    // use the parent path as root? Let's check how it's done!

    for (auto i : parent)
    {
        if (i.isDirectory())
        {
            // recurse into subdirectories
            Path path(parent, i.name);
            example6(path);
        }
    }

    // How this works requires a quick look how this all is implemented.
    // The Path object, when it is being constructed from the "pathname" string,
    // is parsed and everytime a container, such as zip, rar or other supported
    // type is detected a new Path object is created internally using the
    // current path object as it's root object it uses for mapping the memory.

    // This design allows the system to "just work". The downside is that
    // recursively parsing compressed files, say, zip inside zip inside rar,
    // the decompression has to be done and that may consume insane amounts
    // of memory. Such situation is the pathological case and should be
    // avoided. Situations where really large files are compressed the
    // streaming API is much better solution (the large file is memory mapped
    // with File/Path abstraction but the contents are interpreted as raw data
    // and decompressed as a stream so that the memory usage is kept at
    // sensible levels).
}

void example7()
{
    // To recap what we have learned so far, if this works:
    Path path("data.zip/");
    File file(path, "images/test.jpg");

    // So does this
    Bitmap bitmap("data.zip/images/test.jpg");

    // This works even better as we can re-use the mapping (Path object)
    Bitmap bitmap2(file, file.filename());

    // Let's open up the above bitmap creation a little bit..
    ConstMemory memory = file; // memory map view of the file
    std::string filename = file.filename();
    // Decode the image from memory map view of the file
    // The second argument is actually extension of the filename but
    // the decoder parses the parameter and only sees ".jpg" so it's all good.
    Bitmap bitmap3(memory, filename);
}

/*
    Generally, the MANGO(tm) always parses from memory when reading image files,
    decompresses textures or does anything I/O related. This way it is possible
    to layer services on top of each other. Some time I/O needs to be
    serialized so the memory mapped file is read into staging buffers which
    are then processed. This is the way most libraries used to work with the
    difference that such behaviour was built-in and mandated by the library design.
    In our solution this is a choise the developer can make himself based on
    how the asset loading processing pipeline is built. We provide the low-level
    components which can be used as building blocks for higher level constructs.
*/

int main()
{
    // NOTE: This code is compiled for validation purposes
}
