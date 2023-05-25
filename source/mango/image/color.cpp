/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/image/image.hpp>

#define CMS_NO_REGISTER_KEYWORD
#include "../../external/lcms/lcms2.h"

namespace mango::image
{

    ColorProfile::ColorProfile(void* profile)
        : m_profile(profile)
    {
    }

    ColorProfile::~ColorProfile()
    {
        cmsCloseProfile(m_profile);
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
        MANGO_UNREFERENCED(target);
        MANGO_UNREFERENCED(output);
        MANGO_UNREFERENCED(input);

        // TODO: lazy in-place transformation
        /*

        if (target.format.bits == 24)
        {
            cmsHTRANSFORM transform = cmsCreateTransform(inputICC, TYPE_RGB_8,
                outputICC, TYPE_RGB_8,
                INTENT_PERCEPTUAL, cmsFLAGS_BLACKPOINTCOMPENSATION);

            for (int y = 0; y < target.height; ++y)
            {
                u8* image = target.address<u8>(0, y);
                cmsDoTransform(transform, image, image, target.width);
            }
        }
        else
        {
            cmsHTRANSFORM transform = cmsCreateTransform(inputICC, TYPE_RGBA_8,
                outputICC, TYPE_RGBA_8,
                INTENT_PERCEPTUAL, cmsFLAGS_BLACKPOINTCOMPENSATION);

            for (int y = 0; y < target.height; ++y)
            {
                u8* image = target.address<u8>(0, y);
                cmsDoTransform(transform, image, image, target.width);
            }
        }

        */
    }

} // namespace mango::image
