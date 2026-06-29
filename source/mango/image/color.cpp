/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>
#include <mango/core/print.hpp>
#include <lcms2.h>

namespace mango::image
{

    // ------------------------------------------------------------------
    // Color space description
    // ------------------------------------------------------------------

    ColorPrimaries identifyPrimaries(const ColorPoint& white, const ColorPoint& red,
                                     const ColorPoint& green, const ColorPoint& blue,
                                     float tolerance) noexcept
    {
        struct Reference
        {
            ColorPrimaries primaries;
            float wx, wy, rx, ry, gx, gy, bx, by;
        };

        // White points: D65 (0.3127, 0.3290), DCI (0.3140, 0.3510).
        static const Reference table [] =
        {
            { ColorPrimaries::BT709,     0.3127f, 0.3290f, 0.6400f, 0.3300f, 0.3000f, 0.6000f, 0.1500f, 0.0600f },
            { ColorPrimaries::BT2020,    0.3127f, 0.3290f, 0.7080f, 0.2920f, 0.1700f, 0.7970f, 0.1310f, 0.0460f },
            { ColorPrimaries::DisplayP3, 0.3127f, 0.3290f, 0.6800f, 0.3200f, 0.2650f, 0.6900f, 0.1500f, 0.0600f },
            { ColorPrimaries::DCI_P3,    0.3140f, 0.3510f, 0.6800f, 0.3200f, 0.2650f, 0.6900f, 0.1500f, 0.0600f },
            { ColorPrimaries::AdobeRGB,  0.3127f, 0.3290f, 0.6400f, 0.3300f, 0.2100f, 0.7100f, 0.1500f, 0.0600f },
            { ColorPrimaries::BT601_625, 0.3127f, 0.3290f, 0.6400f, 0.3300f, 0.2900f, 0.6000f, 0.1500f, 0.0600f },
            { ColorPrimaries::BT601_525, 0.3127f, 0.3290f, 0.6300f, 0.3400f, 0.3100f, 0.5950f, 0.1550f, 0.0700f },
        };

        auto near = [tolerance] (float a, float b)
        {
            return std::fabs(a - b) < tolerance;
        };

        for (const Reference& ref : table)
        {
            if (near(white.x, ref.wx) && near(white.y, ref.wy) &&
                near(red.x,   ref.rx) && near(red.y,   ref.ry) &&
                near(green.x, ref.gx) && near(green.y, ref.gy) &&
                near(blue.x,  ref.bx) && near(blue.y,  ref.by))
            {
                return ref.primaries;
            }
        }

        return ColorPrimaries::Unspecified;
    }

    ColorPrimaries colorPrimariesFromCICP(u8 code_point) noexcept
    {
        switch (code_point)
        {
            case 1:  return ColorPrimaries::BT709;
            case 4:  return ColorPrimaries::BT470M;
            case 5:  return ColorPrimaries::BT601_625;
            case 6:  return ColorPrimaries::BT601_525;
            case 7:  return ColorPrimaries::BT601_525; // SMPTE 240M (shares 170M primaries)
            case 9:  return ColorPrimaries::BT2020;
            case 10: return ColorPrimaries::SMPTE428;
            case 11: return ColorPrimaries::DCI_P3;
            case 12: return ColorPrimaries::DisplayP3;
            default: return ColorPrimaries::Unspecified;
        }
    }

    TransferFunction transferFunctionFromCICP(u8 code_point) noexcept
    {
        switch (code_point)
        {
            case 1:  return TransferFunction::BT709;
            case 4:  return TransferFunction::Gamma22;
            case 5:  return TransferFunction::Gamma28;
            case 6:  return TransferFunction::BT709; // BT.601 shares the BT.709 curve
            case 8:  return TransferFunction::Linear;
            case 13: return TransferFunction::sRGB;
            case 14: return TransferFunction::BT709; // BT.2020 10-bit
            case 15: return TransferFunction::BT709; // BT.2020 12-bit
            case 16: return TransferFunction::PQ;
            case 18: return TransferFunction::HLG;
            default: return TransferFunction::Unspecified;
        }
    }

