/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/memory.hpp>

namespace mango {

    // -----------------------------------------------------------------------
    // Memory
    // -----------------------------------------------------------------------

    Memory::Memory()
    : object(NULL), size(0), address(NULL)
    {
    }

    Memory::Memory(const uint8* _address, size_t _size)
    : object(NULL), size(_size), address(_address)
    {
    }

	Memory::Memory(const Memory& memory)
	: object(memory.object), size(memory.size), address(memory.address)
	{
		if (object) {
			object->retain();
		}
	}

    Memory::~Memory()
    {
		if (object) {
			int count = object->release();
			if (!count) {
				delete[] address;
			}
		}
    }

	const Memory& Memory::operator = (const Memory& memory)
	{
		if (this != &memory) {
			if (object)	{
				int count = object->release();
				if (!count)	{
					delete[] address;
				}
			}

			object = memory.object;
			size = memory.size;
			address = memory.address;

			if (object) {
				object->retain();
			}
		}

		return *this;
	}

	Memory::operator const uint8* () const
	{
		return address;
	}

	Memory::operator const char* () const
	{
		return reinterpret_cast<const char*>(address);
	}

    Memory Memory::getBlock(size_t offset, size_t block_size) const
    {
        Memory block(address + offset, size - offset);
        if (block_size) {
            block.size = std::min(block.size, block_size);
        }
        return block;
    }

    // -----------------------------------------------------------------------
    // ManagedMemory
    // -----------------------------------------------------------------------

    ManagedMemory::ManagedMemory(const uint8* _address, size_t _size)
    : Memory(_address, _size)
    {
        object = new Object();
    }

    ManagedMemory::ManagedMemory(size_t _size)
	: Memory(NULL, _size)
	{
		object = new Object();
		address = new uint8[size];
	}

	ManagedMemory::operator uint8* () const
	{
		return const_cast<uint8*>(address);
	}

	ManagedMemory::operator char* () const
	{
		const char* p = reinterpret_cast<const char*>(address);
		return const_cast<char*>(p);
	}

    // -----------------------------------------------------------------------
    // aligned malloc/free
    // -----------------------------------------------------------------------

#if defined(MANGO_PLATFORM_OSX)

    // NOTE: OSX ABI already aligns memory allocations to 16 bytes

    void* aligned_malloc(size_t size)
    {
        return std::malloc(size);
    }

    void aligned_free(void* aligned)
    {
        std::free(aligned);
    }

#elif defined(MANGO_COMPILER_MICROSOFT)

    void* aligned_malloc(size_t size)
    {
        return _aligned_malloc(size, MANGO_MEMORY_ALIGNMENT);
    }

    void aligned_free(void* aligned)
    {
        _aligned_free(aligned);
    }

#elif defined(MANGO_PLATFORM_UNIX)

    void* aligned_malloc(size_t size)
    {
        return memalign(MANGO_MEMORY_ALIGNMENT, size);
    }

    void aligned_free(void* aligned)
    {
        free(aligned);
    }

#else

    // generic implementation

    void* aligned_malloc(size_t size)
    {
        void* block = std::malloc(size + (MANGO_MEMORY_ALIGNMENT - 1) + sizeof(void*));
        char* aligned = reinterpret_cast<char*>(block) + sizeof(void*);

        if (block) {
            aligned += MANGO_MEMORY_ALIGNMENT - (reinterpret_cast<ptrdiff_t>(aligned) & (MANGO_MEMORY_ALIGNMENT - 1));
            reinterpret_cast<void**>(aligned)[-1] = block;
        }
        else {
            aligned = NULL;
        }

        return aligned;
    }

    void aligned_free(void* aligned)
    {
        if (aligned) {
            void* block = reinterpret_cast<void**>(aligned)[-1];
            std::free(block);
        }
    }

#endif

} // namespace mango
