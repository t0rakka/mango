/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdio>
#include <cmath>
#include <mango/core/endian.hpp>
#include <mango/image/exif.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    enum
    {
        EXIF_BYTE       = 1,
        EXIF_ASCII      = 2,
        EXIF_SHORT      = 3,
        EXIF_LONG       = 4,
        EXIF_RATIONAL   = 5,
        EXIF_UNDEFINED  = 7,
        EXIF_SLONG      = 9,
        EXIF_SRATIONAL  = 10
    };

    enum
    {
        // image data structure
        ImageWidth					= 0x0100,
        ImageLength					= 0x0101,
        BitsPerSample				= 0x0102,
        Compression					= 0x0103,
        PhotometricInterpretation	= 0x0106,
        Orientation					= 0x0112,
        SamplesPerPixel				= 0x0115,
        PlanarConfiguration			= 0x011c,
        YCbCrSubSampling			= 0x0212,
        YCbCrPositioning			= 0x0213,
        XResolution					= 0x011a,
        YResolution					= 0x011b,
        ResolutionUnit				= 0x0128,
        
        // recording offset
        StripOffsets				= 0x0111,
        RowsPerStrip				= 0x0116,
        StripByteCounts				= 0x0117,
        //JPEGInterchangeFormat		= 0x0201,
        //JPEGInterchangeFormatLength	= 0x0202,

        // image data characteristics
        TransferFunction			= 0x012d,
        WhitePoint					= 0x013e,
        PrimaryChromaticities		= 0x013f,
        YCbCrCoefficients			= 0x0211,
        ReferenceBlackWhite			= 0x0214,
        
        // other tags
        DateTime					= 0x0132,
        ImageDescription			= 0x010e,
        Make						= 0x010f,
        Model						= 0x0110,
        Software					= 0x0131,
        Artist						= 0x013b,
        Copyright					= 0x8298,
        
        Exif_IFD					= 0x8769,   // A pointer to the Exif IFD.
        GPS_IFD                     = 0x8825,   // A pointer to the Exif-related GPS Info IFD.
        Interoperability_IFD        = 0xa005,   // A pointer to the Exif-related Interoperability IFD.
        
        ExposureTime				= 0x829A,	// Exposure time, given in seconds.
        FNumber						= 0x829D,	// The F number.
        ExposureProgram				= 0x8822,	// The class of the program used by the camera to set exposure when the picture is taken.
        SpectralSensitivity			= 0x8824,	// Indicates the spectral sensitivity of each channel of the camera used.
        ISOSpeedRatings				= 0x8827,	// Indicates the ISO Speed and ISO Latitude of the camera or input device as specified in ISO 12232.
        //OECF						= 0x8828,	// Indicates the Opto-Electric Conversion Function (OECF) specified in ISO 14524.
        ExifVersion					= 0x9000,	// The version of the supported Exif standard.
        DateTimeOriginal			= 0x9003,	// The date and time when the original image data was generated.
        DateTimeDigitized			= 0x9004,	// The date and time when the image was stored as digital data.
        ComponentsConfiguration		= 0x9101,	// Specific to compressed data; specifies the channels and complements PhotometricInterpretation
        CompressedBitsPerPixel		= 0x9102,	// Specific to compressed data; states the compressed bits per pixel.
        ShutterSpeedValue			= 0x9201,	// Shutter speed.
        ApertureValue				= 0x9202,	// The lens aperture.
        BrightnessValue				= 0x9203,	// The value of brightness.
        ExposureBiasValue			= 0x9204,	// The exposure bias.
        MaxApertureValue			= 0x9205,	// The smallest F number of the lens.
        SubjectDistance				= 0x9206,	// The distance to the subject, given in meters.
        MeteringMode				= 0x9207,	// The metering mode.
        LightSource					= 0x9208,	// The kind of light source.
        Flash						= 0x9209,	// Indicates the status of flash when the image was shot.
        FocalLength					= 0x920A,	// The actual focal length of the lens, in mm.
        SubjectArea					= 0x9214,	// Indicates the location and area of the main subject in the overall scene.
        MakerNote					= 0x927C,	// Manufacturer specific information.
        UserComment					= 0x9286,	// Keywords or comments on the image; complements ImageDescription.
        SubsecTime					= 0x9290,	// A tag used to record fractions of seconds for the DateTime tag.
        SubsecTimeOriginal			= 0x9291,	// A tag used to record fractions of seconds for the DateTimeOriginal tag.
        SubsecTimeDigitized			= 0x9292,	// A tag used to record fractions of seconds for the DateTimeDigitized tag.
        FlashpixVersion				= 0xA000,	// The Flashpix format version supported by a FPXR file.
        ColorSpace					= 0xA001,	// The color space information tag is always recorded as the color space specifier.
        PixelXDimension				= 0xA002,	// Specific to compressed data; the valid width of the meaningful image.
        PixelYDimension				= 0xA003,	// Specific to compressed data; the valid height of the meaningful image.
        RelatedSoundFile			= 0xA004,	// Used to record the name of an audio file related to the image data.
        FlashEnergy					= 0xA20B,	// Indicates the strobe energy at the time the image is captured, as measured in Beam Candle Power Seconds
        SpatialFrequencyResponse	= 0xA20C,	// Records the camera or input device spatial frequency table and SFR values in the direction of image width, image height, and diagonal direction, as specified in ISO 12233.
        FocalPlaneXResolution		= 0xA20E,	// Indicates the number of pixels in the image width (X) direction per FocalPlaneResolutionUnit on the camera focal plane.
        FocalPlaneYResolution		= 0xA20F,	// Indicates the number of pixels in the image height (Y) direction per FocalPlaneResolutionUnit on the camera focal plane.
        FocalPlaneResolutionUnit	= 0xA210,	// Indicates the unit for measuring FocalPlaneXResolution and FocalPlaneYResolution.
        SubjectLocation				= 0xA214,	// Indicates the location of the main subject in the scene.
        ExposureIndex				= 0xA215,	// Indicates the exposure index selected on the camera or input device at the time the image is captured.
        SensingMethod				= 0xA217,	// Indicates the image sensor type on the camera or input device.
        FileSource					= 0xA300,	// Indicates the image source.
        SceneType					= 0xA301,	// Indicates the type of scene.
        CFAPattern					= 0xA302,	// Indicates the color filter array (CFA) geometric pattern of the image sensor when a one-chip color area sensor is used.
        CustomRendered				= 0xA401,	// Indicates the use of special processing on image data, such as rendering geared to output.
        ExposureMode				= 0xA402,	// Indicates the exposure mode set when the image was shot.
        WhiteBalance				= 0xA403,	// Indicates the white balance mode set when the image was shot.
        DigitalZoomRatio			= 0xA404,	// Indicates the digital zoom ratio when the image was shot.
        FocalLengthIn35mmFilm		= 0xA405,	// Indicates the equivalent focal length assuming a 35mm film camera, in mm.
        SceneCaptureType			= 0xA406,	// Indicates the type of scene that was shot.
        GainControl					= 0xA407,	// Indicates the degree of overall image gain adjustment.
        Contrast					= 0xA408,	// Indicates the direction of contrast processing applied by the camera when the image was shot.
        Saturation					= 0xA409,	// Indicates the direction of saturation processing applied by the camera when the image was shot.
        Sharpness					= 0xA40A,	// Indicates the direction of sharpness processing applied by the camera when the image was shot.
        DeviceSettingDescription	= 0xA40B,	// This tag indicates information on the picture-taking conditions of a particular camera model.
        SubjectDistanceRange		= 0xA40C,	// Indicates the distance to the subject.
        ImageUniqueID				= 0xA420,	// Indicates an identifier assigned uniquely to each image.

        // GPS
        GPSVersionID                = 0x0000,   // Indicates the version of GPSInfoIFD.
        GPSLatitudeRef              = 0x0001,   // Indicates whether the latitude is north or south latitude.
        GPSLatitude                 = 0x0002,   // Indicates the latitude.
        GPSLongitudeRef             = 0x0003,   // Indicates whether the longitude is east or west longitude.
        GPSLongitude                = 0x0004,   // Indicates the longitude.
        GPSAltitudeRef              = 0x0005,   // Indicates the altitude used as the reference altitude.
        GPSAltitude                 = 0x0006,   // Indicates the altitude based on the reference in GPSAltitudeRef.
        GPSTimeStamp                = 0x0007,   // Indicates the time as UTC (Coordinated Universal Time).
        GPSSatellites               = 0x0008,   // Indicates the GPS satellites used for measurements.
        GPSStatus                   = 0x0009,   // Indicates the status of the GPS receiver when the image is recorded.
        GPSMeasureMode              = 0x000a,   // Indicates the GPS measurement mode.
        GPSDOP                      = 0x000b,   // Indicates the GPS DOP (data degree of precision).
        GPSSpeedRef                 = 0x000c,   // Indicates the unit used to express the GPS receiver speed of movement.
        GPSSpeed                    = 0x000d,   // Indicates the speed of GPS receiver movement.
        GPSTrackRef                 = 0x000e,   // Indicates the reference for giving the direction of GPS receiver movement.
        GPSTrack                    = 0x000f,   // Indicates the direction of GPS receiver movement.
        GPSImgDirectionRef          = 0x0010,   // Indicates the reference for giving the direction of the image when it is captured.
        GPSImgDirection             = 0x0011,   // Indicates the direction of the image when it was captured.
        GPSMapDatum                 = 0x0012,   // Indicates the geodetic survey data used by the GPS receiver.
        GPSDestLatitudeRef          = 0x0013,   // Indicates whether the latitude of the destination point is north or south latitude.
        GPSDestLatitude             = 0x0014,   // Indicates the latitude of the destination point.
        GPSDestLongitudeRef         = 0x0015,   // Indicates whether the longitude of the destination point is east or west longitude.
        GPSDestLongitude            = 0x0016,   // Indicates the longitude of the destination point.
        GPSDestBearingRef           = 0x0017,   // Indicates the reference used for giving the bearing to the destination point.
        GPSDestBearing              = 0x0018,   // Indicates the bearing to the destination point.
        GPSDestDistanceRef          = 0x0019,   // Indicates the unit used to express the distance to the destination point.
        GPSDestDistance             = 0x001a,   // Indicates the distance to the destination point.
        GPSProcessingMethod         = 0x001b,   // A character string recording the name of the method used for location finding.
        GPSAreaInformation          = 0x001c,   // A character string recording the name of the GPS area.
        GPSDateStamp                = 0x001d,   // A character string recording date and time information relative to UTC (Coordinated Universal Time).
        GPSDifferential             = 0x001e,   // Indicates whether differential correction is applied to the GPS receiver.

        // Canon
        CanonLenseName              = 0x0095
    };

    u16 parse16(const u8* p, bool littleEndian)
    {
        return littleEndian ? uload16le(p) : uload16be(p);
    }

    u32 parse32(const u8* p, bool littleEndian)
    {
        return littleEndian ? uload32le(p) : uload32be(p);
    }

    std::string parseAscii(const u8* start, u32 data, int components)
    {
        const char* s;

        if (components <= 4)
        {
            s = reinterpret_cast<const char*>(&data);
        }
        else
        {
            s = reinterpret_cast<const char*>(start + data);
        }

        std::string value;
        value.assign(s, components);

        return value;
    }

    float parseRational(const u8* p, bool littleEndian)
    {
        u32 a = parse32(p + 0, littleEndian);
        u32 b = parse32(p + 4, littleEndian);
        return b ? float(a) / float(b) : 0;
    }

    struct Entry
    {
        u16 tag;
        u16 format;
        u32 length;
        u32 data;

        u8 valueByte;
        u16 valueShort;
        u32 valueLong;
        std::string valueAscii;
        float valueRational;

        Entry(const u8* p, const u8* start, bool littleEndian)
        {
            tag    = parse16(p + 0, littleEndian);
            format = parse16(p + 2, littleEndian);
            length = parse32(p + 4, littleEndian);
            data   = parse32(p + 8, littleEndian);

            //printf("tag: %.4x  ", tag);

            switch (format)
            {
                case EXIF_BYTE:
                    valueByte = p[8];
                    break;
                case EXIF_ASCII:
                    valueAscii = parseAscii(start, data, length);
                    break;
                case EXIF_SHORT:
                    valueShort = parse16(p + 8, littleEndian);
                    break;
                case EXIF_LONG:
                    valueLong = data;
                    break;
                case EXIF_RATIONAL:
                    valueRational = parseRational(start + data, littleEndian);
                    break;
                case EXIF_UNDEFINED:
                case EXIF_SLONG:
                case EXIF_SRATIONAL:
                    // TODO
                    break;
                default:
                    tag = 0xffff; // illegal value
                    break;
            }
        }

        void getByte(u8& value) const
        {
            if (format == EXIF_BYTE)
            {
                value = valueByte;
            }
        }

        void getAscii(std::string& value) const
        {
            if (format == EXIF_ASCII)
            {
                value = valueAscii;
            }
        }
        
        void getShort(u16& value) const
        {
            if (format == EXIF_SHORT)
            {
                value = valueShort;
            }
        }
        
        void getLong(u32& value) const
        {
            if (format == EXIF_LONG)
            {
                value = valueLong;
            }
            else if (format == EXIF_SHORT)
            {
                value = valueShort;
            }
        }

        void getRational(float& value) const
        {
            if (format == EXIF_RATIONAL)
            {
                value = valueRational;
            }
        }
    };

    void initExif(Exif& exif)
    {
        // image data structure
        exif.ImageWidth = 0;
        exif.ImageLength = 0;
        exif.BitsPerSample = 0;
        exif.Compression = 0;
        exif.PhotometricInterpretation = 0;
        exif.Orientation = 1;
        exif.SamplesPerPixel = 0;
        exif.PlanarConfiguration = 0;
        exif.YCbCrSubSampling = 0;
        exif.YCbCrPositioning = 0;
        exif.XResolution = 0;
        exif.YResolution = 0;
        exif.ResolutionUnit = 0;

        // recording offset
        exif.StripOffsets = 0;
        exif.RowsPerStrip = 0;
        exif.StripByteCounts = 0;
        //case EXIF(JPEGInterchangeFormat, );
        //case EXIF(JPEGInterchangeFormatLength, );

        // image data characteristics
        exif.TransferFunction = 0;
        exif.WhitePoint = 0;
        exif.PrimaryChromaticities = 0;
        exif.YCbCrCoefficients = 0;
        exif.ReferenceBlackWhite = 0;

        exif.ExposureTime = 0;
        exif.FNumber = 0;
        exif.ExposureProgram = 0;
        exif.ISOSpeedRatings = 0;
        //case EXIF(OECF, );
        //case EXIF(ExifVersion, );
        //case EXIF(ComponentsConfiguration, );
        exif.CompressedBitsPerPixel = 0;
        exif.ShutterSpeedValue = 0;
        exif.ApertureValue = 0;
        exif.BrightnessValue = 0;
        exif.ExposureBiasValue = 0;
        exif.MaxApertureValue = 0;
        exif.SubjectDistance = 0;
        exif.MeteringMode = 0;
        exif.LightSource = 0;
        exif.Flash = 0;
        exif.FocalLength = 0;
        //case EXIF(SubjectArea, );
        //case EXIF(MakerNote, );
        //case EXIF(UserComment, );
        //SubsecTime
        //SubsecTimeOriginal
        //SubsecTimeDigitized
        //FlashpixVersion
        exif.ColorSpace = 0;
        exif.PixelXDimension = 0;
        exif.PixelYDimension = 0;
        //RelatedSoundFile			= 0xA004,	// Used to record the name of an audio file related to the image data.
        exif.FlashEnergy = 0;
        //SpatialFrequencyResponse	= 0xA20C,	//
        exif.FocalPlaneXResolution = 0;
        exif.FocalPlaneYResolution = 0;
        exif.FocalPlaneResolutionUnit = 0;
        //SubjectLocation				= 0xA214,	// Indicates the location of the main subject in the scene.
        exif.ExposureIndex = 0;
        exif.SensingMethod = 0;
        //FileSource					= 0xA300,	// Indicates the image source.
        //SceneType					= 0xA301,	// Indicates the type of scene.
        //CFAPattern					= 0xA302,	// is used.
        exif.CustomRendered = 0;
        exif.ExposureMode = 0;
        exif.WhiteBalance = 0;
        exif.DigitalZoomRatio = 0;
        exif.FocalLengthIn35mmFilm = 0;
        exif.SceneCaptureType = 0;
        exif.GainControl = 0;
        exif.Contrast = 0;
        exif.Saturation = 0;
        exif.Sharpness = 0;
        //DeviceSettingDescription	= 0xA40B,	// This tag indicates information on the picture-taking conditions of a particular camera model.
        exif.SubjectDistanceRange = 0;

        // GPS
        exif.GPSVersionID[0] = 0;
        exif.GPSVersionID[1] = 0;
        exif.GPSVersionID[2] = 0;
        exif.GPSVersionID[3] = 0;
        exif.GPSLatitude[0] = 0;
        exif.GPSLatitude[1] = 0;
        exif.GPSLatitude[2] = 0;
        exif.GPSLongitude[0] = 0;
        exif.GPSLongitude[1] = 0;
        exif.GPSLongitude[2] = 0;
        exif.GPSAltitudeRef = 0;
        exif.GPSAltitude = 0;
        exif.GPSTimeStamp[0] = 0;
        exif.GPSTimeStamp[1] = 0;
        exif.GPSTimeStamp[2] = 0;
        exif.GPSDOP = 0;
        exif.GPSSpeed = 0;
        exif.GPSTrack = 0;
        exif.GPSImgDirection = 0;
        exif.GPSDestLatitude[0] = 0;
        exif.GPSDestLatitude[1] = 0;
        exif.GPSDestLatitude[2] = 0;
        exif.GPSDestLongitude[0] = 0;
        exif.GPSDestLongitude[1] = 0;
        exif.GPSDestLongitude[2] = 0;
        exif.GPSDestBearing = 0;
        exif.GPSDestDistance = 0;
        exif.GPSDifferential = 0;
    }

