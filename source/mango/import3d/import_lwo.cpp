/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/import3d/import_lwo.hpp>

/*
    NewTek LightWave Object importer
*/

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    static
    constexpr float pi = float(math::pi);

    static
    void xyz_to_h(float x, float y, float z, float& h)
    {
        MANGO_UNREFERENCED(y);

        if (x == 0.0f && z == 0.0f)
        {
            h = 0.0f;
        }
        else
        {
            if (z == 0.0f)
                h = x < 0 ? pi * 0.5f : pi * -0.5f;
            else if (z < 0.0f)
                h = -(std::atan(x / z) + pi);
            else
                h = -std::atan(x / z);
        }
    }

    static
    void xyz_to_hp(float x, float y, float z, float& h, float& p)
    {
        if (x == 0.0f && z == 0.0f)
        {
            h = 0.0f;
            if (y != 0.0f)
                p = y < 0.0f ? pi * -0.5f : pi * 0.5f;
            else
                p = 0.0f;
        }
        else
        {
            if (z == 0.0f)
                h = x < 0.0f ? pi * 0.5f : pi * -0.5f;
            else if (z < 0.0f)
                h = -(std::atan(x / z) + pi);
            else
                h = -std::atan(x / z);

            x = std::sqrt(x * x + z * z);
            if (x == 0.0f)
                p = y < 0.0f ? pi * -0.5f : pi * 0.5f;
            else
                p = std::atan(y / x);
        }
    }

    static inline
    float fract(float v)
    {
        return v - std::floor(v);
    }

    inline
    float32x3 read_vec3(BigEndianConstPointer& p)
    {
        float x = p.read32f();
        float y = p.read32f();
        float z = p.read32f();
        return float32x3(x, y, z);
    }

    inline
    std::string read_string(BigEndianConstPointer& p)
    {
        const char* s = p.cast<const char>();
        std::string str(s);
        size_t length = str.length() + 1;
        p += length + (length & 1);
        return str;
    }

    inline
    u32 read_index(BigEndianConstPointer& p)
    {
        u32 index = (p[0] == 0xff) ? p.read32() & 0x00ffffff : p.read16();
        return index;
    }

} // namespace

namespace mango::import3d
{

    // --------------------------------------------------------------------------
    // LightWave v5
    // --------------------------------------------------------------------------

    struct TextureLWO
    {
        enum Flags
        {
            XAxis         = 0x0001,
            YAxis         = 0x0002,
            ZAxis         = 0x0004,
            WorldCoords   = 0x0008,
            NegativeImage = 0x0010,
            PixelBlending = 0x0020,
            Antialiasing  = 0x0040,
        };

        enum Type
        {
            Planar      = 0,
            Cylindrical = 1,
            Spherical   = 2,
            Cubic       = 3,
            FrontProj   = 4,
            UV          = 5,
        };

        u16 flags = XAxis;
        Type type = Planar;

        std::string name;

        image::Color color = 0xffffffff;
        float value = 1.0f;
        float bumpAmplitude = 1.0f;

        float32x3 center { 0.0f, 0.0f, 0.0f };
        float32x3 velocity { 0.0f, 0.0f, 0.0f };
        float32x3 falloff { 0.0f, 0.0f, 0.0f };
        float32x3 size { 1.0f, 1.0f, 1.0f };
        float32x3 tile { 1.0f, 1.0f, 1.0f };

        float32x2 texcoord(const float32x3& point, float time = 0.0f) const
        {
            float32x3 p = point - center + (velocity - falloff * time) * time;

            float32x2 texcoord(0.0f, 0.0f);

            switch (type)
            {
                case Planar:
                {
                    float s = (flags & XAxis) ? p.z / size.z + 0.5f :  p.x / size.x + 0.5f;
                    float t = (flags & YAxis) ? p.z / size.z + 0.5f :  p.y / size.y + 0.5f;

                    texcoord = float32x2(s * tile.x, -t * tile.y);
                    break;
                }

                case Cylindrical:
                {
                    float lon;
                    float t;

                    if (flags & XAxis)
                    {
                        xyz_to_h(p.z, p.x, -p.y, lon);
                        t = -p.x / size.x + 0.5f;
                    }
                    else if (flags & YAxis)
                    {
                        xyz_to_h(-p.x, p.y, p.z, lon);
                        t = -p.y / size.y + 0.5f;
                    }
                    else
                    {
                        xyz_to_h(-p.x, p.z, -p.y, lon);
                        t = -p.z / size.z + 0.5f;
                    }

                    lon = 1.0f - lon / (pi * 2.0f);
                    if (tile.x != 1.0f)
                        lon = fract(lon) * tile.x;
                    float s = fract(lon) / 4.0f;
                    t = fract(t);

                    texcoord = float32x2(s, t);
                    break;
                }

                case Spherical:
                {
                    float lon;
                    float lat;

                    if (flags & XAxis)
                        xyz_to_hp( p.z, p.x, -p.y, lon, lat);
                    else if (flags & YAxis)
                        xyz_to_hp(-p.x, p.y, p.z, lon, lat);
                    else
                        xyz_to_hp(-p.x, p.z, -p.y, lon, lat);

                    lon = 1.0f - lon / (pi * 2.0f);
                    lat = 0.5f - lat / pi;
                    float s = fract(fract(lon) * tile.x) / 4.0f;
                    float t = fract(fract(lat) * tile.y);

                    texcoord = float32x2(s, t);
                    break;
                };

                case Cubic:
                case FrontProj:
                case UV:
                default:
                    break;
            }

            return texcoord;
        }
    };

    struct TriangleLWO
    {
        u16 point[3];
    };

