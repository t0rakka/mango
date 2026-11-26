/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <mango/core/half.hpp>
#include <mango/core/print.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/timer.hpp>
#include <mango/image/bicubic.hpp>
#include <mango/image/decoder.hpp>
#include <mango/image/jpeg.hpp>
#include <mango/image/surface.hpp>
#include <mango/vulkan/context.hpp>
#include <mango/vulkan/texture_decoder.hpp>
#include <mango/vulkan/vulkan.hpp>

#include "jpeg_compute.hpp"

namespace mango::vulkan
{

namespace
{

using image::Bitmap;
using image::ColorInfo;
using image::ColorPrimaries;
using image::Format;
using image::ImageDecoder;
using image::ImageHeader;
using image::Surface;
using image::TextureCompression;
using image::TransferFunction;

double elapsedMs(u64 t0)
{
    return double(Time::us() - t0) / 1000.0;
}

// ---------------------------------------------------------------------------
// colour plan (iFap classifyColor)
// ---------------------------------------------------------------------------

Format formatU8()  { return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8); }
Format formatU16() { return Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16); }
Format formatF16() { return Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16); }
Format formatF32() { return Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32); }

Format decodeBitmapFormat(const ImageHeader& header)
{
    // Palette/indexed sources decode into RGBA; mango resolves the palette on
    // decode when the destination format does not match the file format.
    if (header.format.isIndexed())
        return header.format.bits > 8 ? formatU16() : formatU8();

    if (header.format.isFloat())
        return header.format.bits > 64 ? formatF32() : formatF16();

    if (header.format.bits > 32)
        return formatU16();

    return formatU8();
}

struct ColorPlan
{
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;  // sampled texture format
    Format bitmap_format = formatU8();          // CPU decode layout
    bool convert = false;                       // linearize() to scene-linear BT.709 fp16
};