#define EXIF(name, type) name: entry.get##type(exif.name); break

    void parseGPS(Exif& exif, const u8* p, const u8* start, bool littleEndian)
    {
        MANGO_UNREFERENCED(exif);

        int count = parse16(p, littleEndian);
        p += 2;
        for (int i = 0; i < count; ++i)
        {
            Entry entry(p, start, littleEndian);

            switch (entry.tag)
            {
                case 0xffff: // TODO: silence compiler warning
                    break;
#if 0 // TODO
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
#endif
                default:
                    //printf("gps tag: %.4x, format: %d, length: %d\n", entry.tag, entry.format, entry.length); // TODO: debug
                    break;
            }

            p += 12;
        }
    }

    void parseMakerNote(Exif& exif, const u8* p, const u8* start, bool littleEndian)
    {
        // support MakerNote only for "Canon" manufacturer for now
        //if (exif.Make != "Canon")
        //    return;

        int count = parse16(p, littleEndian);
        if (count > 255)
        {
            // MakerNote is sometimes in different endianess, this bails out of any high-byte bits are set
            return;
        }
        p += 2;

        for (int i = 0; i < count; ++i)
        {
            Entry entry(p, start, littleEndian);
            if (entry.format > 10)
            {
                // Sanity-check for corrupted files.
                return;
            }

            switch (entry.tag)
            {
                case CanonLenseName:
                    entry.getAscii(exif.LenseName);
                    break;

                default:
                    //printf("MakerNote tag: %.4x, format: %d, length: %d\n", entry.tag, entry.format, entry.length);
                    break;
            }
            
            p += 12;
        }
    }

    void parseIFD(Exif& exif, const u8* p, const u8* start, bool littleEndian)
    {
        int count = parse16(p, littleEndian);
        p += 2;
        for (int i = 0; i < count; ++i)
        {
            Entry entry(p, start, littleEndian);

            switch (entry.tag)
            {
                // image data structure
                case EXIF(ImageWidth, Long);
                case EXIF(ImageLength, Long);
                case EXIF(BitsPerSample, Short);
                case EXIF(Compression, Short);
                case EXIF(PhotometricInterpretation, Short);
                case EXIF(Orientation, Short);
                case EXIF(SamplesPerPixel, Short);
                case EXIF(PlanarConfiguration, Short);
                case EXIF(YCbCrSubSampling, Short);
                case EXIF(YCbCrPositioning, Short);
                case EXIF(XResolution, Rational);
                case EXIF(YResolution, Rational);
                case EXIF(ResolutionUnit, Short);

                // recording offset
                //case EXIF(StripOffsets, );
                //case EXIF(RowsPerStrip, );
                //case EXIF(StripByteCounts, );
                //case EXIF(JPEGInterchangeFormat, );
                //case EXIF(JPEGInterchangeFormatLength, );

                // image data characteristics
                case EXIF(TransferFunction, Short);
                case EXIF(WhitePoint, Rational);
                case EXIF(PrimaryChromaticities, Rational);
                case EXIF(YCbCrCoefficients, Rational);
                case EXIF(ReferenceBlackWhite, Rational);

                // other tags
                case EXIF(DateTime, Ascii);
                case EXIF(ImageDescription, Ascii);
                case EXIF(Make, Ascii);
                case EXIF(Model, Ascii);
                case EXIF(Software, Ascii);
                case EXIF(Artist, Ascii);
                case EXIF(Copyright, Ascii);

                case EXIF(ExposureTime, Rational);
                case EXIF(FNumber, Rational);
                case EXIF(ExposureProgram, Short);
                case EXIF(SpectralSensitivity, Ascii);
                case EXIF(ISOSpeedRatings, Short);
                //case EXIF(OECF, );
                //case EXIF(ExifVersion, ); // 7
                case EXIF(DateTimeOriginal, Ascii);
                case EXIF(DateTimeDigitized, Ascii);
                //case EXIF(ComponentsConfiguration, ); // 7
                case EXIF(CompressedBitsPerPixel, Rational);
                case EXIF(ShutterSpeedValue, Rational);
                case EXIF(ApertureValue, Rational);
                case EXIF(BrightnessValue, Rational);
                case EXIF(ExposureBiasValue, Rational);
                case EXIF(MaxApertureValue, Rational);
                case EXIF(SubjectDistance, Rational);
                case EXIF(MeteringMode, Short);
                case EXIF(LightSource, Short);
                case EXIF(Flash, Short);
                case EXIF(FocalLength, Rational);
                //case EXIF(SubjectArea, );
                //case EXIF(MakerNote, );
                //case EXIF(UserComment, ); // 7
                //SubsecTime
                //SubsecTimeOriginal
                //SubsecTimeDigitized
                //FlashpixVersion, ); // 7
                case EXIF(ColorSpace, Short);
                case EXIF(PixelXDimension, Long); // TODO: could also be Short
                case EXIF(PixelYDimension, Long); // TODO: could also be Short
                //RelatedSoundFile			= 0xA004,	// Used to record the name of an audio file related to the image data.
                case EXIF(FlashEnergy, Rational);
                //SpatialFrequencyResponse	= 0xA20C,	//
                case EXIF(FocalPlaneXResolution, Rational);
                case EXIF(FocalPlaneYResolution, Rational);
                case EXIF(FocalPlaneResolutionUnit, Short);
                //SubjectLocation				= 0xA214,	// Indicates the location of the main subject in the scene.
                case EXIF(ExposureIndex, Rational);
                case EXIF(SensingMethod, Short);
                //FileSource					= 0xA300,	// Indicates the image source.
                //SceneType					= 0xA301,	// Indicates the type of scene.
                //CFAPattern					= 0xA302,	// is used.
                case EXIF(CustomRendered, Short);
                case EXIF(ExposureMode, Short);
                case EXIF(WhiteBalance, Short);
                case EXIF(DigitalZoomRatio, Rational);
                case EXIF(FocalLengthIn35mmFilm, Short);
                case EXIF(SceneCaptureType, Short);
                case EXIF(GainControl, Short);
                case EXIF(Contrast, Short);
                case EXIF(Saturation, Short);
                case EXIF(Sharpness, Short);
                //DeviceSettingDescription	= 0xA40B,	// This tag indicates information on the picture-taking conditions of a particular camera model.
                case EXIF(SubjectDistanceRange, Short);
                case EXIF(ImageUniqueID, Ascii);

#if 0
                case ShutterSpeedValue:
                {
                    const u8* p = base + offset;
                    u32 v0 = read32(p, endian);
                    u32 v1 = read32(p, endian);
                    double s = double(v0) / double(v1);
                    float f = float(1.0 / pow(2.0, s));
                    //printf(" (%f sec) ", f);
                    break;
                }

                case ExposureTime:
                {
                    const u8* p = base + offset;
                    u32 v0 = read32(p, endian);
                    u32 v1 = read32(p, endian);
                    //printf(" (%i/%i sec) ", v0, v1);
                    break;
                }

                case ResolutionUnit:
                {
                    //if (offset == 2) printf(" (inches)");
                    //if (offset == 3) printf(" (centimeters)");
                    break;
                }
#endif

                case UserComment:
                    //parseIFD(exif, start + entry.data, start, littleEndian);
                    break;

                case MakerNote:
                    parseMakerNote(exif, start + entry.data, start, littleEndian);
                    break;

                case Exif_IFD:
                    parseIFD(exif, start + entry.data, start, littleEndian);
                    break;

                case GPS_IFD:
                    parseGPS(exif, start + entry.data, start, littleEndian);
                    break;

                case Interoperability_IFD:
                    break;

                default:
                    //printf("tag: %.4x, format: %d, length: %d\n", entry.tag, entry.format, entry.length); // TODO: debug
                    break;
            }

            p += 12;
        }
    }

#undef EXIF

} // namespace

namespace mango {
namespace image {

    Exif::Exif()
    {
        initExif(*this);
    }

    Exif::Exif(Memory memory)
    {
        initExif(*this);

        const u8* start = memory.address;
        const u8* end = start + memory.size;

        const u8* p = start;
        if (p > end - 8)
            return; // ERROR: out of bytes

        u16 endian = parse16(p + 0, true);

        bool littleEndian;
        switch (endian)
        {
            case 0x4949:
                littleEndian = true;
                break;
            case 0x4d4d:
                littleEndian = false;
                break;
            default:
                return; // ERROR: incorrect endian
        }

        u16 magic = parse16(p + 2, littleEndian);
        switch (magic)
        {
            case 0x002a:
                // TIFF/EXIF
                break;
            case 0x4f52:
                // OLYMPUS/EXIF
                break;
            default:
                // ERROR: incorrect magic
                return;
        }

        u32 offset = parse32(p + 4, littleEndian);
        p += offset;
        if (p > end)
            return; // ERROR: out of bytes

        parseIFD(*this, p, start, littleEndian);
    }

} // namespace image
} // namespace mango