    struct SurfaceLWO
    {
        enum Flags : u16
        {
            Luminous         = 0x0001,
            Outline          = 0x0002,
            Smoothing        = 0x0004,
            ColorHighlights  = 0x0008,
            ColorFilter      = 0x0010,
            OpaqueEdge       = 0x0020,
            TransparentEdge  = 0x0040,
            SharpTerminator  = 0x0080,
            DoubleSided      = 0x0100,
            Additive         = 0x0200,
        };

        std::string name;
        u16 flags = 0;
        image::Color color = 0xffffffff;

        float smoothingAngle = 0.0f;

        float luminosity = 0.0f;
        float diffuse = 0.0f;
        float specular = 0.0f;
        float reflection = 0.0f;
        float transparency = 0.0f;

        TextureLWO ctex; // color
        TextureLWO dtex; // diffuse
        TextureLWO stex; // specular
        TextureLWO rtex; // reflection
        TextureLWO ttex; // transparency
        TextureLWO btex; // bump

        std::vector<TriangleLWO> triangles;
    };

    struct ReaderLWO
    {
        std::vector<float32x3> points;
        std::vector<SurfaceLWO> surfaces;

        SurfaceLWO* currentSurface = nullptr;
        TextureLWO* currentTexture = nullptr;

        int level = 0;

        bool has_PNTS = false;
        bool has_SRFS = false;
        bool has_POLS = false;

        ReaderLWO(BigEndianConstPointer p, const u8* end)
        {
            while (p < end)
            {
                parse_chunks(p);
            }
        }

        void parse_chunks(BigEndianConstPointer& p)
        {
            u32 id = p.read32();
            u32 length = p.read32();

            ++level;

            char c[4];
            std::memcpy(c, &id, 4);
            printLine(Print::Verbose, level * 2, "[{}{}{}{}] {} bytes", c[3], c[2], c[1], c[0], length);

            switch (id)
            {
                case u32_mask_rev('P','N','T','S'): chunk_PNTS(p, length); break;
                case u32_mask_rev('S','R','F','S'): chunk_SRFS(p, length); break;
                case u32_mask_rev('P','O','L','S'): chunk_POLS(p, length); break;
                case u32_mask_rev('C','R','V','R'): chunk_CRVS(p, length); break;
                case u32_mask_rev('S','U','R','F'): chunk_SURF(p, length); break;
                case u32_mask_rev('T','A','G','S'): chunk_TAGS(p, length); break;
                case u32_mask_rev('L','A','Y','R'): chunk_LAYR(p, length); break;
                case u32_mask_rev('B','B','O','X'): chunk_BBOX(p, length); break;
                case u32_mask_rev('P','T','A','G'): chunk_PTAG(p, length); break;
                case u32_mask_rev('V','M','A','P'): chunk_VMAP(p, length); break;
                case u32_mask_rev('V','M','A','D'): chunk_VMAD(p, length); break;

                default:
                    printLine(Print::Verbose, level * 2 + 2, "NOT SUPPORTED");
                    break;
            };

            p += length + (length & 1);
            --level;
        }

        void parse_subchunks(BigEndianConstPointer& p)
        {
            u32 id = p.read32();
            u32 length = p.read16();

            ++level;

#if 1
            char c[4];
            std::memcpy(c, &id, 4);
            printLine(Print::Verbose, level * 2, "[{}{}{}{}] {} bytes", c[3], c[2], c[1], c[0], length);
#endif

            switch (id)
            {
                case u32_mask_rev('F','L','A','G'): chunk_FLAG(p, length); break;
                case u32_mask_rev('C','O','L','R'): chunk_COLR(p, length); break;
                case u32_mask_rev('L','U','M','I'): chunk_LUMI(p, length); break;
                case u32_mask_rev('D','I','F','F'): chunk_DIFF(p, length); break;
                case u32_mask_rev('S','P','E','C'): chunk_SPEC(p, length); break;
                case u32_mask_rev('R','E','F','L'): chunk_REFL(p, length); break;
                case u32_mask_rev('T','R','A','N'): chunk_TRAN(p, length); break;
                case u32_mask_rev('S','M','A','N'): chunk_SMAN(p, length); break;
                case u32_mask_rev('G','L','O','S'): chunk_GLOS(p, length); break;
                case u32_mask_rev('V','D','I','F'): chunk_VDIF(p, length); break;
                case u32_mask_rev('V','S','P','C'): chunk_VSPC(p, length); break;
                case u32_mask_rev('C','T','E','X'): chunk_CTEX(p, length); break;
                case u32_mask_rev('D','T','E','X'): chunk_DTEX(p, length); break;
                case u32_mask_rev('S','T','E','X'): chunk_STEX(p, length); break;
                case u32_mask_rev('R','T','E','X'): chunk_RTEX(p, length); break;
                case u32_mask_rev('T','T','E','X'): chunk_TTEX(p, length); break;
                case u32_mask_rev('B','T','E','X'): chunk_BTEX(p, length); break;
                case u32_mask_rev('T','I','M','G'): chunk_TIMG(p, length); break;
                case u32_mask_rev('T','F','L','G'): chunk_TFLG(p, length); break;
                case u32_mask_rev('T','S','I','Z'): chunk_TSIZ(p, length); break;
                case u32_mask_rev('T','C','T','R'): chunk_TCTR(p, length); break;
                case u32_mask_rev('T','F','A','L'): chunk_TFAL(p, length); break;
                case u32_mask_rev('T','V','E','L'): chunk_TVEL(p, length); break;
                case u32_mask_rev('T','W','R','P'): chunk_TWRP(p, length); break;
                case u32_mask_rev('T','F','P','0'): chunk_TFP0(p, length); break;
                case u32_mask_rev('T','F','P','1'): chunk_TFP1(p, length); break;
                case u32_mask_rev('T','A','A','S'): chunk_TAAS(p, length); break;
                case u32_mask_rev('T','A','M','P'): chunk_TAMP(p, length); break;
                case u32_mask_rev('T','I','P','0'): chunk_TIP0(p, length); break;
                case u32_mask_rev('T','C','L','R'): chunk_TCLR(p, length); break;
                case u32_mask_rev('T','V','A','L'): chunk_TVAL(p, length); break;

                case u32_mask_rev('T','F','R','Q'):
                case u32_mask_rev('T','S','P','0'):
                case u32_mask_rev('T','S','P','1'):
                case u32_mask_rev('T','S','P','2'):
                    // not supported
                    break;

                default:
                    printLine(Print::Verbose, level * 2 + 2, "NOT SUPPORTED");
                    break;
            }

            p += length + (length & 1);
            --level;
        }

