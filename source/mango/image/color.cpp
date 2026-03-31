/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/image/image.hpp>
#include <mango/core/print.hpp>
#include <lcms2.h>

namespace mango::image
{

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