    // ------------------------------------------------------------------
    // linearize
    // ------------------------------------------------------------------

    namespace
    {
        // ---- inverse transfer functions (encoded sample -> scene-linear) ----

        inline float srgbToLinearF(float c)
        {
            return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
        }

        // ITU-R BT.709 / SMPTE 170M OETF inverse.
        inline float bt709ToLinearF(float v)
        {
            return v < 0.081f ? v / 4.5f : std::pow((v + 0.099f) / 1.099f, 1.0f / 0.45f);
        }

        // SMPTE ST 2084 (PQ) EOTF. Output normalized so 1.0 == 10,000 nits.
        inline float pqToLinearF(float e)
        {
            const float m1 = 0.1593017578125f;
            const float m2 = 78.84375f;
            const float c1 = 0.8359375f;
            const float c2 = 18.8515625f;
            const float c3 = 18.6875f;
            const float ep = std::pow(std::max(e, 0.0f), 1.0f / m2);
            const float num = std::max(ep - c1, 0.0f);
            const float den = c2 - c3 * ep;
            return std::pow(num / den, 1.0f / m1);
        }

        // ARIB STD-B67 (HLG) OETF inverse -> scene-linear [0,1].
        inline float hlgToLinearF(float e)
        {
            const float a = 0.17883277f;
            const float b = 0.28466892f;
            const float c = 0.55991073f;
            e = std::max(e, 0.0f);
            return e <= 0.5f ? (e * e) / 3.0f : (std::exp((e - c) / a) + b) / 12.0f;
        }

        float transferToLinear(float v, TransferFunction transfer, float gamma)
        {
            if (gamma > 0.0f)
            {
                return std::pow(std::max(v, 0.0f), gamma);
            }

            switch (transfer)
            {
                case TransferFunction::Linear:  return v;
                case TransferFunction::sRGB:    return srgbToLinearF(v);
                case TransferFunction::BT709:   return bt709ToLinearF(v);
                case TransferFunction::Gamma22: return std::pow(std::max(v, 0.0f), 2.2f);
                case TransferFunction::Gamma28: return std::pow(std::max(v, 0.0f), 2.8f);
                case TransferFunction::PQ:      return pqToLinearF(v);
                case TransferFunction::HLG:     return hlgToLinearF(v);
                default:                        return srgbToLinearF(v);
            }
        }

        // ---- primaries conversion ----

        struct Mat3 { float m[9]; };

        Mat3 mat3Mul(const Mat3& a, const Mat3& b)
        {
            Mat3 r;
            for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
            {
                float s = 0.0f;
                for (int k = 0; k < 3; ++k)
                    s += a.m[i * 3 + k] * b.m[k * 3 + j];
                r.m[i * 3 + j] = s;
            }
            return r;
        }

        bool mat3Inverse(const Mat3& a, Mat3& out)
        {
            const float* m = a.m;
            const float c00 = m[4] * m[8] - m[5] * m[7];
            const float c01 = m[5] * m[6] - m[3] * m[8];
            const float c02 = m[3] * m[7] - m[4] * m[6];
            const float det = m[0] * c00 + m[1] * c01 + m[2] * c02;
            if (std::fabs(det) < 1e-12f)
                return false;
            const float id = 1.0f / det;
            out.m[0] = c00 * id;
            out.m[1] = (m[2] * m[7] - m[1] * m[8]) * id;
            out.m[2] = (m[1] * m[5] - m[2] * m[4]) * id;
            out.m[3] = c01 * id;
            out.m[4] = (m[0] * m[8] - m[2] * m[6]) * id;
            out.m[5] = (m[2] * m[3] - m[0] * m[5]) * id;
            out.m[6] = c02 * id;
            out.m[7] = (m[1] * m[6] - m[0] * m[7]) * id;
            out.m[8] = (m[0] * m[4] - m[1] * m[3]) * id;
            return true;
        }