        void chunk_PNTS(BigEndianConstPointer p, u32 length)
        {
            has_PNTS = true;

            u32 count = length / sizeof(float32x3);
            points.resize(count);

            printLine(Print::Verbose, level * 2 + 2, "points: {}", count);

            for (size_t i = 0; i < count; ++i)
            {
                points[i] = read_vec3(p);
            }
        }

        void chunk_SRFS(BigEndianConstPointer p, u32 length)
        {
            has_SRFS = true;

            const u8* end = p + length;

            while (p < end)
            {
                std::string name = read_string(p);
                printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);

                SurfaceLWO surface;
                surface.name = name;

                surfaces.push_back(surface);
            }
        }

        void chunk_POLS(BigEndianConstPointer p, u32 length)
        {
            if (!has_PNTS || !has_SRFS)
            {
                MANGO_EXCEPTION("[ImportLWO] PNTS and SRFS chunks must come before POLS chunk.");
            }

            has_POLS = true;

            const u8* end = p + length;

            size_t count = 0;

            while (p < end)
            {
                u16 numVertex = p.read16();
                ++count;

                BigEndianConstPointer vertexIndexPointer = p;
                p += numVertex * sizeof(u16);

                s16 surfaceIndex = p.read16();
                if (surfaceIndex < 0)
                {
                    surfaceIndex = -surfaceIndex;

                    u16 numDetailPolygon = p.read16();
                    while (numDetailPolygon-- > 0)
                    {
                        u16 numDetailVertex = p.read16();
                        p += numDetailVertex * sizeof(u16);
                    }
                }
                else
                {
                    if (numVertex < 3)
                    {
                        MANGO_EXCEPTION("[ImportLWO] Incorrect polygon (vertices: {}).", numVertex);
                    }

                    SurfaceLWO& surface = surfaces[surfaceIndex - 1];
                    TriangleLWO triangle;

                    triangle.point[0] = vertexIndexPointer.read16();
                    triangle.point[1] = vertexIndexPointer.read16();

                    for (size_t i = 2; i < numVertex; ++i)
                    {
                        triangle.point[2] = vertexIndexPointer.read16();

                        surface.triangles.push_back(triangle);

                        triangle.point[1] = triangle.point[2];
                    }
                }
            }

            printLine(Print::Verbose, level * 2 + 2, "polygons: {}", count);
        }

        void chunk_CRVS(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);

