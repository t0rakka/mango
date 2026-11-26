/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define MANGO_IMPLEMENT_MAIN
#include <mango/core/core.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/image/image.hpp>
#include <mango/vulkan/vulkan.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::vulkan;

namespace
{

struct Args
{
    std::string root;
    bool info = false;
    bool validate = false;
    bool verbose = false;
    bool cpu = false;              // force CPU path
    bool wait_pipelines = true;    // default on for compute timing honesty
    float min_mp = 0.0f;           // 0 = send every JPEG to compute when enabled
    int inflight = 4;              // max_inflight_compute
    int cpu_jobs = 0;              // max_concurrent_cpu (0 = auto from HW threads)
    int loops = 1;                 // repeat the whole set
    int depth = 64;                // target in-flight decodes (submit window)
    int staging_mb = 0;            // staging arena limit (0 = default)
};

struct FileEntry
{
    std::string path;
    u64 size = 0;
};

struct Failure
{
    std::string path;
    std::string reason;
};

struct Stats
{
    std::mutex mutex;

    size_t submitted = 0;
    size_t ready = 0;
    size_t failed = 0;
    size_t cancelled = 0;

    size_t compute = 0;
    size_t cpu = 0;

    double sum_ready_ms = 0.0;
    double sum_gpu_ms = 0.0;
    double sum_decode_ms = 0.0;
    double sum_prepare_ms = 0.0;
    double sum_megapixels = 0.0;
    u64 sum_image_bytes = 0;

    double min_ready_ms = 1e300;
    double max_ready_ms = 0.0;

    std::vector<Failure> failures;
};

std::string megabytes(size_t bytes)
{
    return fmt::format("{:.1f} MB", double(bytes) / (1024.0 * 1024.0));
}

bool isJpegName(std::string_view name)
{
    const std::string ext = mango::toLower(std::string(filesystem::getExtension(name)));
    return ext == ".jpg" || ext == ".jpeg" || ext == ".jpe" || ext == ".jfif";
}

void collectJpegs(const Path& path, std::vector<FileEntry>& out)
{
    for (auto node : path)
    {
        if (node.isDirectory() && !node.isContainer())
        {
            collectJpegs(Path(path, node.name), out);
            continue;
        }
        if (node.isDirectory())
            continue;
        if (!isJpegName(node.name))
            continue;

        FileEntry entry;
        entry.path = path.pathname() + node.name;
        entry.size = node.size;
        out.push_back(std::move(entry));
    }
}

} // namespace

// ------------------------------------------------------------------------------

class JpegBenchWindow : public VulkanWindow
{
protected:
    const Args& m_args;
    std::vector<FileEntry> m_files;

    std::unique_ptr<Allocator> m_allocator;
    std::unique_ptr<TextureDecoder> m_decoder;

    struct Inflight
    {
        std::unique_ptr<File> file;
        std::string path;
    };

    // Compressed bytes must stay alive until the TextureDecoder callback.
    std::unordered_map<TextureDecoder::Handle, Inflight> m_inflight;

    size_t m_next = 0;             // next file index to submit
    size_t m_total_jobs = 0;       // files * loops
    size_t m_completed = 0;

    Stats m_stats;
    u64 m_t0_us = 0;
    bool m_started = false;
    bool m_finished = false;

public:
    JpegBenchWindow(VulkanContext& context, const Args& args, std::vector<FileEntry> files)
        : VulkanWindow(context, 640, 360, 0)
        , m_args(args)
        , m_files(std::move(files))
    {
        m_total_jobs = m_files.size() * size_t(std::max(m_args.loops, 1));
    }

    ~JpegBenchWindow() override
    {
        m_decoder.reset();
        m_inflight.clear();
        m_allocator.reset();
    }

