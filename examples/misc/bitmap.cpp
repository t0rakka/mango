/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <mango/image/image.hpp>

using namespace mango;
using namespace mango::image;

/*
    First let's do trivial image loading using traditional "Bitmap" object technique:
*/
void example1(const std::string& filename)
{
    Bitmap bitmap(filename);
}

/*
    Load the image from a block of memory. The loader needs to know what
    fileformat the bits are in and that is figured out by looking at the
    extension of the filename and contents of the memory.
*/
void example2(const Memory& memory, const std::string& filename)
{
    Bitmap bitmap(memory, filename);
}

/*
    Load image from a pointer. uses the same mechanics as the previous example
    because the Memory block is the abstraction we use. This ties-in with the
    first example where we give a filename; internally the library does open
    the file and memory map it's contents to current process address space and
    does the processing using a Memory block so these all examples are in fact
    convenience methods for the same mechanism doing the work under the hood.
*/
void example3(u8* data, size_t size)
{
    Memory memory(data, size);
    Bitmap bitmap(memory, "jpg"); // we know it's a jpeg file
}

/*
    Different strategies for dealing with pixel formats
*/
void example4()
{
    // Convenience constructor allows the user to define what format he wants the
    // image to be stored in. This is not optimal arrangement as usually artist would
    // choose the format in the image file itself to his liking.
    // We MUST have BGRA5551 and NOTHING else will satisfy us!
    Bitmap bitmap("test.tga", Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1));

    // Another nice feature we have is that one can blit between bitmaps and surfaces.
    // The library will do pixel format conversion using the fastest available converter.
    Bitmap bitmap2(1920, 1080, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    bitmap2.blit(0, 0, bitmap);
}

/*
    Let's see how deep the rabbit hole goes! The image encoding / decoding
    system in MANGO(tm) is in fact a swiss-knife which allows to construct
    different image loading pipelines which work seamlessly with different
    real-world use cases. Integration with OpenGL, DirectX, Vulkan and other
    rendering systems "just works" and can be implemented efficiently.
*/
void example5(const Memory& memory, const std::string& extension)
{
    // First we need to create a "decoder" object which sees the block of memory
    ImageDecoder decoder(memory, extension);

    // We can ask the imaging system if a specific format is supported but in this
    // example we choose to do it this way; we ask the decoder if it can decode
    // the given data (eg. memory and filename extension)
    if (decoder.isDecoder())
    {
        // Next we ask for the header to see what the file contents are
        // The header tells us the image dimensions, format, number of cubemap faces,
        // mipmap levels and other information which we can use to make a decision
        // what to do with the data with the renderer we use and it's capabilities.
        ImageHeader header = decoder.header();

        // It is possible, for example, to create OpenGL texture object,
        // allocate surface and use Pixelbuffer Object to map the texture
        // to get address where we can blit the image we are decoding.

        // Since this is a simple example we just allocate a buffer.
        const size_t stride = header.width * header.format.bytes();
        std::vector<u8> buffer(header.height * stride);

        // This is just "Image Pointer" ; mango::Surface is just surface
        // description so that the decoder / blitter knows how to interpret the
        // memory it sees (the buffer).
        Surface surface(header.width, header.height, header.format, stride, buffer.data());

        // Decode the image. The first parameter is the target surface which
        // describes the decoding target memory. The decoder does clip against
        // the target surface so it can be smaller (or larger) than the image
        // being decoded. If the target has different format then format conversion
        // is done so that the color data is written out in correct format.
        //
        // It is, of course, highly recommended that the format and size match
        // what the image header reports so that the energy consumption is reduced.
        // The blitter is multi-threaded internally so depending on number of
        // CPU cores the realtime spent on the process can be significantly reduced
        // but we are still consuming a lot of CPU time which translates into
        // energy consumption which means that on a mobile device battery is drained
        // quicker. So let's be smart about this even if we make it convenient not to.
        //
        // OKAY, so let's say we did not have any clipping or format conversion
        // going on with our decoding (see above, our surface object is initialized
        // with the recommended values from the image header!). This means that
        // the "blitter" is actually by-passed and we are in the so-called "fast mode",
        // this is a very good thing since the decoders are written to write
        // directly into the memory address given in the surface object.
        //
        // What this in turn means is that we can have this kind of image loading
        // taking place in our graphics system:
        // - decoder reads from a memory mapped file
        // - decoder writes into surface (which could be a memory mapped texture!)
        // A traditional image loading pipeline would first open a file,
        // read it's contents into a buffer. This already involves the reads going
        // through a cache the OS is keeping around. So at least two copies of data
        // are being made (First in the OS, second in buffer user has for reading the file).
        // Then the image loader would parse the data and decode it into user-supplied
        // buffer. Third copy. Then finally, user would copy the data to the API.
        // The last step could be merged in some cases by limiting the texture
        // formats and dimensions which are supported.

        // We on the other hand, can _always_ make the shortest possible
        // connection between the memory mapped file and the surface target
        // memory since we can write directly to the target format, regardless of what
        // it is. This allows to match the graphics capabilities of the target
        // platform and the image file contents in much more flexible ways and reduce
        // memory bandwidth usage significantly. Want to create an texture atlas
        // *dynamically* with very low overhead? Piece of cake. "Just do it."

        // The most EXTREME variant of "image loading" is one which can be done
        // when the "APPLE_client_storage" extension is detected. This allows
        // any supported compressed texture formats to be memory mapped
        // and passed directly to the OpenGL driver so that the hardware will
        // fetch the data from the memory mapped file when it is required. The
        // "loading" is done on demand and no intermediate memory buffers, copies,
        // or decoding is done. The memory management is also very simple:
        // after the texture object is no longer needed the memory mapping is released.
        // This does not even consume any memory in the current process, it only
        // consumes some address space. If there is high memory pressure the OS
        // can page out the data as the physical memory is needed somewhere else.

        // TL;DR - decode the image
        decoder.decode(surface);
    }
}

int main()
{
    // NOTE: This code is compiled for validation purposes
}
