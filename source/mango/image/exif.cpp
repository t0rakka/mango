/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include <mango/core/endian.hpp>
#include <mango/image/exif.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    enum ExifFormat : u16
    {
        EXIF_BYTE      = 1,
        EXIF_ASCII     = 2,
        EXIF_SHORT     = 3,
        EXIF_LONG      = 4,
        EXIF_RATIONAL  = 5,
        EXIF_UNDEFINED = 7,
        EXIF_SLONG     = 9,
        EXIF_SRATIONAL = 10
    };

    enum ExifTag : u16
    {
        ImageWidth                  = 0x0100,
        ImageLength                 = 0x0101,
        BitsPerSample               = 0x0102,
        Compression                 = 0x0103,
        PhotometricInterpretation   = 0x0106,
        Orientation                 = 0x0112,
        SamplesPerPixel             = 0x0115,
        PlanarConfiguration         = 0x011c,
        YCbCrSubSampling            = 0x0212,
        YCbCrPositioning            = 0x0213,
        XResolution                 = 0x011a,
        YResolution                 = 0x011b,
        ResolutionUnit              = 0x0128,
        StripOffsets                = 0x0111,
        RowsPerStrip                = 0x0116,
        StripByteCounts             = 0x0117,
        JPEGInterchangeFormat       = 0x0201,
        JPEGInterchangeFormatLength = 0x0202,
        TransferFunction            = 0x012d,
        WhitePoint                  = 0x013e,
        PrimaryChromaticities       = 0x013f,
        YCbCrCoefficients           = 0x0211,
        ReferenceBlackWhite         = 0x0214,
        DateTime                    = 0x0132,
        ImageDescription            = 0x010e,
        Make                        = 0x010f,
        Model                       = 0x0110,
        Software                    = 0x0131,
        Artist                      = 0x013b,
        Copyright                   = 0x8298,
        Exif_IFD                    = 0x8769,
        GPS_IFD                     = 0x8825,
        Interoperability_IFD        = 0xa005,
        ExposureTime                = 0x829A,
        FNumber                     = 0x829D,
        ExposureProgram             = 0x8822,
        SpectralSensitivity         = 0x8824,
        ISOSpeedRatings             = 0x8827,
        DateTimeOriginal            = 0x9003,
        DateTimeDigitized           = 0x9004,
        CompressedBitsPerPixel      = 0x9102,
        ShutterSpeedValue           = 0x9201,
        ApertureValue               = 0x9202,
        BrightnessValue             = 0x9203,
        ExposureBiasValue           = 0x9204,
        MaxApertureValue            = 0x9205,
        SubjectDistance             = 0x9206,
        MeteringMode                = 0x9207,
        LightSource                 = 0x9208,
        Flash                       = 0x9209,
        FocalLength                 = 0x920A,
        MakerNote                   = 0x927C,
        UserComment                 = 0x9286,
        ColorSpace                  = 0xA001,
        PixelXDimension             = 0xA002,
        PixelYDimension             = 0xA003,
        FlashEnergy                 = 0xA20B,
        FocalPlaneXResolution       = 0xA20E,
        FocalPlaneYResolution       = 0xA20F,
        FocalPlaneResolutionUnit    = 0xA210,
        ExposureIndex               = 0xA215,
        SensingMethod               = 0xA217,
        CustomRendered              = 0xA401,
        ExposureMode                = 0xA402,
        WhiteBalance                = 0xA403,
        DigitalZoomRatio            = 0xA404,
        FocalLengthIn35mmFilm       = 0xA405,
        SceneCaptureType            = 0xA406,
        GainControl                 = 0xA407,
        Contrast                    = 0xA408,
        Saturation                  = 0xA409,
        Sharpness                   = 0xA40A,
        SubjectDistanceRange        = 0xA40C,
        ImageUniqueID               = 0xA420,
        GPSVersionID                = 0x0000,
        GPSLatitudeRef              = 0x0001,
        GPSLatitude                 = 0x0002,
        GPSLongitudeRef             = 0x0003,
        GPSLongitude                = 0x0004,
        GPSAltitudeRef              = 0x0005,
        GPSAltitude                 = 0x0006,
        GPSTimeStamp                = 0x0007,
        GPSSatellites               = 0x0008,
        GPSStatus                   = 0x0009,
        GPSMeasureMode              = 0x000a,
        GPSDOP                      = 0x000b,
        GPSSpeedRef                 = 0x000c,
        GPSSpeed                    = 0x000d,
        GPSTrackRef                 = 0x000e,
        GPSTrack                    = 0x000f,
        GPSImgDirectionRef          = 0x0010,
        GPSImgDirection             = 0x0011,
        GPSMapDatum                 = 0x0012,
        GPSDestLatitudeRef          = 0x0013,
        GPSDestLatitude             = 0x0014,
        GPSDestLongitudeRef         = 0x0015,
        GPSDestLongitude            = 0x0016,
        GPSDestBearingRef           = 0x0017,
        GPSDestBearing              = 0x0018,
        GPSDestDistanceRef          = 0x0019,
        GPSDestDistance             = 0x001a,
        GPSDateStamp                = 0x001d,
        GPSDifferential             = 0x001e,
        CanonLenseName              = 0x0095
    };

    constexpr u32 MAX_IFD_ENTRIES = 1024;

    struct TiffView
    {
        const u8* base;
        const u8* begin;
        const u8* end;
        bool little;

        bool contains(const u8* p, size_t size) const
        {
            return p >= begin && p + size <= end;
        }

        bool contains_offset(u32 offset, size_t size) const
        {
            const u8* p = base + offset;
            return p >= begin && p + size <= end;
        }

        const u8* at(u32 offset) const
        {
            return base + offset;
        }

        u16 read16(const u8* p) const
        {
            return little ? littleEndian::uload16(p) : bigEndian::uload16(p);
        }

        u32 read32(const u8* p) const
        {
            return little ? littleEndian::uload32(p) : bigEndian::uload32(p);
        }

        s32 readS32(const u8* p) const
        {
            return s32(read32(p));
        }

        float readRational(const u8* p) const
        {
            u32 numerator = read32(p + 0);
            u32 denominator = read32(p + 4);
            return denominator ? float(numerator) / float(denominator) : 0.0f;
        }

        float readSRational(const u8* p) const
        {
            s32 numerator = readS32(p + 0);
            s32 denominator = readS32(p + 4);
            return denominator ? float(numerator) / float(denominator) : 0.0f;
        }
    };

    static u32 typeSize(u16 format)
    {
        switch (format)
        {
            case EXIF_BYTE:
            case EXIF_ASCII:
            case EXIF_UNDEFINED:
                return 1;
            case EXIF_SHORT:
                return 2;
            case EXIF_LONG:
            case EXIF_SLONG:
                return 4;
            case EXIF_RATIONAL:
            case EXIF_SRATIONAL:
                return 8;
            default:
                return 0;
        }
    }

    struct IfdEntry
    {
        u16 tag = 0;
        u16 format = 0;
        u32 count = 0;
        u32 value_or_offset = 0;
        bool valid = false;

        IfdEntry(const u8* entry, const TiffView& view)
        {
            if (!view.contains(entry, 12))
                return;

            tag = view.read16(entry + 0);
            format = view.read16(entry + 2);
            count = view.read32(entry + 4);
            value_or_offset = view.read32(entry + 8);

            const u32 size = typeSize(format);
            valid = size > 0 && count > 0;
        }

        u32 byteSize() const
        {
            return typeSize(format) * count;
        }

        const u8* valuePtr(const u8* entry, const TiffView& view) const
        {
            const u32 nbytes = byteSize();
            if (!nbytes)
                return nullptr;

            if (nbytes <= 4)
                return entry + 8;

            if (!view.contains_offset(value_or_offset, nbytes))
                return nullptr;

            return view.at(value_or_offset);
        }

        bool readAscii(std::string& value, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_ASCII)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            value.assign(reinterpret_cast<const char*>(data), count);
            const size_t zero = value.find('\0');
            if (zero != std::string::npos)
                value.resize(zero);

            return true;
        }

        bool readByte(u8& value, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_BYTE)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            value = data[0];
            return true;
        }

        bool readBytes(u8* values, u32 max_count, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_BYTE && format != EXIF_UNDEFINED)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            const u32 copy_count = std::min(count, max_count);
            std::memcpy(values, data, copy_count);
            return true;
        }

        bool readShort(u16& value, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_SHORT)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            value = view.read16(data);
            return true;
        }

        bool readLong(u32& value, const u8* entry, const TiffView& view) const
        {
            if (format == EXIF_LONG || format == EXIF_SLONG)
            {
                if (byteSize() <= 4)
                {
                    value = value_or_offset;
                    return true;
                }

                const u8* data = valuePtr(entry, view);
                if (!data)
                    return false;

                value = view.read32(data);
                return true;
            }

            if (format == EXIF_SHORT)
            {
                u16 short_value = 0;
                if (!readShort(short_value, entry, view))
                    return false;

                value = short_value;
                return true;
            }

            return false;
        }

        bool readDimension(u32& value, const u8* entry, const TiffView& view) const
        {
            return readLong(value, entry, view);
        }

        bool readRational(float& value, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_RATIONAL)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            value = view.readRational(data);
            return true;
        }

        bool readSRational(float& value, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_SRATIONAL)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            value = view.readSRational(data);
            return true;
        }

        bool readSignedOrRational(float& value, const u8* entry, const TiffView& view) const
        {
            if (readSRational(value, entry, view))
                return true;

            return readRational(value, entry, view);
        }

        bool readRationalArray(float* values, u32 max_count, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_RATIONAL)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            const u32 copy_count = std::min(count, max_count);
            for (u32 i = 0; i < copy_count; ++i)
            {
                values[i] = view.readRational(data + i * 8);
            }

            return true;
        }

        bool readUserComment(std::string& value, const u8* entry, const TiffView& view) const
        {
            if (format != EXIF_UNDEFINED && format != EXIF_ASCII)
                return false;

            const u8* data = valuePtr(entry, view);
            if (!data)
                return false;

            if (count > 8 && !std::memcmp(data, "ASCII", 5))
            {
                value.assign(reinterpret_cast<const char*>(data + 8), count - 8);
            }
            else if (format == EXIF_ASCII)
            {
                return readAscii(value, entry, view);
            }
            else
            {
                value.assign(reinterpret_cast<const char*>(data), count);
            }

            const size_t zero = value.find('\0');
            if (zero != std::string::npos)
                value.resize(zero);

            return true;
        }
    };

    void parseMakerNote(Exif& exif, const IfdEntry& maker_entry, const u8* entry_ptr, const TiffView& parent)
    {
        const u8* mn = maker_entry.valuePtr(entry_ptr, parent);
        if (!mn)
            return;

        const u8* mn_end = mn + maker_entry.byteSize();
        if (mn_end > parent.end)
            return;

        const u8* ifd = mn;
        const u8* base = mn;

        if (maker_entry.byteSize() >= 6 && !std::memcmp(mn, "Canon", 5))
        {
            if (maker_entry.byteSize() < 10)
                return;

            ifd = mn + 10;
        }

        TiffView view { base, ifd, mn_end, parent.little };
        if (!view.contains(ifd, 2))
            return;

        const u32 count = view.read16(ifd);
        if (count > MAX_IFD_ENTRIES)
            return;

        const u8* p = ifd + 2;
        if (!view.contains(p, count * 12))
            return;

        for (u32 i = 0; i < count; ++i)
        {
            const u8* item = p + i * 12;
            IfdEntry entry(item, view);
            if (!entry.valid)
                continue;

            switch (entry.tag)
            {
                case CanonLenseName:
                    entry.readAscii(exif.LenseName, item, view);
                    break;
            }
        }
    }

    void parseGPS(Exif& exif, const TiffView& view, u32 offset)
    {
        const u8* ifd = view.at(offset);
        if (!view.contains(ifd, 2))
            return;

        const u32 count = view.read16(ifd);
        if (count > MAX_IFD_ENTRIES)
            return;

        const u8* p = ifd + 2;
        if (!view.contains(p, count * 12))
            return;

        for (u32 i = 0; i < count; ++i)
        {
            const u8* item = p + i * 12;
            IfdEntry entry(item, view);
            if (!entry.valid)
                continue;

            switch (entry.tag)
            {
                case GPSVersionID:
                    entry.readBytes(exif.GPSVersionID, 4, item, view);
                    break;
                case GPSLatitudeRef:
                    entry.readAscii(exif.GPSLatitudeRef, item, view);
                    break;
                case GPSLatitude:
                    entry.readRationalArray(exif.GPSLatitude, 3, item, view);
                    break;
                case GPSLongitudeRef:
                    entry.readAscii(exif.GPSLongitudeRef, item, view);
                    break;
                case GPSLongitude:
                    entry.readRationalArray(exif.GPSLongitude, 3, item, view);
                    break;
                case GPSAltitudeRef:
                    entry.readByte(exif.GPSAltitudeRef, item, view);
                    break;
                case GPSAltitude:
                    entry.readRational(exif.GPSAltitude, item, view);
                    break;
                case GPSTimeStamp:
                    entry.readRationalArray(exif.GPSTimeStamp, 3, item, view);
                    break;
                case GPSSatellites:
                    entry.readAscii(exif.GPSSatellites, item, view);
                    break;
                case GPSStatus:
                    entry.readAscii(exif.GPSStatus, item, view);
                    break;
                case GPSMeasureMode:
                    entry.readAscii(exif.GPSMeasureMode, item, view);
                    break;
                case GPSDOP:
                    entry.readRational(exif.GPSDOP, item, view);
                    break;
                case GPSSpeedRef:
                    entry.readAscii(exif.GPSSpeedRef, item, view);
                    break;
                case GPSSpeed:
                    entry.readRational(exif.GPSSpeed, item, view);
                    break;
                case GPSTrackRef:
                    entry.readAscii(exif.GPSTrackRef, item, view);
                    break;
                case GPSTrack:
                    entry.readRational(exif.GPSTrack, item, view);
                    break;
                case GPSImgDirectionRef:
                    entry.readAscii(exif.GPSImgDirectionRef, item, view);
                    break;
                case GPSImgDirection:
                    entry.readRational(exif.GPSImgDirection, item, view);
                    break;
                case GPSMapDatum:
                    entry.readAscii(exif.GPSMapDatum, item, view);
                    break;
                case GPSDestLatitudeRef:
                    entry.readAscii(exif.GPSDestLatitudeRef, item, view);
                    break;
                case GPSDestLatitude:
                    entry.readRationalArray(exif.GPSDestLatitude, 3, item, view);
                    break;
                case GPSDestLongitudeRef:
                    entry.readAscii(exif.GPSDestLongitudeRef, item, view);
                    break;
                case GPSDestLongitude:
                    entry.readRationalArray(exif.GPSDestLongitude, 3, item, view);
                    break;
                case GPSDestBearingRef:
                    entry.readAscii(exif.GPSDestBearingRef, item, view);
                    break;
                case GPSDestBearing:
                    entry.readRational(exif.GPSDestBearing, item, view);
                    break;
                case GPSDestDistanceRef:
                    entry.readAscii(exif.GPSDestDistanceRef, item, view);
                    break;
                case GPSDestDistance:
                    entry.readRational(exif.GPSDestDistance, item, view);
                    break;
                case GPSDateStamp:
                    entry.readAscii(exif.GPSDateStamp, item, view);
                    break;
                case GPSDifferential:
                    entry.readShort(exif.GPSDifferential, item, view);
                    break;
            }
        }
    }