            if (!has_PNTS || !has_SRFS)
            {
                MANGO_EXCEPTION("[ImportLWO] PNTS and SRFS chunks must come before CRVS chunk.");
            }
        }

        void chunk_SURF(BigEndianConstPointer p, u32 length)
        {
            const u8* end = p + length;

            std::string name = read_string(p);
            printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);

            // NOTE: brute-force search
            for (SurfaceLWO& surface : surfaces)
            {
                if (surface.name == name)
                {
                    currentSurface = &surface;
                    break;
                }
            }

            if (currentSurface)
            {
                while (p < end)
                {
                    parse_subchunks(p);
                }
            }
        }

        void chunk_TAGS(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_LAYR(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_BBOX(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_PTAG(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_VMAP(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_VMAD(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        // subchunks

        void chunk_FLAG(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            u16 flags = p.read16();
            currentSurface->flags = flags;

            if (flags & SurfaceLWO::Smoothing)
            {
                currentSurface->smoothingAngle = 1.57f; // 90 degrees in radians
            }
        }

        void chunk_COLR(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            u8 r = p[0];
            u8 g = p[1];
            u8 b = p[2];
            currentSurface->color = image::Color(r, g, b, 0xff);
        }

        void chunk_LUMI(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentSurface->luminosity = p.read16() / 256.0f;
        }

        void chunk_DIFF(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentSurface->diffuse = p.read16() / 256.0f;
        }

        void chunk_SPEC(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentSurface->specular = p.read16() / 256.0f;
        }

        void chunk_REFL(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentSurface->reflection = p.read16() / 256.0f;
        }

        void chunk_TRAN(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentSurface->transparency = p.read16() / 256.0f;
        }

        void chunk_SMAN(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentSurface->smoothingAngle = p.read32f();
        }

        void chunk_GLOS(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_VDIF(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_VSPC(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        // texture chunks

        void chunk_CTEX(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture = &currentSurface->ctex;
            currentTexture->type = read_texture_type(p);
        }

        void chunk_DTEX(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture = &currentSurface->dtex;
            currentTexture->type = read_texture_type(p);
        }

        void chunk_STEX(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture = &currentSurface->stex;
            currentTexture->type = read_texture_type(p);
        }

        void chunk_RTEX(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture = &currentSurface->rtex;
            currentTexture->type = read_texture_type(p);
        }

        void chunk_TTEX(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture = &currentSurface->ttex;
            currentTexture->type = read_texture_type(p);
        }

        void chunk_BTEX(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture = &currentSurface->btex;
            currentTexture->type = read_texture_type(p);
        }

        // texture parameter chunks

        void chunk_TIMG(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);

            std::string name = read_string(p);
            printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);

            currentTexture->name = name;
        }

        void chunk_TFLG(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->flags = p.read16();
        }

        void chunk_TSIZ(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->size = read_vec3(p);
        }

        void chunk_TCTR(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->center = read_vec3(p);
        }

        void chunk_TFAL(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->falloff = read_vec3(p);
        }

        void chunk_TVEL(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->velocity = read_vec3(p);
        }

        void chunk_TWRP(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
            //ReadBigEndian<uint32>(stream); // wrap
        }

        void chunk_TFP0(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
            //curtexture->tile.x = ReadBigEndian<float>(stream);
        }

        void chunk_TFP1(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
            //curtexture->tile.y = ReadBigEndian<float>(stream);
        }

        void chunk_TAAS(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_TAMP(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->bumpAmplitude = p.read16() / 256.0f;
        }

        void chunk_TIP0(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_TCLR(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            u8 r = p[0];
            u8 g = p[1];
            u8 b = p[2];
            currentTexture->color = image::Color(r, g, b, 0xff);
        }

        void chunk_TVAL(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(length);
            currentTexture->value = p.read16() / 256.0f;
        }

        TextureLWO::Type read_texture_type(BigEndianConstPointer& p)
        {
            std::string name = read_string(p);
            printLine(Print::Verbose, level * 2 + 2, "Type: \"{}\"", name);

            TextureLWO::Type type = TextureLWO::Planar;

            if (name == "Planar Image Map")
            {
                type = TextureLWO::Planar;
            }
            else if (name == "Cylindrical Image Map")
            {
                type = TextureLWO::Cylindrical;
            }
            else if (name == "Spherical Image Map")
            {
                type = TextureLWO::Spherical;
            }
            else if (name == "Cubic Image Map")
            {
                type = TextureLWO::Cubic;
            }
            else if (name == "Front")
            {
                type = TextureLWO::FrontProj;
            }

            return type;
        }
    };

    void import_LWOB(Scene& scene, const filesystem::Path& path, BigEndianConstPointer p, const u8* end)
    {
        ReaderLWO reader(p, end);

        std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
        IndexedMesh& mesh = *ptr;

        u32 materialIndex = 0;

        for (const auto& surface : reader.surfaces)
        {
            Material material;

            material.name = surface.name;

            if (!surface.ctex.name.empty())
            {
                std::string filename = filesystem::removePath(surface.ctex.name);
                try
                {
                    Texture ctex = createTexture(path, filename);
                    material.baseColorTexture = ctex;
                }
                catch(...)
                {
                }
            }

            scene.materials.push_back(material);

            Mesh trimesh;

            trimesh.flags = Vertex::Position | Vertex::Normal | Vertex::Texcoord;

            // triangles sharing positions
            std::multimap<u32, u32> sharing;

            // resolve vertex position sharing between triangles
            for (size_t i = 0; i < surface.triangles.size(); ++i)
            {
                u32 triangleIndex = u32(i);

                // store triangle index into sharing map for all the positions
                sharing.emplace(surface.triangles[i].point[0], triangleIndex);
                sharing.emplace(surface.triangles[i].point[1], triangleIndex);
                sharing.emplace(surface.triangles[i].point[2], triangleIndex);
            }

            float angleLimit = std::cos(surface.smoothingAngle);

            for (size_t i = 0; i < surface.triangles.size(); ++i)
            {
                const TriangleLWO& current = surface.triangles[i];

                Triangle triangle;

                float32x3 p0 = reader.points[current.point[0]];
                float32x3 p1 = reader.points[current.point[1]];
                float32x3 p2 = reader.points[current.point[2]];

                // compute triangle normal
                float32x3 currentNormal = normalize(cross(p0 - p1, p0 - p2));

                // compute vertex normals
                for (int j = 0; j < 3; ++j)
                {
                    Vertex& vertex = triangle.vertex[j];
                    u32 pointIndex = current.point[j];

                    float32x3 normal = currentNormal;

                    // lookup all triangles sharing the position
                    auto share = sharing.equal_range(pointIndex);

                    for (auto s = share.first; s != share.second; ++s)
                    {
                        u32 triangleIndex = s->second;
                        if (triangleIndex == i)
                        {
                            // we already accumulated current triangle normal
                            continue;
                        }

                        const TriangleLWO& shared = surface.triangles[triangleIndex];

                        float32x3 s0 = reader.points[shared.point[0]];
                        float32x3 s1 = reader.points[shared.point[1]];
                        float32x3 s2 = reader.points[shared.point[2]];
                        float32x3 sharedNormal = normalize(cross(s0 - s1, s0 - s2));

                        float angle = dot(currentNormal, sharedNormal);
                        if (angle >= angleLimit)
                        {
                            normal += sharedNormal;
                        }
                    }

                    vertex.normal = normalize(normal);
                }

                triangle.vertex[0].position = p0;
                triangle.vertex[1].position = p1;
                triangle.vertex[2].position = p2;

                triangle.vertex[0].texcoord = surface.ctex.texcoord(p0);
                triangle.vertex[1].texcoord = surface.ctex.texcoord(p1);
                triangle.vertex[2].texcoord = surface.ctex.texcoord(p2);

                trimesh.triangles.push_back(triangle);
            }

            mesh.append(trimesh, materialIndex++);
        }

        scene.meshes.push_back(std::move(ptr));

        // nodes

        Node node;

        node.name = "lightwave.object";
        node.transform = matrix4x4(1.0f);
        node.mesh = 0;

        scene.nodes.push_back(node);
        scene.roots.push_back(0);
    }

    // --------------------------------------------------------------------------
    // LightWave v6
    // --------------------------------------------------------------------------

    struct TextureLWO2
    {
        std::string filename;
        bool enable = false;
    };

    /*
    enum
    {
        LWO2_TEXTURE_COLOR,
        LWO2_TEXTURE_DIFFUSE,
        LWO2_TEXTURE_LUMINOSITY,
        LWO2_TEXTURE_SPECULAR,
        LWO2_TEXTURE_REFLECTION,
        LWO2_TEXTURE_TRANSPARENCY,
        LWO2_TEXTURE_TRANSLUCENCY,
        LWO2_TEXTURE_GLOSSINESS,
        LWO2_TEXTURE_SHARPNESS,
        LWO2_TEXTURE_BUMP,
    };
    */

    struct SurfaceLWO2
    {
        std::string name;

        float32x4 color { 0.7f, 0.7f, 0.7f, 1.0f };

        float diffuse = 1.0f;
        float luminosity = 0.0f;
        float specular = 0.0f;
        float reflection = 0.0f;
        float transparency = 0.0f;
        float translucency = 0.0f;

        float glossiness = 0.2f;
        float sharpness = 0.5f;
        float bump = 1.0f;

        bool twosided = false;
        float smoothingAngle = 0.0f;

        //TextureLWO2 texture[LWO2_TEXTURE_COUNT];
    };

    template <typename T>
    struct IndexRange
    {
        T start;
        T count;
    };

    struct PolygonLWO2
    {
        IndexRange<u32> range;
        u32 surface = 0;
        u32 part = 0;
        u32 smoothing = 0;
    };

    struct LayerLWO2
    {
        std::vector<float32x3> points;
        std::vector<float32x2> texcoords;
        std::vector<float32x4> colors;

        std::vector<u32> pointIndices;
        std::vector<u32> texcoordIndices;
        std::vector<u32> colorIndices;

        std::vector<PolygonLWO2> polygons;
    };

    struct ReaderLWO2
    {
        std::vector<LayerLWO2> layers;
        LayerLWO2* currentLayer = nullptr;

        std::vector<SurfaceLWO2> surfaces;
        SurfaceLWO2* currentSurface = nullptr;

        int level = 0;

        ReaderLWO2(BigEndianConstPointer p, const u8* end)
        {
            while (p < end)
            {
                parse_chunks(p);
            }
        }

        void parse_chunks(BigEndianConstPointer& p)
        {
            u32 id = p.read32();
            u32 length = p.read32();

            ++level;

            char c[4];
            std::memcpy(c, &id, 4);
            printLine(Print::Verbose, level * 2, "[{}{}{}{}] {} bytes", c[3], c[2], c[1], c[0], length);

            switch (id)
            {
                case u32_mask_rev('T','A','G','S'): chunk_TAGS(p, length); break;
                case u32_mask_rev('L','A','Y','R'): chunk_LAYR(p, length); break;
                case u32_mask_rev('P','N','T','S'): chunk_PNTS(p, length); break;
                case u32_mask_rev('B','B','O','X'): chunk_BBOX(p, length); break;
                case u32_mask_rev('P','O','L','S'): chunk_POLS(p, length); break;
                case u32_mask_rev('P','T','A','G'): chunk_PTAG(p, length); break;
                case u32_mask_rev('V','M','A','P'): chunk_VMAP(p, length); break;
                case u32_mask_rev('V','M','A','D'): chunk_VMAD(p, length); break;
                case u32_mask_rev('E','N','V','L'): chunk_ENVL(p, length); break;
                case u32_mask_rev('C','L','I','P'): chunk_CLIP(p, length); break;
                case u32_mask_rev('S','U','R','F'): chunk_SURF(p, length); break;

                default:
                    printLine(Print::Verbose, level * 2 + 2, "NOT SUPPORTED");
                    break;
            };

            p += length + (length & 1);
            --level;
        }

        void parse_subchunks(BigEndianConstPointer& p)
        {
            u32 id = p.read32();
            u32 length = p.read16();

            ++level;

#if 1
            char c[4];
            std::memcpy(c, &id, 4);
            printLine(Print::Verbose, level * 2, "[{}{}{}{}] {} bytes", c[3], c[2], c[1], c[0], length);
#endif

            switch (id)
            {
                case u32_mask_rev('S','T','I','L'): chunk_STIL(p, length); break;
                case u32_mask_rev('C','O','L','R'): chunk_COLR(p, length); break;
                case u32_mask_rev('D','I','F','F'): chunk_DIFF(p, length); break;
                case u32_mask_rev('L','U','M','I'): chunk_LUMI(p, length); break;
                case u32_mask_rev('S','P','E','C'): chunk_SPEC(p, length); break;
                case u32_mask_rev('R','E','F','L'): chunk_REFL(p, length); break;
                case u32_mask_rev('T','R','A','N'): chunk_TRAN(p, length); break;
                case u32_mask_rev('T','R','N','L'): chunk_TRNL(p, length); break;
                case u32_mask_rev('G','L','O','S'): chunk_GLOS(p, length); break;
                case u32_mask_rev('S','H','R','P'): chunk_SHRP(p, length); break;
                case u32_mask_rev('B','U','M','P'): chunk_BUMP(p, length); break;
                case u32_mask_rev('S','I','D','E'): chunk_SIDE(p, length); break;
                case u32_mask_rev('B','L','O','K'): chunk_BLOK(p, length); break;
                case u32_mask_rev('I','M','A','P'): chunk_IMAP(p, length); break;
                case u32_mask_rev('T','M','A','P'): chunk_TMAP(p, length); break;
                case u32_mask_rev('I','M','A','G'): chunk_IMAG(p, length); break;
                case u32_mask_rev('C','H','A','N'): chunk_CHAN(p, length); break;
                case u32_mask_rev('S','M','A','N'): chunk_SMAN(p, length); break;

                case u32_mask_rev('R','I','N','D'):
                case u32_mask_rev('R','B','L','R'):
                case u32_mask_rev('C','L','R','F'):
                case u32_mask_rev('N','O','R','M'):
                case u32_mask_rev('V','E','R','S'):
                case u32_mask_rev('N','O','D','S'):
                    break;

                default:
                    printLine(Print::Verbose, level * 2 + 2, "NOT SUPPORTED");
                    break;
            }

            p += length + (length & 1);
            --level;
        }

        // chunks

        void chunk_TAGS(BigEndianConstPointer p, u32 length)
        {
            const u8* end = p + length;
            while (p < end)
            {
                std::string name = read_string(p);
                printLine(Print::Verbose, level * 2 + 2, "name; \"{}\"", name);
            }
        }

        void chunk_LAYR(BigEndianConstPointer p, u32 length)
        {
            const u8* end = p + length;

            u16 number = p.read16();
            u16 flags = p.read16();
            float32x3 pivot = read_vec3(p);
            std::string name = read_string(p);

            u16 parent = 0xffff;

            if (end - p >= 2)
            {
                parent = p.read16();
            }

            // start a new layer
            LayerLWO2 layer;
            layers.push_back(layer);
            currentLayer = &layers.back();

            printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);

            // TODO
            MANGO_UNREFERENCED(number);
            MANGO_UNREFERENCED(flags);
            MANGO_UNREFERENCED(pivot);
            MANGO_UNREFERENCED(parent);
        }

        void chunk_PNTS(BigEndianConstPointer p, u32 length)
        {
            if (!currentLayer)
            {
                // TODO: error
                return;
            }

            size_t count = length / sizeof(float32x3);
            printLine(Print::Verbose, level * 2 + 2, "points: {}", count);

            currentLayer->points.resize(count);
            currentLayer->texcoords.resize(count);
            currentLayer->colors.resize(count);

            for (size_t i = 0; i < count; ++i)
            {
                currentLayer->points[i] = read_vec3(p);
                currentLayer->texcoords[i] = float32x2(0.0f, 0.0f);
                currentLayer->colors[i] = float32x4(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        void chunk_BBOX(BigEndianConstPointer p, u32 length)
        {
            float32x3 p0 = read_vec3(p);
            float32x3 p1 = read_vec3(p);
            printLine(Print::Verbose, level * 2 + 2, "min: [{}, {}, {}]", p0.x, p0.y, p0.z);
            printLine(Print::Verbose, level * 2 + 2, "max: [{}, {}, {}]", p1.x, p1.y, p1.z);
            MANGO_UNREFERENCED(length);
        }

        void chunk_POLS(BigEndianConstPointer p, u32 length)
        {
            if (!currentLayer)
            {
                // TODO: error
                return;
            }

            const u8* end = p + length;

            u32 type = p.read32();

            printLine(Print::Verbose, level * 2 + 2, "type: {}{}{}{}",
                char((type >> 24) & 0xff),
                char((type >> 16) & 0xff),
                char((type >>  8) & 0xff),
                char((type >>  0) & 0xff));

            switch (type)
            {
                case u32_mask_rev('F','A','C','E'):
                {
                    // polygons
                    while (p < end)
                    {
                        u16 value = p.read16();
                        u16 count = value & 0x03ff;
                        //u16 flags = count >> 10;

                        PolygonLWO2 polygon;
                        polygon.range.start = u32(currentLayer->pointIndices.size());
                        polygon.range.count = count;

                        currentLayer->polygons.push_back(polygon);

                        for (size_t i = 0; i < count; ++i)
                        {
                            u32 index = read_index(p);
                            currentLayer->pointIndices.push_back(index);
                            currentLayer->texcoordIndices.push_back(index);
                            currentLayer->colorIndices.push_back(index);
                        }
                    }

                    break;
                }

                case u32_mask_rev('C','U','R','V'):
                    // catmull-rom splines
                    break;

                case u32_mask_rev('P','T','C','H'):
                    // subdivision patches
                    break;

                case u32_mask_rev('M','B','A','L'):
                    // metaballs
                    break;

                case u32_mask_rev('B','O','N','E'):
                    // skinning bones
                    break;
            }
        }

        void chunk_PTAG(BigEndianConstPointer p, u32 length)
        {
            if (!currentLayer)
            {
                // TODO: error
                return;
            }

            const u8* end = p + length;

            u32 type = p.read32();

            printLine(Print::Verbose, level * 2 + 2, "type: {}{}{}{}",
                char((type >> 24) & 0xff),
                char((type >> 16) & 0xff),
                char((type >>  8) & 0xff),
                char((type >>  0) & 0xff));

            while (p < end)
            {
                u32 index = read_index(p);
                u16 tag = p.read16();

                PolygonLWO2& polygon = currentLayer->polygons[index];

                switch (type)
                {
                    case u32_mask_rev('S','U','R','F'):
                        polygon.surface = tag;
                        break;

                    case u32_mask_rev('P','A','R','T'):
                        polygon.part = tag;
                        break;

                    case u32_mask_rev('S','M','G','P'):
                        polygon.smoothing = tag;
                        break;
                }
            }
        }

        void chunk_VMAP(BigEndianConstPointer p, u32 length)
        {
            if (!currentLayer)
            {
                // TODO: error
                return;
            }

            const u8* end = p + length;

            u32 type = p.read32();
            u16 dimension = p.read16();
            std::string name = read_string(p);

            printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);
            printLine(Print::Verbose, level * 2 + 2, "type: {}{}{}{}",
                char((type >> 24) & 0xff),
                char((type >> 16) & 0xff),
                char((type >>  8) & 0xff),
                char((type >>  0) & 0xff));
            printLine(Print::Verbose, level * 2 + 2, "dimension: {}", dimension);

            switch (type)
            {
                case u32_mask_rev('T','X','U','V'):
                    if (dimension == 2)
                    {
                        while (p < end)
                        {
                            u32 index = read_index(p);
                            float32x2& dest = currentLayer->texcoords[index];

                            for (u16 i = 0; i < dimension; ++i)
                            {
                                dest[i] = p.read32f();
                            }
                        }
                    }
                    break;

                case u32_mask_rev('R','G','B',' '): // <-- TODO: verify (need .lwo file that uses this)
                case u32_mask_rev('R','G','B','A'):
                    if (dimension == 3 || dimension == 4)
                    {
                        while (p < end)
                        {
                            u32 index = read_index(p);
                            float32x4& dest = currentLayer->colors[index];

                            for (u16 i = 0; i < dimension; ++i)
                            {
                                dest[i] = p.read32f();
                            }
                        }
                    }
                    break;

                case u32_mask_rev('P','I','C','K'):
                case u32_mask_rev('W','G','H','T'):
                case u32_mask_rev('M','N','V','W'):
                case u32_mask_rev('M','O','R','F'):
                case u32_mask_rev('S','P','O','T'):
                default:
                    return;
            }
        }

        void chunk_VMAD(BigEndianConstPointer p, u32 length)
        {
            if (!currentLayer)
            {
                // TODO: error
                return;
            }

            const u8* end = p + length;

            u32 type = p.read32();
            u16 dimension = p.read16();
            std::string name = read_string(p);

            printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);
            printLine(Print::Verbose, level * 2 + 2, "type: {}{}{}{}",
                char((type >> 24) & 0xff),
                char((type >> 16) & 0xff),
                char((type >>  8) & 0xff),
                char((type >>  0) & 0xff));
            printLine(Print::Verbose, level * 2 + 2, "dimension: {}", dimension);

            switch (type)
            {
                case u32_mask_rev('T','X','U','V'):
                    if (dimension == 2)
                    {
                        while (p < end)
                        {
                            u32 vertexIndex = read_index(p);
                            u32 polygonIndex = read_index(p);

                            const PolygonLWO2& polygon = currentLayer->polygons[polygonIndex];
                            float32x2 texcoord;// = currentLayer->texcoords[vertexIndex];

                            for (u16 i = 0; i < dimension; ++i)
                            {
                                texcoord[i] = p.read32f();
                            }

                            for (u32 i = 0; i < polygon.range.count; ++i)
                            {
                                u32 idx = i + polygon.range.start;
                                if (currentLayer->pointIndices[idx] == vertexIndex)
                                {
                                    currentLayer->texcoordIndices[idx] = u32(currentLayer->texcoords.size());
                                    currentLayer->texcoords.push_back(texcoord);
                                    break;
                                }
                            }

                        }
                    }
                    break;

                case u32_mask_rev('R','G','B',' '): // <-- TODO: verify
                case u32_mask_rev('R','G','B','A'):
                case u32_mask_rev('P','I','C','K'):
                case u32_mask_rev('W','G','H','T'):
                case u32_mask_rev('M','N','V','W'):
                case u32_mask_rev('M','O','R','F'):
                case u32_mask_rev('S','P','O','T'):
                default:
                    return;
            }
        }

        void chunk_SURF(BigEndianConstPointer p, u32 length)
        {
            const u8* end = p + length;

            std::string name = read_string(p);
            std::string source = read_string(p);

            printLine(Print::Verbose, level * 2 + 2, "name: \"{}\"", name);
            printLine(Print::Verbose, level * 2 + 2, "source: \"{}\"", name);

            // start a new surface
            SurfaceLWO2 surface;

            // TODO: copy source surface

            surfaces.push_back(surface);
            currentSurface = &surfaces.back();

            while (p < end)
            {
                parse_subchunks(p);
            }
        }

        void chunk_CLIP(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_ENVL(BigEndianConstPointer p, u32 length)
        {
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        // subchunks

        void chunk_COLR(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 14)
                return;

            currentSurface->color[0] = p.read32f();
            currentSurface->color[1] = p.read32f();
            currentSurface->color[2] = p.read32f();
            currentSurface->color[3] = 1.0f;

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_DIFF(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->diffuse = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_LUMI(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->luminosity = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_SPEC(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->specular = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_REFL(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->reflection = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_TRAN(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->transparency = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_TRNL(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->translucency = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_GLOS(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->glossiness = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_SHRP(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->sharpness = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_BUMP(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 6)
                return;

            currentSurface->bump = p.read32f();

            u16 envelope = p.read16();
            MANGO_UNREFERENCED(envelope);
        }

        void chunk_STIL(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_SIDE(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 2)
                return;

            u16 sidedness = p.read16();
            currentSurface->twosided = (sidedness == 3);
        }

        void chunk_BLOK(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_IMAP(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_TMAP(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_IMAG(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_CHAN(BigEndianConstPointer p, u32 length)
        {
            // TODO
            MANGO_UNREFERENCED(p);
            MANGO_UNREFERENCED(length);
        }

        void chunk_SMAN(BigEndianConstPointer p, u32 length)
        {
            if (!currentSurface || length != 4)
                return;

            currentSurface->smoothingAngle = p.read32f();
        }

        /*
        void chunk_STIL(BigEndianConstPointer p, u32 length)
        {
            ClipLW6& clip = GetCurrentClip();

            std::string filename = read_string(p);
            clip.filename = GetFilenameNOPATH(filename);
        }

        void chunk_BLOK(BigEndianConstPointer p, u32 length)
        {
            // subchunks
            while (length > 0)
            {
                length -= ReadSubChunk();
            }
        }

        void chunk_IMAP(BigEndianConstPointer p, u32 length)
        {
            std::string ordinal = read_string(p);

            // subchunks
            while (length > 0)
            {
                length -= ReadSubChunk();
            }
        }

        void chunk_TMAP(BigEndianConstPointer p, u32 length)
        {
            // subchunks
            while (length > 0)
            {
                length -= ReadSubChunk();
            }
        }

        void chunk_IMAG(BigEndianConstPointer p, u32 length)
        {
            u32 index = read_index(p);

            ClipLW6* clip = FindClip(index);
            if (clip)
            {
                SurfaceLWO2& surface = GetCurrentSurface();
                TextureLWO2& texture = surface.texture[texchannel];

                if (clip->filename.GetLength() > 0)
                {
                    texture.filename = clip->filename;
                    texture.enable = true;
                }
            }
        }

        void chunk_CHAN(BigEndianConstPointer p, u32 length)
        {
            u32 chan = p.read32();
            switch (chan)
            {
                case u32_mask_rev('C','O','L','R'): texchannel = LWO2_TEXTURE_COLOR; break;
                case u32_mask_rev('D','I','F','F'): texchannel = LWO2_TEXTURE_DIFFUSE; break;
                case u32_mask_rev('L','U','M','I'): texchannel = LWO2_TEXTURE_LUMINOSITY; break;
                case u32_mask_rev('S','P','E','C'): texchannel = LWO2_TEXTURE_SPECULAR; break;
                case u32_mask_rev('G','L','O','S'): texchannel = LWO2_TEXTURE_REFLECTION; break;
                case u32_mask_rev('R','E','F','L'): texchannel = LWO2_TEXTURE_TRANSPARENCY; break;
                case u32_mask_rev('T','R','A','N'): texchannel = LWO2_TEXTURE_TRANSLUCENCY; break;
                case u32_mask_rev('R','I','N','D'): texchannel = LWO2_TEXTURE_GLOSSINESS; break;
                case u32_mask_rev('T','R','N','L'): texchannel = LWO2_TEXTURE_SHARPNESS; break;
                case u32_mask_rev('B','U','M','P'): texchannel = LWO2_TEXTURE_BUMP; break;
            }
        }
        */
    };

    void import_LWO2(Scene& scene, const filesystem::Path& path, BigEndianConstPointer p, const u8* end)
    {
        ReaderLWO2 reader(p, end);

        std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
        IndexedMesh& mesh = *ptr;

        Material material;
        //material.baseColorTexture = createTexture(filesystem::Path("./"), "texture.jpg"); // HACK
        scene.materials.push_back(material);

        for (const LayerLWO2& layer : reader.layers)
        {
            Mesh trimesh;
            trimesh.flags = Vertex::Position | Vertex::Normal | Vertex::Texcoord;

            for (const PolygonLWO2& polygon : layer.polygons)
            {
                if (polygon.range.count < 3)
                {
                    continue;
                }

                size_t count = polygon.range.count - 2;
                const u32* pointIndices = &layer.pointIndices[polygon.range.start];
                const u32* texcoordIndices = &layer.texcoordIndices[polygon.range.start];

                for (size_t i = 0; i < count; ++i)
                {
                    float32x3 position0 = layer.points[pointIndices[0]];
                    float32x3 position1 = layer.points[pointIndices[i + 1]];
                    float32x3 position2 = layer.points[pointIndices[i + 2]];

                    float32x3 normal = normalize(cross(position0 - position1, position0 - position2));

                    Triangle triangle;

                    triangle.vertex[0].position = position0;
                    triangle.vertex[1].position = position1;
                    triangle.vertex[2].position = position2;

                    triangle.vertex[0].normal = normal;
                    triangle.vertex[1].normal = normal;
                    triangle.vertex[2].normal = normal;

                    triangle.vertex[0].texcoord = layer.texcoords[texcoordIndices[0]];
                    triangle.vertex[1].texcoord = layer.texcoords[texcoordIndices[i + 1]];
                    triangle.vertex[2].texcoord = layer.texcoords[texcoordIndices[i + 2]];

                    trimesh.triangles.push_back(triangle);
                }
            }

            mesh.append(trimesh, 0);
        }

        scene.meshes.push_back(std::move(ptr));

        // nodes

        Node node;

        node.name = "lightwave.object";
        node.transform = matrix4x4(1.0f);
        node.mesh = 0;

        scene.nodes.push_back(node);
        scene.roots.push_back(0);

        // TODO
        MANGO_UNREFERENCED(path);
    }

    // --------------------------------------------------------------------------
    // import
    // --------------------------------------------------------------------------

    ImportLWO::ImportLWO(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        ConstMemory memory = file;

        BigEndianConstPointer p = memory;

        u32 magic = p.read32();
        if (magic != u32_mask_rev('F', 'O', 'R', 'M'))
        {
            MANGO_EXCEPTION("[ImportLWO] FORM chunk must be first.");
        }

        u32 length = p.read32();
        u32 id = p.read32();

        if (length > memory.size - 8)
        {
            MANGO_EXCEPTION("[ImportLWO] IFF length is larger than the file.");
        }

        switch (id)
        {
            case u32_mask_rev('L', 'W', 'O', 'B'):
                import_LWOB(*this, path, p, memory.end());
                break;

            case u32_mask_rev('L', 'W', 'O', '2'):
                import_LWO2(*this, path, p, memory.end());
                break;

            default:
            {
                char c[4];
                std::memcpy(c, &magic, 4);
                MANGO_EXCEPTION("[ImportLWO] Incorrect FORM identifier: \"{}{}{}{}\".", c[3], c[2], c[1], c[0]);
                break;
            }
        }
    }

} // namespace mango::import3d