        // RGB -> CIE XYZ from CIE 1931 xy chromaticities (white normalized to Y = 1).
        bool rgbToXyz(const ColorPoint& w, const ColorPoint& r,
                      const ColorPoint& g, const ColorPoint& b, Mat3& out)
        {
            auto valid = [] (const ColorPoint& p) { return p.y > 1e-6f; };
            if (!valid(w) || !valid(r) || !valid(g) || !valid(b))
                return false;

            auto X = [] (const ColorPoint& p) { return p.x / p.y; };
            auto Z = [] (const ColorPoint& p) { return (1.0f - p.x - p.y) / p.y; };

            Mat3 M =
            {
                X(r), X(g), X(b),
                1.0f, 1.0f, 1.0f,
                Z(r), Z(g), Z(b)
            };

            Mat3 Mi;
            if (!mat3Inverse(M, Mi))
                return false;

            const float Wx = X(w);
            const float Wy = 1.0f;
            const float Wz = Z(w);
            const float Sr = Mi.m[0] * Wx + Mi.m[1] * Wy + Mi.m[2] * Wz;
            const float Sg = Mi.m[3] * Wx + Mi.m[4] * Wy + Mi.m[5] * Wz;
            const float Sb = Mi.m[6] * Wx + Mi.m[7] * Wy + Mi.m[8] * Wz;

            out.m[0] = M.m[0] * Sr; out.m[1] = M.m[1] * Sg; out.m[2] = M.m[2] * Sb;
            out.m[3] = M.m[3] * Sr; out.m[4] = M.m[4] * Sg; out.m[5] = M.m[5] * Sb;
            out.m[6] = M.m[6] * Sr; out.m[7] = M.m[7] * Sg; out.m[8] = M.m[8] * Sb;
            return true;
        }

        bool standardPrimaries(ColorPrimaries p, ColorPoint& w, ColorPoint& r,
                               ColorPoint& g, ColorPoint& b)
        {
            const ColorPoint D65 { 0.3127f, 0.3290f };
            switch (p)
            {
                case ColorPrimaries::BT709:
                    w = D65; r = { 0.640f, 0.330f }; g = { 0.300f, 0.600f }; b = { 0.150f, 0.060f }; return true;
                case ColorPrimaries::BT2020:
                    w = D65; r = { 0.708f, 0.292f }; g = { 0.170f, 0.797f }; b = { 0.131f, 0.046f }; return true;
                case ColorPrimaries::DisplayP3:
                    w = D65; r = { 0.680f, 0.320f }; g = { 0.265f, 0.690f }; b = { 0.150f, 0.060f }; return true;
                case ColorPrimaries::DCI_P3:
                    w = { 0.314f, 0.351f }; r = { 0.680f, 0.320f }; g = { 0.265f, 0.690f }; b = { 0.150f, 0.060f }; return true;
                case ColorPrimaries::AdobeRGB:
                    w = D65; r = { 0.640f, 0.330f }; g = { 0.210f, 0.710f }; b = { 0.150f, 0.060f }; return true;
                case ColorPrimaries::BT601_625:
                    w = D65; r = { 0.640f, 0.330f }; g = { 0.290f, 0.600f }; b = { 0.150f, 0.060f }; return true;
                case ColorPrimaries::BT601_525:
                    w = D65; r = { 0.630f, 0.340f }; g = { 0.310f, 0.595f }; b = { 0.155f, 0.070f }; return true;
                default:
                    return false;
            }
        }