    void onDeviceReady() override
    {
        m_allocator = std::make_unique<Allocator>(
            instance(), physicalDevice(), device(), VK_API_VERSION_1_3);

        TextureDecoderConfig config;
        config.compute = !m_args.cpu;
        config.compute_min_pixels = u64(double(m_args.min_mp) * 1e6);
        config.wait_for_pipelines = m_args.wait_pipelines;
        config.max_inflight_compute = std::max(m_args.inflight, 1);
        config.max_concurrent_cpu = std::max(m_args.cpu_jobs, 1);
        if (m_args.staging_mb > 0)
        {
            config.staging.limit = VkDeviceSize(m_args.staging_mb) << 20;
            config.staging.initial_chunk = std::min(config.staging.initial_chunk, config.staging.limit);
        }
        config.wake = [this]
        {
            invalidate();
            requestFrameIn(0);
        };

        m_decoder = std::make_unique<TextureDecoder>(context(), *m_allocator, config);

        VkPhysicalDeviceProperties props {};
        vkGetPhysicalDeviceProperties(physicalDevice(), &props);

        printLine("device     : {}", props.deviceName);
        printLine("files      : {} JPEG(s), loops {}, total jobs {}",
            m_files.size(), std::max(m_args.loops, 1), m_total_jobs);
        printLine("decoder    : compute={} min_mp={:.2f} wait_pipelines={} inflight={} cpu_jobs={} depth={}",
            config.compute ? "on" : "off",
            m_args.min_mp,
            config.wait_for_pipelines,
            config.max_inflight_compute,
            config.max_concurrent_cpu,
            m_args.depth);
        printLine("");

        m_t0_us = Time::us();
        m_started = true;
        pump();
        requestFrameIn(0);
    }

    void onFrame(const FrameInfo& info) override
    {
        MANGO_UNREFERENCED(info);
        if (!m_decoder || m_finished)
            return;

        m_decoder->process();
        pump();

        if (m_completed >= m_total_jobs && m_decoder->pending() == 0)
        {
            finish();
            return;
        }

        // Keep the owner thread spinning while the pipeline has room or work.
        if (m_next < m_total_jobs || m_decoder->pending() > 0)
            requestFrameIn(0);
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        MANGO_UNREFERENCED(mask);
        if (code == KEYCODE_ESC)
            requestQuit();
    }

private:
    void pump()
    {
        if (!m_decoder)
            return;

        const size_t depth = size_t(std::max(m_args.depth, 1));
        while (m_next < m_total_jobs && m_decoder->pending() < depth)
            submitOne();
    }

    void submitOne()
    {
        const size_t job = m_next++;
        const FileEntry& entry = m_files[job % m_files.size()];

        auto file = std::make_unique<File>(entry.path);
        ConstMemory memory = *file;
        if (!memory.size)
        {
            recordFailure(entry.path, "empty file / unreadable");
            ++m_completed;
            return;
        }

        // Extension selects the JPEG decoder; bare ".jpg" is enough, full path is fine.
        TextureDecoder::Texture texture = m_decoder->decode(memory, entry.path,
            [this] (const TextureResult& result)
            {
                onDecoded(result);
            });

        if (!texture)
        {
            recordFailure(entry.path,
                texture.error.empty() ? "decode() rejected (header/format)" : texture.error);
            ++m_completed;
            return;
        }

        Inflight slot;
        slot.file = std::move(file);
        slot.path = entry.path;
        m_inflight[texture.handle] = std::move(slot);

        {
            std::lock_guard lock(m_stats.mutex);
            ++m_stats.submitted;
        }
    }

    void onDecoded(const TextureResult& result)
    {
        std::string path;
        auto it = m_inflight.find(result.handle);
        if (it != m_inflight.end())
        {
            path = it->second.path;
            m_inflight.erase(it);
        }

        {
            std::lock_guard lock(m_stats.mutex);

            if (result.status == TextureStatus::Ready)
            {
                ++m_stats.ready;
                const TextureReport& r = result.report;
                const TextureInfo& info = result.info;
                const double mp = double(info.source_width) * double(info.source_height) / 1e6;

                m_stats.sum_ready_ms += r.ready_ms;
                m_stats.sum_gpu_ms += r.gpu_ms;
                m_stats.sum_prepare_ms += r.prepare_ms;
                m_stats.sum_decode_ms += r.decode_ms + r.convert_ms + r.downscale_ms;
                m_stats.sum_megapixels += mp;
                m_stats.sum_image_bytes += r.image_bytes;
                m_stats.min_ready_ms = std::min(m_stats.min_ready_ms, r.ready_ms);
                m_stats.max_ready_ms = std::max(m_stats.max_ready_ms, r.ready_ms);

                if (r.path == TextureDecodePath::Compute)
                    ++m_stats.compute;
                else if (r.path == TextureDecodePath::Cpu)
                    ++m_stats.cpu;

                if (m_args.verbose)
                {
                    const std::string name = path.empty() ? std::string("?") : path;
                    printLine("READY {:>6.2f} ms  {:>5.1f} MP  {:<8}  {}{}",
                        r.ready_ms, mp, getString(r.path),
                        filesystem::removePath(name),
                        r.fallback_reason.empty() ? "" : " [" + r.fallback_reason + "]");
                }
            }
            else if (result.status == TextureStatus::Cancelled)
            {
                ++m_stats.cancelled;
            }
        }

        if (result.status != TextureStatus::Ready && result.status != TextureStatus::Cancelled)
        {
            // Outside the stats lock — recordFailure takes it.
            const std::string reason = result.error.empty()
                ? std::string(getString(result.status))
                : result.error;
            recordFailure(path.empty() ? "?" : path, reason);
        }

        // Drop the VkImage immediately — this bench measures READY throughput,
        // not residency. Destroy only after Ready/Failed/Cancelled callback.
        if (result.image.image != VK_NULL_HANDLE && m_allocator)
            m_allocator->destroyImage(result.image);

        ++m_completed;
    }

