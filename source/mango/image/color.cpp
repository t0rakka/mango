/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/image/image.hpp>

#define CMS_NO_REGISTER_KEYWORD
#include "../../source/external/lcms/lcms2.h"

namespace mango::image
{

    // TODO

    /*

        cmsHPROFILE inputICC = cmsOpenProfileFromMem(icc_buffer.data(), icc_buffer.size());
        cmsHPROFILE outputICC = cmsCreate_sRGBProfile();
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

} // namespace mango::image
