/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/memory.hpp>

namespace mango::image
{

    class Surface;

    static constexpr
    u32 makeBGRA(u32 red, u32 green, u32 blue, u32 alpha) noexcept
    {
        return (alpha << 24) | (red << 16) | (green << 8) | blue;
    }

    static constexpr
    u32 makeRGBA(u32 red, u32 green, u32 blue, u32 alpha) noexcept
    {
        return (alpha << 24) | (blue << 16) | (green << 8) | red;
    }

    // ------------------------------------------------------------------
    // Color
    // ------------------------------------------------------------------

    union Color
    {
        u8 component[4] {};
        struct
        {
            u8 r, g, b, a;
        };

        Color()
        {
            littleEndian::ustore32(this, 0);
        }

        Color(u32 red, u32 green, u32 blue, u32 alpha)
        {
            littleEndian::ustore32(this, makeRGBA(red, green, blue, alpha));
        }

        Color(u32 value)
        {
            littleEndian::ustore32(this, value);
        }

        operator u32 () const
        {
            return littleEndian::uload32(this);
        }

        u8& operator [] (size_t index)
        {
            return component[index];
        }

        u8 operator [] (size_t index) const
        {
            return component[index];
        }
    };

    // ------------------------------------------------------------------
    // Palette
    // ------------------------------------------------------------------

    struct Palette
    {
        u32 size;
        Color color[256];

        Palette()
            : size(0)
        {
        }

        Palette(u32 size)
            : size(size)
        {
        }

        Color& operator [] (size_t index)
        {
            return color[index];
        }

        Color operator [] (size_t index) const
        {
            return color[index];
        }
    };

    // ------------------------------------------------------------------
    // Color space description
    // ------------------------------------------------------------------

    /*
        ColorPrimaries and TransferFunction use the CICP code points
        (ITU-T H.273 / ISO/IEC 23091-2) wherever a code point exists. CICP is
        the shared vocabulary used by PNG (cICP chunk), AVIF, HEIF and HEVC, and
        the KTX2 data-format-descriptor values map directly onto the same set.

        This lets decoders forward a file's color signalling without loss, and
        lets clients interoperate with other CICP-based pipelines. Values that
        have no CICP code point (e.g. Adobe RGB, ACES) are assigned private
        identifiers outside the standard range (>= 128).
    */

    enum class ColorPrimaries : u8
    {
        Unspecified = 0,
        BT709       = 1,   // ITU-R BT.709, sRGB, Display ("Rec.709")
        BT470M      = 4,   // ITU-R BT.470 System M
        BT601_625   = 5,   // ITU-R BT.601 625-line (EBU 3213)
        BT601_525   = 6,   // ITU-R BT.601 525-line (SMPTE 170M / 240M)
        BT2020      = 9,   // ITU-R BT.2020 / BT.2100
        SMPTE428    = 10,  // SMPTE ST 428-1 (CIE 1931 XYZ)
        DCI_P3      = 11,  // SMPTE RP 431-2 (DCI-P3, theater white)
        DisplayP3   = 12,  // SMPTE EG 432-1 (Display P3, D65 white)

        // Not CICP code points (private identifiers)
        AdobeRGB    = 128, // Adobe RGB (1998)
        ACES_AP0    = 129, // ACES 2065-1 primaries (AP0)
        ACES_AP1    = 130, // ACEScg primaries (AP1)
    };

    enum class TransferFunction : u8
    {
        Unspecified = 0,
        BT709       = 1,   // ITU-R BT.709 (also BT.601 / BT.2020 SDR), ~1/0.45
        Gamma22     = 4,   // assumed display gamma 2.2 (BT.470 System M)
        Gamma28     = 5,   // assumed display gamma 2.8 (BT.470 System B/G)
        Linear      = 8,   // linear (radiometric, e.g. OpenEXR, Radiance HDR)
        sRGB        = 13,  // IEC 61966-2-1 (sRGB / sYCC piecewise curve)
        PQ          = 16,  // SMPTE ST 2084 (PQ, BT.2100)
        HLG         = 18,  // ARIB STD-B67 (HLG, BT.2100)
    };

    // CIE 1931 xy chromaticity coordinate.
    struct ColorPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    /*
        Mastering display color volume (HDR static metadata; PNG mDCV, SMPTE ST 2086).
        Describes the display the content was graded on, so a target display can adapt
        its tone mapping. Luminances are absolute cd/m2 (nits). Gated by
        ColorInfo::has_mastering_display.
    */
    struct MasteringDisplay
    {
        ColorPoint red;
        ColorPoint green;
        ColorPoint blue;
        ColorPoint white;
        float max_luminance = 0.0f; // cd/m2
        float min_luminance = 0.0f; // cd/m2
    };

    /*
        Content light level (HDR static metadata; PNG cLLI). Absolute cd/m2 (nits);
        a value of 0 means "unknown / not calculable". Gated by
        ColorInfo::has_content_light_level.
    */
    struct ContentLightLevel
    {
        float max_cll = 0.0f;  // Maximum Content Light Level
        float max_fall = 0.0f; // Maximum Frame-Average Light Level
    };

