/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "../core/configure.hpp"
#include "../core/memory.hpp"

namespace mango {
namespace image {

    struct Exif
    {
        // image data structure
        u32		    ImageWidth;
        u32		    ImageLength;
        u16		    BitsPerSample;
        u16		    Compression;
        u16		    PhotometricInterpretation;
        u16		    Orientation;
        u16		    SamplesPerPixel;
        u16		    PlanarConfiguration;
        u16		    YCbCrSubSampling;
        u16		    YCbCrPositioning;
        float		XResolution;
        float		YResolution;
        u16		    ResolutionUnit;

        // recording offset
        u32		    StripOffsets;
        u32		    RowsPerStrip;
        u32		    StripByteCounts;
        //case EXIF(JPEGInterchangeFormat, );
        //case EXIF(JPEGInterchangeFormatLength, );

        // image data characteristics
        u16		    TransferFunction;
        float		WhitePoint;
        float		PrimaryChromaticities;
        float		YCbCrCoefficients;
        float		ReferenceBlackWhite;

        // other tags
        std::string	DateTime;
        std::string	ImageDescription;
        std::string	Make;
        std::string	Model;
        std::string	Software;
        std::string	Artist;
        std::string	Copyright;

        float		ExposureTime;
        float		FNumber;
        u16		    ExposureProgram;
        std::string	SpectralSensitivity;
        u16		    ISOSpeedRatings;
        std::string	DateTimeOriginal;
        std::string	DateTimeDigitized;
        //ComponentsConfiguration		= 0x9101,	// Specific to compressed data; specifies the channels and complements PhotometricInterpretation
        float		CompressedBitsPerPixel;
        float		ShutterSpeedValue;
        float		ApertureValue;
        float		BrightnessValue;
        float		ExposureBiasValue;
        float		MaxApertureValue;
        float		SubjectDistance;
        u16		    MeteringMode;
        u16		    LightSource;
        u16		    Flash;
        float		FocalLength;
        //SubjectArea					= 0x9214,	// Indicates the location and area of the main subject in the overall scene.
        //MakerNote					= 0x927C,	// Manufacturer specific information.
        //UserComment					= 0x9286,	// Keywords or comments on the image; complements ImageDescription.
        //SubsecTime					= 0x9290,	// A tag used to record fractions of seconds for the DateTime tag.
        //SubsecTimeOriginal			= 0x9291,	// A tag used to record fractions of seconds for the DateTimeOriginal tag.
        //SubsecTimeDigitized			= 0x9292,	// A tag used to record fractions of seconds for the DateTimeDigitized tag.
        //FlashpixVersion				= 0xA000,	// The Flashpix format version supported by a FPXR file.
        u16		    ColorSpace;
        u32		    PixelXDimension;
        u32		    PixelYDimension;
        //RelatedSoundFile			= 0xA004,	// Used to record the name of an audio file related to the image data.
        float		FlashEnergy;
        //SpatialFrequencyResponse	= 0xA20C,	//
        float		FocalPlaneXResolution;
        float		FocalPlaneYResolution;
        u16		    FocalPlaneResolutionUnit;
        //SubjectLocation				= 0xA214,	// Indicates the location of the main subject in the scene.
        float		ExposureIndex;
        u16		    SensingMethod;
        //FileSource					= 0xA300,	// Indicates the image source.
        //SceneType					= 0xA301,	// Indicates the type of scene.
        //CFAPattern					= 0xA302,	// is used.
        u16		    CustomRendered;
        u16		    ExposureMode;
        u16		    WhiteBalance;
        float		DigitalZoomRatio;
        u16		    FocalLengthIn35mmFilm;
        u16		    SceneCaptureType;
        u16		    GainControl;
        u16		    Contrast;
        u16		    Saturation;
        u16		    Sharpness;
        //DeviceSettingDescription	= 0xA40B,	// This tag indicates information on the picture-taking conditions of a particular camera model.
        u16		    SubjectDistanceRange;
        std::string	ImageUniqueID;

        // GPS
        u8		     GPSVersionID[4];
        std::string	 GPSLatitudeRef;
        float		 GPSLatitude[3];
        std::string	 GPSLongitudeRef;
        float		 GPSLongitude[3];
        u8		     GPSAltitudeRef;
        float		 GPSAltitude;
        float		 GPSTimeStamp[3];
        std::string	 GPSSatellites;
        std::string	 GPSStatus;
        std::string	 GPSMeasureMode;
        float		 GPSDOP;
        std::string	 GPSSpeedRef;
        float		 GPSSpeed;
        std::string	 GPSTrackRef;
        float		 GPSTrack;
        std::string	 GPSImgDirectionRef;
        float		 GPSImgDirection;
        std::string	 GPSMapDatum;
        std::string	 GPSDestLatitudeRef;
        float		 GPSDestLatitude[3];
        std::string	 GPSDestLongitudeRef;
        float		 GPSDestLongitude[3];
        std::string	 GPSDestBearingRef;
        float		 GPSDestBearing;
        std::string	 GPSDestDistanceRef;
        float		 GPSDestDistance;
        std::string	 GPSDateStamp;
        u16		     GPSDifferential;

        // Canon
        std::string	LenseName;

        Exif();
        Exif(Memory memory);
    };

} // namespace image
} // namespace mango
