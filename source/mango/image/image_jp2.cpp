/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_JP2)

#include <openjpeg.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // OpenJPEG interface
    // ------------------------------------------------------------

#if 0

    struct opj_memory_stream
    {
        OPJ_UINT8* data;
        OPJ_SIZE_T size;
        OPJ_SIZE_T offset;
    };

    static
    OPJ_SIZE_T opj_memory_stream_read(void* buffer, OPJ_SIZE_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (memory->offset >= memory->size)
            return (OPJ_SIZE_T) -1;

        if (bytes > (memory->size - memory->offset))
            bytes = memory->size - memory->offset;

        std::memcpy(buffer, memory->data + memory->offset, bytes);
        memory->offset += bytes;

        return bytes;
    }

    static
    OPJ_SIZE_T opj_memory_stream_write(void* buffer, OPJ_SIZE_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (memory->offset >= memory->size)
            return (OPJ_SIZE_T) -1;

        if (bytes > (memory->size - memory->offset))
            bytes = memory->size - memory->offset;

        std::memcpy(memory->data + memory->offset, buffer, bytes);
        memory->offset += bytes;

        return bytes;
    }

    static
    OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (bytes < 0)
            return -1;

        if (bytes > memory->size - memory->offset)
            bytes = memory->size - memory->offset;

        memory->offset += bytes;

        return bytes;
    }

    static
    OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T bytes, void* user_data)
    {
        opj_memory_stream* memory = (opj_memory_stream*)user_data;

        if (bytes < 0 || bytes > (OPJ_OFF_T)memory->size)
            return OPJ_FALSE;

        memory->offset = (OPJ_SIZE_T)bytes;

        return OPJ_TRUE;
    }

    static
    void opj_memory_stream_do_nothing(void* user_data)
    {
        OPJ_ARG_NOT_USED(user_data);
    }

    opj_stream_t* opj_stream_create_default_memory_stream(opj_memory_stream* memory, OPJ_BOOL is_read_stream)
    {
        opj_stream_t* stream = opj_stream_default_create(is_read_stream);
        if (stream)
        {
            if (is_read_stream)
                opj_stream_set_read_function(stream, opj_memory_stream_read);
            else
                opj_stream_set_write_function(stream, opj_memory_stream_write);

            opj_stream_set_seek_function(stream, opj_memory_stream_seek);
            opj_stream_set_skip_function(stream, opj_memory_stream_skip);
            opj_stream_set_user_data(stream, memory, opj_memory_stream_do_nothing);
            opj_stream_set_user_data_length(stream, memory->size);
        }

        return stream;
    }

#endif

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;
        Memory m_icc;

        Interface(ConstMemory memory)
        {
            // TODO
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory icc() override
        {
            return m_icc;
        }

        ConstMemory exif() override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;
            // TODO

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;
        // TODO

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecJP2()
    {
        // TODO
        //registerImageDecoder(createInterface, ".jp2");
        //registerImageEncoder(imageEncode, ".jp2");

        (void) createInterface;
        (void) imageEncode;
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_JP2)