#define MANGO_EXIF_TAG(name, reader) \
    case name: entry.reader(exif.name, item, view); break

    void parseIFD(Exif& exif, const TiffView& view, u32 offset)
    {
        const u8* ifd = view.at(offset);
        if (!view.contains(ifd, 2))
            return;

        const u32 count = view.read16(ifd);
        if (count > MAX_IFD_ENTRIES)
            return;

        const u8* p = ifd + 2;
        if (!view.contains(p, count * 12))
            return;

        for (u32 i = 0; i < count; ++i)
        {
            const u8* item = p + i * 12;
            IfdEntry entry(item, view);
            if (!entry.valid)
                continue;

            switch (entry.tag)
            {
                MANGO_EXIF_TAG(ImageWidth, readLong);
                MANGO_EXIF_TAG(ImageLength, readLong);
                MANGO_EXIF_TAG(BitsPerSample, readShort);
                MANGO_EXIF_TAG(Compression, readShort);
                MANGO_EXIF_TAG(PhotometricInterpretation, readShort);
                MANGO_EXIF_TAG(Orientation, readShort);
                MANGO_EXIF_TAG(SamplesPerPixel, readShort);
                MANGO_EXIF_TAG(PlanarConfiguration, readShort);
                MANGO_EXIF_TAG(YCbCrSubSampling, readShort);
                MANGO_EXIF_TAG(YCbCrPositioning, readShort);
                MANGO_EXIF_TAG(XResolution, readRational);
                MANGO_EXIF_TAG(YResolution, readRational);
                MANGO_EXIF_TAG(ResolutionUnit, readShort);
                MANGO_EXIF_TAG(StripOffsets, readLong);
                MANGO_EXIF_TAG(RowsPerStrip, readLong);
                MANGO_EXIF_TAG(StripByteCounts, readLong);
                MANGO_EXIF_TAG(JPEGInterchangeFormat, readLong);
                MANGO_EXIF_TAG(JPEGInterchangeFormatLength, readLong);
                MANGO_EXIF_TAG(TransferFunction, readShort);
                MANGO_EXIF_TAG(WhitePoint, readRational);
                MANGO_EXIF_TAG(PrimaryChromaticities, readRational);
                MANGO_EXIF_TAG(YCbCrCoefficients, readRational);
                MANGO_EXIF_TAG(ReferenceBlackWhite, readRational);
                MANGO_EXIF_TAG(DateTime, readAscii);
                MANGO_EXIF_TAG(ImageDescription, readAscii);
                MANGO_EXIF_TAG(Make, readAscii);
                MANGO_EXIF_TAG(Model, readAscii);
                MANGO_EXIF_TAG(Software, readAscii);
                MANGO_EXIF_TAG(Artist, readAscii);
                MANGO_EXIF_TAG(Copyright, readAscii);
                MANGO_EXIF_TAG(ExposureTime, readRational);
                MANGO_EXIF_TAG(FNumber, readRational);
                MANGO_EXIF_TAG(ExposureProgram, readShort);
                MANGO_EXIF_TAG(SpectralSensitivity, readAscii);
                MANGO_EXIF_TAG(ISOSpeedRatings, readShort);
                MANGO_EXIF_TAG(DateTimeOriginal, readAscii);
                MANGO_EXIF_TAG(DateTimeDigitized, readAscii);
                MANGO_EXIF_TAG(CompressedBitsPerPixel, readRational);
                MANGO_EXIF_TAG(ShutterSpeedValue, readSignedOrRational);
                MANGO_EXIF_TAG(ApertureValue, readSignedOrRational);
                MANGO_EXIF_TAG(BrightnessValue, readSignedOrRational);
                MANGO_EXIF_TAG(ExposureBiasValue, readSignedOrRational);
                MANGO_EXIF_TAG(MaxApertureValue, readSignedOrRational);
                MANGO_EXIF_TAG(SubjectDistance, readRational);
                MANGO_EXIF_TAG(MeteringMode, readShort);
                MANGO_EXIF_TAG(LightSource, readShort);
                MANGO_EXIF_TAG(Flash, readShort);
                MANGO_EXIF_TAG(FocalLength, readRational);
                MANGO_EXIF_TAG(ColorSpace, readShort);
                MANGO_EXIF_TAG(PixelXDimension, readDimension);
                MANGO_EXIF_TAG(PixelYDimension, readDimension);
                MANGO_EXIF_TAG(FlashEnergy, readRational);
                MANGO_EXIF_TAG(FocalPlaneXResolution, readRational);
                MANGO_EXIF_TAG(FocalPlaneYResolution, readRational);
                MANGO_EXIF_TAG(FocalPlaneResolutionUnit, readShort);
                MANGO_EXIF_TAG(ExposureIndex, readRational);
                MANGO_EXIF_TAG(SensingMethod, readShort);
                MANGO_EXIF_TAG(CustomRendered, readShort);
                MANGO_EXIF_TAG(ExposureMode, readShort);
                MANGO_EXIF_TAG(WhiteBalance, readShort);
                MANGO_EXIF_TAG(DigitalZoomRatio, readRational);
                MANGO_EXIF_TAG(FocalLengthIn35mmFilm, readShort);
                MANGO_EXIF_TAG(SceneCaptureType, readShort);
                MANGO_EXIF_TAG(GainControl, readShort);
                MANGO_EXIF_TAG(Contrast, readShort);
                MANGO_EXIF_TAG(Saturation, readShort);
                MANGO_EXIF_TAG(Sharpness, readShort);
                MANGO_EXIF_TAG(SubjectDistanceRange, readShort);
                MANGO_EXIF_TAG(ImageUniqueID, readAscii);

                case UserComment:
                    entry.readUserComment(exif.UserComment, item, view);
                    break;

                case MakerNote:
                    parseMakerNote(exif, entry, item, view);
                    break;

                case Exif_IFD:
                {
                    u32 sub_offset = 0;
                    if (entry.readLong(sub_offset, item, view))
                        parseIFD(exif, view, sub_offset);
                    break;
                }

                case GPS_IFD:
                {
                    u32 sub_offset = 0;
                    if (entry.readLong(sub_offset, item, view))
                        parseGPS(exif, view, sub_offset);
                    break;
                }

                case Interoperability_IFD:
                    break;

                default:
                    break;
            }
        }
    }

#undef MANGO_EXIF_TAG

} // namespace

namespace mango::image
{

    Exif::Exif(ConstMemory memory)
    {
        const u8* start = memory.address;
        const u8* end = memory.end();

        if (!memory.address || memory.size < 8)
            return;

        const u16 endian = littleEndian::uload16(start);
        bool is_little_endian = false;

        switch (endian)
        {
            case 0x4949:
                is_little_endian = true;
                break;
            case 0x4d4d:
                is_little_endian = false;
                break;
            default:
                return;
        }

        TiffView view { start, start, end, is_little_endian };

        const u16 magic = view.read16(start + 2);
        switch (magic)
        {
            case 0x002a: // TIFF / EXIF
            case 0x4f52: // Olympus ORF header in some APP1 blocks
                break;
            default:
                return;
        }

        const u32 offset = view.read32(start + 4);
        if (!view.contains_offset(offset, 2))
            return;

        parseIFD(*this, view, offset);
    }

} // namespace mango::image
