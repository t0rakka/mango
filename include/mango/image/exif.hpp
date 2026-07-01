/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/core/configure.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/memory.hpp>

namespace mango::image
{

    struct Exif
    {
        Status      status;

        // image data structure
        u32         ImageWidth = 0;
        u32         ImageLength = 0;
        u16         BitsPerSample = 0;
        u16         Compression = 0;
        u16         PhotometricInterpretation = 0;
        u16         Orientation = 1;
        u16         SamplesPerPixel = 0;
        u16         PlanarConfiguration = 0;
        u16         YCbCrSubSampling = 0;
        u16         YCbCrPositioning = 0;
        float       XResolution = 0;
        float       YResolution = 0;
        u16         ResolutionUnit = 0;

        // recording offset
        u32         StripOffsets = 0;
        u32         RowsPerStrip = 0;
        u32         StripByteCounts = 0;
        u32         JPEGInterchangeFormat = 0;
        u32         JPEGInterchangeFormatLength = 0;

        // image data characteristics
        u16         TransferFunction = 0;
        float       WhitePoint = 0;
        float       PrimaryChromaticities = 0;
        float       YCbCrCoefficients = 0;
        float       ReferenceBlackWhite = 0;

        // other tags
        std::string DateTime;
        std::string ImageDescription;
        std::string Make;
        std::string Model;
        std::string Software;
        std::string Artist;
        std::string Copyright;

        float       ExposureTime = 0;
        float       FNumber = 0;
        u16         ExposureProgram = 0;
        std::string SpectralSensitivity;
        u16         ISOSpeedRatings = 0;
        std::string DateTimeOriginal;
        std::string DateTimeDigitized;
        float       CompressedBitsPerPixel = 0;
        float       ShutterSpeedValue = 0;
        float       ApertureValue = 0;
        float       BrightnessValue = 0;
        float       ExposureBiasValue = 0;
        float       MaxApertureValue = 0;
        float       SubjectDistance = 0;
        u16         MeteringMode = 0;
        u16         LightSource = 0;
        u16         Flash = 0;
        float       FocalLength = 0;
        std::string MakerNote;
        std::string UserComment;
        u16         ColorSpace = 0;
        u32         PixelXDimension = 0;
        u32         PixelYDimension = 0;
        float       FlashEnergy = 0;
        float       FocalPlaneXResolution = 0;
        float       FocalPlaneYResolution = 0;
        u16         FocalPlaneResolutionUnit = 0;
        float       ExposureIndex = 0;
        u16         SensingMethod = 0;
        u16         CustomRendered = 0;
        u16         ExposureMode = 0;
        u16         WhiteBalance = 0;
        float       DigitalZoomRatio = 0;
        u16         FocalLengthIn35mmFilm = 0;
        u16         SceneCaptureType = 0;
        u16         GainControl = 0;
        u16         Contrast = 0;
        u16         Saturation = 0;
        u16         Sharpness = 0;
        u16         SubjectDistanceRange = 0;
        std::string ImageUniqueID;

        // GPS
        u8          GPSVersionID[4] {};
        std::string GPSLatitudeRef;
        float       GPSLatitude[3] {};
        std::string GPSLongitudeRef;
        float       GPSLongitude[3] {};
        u8          GPSAltitudeRef = 0;
        float       GPSAltitude = 0;
        float       GPSTimeStamp[3] {};
        std::string GPSSatellites;
        std::string GPSStatus;
        std::string GPSMeasureMode;
        float       GPSDOP = 0;
        std::string GPSSpeedRef;
        float       GPSSpeed = 0;
        std::string GPSTrackRef;
        float       GPSTrack = 0;
        std::string GPSImgDirectionRef;
        float       GPSImgDirection = 0;
        std::string GPSMapDatum;
        std::string GPSDestLatitudeRef;
        float       GPSDestLatitude[3] {};
        std::string GPSDestLongitudeRef;
        float       GPSDestLongitude[3] {};
        std::string GPSDestBearingRef;
        float       GPSDestBearing = 0;
        std::string GPSDestDistanceRef;
        float       GPSDestDistance = 0;
        std::string GPSDateStamp;
        u16         GPSDifferential = 0;

        // MakerNote lens model (Canon, Nikon, Fuji, Olympus, Pentax, ...)
        std::string LenseName;

        operator bool () const
        {
            return status;
        }

        Exif() = default;
        Exif(ConstMemory memory);
    };

} // namespace mango::image
