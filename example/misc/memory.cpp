/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <mango/core/memory.hpp>

using namespace mango;

void example1()
{
    // allocate memory
    std::vector<u8> buffer(20000);

    // point to the allocated memory
    Memory memory(buffer.data(), buffer.size());

    // Memory is a block of memory "owned by someone else"
    // Memory does not "own" or manage the memory it points to; it just like a raw pointer with size

    // It is possible to slice the memory; slice a 500 byte block at offset 1000
    Memory block = memory.slice(1000, 500);

    // That's all there is to know about Memory!

    MANGO_UNREFERENCED(block);
}

void example2()
{
    // SharedMemory is a simple wrapper on top of std::shared_ptr that gives the shared memory
    // The same exact interface as Memory so it can be consumed just like Memory. The difference is,
    // of course that the memory is shared and easy to pass between threads and the last instance
    // releases the allocated memory.

    // Allocate 16 KB of memory
    SharedMemory shared(1024 * 16);

    // A bit different use case.. take ownership of memory allocated by someone else:
    u8* buffer = new u8[40000];
    SharedMemory shared2(buffer, 40000);

    // The buffer is now owned by shared2 and releasing the memory would be a programming error:
    delete [] buffer; // ERROR! 
}

void example3(VirtualMemory *vm)
{
    // What on EARTH is VirtualMemory!? It is a type of Memory that is created by some
    // unknown superpower, who also knows how to release the memory. For example, the MANGO
    // memory mapped files are VirtualMemory as the implemented interface will know how to
    // unmap the file, free the memory and do other clean up tasks.

    // Another example of VirtualMemory is compressed files. We cannot know if the file will be
    // compressed or stored in the archive file and the back-end treatment will vary. If the file is
    // compressed the decompressed buffer will be returned by the VirtualMemory implementation. If the
    // file is merely stored, the implementation can for example mmap the file AT THE OFFSET the
    // stored file is at; this is zero-cost abstraction! 

    // Thus, the VirtualMemory again has the same Memory interface the other memory types:
    ConstMemory memory = *vm;

    // So any code that knows how to read from Memory, knows how to read from both SharedMemory
    // and VirtualMemory. These interfaces might at initial glance look superficial but they
    // make it possible to work with compressed files, memory mapped files, allocated memory buffers,
    // OS virtual memory and other kinds of memory using unified interface which is as light-weight
    // as raw pointers.

    MANGO_UNREFERENCED(memory);
}

int main()
{
    // NOTE: This code is compiled for validation purposes
}