        // RGB -> XYZ for a primaries set, taking exact chromaticities when present.
        bool primariesToXyz(const ColorInfo& color, ColorPrimaries prim, bool use_chromaticities, Mat3& out)
        {
            if (use_chromaticities && color.has_chromaticities)
            {
                if (rgbToXyz(color.white, color.red, color.green, color.blue, out))
                    return true;
                // Degenerate chromaticities (e.g. CIE XYZ primaries): channels are
                // already XYZ, so RGB->XYZ is identity.
                out = Mat3 { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
                return true;
            }

            ColorPoint w, r, g, b;
            if (standardPrimaries(prim, w, r, g, b))
                return rgbToXyz(w, r, g, b, out);

            return false;
        }

        // Build source-primaries -> target-primaries (linear) 3x3 matrix. Falls back
        // to identity when source and target match or cannot be resolved (safe no-op).
        // White-point adaptation is not applied, so non-D65 sources get a small
        // residual tint (matches the viewer reference; TODO: Bradford adaptation).
        void buildPrimariesMatrix(const ColorInfo& color, ColorPrimaries srcPrim,
                                  ColorPrimaries targetPrim, float out[9], bool& identity)
        {
            identity = true;
            out[0] = 1; out[1] = 0; out[2] = 0;
            out[3] = 0; out[4] = 1; out[5] = 0;
            out[6] = 0; out[7] = 0; out[8] = 1;

            // Same named primaries and no exact chromaticities to honour -> no-op.
            if (srcPrim == targetPrim && !color.has_chromaticities)
                return;

            Mat3 srcToXyz;
            Mat3 dstToXyz;
            if (!primariesToXyz(color, srcPrim, true, srcToXyz))
                return;
            if (!primariesToXyz(color, targetPrim, false, dstToXyz))
                return;

            Mat3 dstInv;
            if (!mat3Inverse(dstToXyz, dstInv))
                return;

            const Mat3 m = mat3Mul(dstInv, srcToXyz);
            for (int i = 0; i < 9; ++i)
                out[i] = m.m[i];

            // Detect a (near) identity result to keep the inner loop on the fast path.
            const float eps = 1e-6f;
            identity =
                std::fabs(out[0] - 1) < eps && std::fabs(out[1]) < eps && std::fabs(out[2]) < eps &&
                std::fabs(out[3]) < eps && std::fabs(out[4] - 1) < eps && std::fabs(out[5]) < eps &&
                std::fabs(out[6]) < eps && std::fabs(out[7]) < eps && std::fabs(out[8] - 1) < eps;
        }

    } // namespace

