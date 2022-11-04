/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_HEIF)

#include <libheif/heif.h>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ImageHeader m_header;

        heif_context* m_context = nullptr;
        heif_image_handle* m_image_handle = nullptr;

        Interface(ConstMemory memory)
        {
            heif_error error;

            error = heif_init(nullptr);
            if (error.code != heif_error_Ok)
            {
                // ...
                debugPrint("%s\n", error.message);
                return;
            }

            if (memory.size < 12)
            {
                // ...
                return;
            }

            heif_filetype_result filetype = heif_check_filetype(memory.address, memory.size);
            debugPrint("  - filetype: %d\n", filetype);

            heif_brand2 brand = heif_read_main_brand(memory.address, memory.size);
            debugPrint("  - brand: %d\n", brand);

            const char* mime = heif_get_file_mime_type(memory.address, memory.size);
            debugPrint("  - mime: %s\n", mime);

            /*
            heif_filetype_result filetype = heif_check_filetype(memory.address, memory.size);
            if (filetype != heif_filetype_yes_supported)
            {
                // ...
                debugPrint("not heif.\n");
                return;
            }
            */

            m_context = heif_context_alloc();
            if (!m_context)
            {
                // ...
                return;
            }

            error = heif_context_read_from_memory_without_copy(m_context, memory.address, memory.size, nullptr);
            if (error.code != heif_error_Ok)
            {
                // ...
                debugPrint("%s\n", error.message);
                return;
            }

            error = heif_context_get_primary_image_handle(m_context, &m_image_handle);
            if (error.code != heif_error_Ok)
            {
                // ...
                debugPrint("%s\n", error.message);
                return;
            }

            int width = heif_image_handle_get_width(m_image_handle);
            int height = heif_image_handle_get_height(m_image_handle);
            int bpp = heif_image_handle_get_luma_bits_per_pixel(m_image_handle);
            int cbpp = heif_image_handle_get_chroma_bits_per_pixel(m_image_handle);
            int alpha = heif_image_handle_has_alpha_channel(m_image_handle);
            int luma = heif_image_handle_get_luma_bits_per_pixel(m_image_handle);

            printf("image: %d x %d, bits: %d, chroma: %d, alpha: %d, luma: %d\n", 
                width, height, bpp, cbpp, alpha, luma);

            // TODO
            Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
            m_header.palette = false;
            m_header.format  = format;
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
            if (m_image_handle)
            {
                heif_image_handle_release(m_image_handle);
            }

            if (m_context)
            {
                heif_context_free(m_context);
            }

            heif_deinit();
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ConstMemory icc() override
        {
            return ConstMemory();
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

            heif_decoding_options* decode_options = heif_decoding_options_alloc();

            printf("version: %d\n", decode_options->version);
            decode_options->convert_hdr_to_8bit = true;
            decode_options->ignore_transformations = true;

            heif_image* image = nullptr;
            heif_error error = heif_decode_image(m_image_handle, &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);

            heif_decoding_options_free(decode_options);

            if (error.code != heif_error_Ok)
            {
                // ...
                debugPrint("%s\n", error.message);
                return status;
            }

            // TODO: hdr image, > 8 bits per channel

            /*
            heif_channel_Y = 0,
            heif_channel_Cb = 1,
            heif_channel_Cr = 2,
            heif_channel_R = 3,
            heif_channel_G = 4,
            heif_channel_B = 5,
            heif_channel_Alpha = 6,
            heif_channel_interleaved = 10
            */
            heif_channel ch = heif_channel_interleaved;
            int s0 = heif_image_get_bits_per_pixel(image, ch);
            int s1 = heif_image_get_bits_per_pixel_range(image, ch);
            printf("s0: %d, s1: %d\n", s0, s1);
            if (s0 != 32)
            {
                heif_image_release(image);
                // ... unsupported format ...
                return status;
            }

            int stride = 0;
            const u8* p = heif_image_get_plane_readonly(image, heif_channel_interleaved, &stride);
            if (!p)
            {
                heif_image_release(image);
                // ...
                printf("p: null\n");
                return status;
            }

            Surface temp(m_header.width, m_header.height, m_header.format, stride, p);
            dest.blit(0, 0, temp);

            heif_image_release(image);

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

    ImageEncodeStatus imageEncode(Stream& output, const Surface& surface, const ImageEncodeOptions& options)
    {

/*

 bool has_alpha = (color_type & PNG_COLOR_MASK_ALPHA);

  if (band == 1 && bit_depth==8) {
    err = heif_image_create((int) width, (int) height,
                            heif_colorspace_monochrome,
                            heif_chroma_monochrome,
                            &image);
    (void) err;

    heif_image_add_plane(image, heif_channel_Y, (int) width, (int) height, 8);

    int y_stride;
    int a_stride;
    uint8_t* py = heif_image_get_plane(image, heif_channel_Y, &y_stride);
    uint8_t* pa = nullptr;

    if (has_alpha) {
      heif_image_add_plane(image, heif_channel_Alpha, (int) width, (int) height, 8);

      pa = heif_image_get_plane(image, heif_channel_Alpha, &a_stride);
    }


    for (uint32_t y = 0; y < height; y++) {
      uint8_t* p = row_pointers[y];

      if (has_alpha) {
        for (uint32_t x = 0; x < width; x++) {
          py[y * y_stride + x] = *p++;
          pa[y * a_stride + x] = *p++;
        }
      }
      else {
        memcpy(&py[y * y_stride], p, width);
      }
    }
  }
  else if (band == 1) {
    assert(bit_depth>8);

    err = heif_image_create((int) width, (int) height,
                            heif_colorspace_monochrome,
                            heif_chroma_monochrome,
                            &image);
    (void) err;

    int bdShift = 16 - output_bit_depth;

    heif_image_add_plane(image, heif_channel_Y, (int) width, (int) height, output_bit_depth);

    int y_stride;
    int a_stride = 0;
    uint16_t* py = (uint16_t*)heif_image_get_plane(image, heif_channel_Y, &y_stride);
    uint16_t* pa = nullptr;

    if (has_alpha) {
      heif_image_add_plane(image, heif_channel_Alpha, (int) width, (int) height, output_bit_depth);

      pa = (uint16_t*)heif_image_get_plane(image, heif_channel_Alpha, &a_stride);
    }

    y_stride /= 2;
    a_stride /= 2;

    for (uint32_t y = 0; y < height; y++) {
      uint8_t* p = row_pointers[y];

      if (has_alpha) {
        for (uint32_t x = 0; x < width; x++) {
          uint16_t vp = (uint16_t) (((p[0] << 8) | p[1]) >> bdShift);
          uint16_t va = (uint16_t) (((p[2] << 8) | p[3]) >> bdShift);

          py[x + y * y_stride] = vp;
          pa[x + y * y_stride] = va;

          p += 4;
        }
      }
      else {
        for (uint32_t x = 0; x < width; x++) {
          uint16_t vp = (uint16_t) (((p[0] << 8) | p[1]) >> bdShift);

          py[x + y * y_stride] = vp;

          p += 2;
        }
      }
    }
  }
  else if (bit_depth == 8) {
    err = heif_image_create((int) width, (int) height,
                            heif_colorspace_RGB,
                            has_alpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
                            &image);
    (void) err;

    heif_image_add_plane(image, heif_channel_interleaved, (int) width, (int) height,
                         has_alpha ? 32 : 24);

    int stride;
    uint8_t* p = heif_image_get_plane(image, heif_channel_interleaved, &stride);

    for (uint32_t y = 0; y < height; y++) {
      if (has_alpha) {
        memcpy(p + y * stride, row_pointers[y], width * 4);
      }
      else {
        memcpy(p + y * stride, row_pointers[y], width * 3);
      }
    }
  }
  else {
    err = heif_image_create((int) width, (int) height,
                            heif_colorspace_RGB,
                            has_alpha ?
                            heif_chroma_interleaved_RRGGBBAA_LE :
                            heif_chroma_interleaved_RRGGBB_LE,
                            &image);
    (void) err;

    int bdShift = 16 - output_bit_depth;

    heif_image_add_plane(image, heif_channel_interleaved, (int) width, (int) height, output_bit_depth);

    int stride;
    uint8_t* p_out = (uint8_t*) heif_image_get_plane(image, heif_channel_interleaved, &stride);

    for (uint32_t y = 0; y < height; y++) {
      uint8_t* p = row_pointers[y];

      uint32_t nVal = (has_alpha ? 4 : 3) * width;

      for (uint32_t x = 0; x < nVal; x++) {
        uint16_t v = (uint16_t) (((p[0] << 8) | p[1]) >> bdShift);
        p_out[2 * x + y * stride + 1] = (uint8_t) (v >> 8);
        p_out[2 * x + y * stride + 0] = (uint8_t) (v & 0xFF);
        p += 2;
      }
    }
  }

  if (profile_data && profile_length > 0) {
    heif_image_set_raw_color_profile(image, "prof", profile_data, (size_t) profile_length);
  }

  free(profile_data);
  for (uint32_t y = 0; y < height; y++) {
    free(row_pointers[y]);
  } // for

  delete[] row_pointers;

  input_image.image = std::shared_ptr<heif_image>(image,
                                                  [](heif_image* img) { heif_image_release(img); });

  return input_image;

*/



        // TODO
        ImageEncodeStatus status;
        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecHEIF()
    {
        registerImageDecoder(createInterface, ".heif");
        registerImageDecoder(createInterface, ".heic");

        // TODO
        MANGO_UNREFERENCED(imageEncode);
        /*
        registerImageEncoder(imageEncode, ".xxx");
        */
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_HEIF)