    /*
        Color space description attached to a decoded image.

        'primaries' and 'transfer' carry the named (CICP) signalling. When a
        file specifies exact chromaticities (PNG cHRM, OpenEXR chromaticities),
        'has_chromaticities' is set and the coordinates take precedence over the
        named primaries. 'gamma' carries an explicit display-gamma exponent when
        a file provides one (PNG gAMA); 0 means "not specified".

        'mastering_display' and 'content_light_level' carry optional HDR static
        metadata (PNG mDCV / cLLI) for tone mapping; each is gated by its own flag.

        An empty ColorInfo (all Unspecified) means the decoder reported no color
        signalling; clients should then assume sRGB for integer formats.
    */
    struct ColorInfo
    {
        ColorPrimaries   primaries = ColorPrimaries::Unspecified;
        TransferFunction transfer  = TransferFunction::Unspecified;

        bool       has_chromaticities = false;
        ColorPoint white;
        ColorPoint red;
        ColorPoint green;
        ColorPoint blue;

        float gamma = 0.0f;

        bool has_mastering_display = false;
        MasteringDisplay mastering_display;

        bool has_content_light_level = false;
        ContentLightLevel content_light_level;

        bool isLinear() const noexcept
        {
            return transfer == TransferFunction::Linear;
        }
    };

    // Identify a named primaries set from explicit CIE 1931 xy chromaticities (e.g.
    // PNG cHRM, OpenEXR chromaticities), within a small tolerance. Returns
    // ColorPrimaries::Unspecified when no known set matches, in which case the exact
    // coordinates should be used directly. This lets decoders report a recognizable
    // enum alongside the precise coordinates for the common color spaces.
    ColorPrimaries identifyPrimaries(const ColorPoint& white, const ColorPoint& red,
                                     const ColorPoint& green, const ColorPoint& blue,
                                     float tolerance = 0.001f) noexcept;

    // Translate CICP (ITU-T H.273) code points to the corresponding enum. Unknown or
    // reserved code points map to Unspecified. Shared by the formats that signal color
    // with CICP code points (PNG cICP, AVIF, HEIF).
    ColorPrimaries colorPrimariesFromCICP(u8 code_point) noexcept;
    TransferFunction transferFunctionFromCICP(u8 code_point) noexcept;

    // ------------------------------------------------------------------
    // linearize
    // ------------------------------------------------------------------

    /*
        Options controlling linearize().

        'target' selects the output (linear) RGB primaries. The source is adapted
        into these primaries with a 3x3 matrix; the default BT.709 is the
        conventional scene-linear working space.

        When 'preserve_gamut' is set the primaries conversion is skipped and only
        the transfer function is inverted, so the output keeps the source
        primaries. Use this for wide-gamut working spaces that must not be clipped
        to 'target'.

        PQ is absolute (EOTF yields 1.0 == peak_nits). Graphics pipelines and the
        UltraHDR / gain-map float contract use relative scene-linear where
        1.0 == sdr_white_nits (BT.2408 graphics white, matching Vulkan
        SdrToHdrPQ). When sdr_white_nits > 0, PQ is scaled by peak_nits /
        sdr_white_nits so SDR white lands at ~1 and HDR highlights go above 1.
        Set sdr_white_nits to 0 to keep absolute PQ linear (1.0 == peak_nits).
    */
    struct LinearizeOptions
    {
        ColorPrimaries target = ColorPrimaries::BT709;
        bool preserve_gamut = false;
        float sdr_white_nits = 203.0f; // BT.2408; 0 = leave PQ absolute
        float peak_nits = 10000.0f;    // PQ peak (ST 2084)
    };

    /*
        Convert an image to scene-linear light using its color signalling (ColorInfo).

        Two steps, mirroring a standard HDR-aware display pipeline:
          1. invert the transfer function (sRGB / BT.709 / PQ / HLG / explicit
             gamma) to recover linear light;
          2. convert the source primaries to 'options.target' with a 3x3 matrix
             (a no-op when they already match or when preserve_gamut is set).

        'source' may be any format: it is read as normalized float, so integer
        samples map to [0,1]. PQ is then scaled into relative scene-linear
        (1.0 == options.sdr_white_nits) unless sdr_white_nits is 0.
        'dest' should be a float (FLOAT16 or FLOAT32) RGBA surface, since wide-gamut
        and HDR conversions produce unbounded and negative values; 'dest' and
        'source' must have the same dimensions. Alpha is passed through unchanged.

        The decoder pipeline never calls this: clients opt in, choosing when and to
        which working space to convert (or skipping it when ColorInfo already
        matches their pipeline). 'source' is not modified.
    */
    void linearize(const Surface& dest, const Surface& source, const ColorInfo& color,
                   const LinearizeOptions& options = LinearizeOptions());

    // ------------------------------------------------------------------
    // ColorManager
    // ------------------------------------------------------------------

    class ColorProfile : public NonCopyable
    {
    protected:
        void* m_profile;

    public:
        ColorProfile(void* profile);
        ~ColorProfile();

        operator void* () const;
    };

    class ColorManager : public NonCopyable
    {
    private:
        void* m_context;

    public:
        ColorManager();
        ~ColorManager();

        ColorProfile create(ConstMemory icc);
        ColorProfile createSRGB();

        void transform(const Surface& target, const ColorProfile& output, const ColorProfile& input);
    };

} // namespace mango::image