    void recordFailure(const std::string& path, const std::string& reason)
    {
        Failure failure { path, reason };
        {
            std::lock_guard lock(m_stats.mutex);
            ++m_stats.failed;
            m_stats.failures.push_back(failure);
        }
        printLine("FAIL: {} ({})", path, reason);
    }

    void finish()
    {
        if (m_finished)
            return;
        m_finished = true;

        const u64 t1_us = Time::us();
        const double wall_ms = m_started ? double(t1_us - m_t0_us) / 1000.0 : 0.0;
        const double wall_s = wall_ms / 1000.0;

        std::lock_guard lock(m_stats.mutex);
        const size_t ok = m_stats.ready;
        const double avg_ready = ok ? m_stats.sum_ready_ms / double(ok) : 0.0;
        const double ips = wall_s > 0.0 ? double(ok) / wall_s : 0.0;
        const double mps = wall_s > 0.0 ? m_stats.sum_megapixels / wall_s : 0.0;

        std::string summary = fmt::format(
            "==================================================\n"
            "vkjpegbench  (TextureDecoder stress)\n"
            "  wall       : {:.2f} ms ({:.2f} s)\n"
            "  jobs       : submitted={} ready={} failed={} cancelled={}\n"
            "  paths      : compute={} cpu={}\n"
            "  READY avg  : {:.2f} ms  (min {:.2f}, max {:.2f})\n",
            wall_ms, wall_s,
            m_stats.submitted, m_stats.ready, m_stats.failed, m_stats.cancelled,
            m_stats.compute, m_stats.cpu,
            avg_ready,
            ok ? m_stats.min_ready_ms : 0.0,
            ok ? m_stats.max_ready_ms : 0.0);

        if (m_stats.compute)
        {
            summary += fmt::format("  GPU avg    : {:.2f} ms  (compute timestamps)\n",
                m_stats.sum_gpu_ms / double(m_stats.compute));
        }
        if (m_stats.cpu)
        {
            summary += fmt::format("  CPU decode : {:.2f} ms avg (decode+convert+downscale)\n",
                m_stats.sum_decode_ms / double(std::max(m_stats.cpu, size_t(1))));
        }

        const StagingArenaStats staging = m_decoder->stagingStats();
        summary += fmt::format(
            "  throughput : {:.1f} images/s, {:.0f} MP/s  ({:.1f} MP total)\n"
            "  image mem  : {} total allocations (destroyed on Ready)\n"
            "  staging    : {} reserved in {} chunk(s), peak use {}, {} chunk allocation(s), {} wait(s)\n"
            "  peak RSS   : {}\n",
            ips, mps, m_stats.sum_megapixels,
            megabytes(size_t(m_stats.sum_image_bytes)),
            megabytes(size_t(staging.reserved_bytes)), staging.chunks,
            megabytes(size_t(staging.peak_used_bytes)), staging.chunk_allocations, staging.waits,
            megabytes(processPeakBytes()));

        if (!m_stats.failures.empty())
        {
            summary += fmt::format("  failures   : {}\n", m_stats.failures.size());
            for (const Failure& failure : m_stats.failures)
            {
                summary += fmt::format("    - {}  ({})\n",
                    failure.path, failure.reason);
            }
        }

        summary += "==================================================\n";

        printLine("{}", summary);
        requestQuit();
    }
};

// ------------------------------------------------------------------------------