    void linearize(const Surface& dest, const Surface& source, const ColorInfo& color,
                   const LinearizeOptions& options)
    {
        if (dest.width != source.width || dest.height != source.height)
        {
            printLine(Print::Warning, "[linearize] dest and source dimensions differ.");
            return;
        }

        if (source.width <= 0 || source.height <= 0)
            return;

        // Read the source as normalized fp32 RGBA: integer samples become [0,1] and
        // the channel order is canonicalized. Float sources keep their values. This
        // never mutates 'source'.
        const Format rgba_f32(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
        Bitmap work(source, rgba_f32);

        // Resolve the effective transfer function. Unspecified follows the format
        // convention: float is linear, integer is sRGB.
        TransferFunction transfer = color.transfer;
        if (transfer == TransferFunction::Unspecified)
            transfer = source.format.isFloat() ? TransferFunction::Linear : TransferFunction::sRGB;

        const float gamma = color.gamma;

        // Resolve the source primaries (exact chromaticities take precedence).
        ColorPrimaries srcPrim = color.primaries;
        if (color.has_chromaticities)
            srcPrim = identifyPrimaries(color.white, color.red, color.green, color.blue);

        float matrix[9];
        bool identity = true;
        if (!options.preserve_gamut)
            buildPrimariesMatrix(color, srcPrim, options.target, matrix, identity);

        for (int y = 0; y < work.height; ++y)
        {
            float* p = work.address<float>(0, y);

            for (int x = 0; x < work.width; ++x)
            {
                float r = transferToLinear(p[0], transfer, gamma);
                float g = transferToLinear(p[1], transfer, gamma);
                float b = transferToLinear(p[2], transfer, gamma);

                if (!identity)
                {
                    const float R = matrix[0] * r + matrix[1] * g + matrix[2] * b;
                    const float G = matrix[3] * r + matrix[4] * g + matrix[5] * b;
                    const float B = matrix[6] * r + matrix[7] * g + matrix[8] * b;
                    r = R; g = G; b = B;
                }

                p[0] = r;
                p[1] = g;
                p[2] = b;
                // p[3] (alpha) passes through unchanged.
                p += 4;
            }
        }

        // Deliver into the caller's surface (float dest recommended; blit converts).
        dest.blit(0, 0, work);
    }

    // ------------------------------------------------------------------
    // ColorManager
    // ------------------------------------------------------------------

    ColorProfile::ColorProfile(void* profile)
        : m_profile(profile)
    {
    }

    ColorProfile::~ColorProfile()
    {
        if (m_profile)
        {
            cmsCloseProfile(reinterpret_cast<cmsHPROFILE>(m_profile));
        }
    }

    ColorProfile::operator void* () const
    {
        return m_profile;
    }

    ColorManager::ColorManager()
    {
        m_context = cmsCreateContext(nullptr, nullptr);
    }

    ColorManager::~ColorManager()
    {
        cmsContext context = reinterpret_cast<cmsContext>(m_context);
        cmsDeleteContext(context);
    }

    ColorProfile ColorManager::create(ConstMemory icc)
    {
        cmsContext context = reinterpret_cast<cmsContext>(m_context);
        cmsHPROFILE profile = cmsOpenProfileFromMemTHR(context, icc.address, cmsUInt32Number(icc.size));
        if (!profile)
        {
            MANGO_EXCEPTION("[ColorManager] Failed to open embedded ICC profile.");
        }
        return ColorProfile(profile);
    }

    ColorProfile ColorManager::createSRGB()
    {
        cmsContext context = reinterpret_cast<cmsContext>(m_context);
        cmsHPROFILE profile = cmsCreate_sRGBProfileTHR(context);
        return ColorProfile(profile);
    }

    void ColorManager::transform(const Surface& target, const ColorProfile& output, const ColorProfile& input)
    {
        const Format rgba_u8(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        const Format rgba_f32(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);

        cmsUInt32Number inType = 0;
        cmsUInt32Number outType = 0;

        if (target.format == rgba_u8)
        {
            inType = TYPE_RGBA_8;
            outType = TYPE_RGBA_8;
        }
        else if (target.format == rgba_f32)
        {
            inType = TYPE_RGBA_FLT;
            outType = TYPE_RGBA_FLT;
        }
        else
        {
            printLine(Print::Warning, "[ColorManager] transform() requires RGBA8 or RGBA float32 surface format.");
            return;
        }

        if (!static_cast<cmsHPROFILE>(input) || !static_cast<cmsHPROFILE>(output))
        {
            MANGO_EXCEPTION("[ColorManager] transform() received a null profile handle.");
        }

        cmsUInt32Number flags;
        cmsUInt32Number intent;

        if (inType == TYPE_RGBA_FLT)
        {
            intent = INTENT_RELATIVE_COLORIMETRIC;
            flags = cmsFLAGS_COPY_ALPHA;
        }
        else
        {
            intent = INTENT_PERCEPTUAL;
            flags = cmsFLAGS_BLACKPOINTCOMPENSATION;
        }

        cmsHTRANSFORM transform = cmsCreateTransform(
            input, inType,
            output, outType,
            intent, flags);
        if (!transform)
        {
            MANGO_EXCEPTION("[ColorManager] cmsCreateTransform() failed.");
        }

        const cmsUInt32Number width = cmsUInt32Number(target.width);

        if (inType == TYPE_RGBA_FLT)
        {
            for (int y = 0; y < target.height; ++y)
            {
                float* row = target.address<float>(0, y);
                cmsDoTransform(transform, row, row, width);
            }
        }
        else
        {
            for (int y = 0; y < target.height; ++y)
            {
                u8* row = target.address<u8>(0, y);
                cmsDoTransform(transform, row, row, width);
            }
        }

        cmsDeleteTransform(transform);
    }

} // namespace mango::image
