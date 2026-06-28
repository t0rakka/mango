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
