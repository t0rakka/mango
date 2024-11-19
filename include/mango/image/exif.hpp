/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>

namespace mango::image
{

    struct Exif
    {
        // image data structure
        u32         ImageWidth;
        u32         ImageLength;
        u16         BitsPerSample;
        u16         Compression;
        u16         PhotometricInterpretation;
        u16         Orientation;
        u16         SamplesPerPixel;
        u16         PlanarConfiguration;
        u16         YCbCrSubSampling;
        u16         YCbCrPositioning;
        float       XResolution;
        float       YResolution;
        u16         ResolutionUnit;

        // recording offset
        u32         StripOffsets;
        u32         RowsPerStrip;
        u32         StripByteCounts;
        u32         JPEGInterchangeFormat;
        u32         JPEGInterchangeFormatLength;

        // image data characteristics
        u16         TransferFunction;
        float       WhitePoint;
        float       PrimaryChromaticities;
        float       YCbCrCoefficients;
        float       ReferenceBlackWhite;

        // other tags
        std::string DateTime;
        std::string ImageDescription;
        std::string Make;
        std::string Model;
        std::string Software;
        std::string Artist;
        std::string Copyright;

        float       ExposureTime;
        float       FNumber;
        u16         ExposureProgram;
        std::string SpectralSensitivity;
        u16         ISOSpeedRatings;
        std::string DateTimeOriginal;
        std::string DateTimeDigitized;
        float       CompressedBitsPerPixel;
        float       ShutterSpeedValue;
        float       ApertureValue;
        float       BrightnessValue;
        float       ExposureBiasValue;
        float       MaxApertureValue;
        float       SubjectDistance;
        u16         MeteringMode;
        u16         LightSource;
        u16         Flash;
        float       FocalLength;
        std::string MakerNote;
        std::string UserComment;
        u16         ColorSpace;
        u32         PixelXDimension;
        u32         PixelYDimension;
        float       FlashEnergy;
        float       FocalPlaneXResolution;
        float       FocalPlaneYResolution;
        u16         FocalPlaneResolutionUnit;
        float       ExposureIndex;
        u16         SensingMethod;
        u16         CustomRendered;
        u16         ExposureMode;
        u16         WhiteBalance;
        float       DigitalZoomRatio;
        u16         FocalLengthIn35mmFilm;
        u16         SceneCaptureType;
        u16         GainControl;
        u16         Contrast;
        u16         Saturation;
        u16         Sharpness;
        u16         SubjectDistanceRange;
        std::string ImageUniqueID;

        // GPS
        u8          GPSVersionID[4];
        std::string GPSLatitudeRef;
        float       GPSLatitude[3];
        std::string GPSLongitudeRef;
        float       GPSLongitude[3];
        u8          GPSAltitudeRef;
        float       GPSAltitude;
        float       GPSTimeStamp[3];
        std::string GPSSatellites;
        std::string GPSStatus;
        std::string GPSMeasureMode;
        float       GPSDOP;
        std::string GPSSpeedRef;
        float       GPSSpeed;
        std::string GPSTrackRef;
        float       GPSTrack;
        std::string GPSImgDirectionRef;
        float       GPSImgDirection;
        std::string GPSMapDatum;
        std::string GPSDestLatitudeRef;
        float       GPSDestLatitude[3];
        std::string GPSDestLongitudeRef;
        float       GPSDestLongitude[3];
        std::string GPSDestBearingRef;
        float       GPSDestBearing;
        std::string GPSDestDistanceRef;
        float       GPSDestDistance;
        std::string GPSDateStamp;
        u16         GPSDifferential;

        // Canon
        std::string LenseName;

        Exif();
        Exif(ConstMemory memory);
    };

} // namespace mango::image