// Fast paths keep BT.709 sRGB/linear content in a format the sampler decodes;
// everything else (other primaries, exact chromaticities, PQ/HLG, explicit
// gamma) is baked to scene-linear BT.709 fp16. ICC-tagged images stay on the
// sRGB path.
ColorPlan classifyColor(const ImageHeader& header, bool has_icc)
{
    const ColorInfo& color = header.color;
    const bool is_float = header.format.isFloat();

    ColorPlan plan;

    ColorPrimaries prim = color.primaries;
    if (color.has_chromaticities)
    {
        prim = image::identifyPrimaries(color.white, color.red, color.green, color.blue);
    }

    TransferFunction transfer = color.transfer;
    if (transfer == TransferFunction::Unspecified)
    {
        transfer = is_float ? TransferFunction::Linear : TransferFunction::sRGB;
    }

    const bool explicit_gamma = color.gamma != 0.0f;
    const bool bt709 = prim == ColorPrimaries::BT709;

    if (header.compression != TextureCompression::NONE && !is_float)
    {
        TextureCompression info(header.compression);
        plan.bitmap_format = decodeBitmapFormat(header);
        if (!info.isLinear())
        {
            plan.convert = true;
            plan.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        }
        else
        {
            plan.format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        return plan;
    }

    if (bt709 && !has_icc && !explicit_gamma)
    {
        if (transfer == TransferFunction::Linear)
        {
            if (is_float)
            {
                const bool wide = header.format.bits > 64;
                plan.format = wide ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R16G16B16A16_SFLOAT;
                plan.bitmap_format = wide ? formatF32() : formatF16();
            }
            else if (header.format.bits > 32)
            {
                plan.convert = true;
                plan.format = VK_FORMAT_R16G16B16A16_SFLOAT;
                plan.bitmap_format = formatU16();
            }
            else
            {
                plan.format = VK_FORMAT_R8G8B8A8_UNORM;
                plan.bitmap_format = decodeBitmapFormat(header);
            }
            return plan;
        }

        if (transfer == TransferFunction::sRGB && !is_float)
        {
            if (header.format.bits > 32)
            {
                plan.convert = true;
                plan.format = VK_FORMAT_R16G16B16A16_SFLOAT;
                plan.bitmap_format = formatU16();
            }
            else
            {
                plan.format = VK_FORMAT_R8G8B8A8_SRGB;
                plan.bitmap_format = decodeBitmapFormat(header);
            }
            return plan;
        }
    }

    if (has_icc)
    {
        if (is_float)
        {
            plan.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            plan.bitmap_format = formatF16();
        }
        else if (header.format.bits > 32)
        {
            plan.convert = true;
            plan.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            plan.bitmap_format = formatU16();
        }
        else
        {
            plan.format = VK_FORMAT_R8G8B8A8_SRGB;
            plan.bitmap_format = decodeBitmapFormat(header);
        }
        return plan;
    }

    plan.convert = true;
    plan.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    plan.bitmap_format = decodeBitmapFormat(header);
    return plan;
}

bool isHdrContent(const ImageHeader& header, const ColorPlan& plan)
{
    if (header.format.isFloat() || header.linear)
        return true;

    TransferFunction transfer = header.color.transfer;
    if (transfer == TransferFunction::Unspecified)
        transfer = TransferFunction::sRGB;

    if (transfer == TransferFunction::PQ || transfer == TransferFunction::HLG)
        return true;

    if (header.compression != TextureCompression::NONE)
        return false;

    if (plan.convert && header.format.bits > 32)
        return transfer == TransferFunction::Linear;

    return false;
}

// ---------------------------------------------------------------------------
// report strings
// ---------------------------------------------------------------------------

std::string primariesName(ColorPrimaries p)
{
    switch (p)
    {
        case ColorPrimaries::BT709: return "BT.709";
        case ColorPrimaries::BT470M: return "BT.470M";
        case ColorPrimaries::BT601_625: return "BT.601-625";
        case ColorPrimaries::BT601_525: return "BT.601-525";
        case ColorPrimaries::BT2020: return "BT.2020";
        case ColorPrimaries::SMPTE428: return "XYZ";
        case ColorPrimaries::DCI_P3: return "DCI-P3";
        case ColorPrimaries::DisplayP3: return "Display P3";
        case ColorPrimaries::AdobeRGB: return "Adobe RGB";
        case ColorPrimaries::ACES_AP0: return "ACES AP0";
        case ColorPrimaries::ACES_AP1: return "ACES AP1";
        default: return "unspecified";
    }
}

std::string transferName(TransferFunction t)
{
    switch (t)
    {
        case TransferFunction::BT709: return "BT.709";
        case TransferFunction::Gamma22: return "gamma 2.2";
        case TransferFunction::Gamma28: return "gamma 2.8";
        case TransferFunction::Linear: return "linear";
        case TransferFunction::sRGB: return "sRGB";
        case TransferFunction::PQ: return "PQ";
        case TransferFunction::HLG: return "HLG";
        default: return "unspecified";
    }
}

std::string colorSourceName(const ColorInfo& color)
{
    std::string s = primariesName(color.primaries);
    if (color.has_chromaticities)
    {
        const ColorPrimaries id = image::identifyPrimaries(color.white, color.red, color.green, color.blue);
        s = id == ColorPrimaries::Unspecified ? std::string("custom chromaticities") : primariesName(id) + " (chromaticities)";
    }
    s += " ";
    s += transferName(color.transfer);
    if (color.gamma != 0.0f)
        s += fmt::format(" gamma {:.3f}", color.gamma);
    return s;
}

std::string formatName(const Format& format)
{
    if (format.isIndexed())
        return "indexed";

    const char* type = format.isFloat() ? "float" : "integer";
    std::string s = fmt::format("{}-bit {}", format.bits, type);
    if (format.isLuminance())
        s += " luminance";
    return s;
}

std::string lowerExtension(const std::string& filename)
{
    const size_t dot = filename.find_last_of('.');
    std::string ext = dot == std::string::npos ? filename : filename.substr(dot);
    for (char& c : ext)
        c = char(std::tolower(u8(c)));
    return ext;
}

bool isJpegExtension(const std::string& ext)
{
    return ext == ".jpg" || ext == ".jpeg" || ext == ".jpe" || ext == ".jfif" || ext == ".jfi";
}

u32 readBE32(const u8* p)
{
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}

// The profile's 'desc' tag: textDescriptionType in ICC v2, multiLocalizedUnicodeType in v4.
std::string iccDescription(ConstMemory icc)
{
    if (icc.size < 132)
        return {};

    const u8* base = icc.address;
    const u32 count = readBE32(base + 128);
    for (u32 i = 0; i < count; ++i)
    {
        const size_t entry = 132 + size_t(i) * 12;
        if (entry + 12 > icc.size)
            break;

        if (std::memcmp(base + entry, "desc", 4) != 0)
            continue;

        const size_t offset = readBE32(base + entry + 4);
        const size_t size = readBE32(base + entry + 8);
        if (offset + size > icc.size || size < 12)
            return {};

        const u8* tag = base + offset;
        std::string text;

        if (std::memcmp(tag, "desc", 4) == 0)
        {
            const size_t n = std::min<size_t>(readBE32(tag + 8), size - 12);
            for (size_t k = 0; k < n && tag[12 + k]; ++k)
                text += char(tag[12 + k]);
        }
        else if (std::memcmp(tag, "mluc", 4) == 0 && size >= 28)
        {
            const u32 records = readBE32(tag + 8);
            if (records > 0)
            {
                const size_t length = readBE32(tag + 20);
                const size_t start = readBE32(tag + 24);
                if (start + length <= size)
                {
                    for (size_t k = 0; k + 1 < length; k += 2)
                    {
                        const u16 c = u16((tag[start + k] << 8) | tag[start + k + 1]);
                        if (!c)
                            break;
                        text += c < 128 ? char(c) : '?';
                    }
                }
            }
        }

        while (!text.empty() && (text.back() == ' ' || text.back() == '\0'))
            text.pop_back();
        return text;
    }

    return {};
}

// ---------------------------------------------------------------------------
// downscale
// ---------------------------------------------------------------------------

void computeDownscaleDimensions(u32 width, u32 height, u32 max_dimension, u32& out_width, u32& out_height)
{
    out_width = width;
    out_height = height;

    if (width <= max_dimension && height <= max_dimension)
        return;

    const double scale = std::min(double(max_dimension) / double(width),
                                  double(max_dimension) / double(height));
    out_width = std::clamp(u32(double(width) * scale), 1u, max_dimension);
    out_height = std::clamp(u32(double(height) * scale), 1u, max_dimension);
}

template <typename T>
float loadComponent(const T& v)
{
    return float(v);
}

template <typename T>
T storeComponent(float v)
{
    return T(v);
}

template <>
u16 storeComponent<u16>(float v)
{
    return u16(std::clamp(v + 0.5f, 0.0f, 65535.0f));
}

// Box filter with fractional edge coverage on 4-channel pixels.
template <typename T>
void areaDownscale(const Surface& dest, const Surface& source)
{
    const double sx = double(source.width) / double(dest.width);
    const double sy = double(source.height) / double(dest.height);

    ConcurrentQueue q;

    const int block = 16;
    for (int y0 = 0; y0 < dest.height; y0 += block)
    {
        const int y1 = std::min(dest.height, y0 + block);
        q.enqueue([=, &dest, &source]
        {
            std::vector<float> acc(size_t(dest.width) * 4);

            for (int y = y0; y < y1; ++y)
            {
                const double fy0 = y * sy;
                const double fy1 = (y + 1) * sy;
                const int iy0 = int(fy0);
                const int iy1 = std::min(source.height, int(std::ceil(fy1)));

                std::fill(acc.begin(), acc.end(), 0.0f);

                for (int iy = iy0; iy < iy1; ++iy)
                {
                    const float wy = float(std::min(fy1, double(iy + 1)) - std::max(fy0, double(iy)));
                    const T* src = reinterpret_cast<const T*>(source.address(0, iy));

                    for (int x = 0; x < dest.width; ++x)
                    {
                        const double fx0 = x * sx;
                        const double fx1 = (x + 1) * sx;
                        const int ix0 = int(fx0);
                        const int ix1 = std::min(source.width, int(std::ceil(fx1)));

                        float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
                        for (int ix = ix0; ix < ix1; ++ix)
                        {
                            const float wx = float(std::min(fx1, double(ix + 1)) - std::max(fx0, double(ix)));
                            const T* p = src + size_t(ix) * 4;
                            r += loadComponent(p[0]) * wx;
                            g += loadComponent(p[1]) * wx;
                            b += loadComponent(p[2]) * wx;
                            a += loadComponent(p[3]) * wx;
                        }

                        float* o = acc.data() + size_t(x) * 4;
                        o[0] += r * wy;
                        o[1] += g * wy;
                        o[2] += b * wy;
                        o[3] += a * wy;
                    }
                }

                const float scale = float(1.0 / (sx * sy));
                T* dst = reinterpret_cast<T*>(dest.address(0, y));
                for (int i = 0; i < dest.width * 4; ++i)
                {
                    dst[i] = storeComponent<T>(acc[i] * scale);
                }
            }
        });
    }

    q.wait();
}

void downscaleSurface(const Surface& dest, const Surface& source)
{
    const Format& format = source.format;
    if (format.bits == 32 && !format.isFloat())
    {
        image::u32_bicubic_blit(dest, source, 0.0f, 0.0f, float(source.width), float(source.height));
    }
    else if (format.type == Format::FLOAT16)
    {
        areaDownscale<float16>(dest, source);
    }
    else if (format.type == Format::FLOAT32)
    {
        areaDownscale<float>(dest, source);
    }
    else
    {
        areaDownscale<u16>(dest, source);
    }
}

size_t surfaceBytes(const Surface& s)
{
    return size_t(s.width) * size_t(s.height) * size_t(s.format.bytes());
}

} // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct TextureDecoder::Impl
{
    enum class Path
    {
        Cpu,
        Compute,
    };

    struct Job
    {
        Handle handle = 0;
        Callback callback;
        ConstMemory memory;
        std::string extension;

        std::unique_ptr<ImageDecoder> decoder;
        ImageHeader header;
        ColorPlan plan;
        bool downscale = false;
        bool jpeg = false;
        bool storage = false;              // image created for compute writes
        std::string compute_reason;        // set when a JPEG cannot take compute before parsing

        ImageAllocation image;
        TextureInfo info;
        TextureReport report;
        TextureStatus status = TextureStatus::Failed;
        std::string error;
        std::atomic<bool> cancelled { false };
        u64 start_us = 0;
        u64 submit_us = 0; // set at vkQueueSubmit; ready_ms is fence − this

        Path path = Path::Cpu;

        // CPU path
        BufferAllocation staging;
        u32 staging_bytes_per_pixel = 0;
        std::future<void> future;

        // compute path
        std::unique_ptr<detail::JpegComputeJob> compute;
        VkImageView storage_view = VK_NULL_HANDLE;

        // owner thread
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        bool submitted = false;
    };

    using JobPtr = std::shared_ptr<Job>;

    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    VkQueue m_queue;
    u32 m_queue_family;
    Allocator& m_allocator;
    TextureDecoderConfig m_config;

    u32 m_max_dimension = 0;
    bool m_storage_supported = false;
    std::unique_ptr<StagingArena> m_staging;
    std::future<void> m_staging_reserve;
    std::unique_ptr<detail::JpegCompute> m_compute;

    // owner thread resources
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_free_cmds;
    std::vector<VkFence> m_free_fences;

    mutable std::mutex m_mutex;
    std::condition_variable m_worker_cv;
    std::condition_variable m_waiter_cv;
    std::condition_variable m_owner_cv;
    u64 m_owner_signal = 0;
    bool m_stop = false;

    Handle m_next_handle = 0;
    std::unordered_map<Handle, JobPtr> m_jobs;
    std::deque<JobPtr> m_prepare;       // worker: JPEG classification + compute prep
    std::deque<JobPtr> m_cpu_waiting;   // CPU decodes not started yet
    int m_cpu_running = 0;
    std::deque<JobPtr> m_ready;         // owner: needs submit
    std::deque<JobPtr> m_inflight;      // waiter: submitted, in fence order
    std::deque<JobPtr> m_finished;      // owner: retire + callback

    std::thread m_worker;
    std::thread m_waiter;

    Impl(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, u32 family,
         Allocator& allocator, const TextureDecoderConfig& config)
        : m_physical_device(physicalDevice)
        , m_device(device)
        , m_queue(queue)
        , m_queue_family(family)
        , m_allocator(allocator)
        , m_config(config)
    {
        m_config.max_inflight_compute = std::max(m_config.max_inflight_compute, 1);
        m_config.max_concurrent_cpu = std::max(m_config.max_concurrent_cpu, 1);

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        m_max_dimension = props.limits.maxImageDimension2D;

        VkCommandPoolCreateInfo pool_info =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = family,
        };
        vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool);

        if (m_config.compute)
        {
            m_staging = std::make_unique<StagingArena>(allocator, m_config.staging);
            m_compute = std::make_unique<detail::JpegCompute>(physicalDevice, device, allocator,
                *m_staging, m_config.max_inflight_compute);
            m_storage_supported = m_compute->supported();
            if (!m_storage_supported)
            {
                m_compute.reset();
                m_staging.reset();
            }
            else
            {
                // The first chunk costs tens of ms on some drivers; keep it off the first decode.
                m_staging_reserve = std::async(std::launch::async, [this]
                {
                    m_staging->reserve(m_config.staging.initial_chunk);
                });
            }
        }

        m_worker = std::thread([this] { workerMain(); });
        m_waiter = std::thread([this] { waiterMain(); });
    }

    ~Impl()
    {
        {
            std::lock_guard lock(m_mutex);
            for (auto& it : m_jobs)
            {
                cancelLocked(*it.second);
            }
        }

        wait();

        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_worker_cv.notify_all();
        m_waiter_cv.notify_all();
        m_worker.join();
        m_waiter.join();

        for (VkFence fence : m_free_fences)
            vkDestroyFence(m_device, fence, nullptr);

        if (m_command_pool)
            vkDestroyCommandPool(m_device, m_command_pool, nullptr);

        if (m_staging_reserve.valid())
            m_staging_reserve.wait();
        m_compute.reset();
        m_staging.reset();
    }

    StagingArenaStats stagingStats() const
    {
        return m_staging ? m_staging->stats() : StagingArenaStats();
    }

    void trimStaging()
    {
        if (m_staging)
            m_staging->trim();
    }

    void notifyOwner()
    {
        {
            std::lock_guard lock(m_mutex);
            ++m_owner_signal;
        }
        m_owner_cv.notify_all();
        if (m_config.wake)
            m_config.wake();
    }

    void cancelLocked(Job& job)
    {
        job.cancelled = true;
        if (job.decoder)
            job.decoder->cancel();
    }

    // -----------------------------------------------------------------------
    // decode (any thread)
    // -----------------------------------------------------------------------

    Texture decode(ConstMemory memory, const std::string& filename, Callback callback)
    {
        Texture texture;

        const u64 t0 = Time::us();
        auto job = std::make_shared<Job>();
        job->start_us = t0;
        job->callback = std::move(callback);
        job->memory = memory;
        job->extension = lowerExtension(filename);
        job->jpeg = isJpegExtension(job->extension);

        job->decoder = std::make_unique<ImageDecoder>(memory, filename);
        if (!job->decoder->isDecoder())
        {
            texture.error = "unsupported format: " + job->extension;
            return texture;
        }

        ImageHeader& header = job->header;
        header = job->decoder->header();
        if (!header || header.width <= 0 || header.height <= 0)
        {
            texture.error = header.info.empty() ? std::string("header parse failed") : header.info;
            return texture;
        }

        const ConstMemory icc = job->decoder->icc();
        ColorPlan& plan = job->plan;
        plan = classifyColor(header, icc.size > 0);

        TextureReport& report = job->report;
        TextureInfo& info = job->info;

        info.source_width = u32(header.width);
        info.source_height = u32(header.height);
        computeDownscaleDimensions(info.source_width, info.source_height, m_max_dimension, info.width, info.height);
        job->downscale = info.width != info.source_width || info.height != info.source_height;

        info.format = plan.format;
        info.image_format = plan.format;
        info.linear = plan.convert || plan.format != VK_FORMAT_R8G8B8A8_SRGB;
        info.needs_tonemap = isHdrContent(header, plan);
        info.color = header.color;

        std::string ext = job->extension.size() > 1 ? job->extension.substr(1) : job->extension;
        for (char& c : ext)
            c = char(std::toupper(u8(c)));
        report.source_format = ext + " " + formatName(header.format);
        report.downscaled = job->downscale;
        report.icc_present = icc.size > 0;
        if (report.icc_present)
        {
            report.icc_description = iccDescription(icc);
            report.icc_handling = plan.convert ? "ignored, values baked by colour signalling" : "assumed sRGB (values used as-is)";
        }
        report.color_transform = plan.convert;
        report.color_source = colorSourceName(header.color);
        report.hdr = info.needs_tonemap;

        // A JPEG decoded to float carries a gain map the CPU decoder applies.
        report.ultrahdr = job->jpeg && header.format.isFloat();

        if (job->jpeg)
        {
            const u64 pixels = u64(header.width) * u64(header.height);
            if (!m_config.compute)
                job->compute_reason = "compute disabled";
            else if (!m_storage_supported)
                job->compute_reason = "device lacks RGBA8 storage images";
            else if (report.ultrahdr)
                job->compute_reason = "UltraHDR gain map";
            else if (plan.format != VK_FORMAT_R8G8B8A8_SRGB || plan.convert)
                job->compute_reason = "colour plan needs CPU (" + report.color_source + ")";
            else if (job->downscale)
                job->compute_reason = "exceeds maxImageDimension2D";
            else if (pixels < m_config.compute_min_pixels)
                job->compute_reason = fmt::format("below {:.2f} MP", double(m_config.compute_min_pixels) / 1e6);

            job->storage = job->compute_reason.empty();
        }

        report.header_ms = elapsedMs(t0);
        const u64 t1 = Time::us();
        VkFormat view_formats[2] = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB };
        VkImageFormatListCreateInfo format_list =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
            .viewFormatCount = 2,
            .pViewFormats = view_formats,
        };

        VkImageCreateInfo image_info =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = plan.format,
            .extent = { info.width, info.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (job->storage)
        {
            image_info.pNext = &format_list;
            image_info.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
            image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            info.image_format = VK_FORMAT_R8G8B8A8_UNORM;
        }

        job->image = m_allocator.createImage(image_info, MemoryUsage::GpuOnly);
        if (!job->image)
        {
            texture.error = fmt::format("image allocation failed ({} x {})", info.width, info.height);
            return texture;
        }

        VkMemoryRequirements requirements = {};
        vkGetImageMemoryRequirements(m_device, job->image.image, &requirements);
        report.image_bytes = size_t(requirements.size);
        report.image_ms = elapsedMs(t1);

        {
            std::lock_guard lock(m_mutex);
            job->handle = ++m_next_handle;
            m_jobs[job->handle] = job;

            if (job->jpeg)
            {
                m_prepare.push_back(job);
                m_worker_cv.notify_one();
            }
            else
            {
                startCpuLocked(job);
            }
        }

        texture.handle = job->handle;
        texture.image = job->image;
        texture.info = info;
        return texture;
    }

    void cancel(Handle handle)
    {
        std::lock_guard lock(m_mutex);
        auto it = m_jobs.find(handle);
        if (it != m_jobs.end())
        {
            cancelLocked(*it->second);
        }
    }

    size_t pending() const
    {
        std::lock_guard lock(m_mutex);
        return m_jobs.size();
    }

    // -----------------------------------------------------------------------
    // worker: JPEG classification and compute preparation
    // -----------------------------------------------------------------------

    void workerMain()
    {
        for (;;)
        {
            JobPtr job;
            {
                std::unique_lock lock(m_mutex);
                m_worker_cv.wait(lock, [this] { return m_stop || !m_prepare.empty(); });
                if (m_prepare.empty())
                    return;
                job = std::move(m_prepare.front());
                m_prepare.pop_front();
            }

            if (job->cancelled)
            {
                finish(job, TextureStatus::Cancelled);
                continue;
            }

            prepareJpeg(*job);

            if (job->path == Path::Compute)
            {
                {
                    std::lock_guard lock(m_mutex);
                    m_ready.push_back(job);
                }
                notifyOwner();
            }
            else
            {
                std::lock_guard lock(m_mutex);
                startCpuLocked(job);
            }
        }
    }

    void prepareJpeg(Job& job)
    {
        const u64 t0 = Time::us();

        image::jpeg::JpegDecoder decoder(job.memory);
        const image::jpeg::JpegInfo& jinfo = decoder.info();

        TextureReport& report = job.report;
        if (jinfo.valid)
        {
            report.source_format = "JPEG " + jinfo.encoding;
            if (!jinfo.chroma.empty())
                report.source_format += " " + jinfo.chroma;
            if (jinfo.restart_intervals)
                report.source_format += " DRI";
            if (jinfo.mango1)
                report.source_format += " Mango1";
        }

        const char* reason = detail::JpegCompute::classify(jinfo);
        report.prepare_parse_ms = elapsedMs(t0);
        if (reason)
        {
            report.fallback_reason = reason;
        }
        else if (!job.compute_reason.empty())
        {
            report.fallback_reason = job.compute_reason;
        }
        else
        {
            auto compute = std::make_unique<detail::JpegComputeJob>();
            std::string error;
            const bool prepared = m_compute->prepare(decoder, *compute, m_config.wait_for_pipelines, error);
            report.prepare_extract_ms = compute->extract_ms;
            report.prepare_unstuff_ms = compute->unstuff_ms;
            report.prepare_tables_ms = compute->tables_ms;
            report.prepare_pipelines_ms = compute->pipelines_ms;
            report.prepare_alloc_ms = compute->alloc_ms;
            report.prepare_upload_ms = compute->upload_ms;
            report.staging_reserved_bytes = size_t(m_staging->stats().reserved_bytes);
            if (prepared)
            {
                job.compute = std::move(compute);
                job.path = Path::Compute;
                report.compile_ms = job.compute->compile_ms;
                report.staging_bytes = job.compute->staging_bytes;
                report.peak_host_bytes = job.compute->staging_bytes;
            }
            else
            {
                report.fallback_reason = error;
            }
        }

        report.prepare_ms = elapsedMs(t0);
    }

    // -----------------------------------------------------------------------
    // CPU decode
    // -----------------------------------------------------------------------

    void startCpuLocked(const JobPtr& job)
    {
        if (m_cpu_running < m_config.max_concurrent_cpu)
        {
            ++m_cpu_running;
            launchCpuLocked(job);
        }
        else
        {
            m_cpu_waiting.push_back(job);
        }
    }

    void launchCpuLocked(const JobPtr& job)
    {
        // The task holds a raw pointer: the future lives in the job, and a task
        // dropping the last reference would destroy its own future.
        Job* raw = job.get();
        raw->future = std::async(std::launch::async, [this, raw]
        {
            runCpu(*raw);

            std::lock_guard lock(m_mutex);
            --m_cpu_running;
            if (!m_cpu_waiting.empty())
            {
                JobPtr next = std::move(m_cpu_waiting.front());
                m_cpu_waiting.pop_front();
                ++m_cpu_running;
                launchCpuLocked(next);
            }
        });
    }

    // Every exit hands self over with std::move so this task never holds the
    // last reference to its own job.
    void runCpu(Job& job)
    {
        JobPtr self;
        {
            std::lock_guard lock(m_mutex);
            self = m_jobs.at(job.handle);
        }

        if (job.cancelled)
        {
            finish(std::move(self), TextureStatus::Cancelled);
            return;
        }

        job.path = Path::Cpu;
        TextureReport& report = job.report;
        report.path = TextureDecodePath::Cpu;
        report.decoder = "cpu";

        const ImageHeader& header = job.header;
        const ColorPlan& plan = job.plan;
        size_t live = 0;
        size_t peak = 0;
        auto track = [&] (ptrdiff_t delta)
        {
            live = size_t(ptrdiff_t(live) + delta);
            peak = std::max(peak, live);
        };

        u64 t = Time::us();
        std::unique_ptr<Bitmap> bitmap;
        image::ImageDecodeStatus status;
        try
        {
            bitmap = std::make_unique<Bitmap>(header.width, header.height, plan.bitmap_format);
            track(ptrdiff_t(surfaceBytes(*bitmap)));
            status = job.decoder->decode(*bitmap);
        }
        catch (const std::exception& e)
        {
            status.setError(e.what());
        }
        report.decode_ms = elapsedMs(t);

        if (job.cancelled)
        {
            finish(std::move(self), TextureStatus::Cancelled);
            return;
        }

        if (!status)
        {
            job.error = status.info.empty() ? std::string("decode failed") : status.info;
            finish(std::move(self), TextureStatus::Failed);
            return;
        }

        std::unique_ptr<Bitmap> current = std::move(bitmap);

        // Downscale before linearize: the bake then runs on the small image.
        if (job.downscale)
        {
            t = Time::us();
            auto scaled = std::make_unique<Bitmap>(int(job.info.width), int(job.info.height), current->format);
            track(ptrdiff_t(surfaceBytes(*scaled)));
            downscaleSurface(*scaled, *current);
            track(-ptrdiff_t(surfaceBytes(*current)));
            current = std::move(scaled);
            report.downscale_ms = elapsedMs(t);
        }

        if (plan.convert)
        {
            t = Time::us();
            auto linear = std::make_unique<Bitmap>(current->width, current->height, formatF16());
            track(ptrdiff_t(surfaceBytes(*linear)));
            image::linearize(*linear, *current, header.color);
            track(-ptrdiff_t(surfaceBytes(*current)));
            current = std::move(linear);
            report.convert_ms = elapsedMs(t);
        }

        if (job.cancelled)
        {
            finish(std::move(self), TextureStatus::Cancelled);
            return;
        }

        t = Time::us();
        const size_t bpp = size_t(current->format.bytes());
        const size_t row = size_t(current->width) * bpp;
        const size_t bytes = row * size_t(current->height);
        job.staging = m_allocator.createBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryUsage::Upload, true);
        if (!job.staging || !job.staging.mapped)
        {
            if (job.staging)
                m_allocator.destroyBuffer(job.staging);
            job.staging = {};
            job.error = fmt::format("staging allocation failed ({} bytes)", bytes);
            finish(std::move(self), TextureStatus::Failed);
            return;
        }
        track(ptrdiff_t(bytes));

        u8* dst = static_cast<u8*>(job.staging.mapped);
        if (current->stride == row)
        {
            std::memcpy(dst, current->image, bytes);
        }
        else
        {
            for (int y = 0; y < current->height; ++y)
                std::memcpy(dst + size_t(y) * row, current->address(0, y), row);
        }
        m_allocator.flush(job.staging.allocation, 0, bytes);
        job.staging_bytes_per_pixel = u32(bpp);
        current.reset();

        report.staging_ms = elapsedMs(t);
        report.staging_bytes = bytes;
        report.peak_host_bytes = peak;

        {
            std::lock_guard lock(m_mutex);
            m_ready.push_back(std::move(self));
        }
        notifyOwner();
    }

    // Terminal state without GPU work: goes straight to the callback queue.
    void finish(JobPtr job, TextureStatus status)
    {
        job->status = status;
        {
            std::lock_guard lock(m_mutex);
            m_finished.push_back(std::move(job));
        }
        notifyOwner();
    }

    // -----------------------------------------------------------------------
    // waiter: blocks on the oldest fence
    // -----------------------------------------------------------------------

    void waiterMain()
    {
        for (;;)
        {
            JobPtr job;
            {
                std::unique_lock lock(m_mutex);
                m_waiter_cv.wait(lock, [this] { return m_stop || !m_inflight.empty(); });
                if (m_inflight.empty())
                    return;
                job = m_inflight.front();
            }

            VkResult result = vkWaitForFences(m_device, 1, &job->fence, VK_TRUE, 1000ull * 1000ull * 1000ull);
            if (result == VK_TIMEOUT)
                continue;

            if (result != VK_SUCCESS)
            {
                job->error = fmt::format("vkWaitForFences: {}", getString(result));
                job->status = TextureStatus::Failed;
            }
            else
            {
                job->status = job->cancelled ? TextureStatus::Cancelled : TextureStatus::Ready;
            }
            // VkImage usable — same moment as gpujpeg READY. Not the callback,
            // not a swapchain present.
            job->report.ready_ms = job->submit_us
                ? double(Time::us() - job->submit_us) / 1000.0
                : elapsedMs(job->start_us);

            {
                std::lock_guard lock(m_mutex);
                m_inflight.pop_front();
                m_finished.push_back(job);
            }
            notifyOwner();
        }
    }

    // -----------------------------------------------------------------------
    // owner thread
    // -----------------------------------------------------------------------

    VkCommandBuffer acquireCommandBuffer()
    {
        if (!m_free_cmds.empty())
        {
            VkCommandBuffer cmd = m_free_cmds.back();
            m_free_cmds.pop_back();
            vkResetCommandBuffer(cmd, 0);
            return cmd;
        }

        VkCommandBufferAllocateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(m_device, &info, &cmd);
        return cmd;
    }

    VkFence acquireFence()
    {
        if (!m_free_fences.empty())
        {
            VkFence fence = m_free_fences.back();
            m_free_fences.pop_back();
            vkResetFences(m_device, 1, &fence);
            return fence;
        }

        VkFenceCreateInfo info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkFence fence = VK_NULL_HANDLE;
        vkCreateFence(m_device, &info, nullptr, &fence);
        return fence;
    }

    void recordUpload(VkCommandBuffer cmd, Job& job)
    {
        VkImageMemoryBarrier to_dst =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = job.image.image,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &to_dst);

        VkBufferImageCopy region =
        {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { job.info.width, job.info.height, 1 },
        };
        vkCmdCopyBufferToImage(cmd, job.staging.buffer, job.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier to_read =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = job.image.image,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &to_read);
    }

    bool submit(Job& job)
    {
        if (job.path == Path::Compute)
        {
            VkImageViewUsageCreateInfo usage =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT,
            };
            VkImageViewCreateInfo view_info =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = &usage,
                .image = job.image.image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };
            if (vkCreateImageView(m_device, &view_info, nullptr, &job.storage_view) != VK_SUCCESS)
            {
                job.error = "storage view creation failed";
                return false;
            }
        }

        job.cmd = acquireCommandBuffer();
        job.fence = acquireFence();
        if (!job.cmd || !job.fence)
        {
            job.error = "command buffer or fence allocation failed";
            return false;
        }

        VkCommandBufferBeginInfo begin =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(job.cmd, &begin);

        if (job.path == Path::Compute)
        {
            if (!m_compute->record(job.cmd, *job.compute, job.image.image, job.storage_view, job.error))
            {
                vkEndCommandBuffer(job.cmd);
                return false;
            }
        }
        else
        {
            recordUpload(job.cmd, job);
        }

        vkEndCommandBuffer(job.cmd);

        VkSubmitInfo submit_info =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &job.cmd,
        };
        VkResult result = vkQueueSubmit(m_queue, 1, &submit_info, job.fence);
        if (result != VK_SUCCESS)
        {
            job.error = fmt::format("vkQueueSubmit: {}", getString(result));
            return false;
        }

        job.submitted = true;
        job.submit_us = Time::us();
        job.report.submit_ms = elapsedMs(job.start_us);
        return true;
    }

    void retire(Job& job)
    {
        if (job.path == Path::Compute && job.compute)
        {
            if (job.submitted)
                job.report.gpu_ms = m_compute->gpuMilliseconds(*job.compute);
            m_compute->release(*job.compute);
            job.compute.reset();
        }

        if (job.storage_view)
        {
            vkDestroyImageView(m_device, job.storage_view, nullptr);
            job.storage_view = VK_NULL_HANDLE;
        }

        if (job.staging)
        {
            m_allocator.destroyBuffer(job.staging);
            job.staging = {};
        }

        // An unsubmitted fence or command buffer goes back as-is; both are reset on reuse.
        if (job.cmd)
        {
            m_free_cmds.push_back(job.cmd);
            job.cmd = VK_NULL_HANDLE;
        }

        if (job.fence)
        {
            m_free_fences.push_back(job.fence);
            job.fence = VK_NULL_HANDLE;
        }
    }

    void fire(const JobPtr& job)
    {
        if (job->path == Path::Compute)
        {
            job->report.path = TextureDecodePath::Compute;
            job->report.decoder = "jpeg-compute";
        }

        if (job->report.ready_ms == 0.0)
            job->report.ready_ms = job->submit_us
                ? double(Time::us() - job->submit_us) / 1000.0
                : elapsedMs(job->start_us);
        job->report.visible_ms = elapsedMs(job->start_us);

        TextureResult result;
        result.handle = job->handle;
        result.status = job->status;
        result.image = job->image;
        result.info = job->info;
        result.report = job->report;
        result.error = job->error;

        {
            // cancel() reaches the decoder under the same lock.
            std::lock_guard lock(m_mutex);
            job->decoder.reset();
            m_jobs.erase(job->handle);
        }

        if (job->callback)
            job->callback(result);
    }

    void process()
    {
        std::deque<JobPtr> finished;
        std::deque<JobPtr> ready;
        {
            std::lock_guard lock(m_mutex);
            finished.swap(m_finished);
            ready.swap(m_ready);
        }

        // Retire first: it frees compute slots for the submits below.
        for (JobPtr& job : finished)
        {
            retire(*job);
            fire(job);
        }

        std::deque<JobPtr> blocked;
        for (JobPtr& job : ready)
        {
            if (job->cancelled)
            {
                job->status = TextureStatus::Cancelled;
                retire(*job);
                fire(job);
                continue;
            }

            if (job->path == Path::Compute && job->compute->slot < 0 && !m_compute->acquire(*job->compute))
            {
                blocked.push_back(job);
                continue;
            }

            if (!submit(*job))
            {
                job->status = TextureStatus::Failed;
                retire(*job);
                fire(job);
                continue;
            }

            {
                std::lock_guard lock(m_mutex);
                m_inflight.push_back(job);
            }
            m_waiter_cv.notify_one();
        }

        if (!blocked.empty())
        {
            std::lock_guard lock(m_mutex);
            m_ready.insert(m_ready.begin(), blocked.begin(), blocked.end());
        }
    }

    void wait()
    {
        for (;;)
        {
            u64 signal;
            {
                std::lock_guard lock(m_mutex);
                signal = m_owner_signal;
            }

            process();

            std::unique_lock lock(m_mutex);
            if (m_jobs.empty())
                return;

            m_owner_cv.wait(lock, [&] { return m_owner_signal != signal; });
        }
    }
};