int mangoMain(const mango::CommandLine& commands)
{
#if defined(WIN32)
    // WinMain already tried AttachConsole(parent). If that failed (no parent
    // console), open our own so the throughput summary is always visible.
    if (AllocConsole())
    {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
    }
#endif

    Args args;

    commands.usage("[options] <folder>");
    commands.flag("--info", "enable verbose mango info output", [&] { args.info = true; });
    commands.flag("--validate", "enable Khronos validation layer", [&] { args.validate = true; });
    commands.flag("--verbose", "print one line per Ready", [&] { args.verbose = true; });
    commands.flag("--cpu", "force CPU ImageDecoder path (no JPEG compute)", [&] { args.cpu = true; });
    commands.flag("--no-wait", "CPU-fallback while compute pipelines compile",
        [&] { args.wait_pipelines = false; });
    commands.optionFloat("--min-mp", "smallest JPEG (megapixels) for compute (default 0 = all)",
        [&] (float value) { args.min_mp = std::max(value, 0.0f); });
    commands.optionInt("--inflight", "TextureDecoder max_inflight_compute (default 4)",
        [&] (int value) { args.inflight = std::max(value, 1); });
    commands.optionInt("--cpu-jobs", "TextureDecoder max_concurrent_cpu (default: HW/2, clamped 2..16)",
        [&] (int value) { args.cpu_jobs = std::max(value, 1); });
    commands.optionInt("--depth", "submit window: keep this many decodes pending (default 64)",
        [&] (int value) { args.depth = std::max(value, 1); });
    commands.optionInt("--loops", "repeat the whole folder N times (default 1)",
        [&] (int value) { args.loops = std::max(value, 1); });
    commands.optionInt("--staging-limit", "staging arena limit in MB (default 256)",
        [&] (int value) { args.staging_mb = std::max(value, 1); });
    commands.positional([&] (std::string_view token)
    {
        if (args.root.empty())
            args.root = std::string(token);
    });

    if (!commands.parse())
        return 1;

    if (args.root.empty())
    {
        printLine("usage: vkjpegbench [options] <folder>");
        printLine("  Recursively loads every JPEG under <folder> into vulkan::TextureDecoder");
        printLine("  and measures READY throughput with a deep in-flight pipeline.");
        printLine("");
        printLine("  tuning:");
        printLine("    --inflight N   compute slots (device buffers; ~75 MB @ 24 MP each)");
        printLine("    --cpu-jobs N   concurrent CPU decodes");
        printLine("    --depth N      how many decodes to keep pending");
        printLine("    --cpu          force CPU path");
        printLine("    --min-mp F     compute threshold in megapixels (0 = all JPEGs)");
        printLine("    --no-wait      allow CPU fallback while shaders compile");
        printLine("    --loops N      replay the set");
        printLine("    --staging-limit MB  staging arena limit");
        printLine("    --verbose      per-image READY lines");
        return 1;
    }

    if (args.info)
        printEnable(Print::Info, true);

    if (args.cpu_jobs <= 0)
    {
        const int hw = int(std::max(std::thread::hardware_concurrency(), 2u));
        args.cpu_jobs = std::clamp(hw / 2, 2, 16);
    }

    std::vector<FileEntry> files;
    {
        std::string root = args.root;
        if (!root.empty() && root.back() != '/' && root.back() != '\\')
            root.push_back('/');
        collectJpegs(Path(root), files);
    }

    if (files.empty())
    {
        printLine("No JPEG files under \"{}\"", args.root);
        return 1;
    }

    // Largest first — keeps the compute slots busy with real work early.
    std::sort(files.begin(), files.end(), [] (const FileEntry& a, const FileEntry& b)
    {
        return a.size > b.size;
    });

    u64 bytes = 0;
    for (const FileEntry& f : files)
        bytes += f.size;
    printLine("scan      : {} JPEG(s), {} under \"{}\"",
        files.size(), megabytes(size_t(bytes)), args.root);

    std::vector<const char*> layers;
    if (args.validate)
        layers.push_back("VK_LAYER_KHRONOS_validation");

    InstanceExtensionProperties instanceExtensions;
    std::vector<const char*> extensions = vulkan::requiredSurfaceExtensions();
    if (instanceExtensions.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
        extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vkjpegbench",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, layers, extensions);
    VulkanContext context(instance);

    JpegBenchWindow window(context, args, std::move(files));
    window.setTitle("vkjpegbench");

    EventLoopConfig config;
    config.mode = FrameMode::OnDemand;

    EventLoop loop;
    loop.attach(window, config);
    loop.run();

    return 0;
}
