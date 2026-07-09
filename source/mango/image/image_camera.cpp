/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

// Camera RAW decoder backed by LibRaw (https://www.libraw.org/).
//
// LibRaw identifies the camera model directly from the in-memory buffer
// (open_buffer) and decodes the mosaiced sensor data into a demosaiced image.
// Sensors have a wide range of native precisions (12/14/16-bit), so we request
// 16-bit linear output and present it verbatim as linear UNORM16 RGBA. Keeping
// the integer samples (rather than converting to FLOAT16 here) is lossless and
// lets the caller decide: a memcpy when their surface already matches, a blit
// conversion otherwise, or linearize() to reach a scene-linear float space.

#include <libraw/libraw.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    // Apply the shared output configuration: 16-bit linear, camera white balance,
    // sRGB (Rec.709) primaries, no auto-brightening. This keeps the decoded pixels
    // scene-linear so the UNORM16 surface preserves the sensor's dynamic range and
    // precision.
    static void configure(LibRaw& raw)
    {
        libraw_output_params_t& params = raw.imgdata.params;
        params.output_bps = 16;       // 16 bits per sample
        params.output_color = 1;      // sRGB / Rec.709 primaries
        params.gamm[0] = 1.0;         // linear transfer (gamma 1.0)
        params.gamm[1] = 1.0;
        params.no_auto_bright = 1;    // do not auto-scale exposure
        params.use_camera_wb = 1;     // apply the camera's white balance
    }

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            // NOTE: sizeof(LibRaw) is ~768 KB (it embeds libraw_data_t by value), so
            // it MUST live on the heap. A stack local overflows the smaller stacks of
            // worker threads (common when decoding is dispatched asynchronously) and
            // crashes with SIGBUS the moment the object is constructed.
            auto raw = std::make_unique<LibRaw>();
            configure(*raw);

            int result = raw->open_buffer(memory.address, memory.size);
            if (result != LIBRAW_SUCCESS)
            {
                header.setError("[ImageDecoder.CAMERA] open_buffer failed: {}", libraw_strerror(result));
                return;
            }

            // The pre-process size estimate in imgdata.sizes is not always the final
            // output size (interpolated/rotated sensors such as the Fuji Super CCD
            // change it). adjust_sizes_info_only() resolves the true output geometry
            // -- including the sensor flip swap -- without the cost of unpacking, so
            // the actual decode can stay lazy in decode().
            result = raw->adjust_sizes_info_only();
            if (result != LIBRAW_SUCCESS)
            {
                header.setError("[ImageDecoder.CAMERA] adjust_sizes_info_only failed: {}", libraw_strerror(result));
                return;
            }

            const int width = raw->imgdata.sizes.iwidth;
            const int height = raw->imgdata.sizes.iheight;

            if (width <= 0 || height <= 0)
            {
                header.setError("[ImageDecoder.CAMERA] Invalid image dimensions ({} x {}).", width, height);
                return;
            }

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.format  = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16, Format::LINEAR);
            header.compression = TextureCompression::NONE;

            // Scene-linear output with sRGB/Rec.709 primaries (output_color == 1).
            header.linear = true;
            header.color.primaries = ColorPrimaries::BT709;
            header.color.transfer = TransferFunction::Linear;
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            // Heap-allocated: see the note in the constructor (sizeof(LibRaw) ~768 KB
            // would overflow a worker thread's stack and SIGBUS on construction).
            auto raw = std::make_unique<LibRaw>();
            configure(*raw);

            int result = raw->open_buffer(m_memory.address, m_memory.size);
            if (result != LIBRAW_SUCCESS)
            {
                status.setError("[ImageDecoder.CAMERA] open_buffer failed: {}", libraw_strerror(result));
                return status;
            }

            result = raw->unpack();
            if (result != LIBRAW_SUCCESS)
            {
                status.setError("[ImageDecoder.CAMERA] unpack failed: {}", libraw_strerror(result));
                return status;
            }

            result = raw->dcraw_process();
            if (result != LIBRAW_SUCCESS)
            {
                status.setError("[ImageDecoder.CAMERA] dcraw_process failed: {}", libraw_strerror(result));
                return status;
            }

            int errc = 0;
            libraw_processed_image_t* image = raw->dcraw_make_mem_image(&errc);
            if (!image)
            {
                status.setError("[ImageDecoder.CAMERA] dcraw_make_mem_image failed: {}", libraw_strerror(errc));
                return status;
            }

            // dcraw_make_mem_image always produces an interleaved bitmap.
            if (image->type != LIBRAW_IMAGE_BITMAP || (image->colors != 3 && image->colors != 1))
            {
                LibRaw::dcraw_clear_mem(image);
                status.setError("[ImageDecoder.CAMERA] Unsupported processed image (type {}, {} colors).",
                    int(image->type), int(image->colors));
                return status;
            }

            const int width = image->width;
            const int height = image->height;
            const int colors = image->colors;
            const int bits = image->bits;

            // Guard the buffer geometry so the conversion can never read past the
            // data block (adjust_sizes_info_only should have matched these dims).
            const size_t bytes_per_sample = (bits > 8) ? 2 : 1;
            const size_t expected = size_t(width) * height * colors * bytes_per_sample;
            if (size_t(image->data_size) < expected)
            {
                LibRaw::dcraw_clear_mem(image);
                status.setError("[ImageDecoder.CAMERA] Unexpected processed image geometry.");
                return status;
            }

            // Present the linear integer samples as UNORM16 RGBA (header.format). The
            // DecodeTargetBitmap wraps 'dest' directly when it already matches, so the
            // resolve() degrades to a memcpy; otherwise it allocates a temporary in our
            // format and resolve() converts into the caller's surface.
            DecodeTargetBitmap target(dest, width, height, header.format);

            const int channel_bits = int(bytes_per_sample) * 8;
            const size_t stride = size_t(width) * colors * bytes_per_sample;

            // Wrap LibRaw's interleaved buffer and let the blitter normalize it into the
            // (UNORM16 RGBA) target. RGB synthesizes opaque alpha; a monochrome sensor's
            // single channel is broadcast to RGB -- both handled by the generic converter.
            Format source = (colors == 3)
                ? Format(colors * channel_bits, Format::UNORM, Format::RGB,
                         channel_bits, channel_bits, channel_bits, 0, Format::LINEAR)
                : LuminanceFormat(channel_bits, Format::UNORM, channel_bits, 0, Format::LINEAR);

            Surface raw_surface(width, height, source, stride, image->data);
            target.blit(0, 0, raw_surface);

            LibRaw::dcraw_clear_mem(image);

            target.resolve();
            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecCAMERA()
    {
        // Common camera raw extensions handled by LibRaw. ".raw" is intentionally
        // omitted (claimed by the internal FBMP codec and not a reliable signature).
        const char* extensions [] =
        {
            ".dng",  // Adobe / generic (also many phones)
            ".cr2", ".cr3", ".crw",          // Canon
            ".nef", ".nrw",                  // Nikon
            ".arw", ".srf", ".sr2",          // Sony
            ".raf",                          // Fujifilm
            ".orf",                          // Olympus
            ".rw2",                          // Panasonic
            ".pef",                          // Pentax
            ".srw",                          // Samsung
            ".x3f",                          // Sigma
            ".erf",                          // Epson
            ".mrw",                          // Minolta
            ".dcr", ".kdc",                  // Kodak
            ".3fr", ".fff",                  // Hasselblad
            ".iiq",                          // Phase One
            ".mos",                          // Leaf
            ".mef",                          // Mamiya
            ".rwl",                          // Leica
            ".gpr",                          // GoPro
        };

        for (const char* ext : extensions)
        {
            registerImageDecoder(createInterface, ext);
        }
    }

} // namespace mango::image
