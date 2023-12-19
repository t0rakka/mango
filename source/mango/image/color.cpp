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
        // NOTE: lazy in-place ICC profile transformation

        // MANGO TODO:
        // - DoTransform can be multi-threaded
        // - do direct transform when output and input are compatible

        cmsHTRANSFORM transform = cmsCreateTransform(
            input, TYPE_RGBA_8,
            output, TYPE_RGBA_8,
            INTENT_PERCEPTUAL, cmsFLAGS_BLACKPOINTCOMPENSATION);

        Bitmap temp(target, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        for (int y = 0; y < target.height; ++y)
        {
            u8* image = temp.address<u8>(0, y);
            cmsDoTransform(transform, image, image, target.width);
        }

        target.blit(0, 0, temp);
    }

} // namespace mango::image