// ---------------------------------------------------------------------------
// TextureDecoder
// ---------------------------------------------------------------------------

TextureDecoder::TextureDecoder(VulkanContext& context, Allocator& allocator, const TextureDecoderConfig& config)
    : m(std::make_unique<Impl>(context.physicalDevice(), context.device(), context.graphicsQueue(),
                               context.graphicsQueueFamilyIndex(), allocator, config))
{
}

TextureDecoder::TextureDecoder(VkPhysicalDevice physicalDevice, VkDevice device,
                               VkQueue queue, u32 queueFamilyIndex, Allocator& allocator,
                               const TextureDecoderConfig& config)
    : m(std::make_unique<Impl>(physicalDevice, device, queue, queueFamilyIndex, allocator, config))
{
}

TextureDecoder::~TextureDecoder()
{
}

TextureDecoder::Texture TextureDecoder::decode(ConstMemory memory, const std::string& filename, Callback callback)
{
    return m->decode(memory, filename, std::move(callback));
}

void TextureDecoder::cancel(Handle handle)
{
    m->cancel(handle);
}

void TextureDecoder::process()
{
    m->process();
}

void TextureDecoder::wait()
{
    m->wait();
}

size_t TextureDecoder::pending() const
{
    return m->pending();
}

StagingArenaStats TextureDecoder::stagingStats() const
{
    return m->stagingStats();
}

void TextureDecoder::trimStaging()
{
    m->trimStaging();
}

VkImageView createTextureView(VkDevice device, const ImageAllocation& image, const TextureInfo& info)
{
    VkImageViewUsageCreateInfo usage =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
    };
    VkImageViewCreateInfo view_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = &usage,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = info.format,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &view_info, nullptr, &view) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return view;
}

std::string_view getString(TextureStatus status)
{
    switch (status)
    {
        case TextureStatus::Ready: return "Ready";
        case TextureStatus::Failed: return "Failed";
        case TextureStatus::Cancelled: return "Cancelled";
    }
    return "?";
}

std::string_view getString(TextureDecodePath path)
{
    switch (path)
    {
        case TextureDecodePath::None: return "none";
        case TextureDecodePath::Compute: return "compute";
        case TextureDecodePath::Cpu: return "cpu";
    }
    return "?";
}

} // namespace mango::vulkan
