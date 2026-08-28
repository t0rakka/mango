/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <charconv>
#include <string_view>
#include <map>
#include <array>
#include <cfloat>
#include <mango/core/core.hpp>
#include <mango/import3d/import_obj.hpp>
#include "../../external/fast_float/fast_float.h"
#include "../../external/earcut/earcut.hpp"

/*
    Wavefront OBJ importer

    https://en.wikipedia.org/wiki/Wavefront_.obj_file
*/

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    // TU-local parsers: keep fast_float in this translation unit for speed/inlining.

    static inline
    float parseFloat(std::string_view s)
    {
        float value = 0.0f;
        fast_float::from_chars(s.data(), s.data() + s.size(), value);
        return value;
    }

    static inline
    int parseInt(std::string_view s)
    {
        int value = 0;
        std::from_chars(s.data(), s.data() + s.size(), value);
        return value;
    }

    // Parse "pos", "pos/tex", "pos//nrm", "pos/tex/nrm" within view bounds.
    static inline
    void parseFaceCorner(std::string_view s, int value[3])
    {
        value[0] = 0;
        value[1] = 0;
        value[2] = 0;

        size_t pos = 0;

        for (size_t index = 0; index < 3; ++index)
        {
            if (pos >= s.size())
                break;

            if (s[pos] == '/')
            {
                ++pos;
                continue;
            }

            int v = 0;
            const char* begin = s.data() + pos;
            const char* end = s.data() + s.size();
            const auto result = std::from_chars(begin, end, v);

            if (result.ptr == begin)
                break;

            value[index] = v;
            pos = size_t(result.ptr - s.data());

            if (pos < s.size() && s[pos] == '/')
                ++pos;
        }
    }

    static float polygonArea2D(const float* x, const float* y, size_t count)
    {
        float area = 0.0f;

        for (size_t i = 1; i < count; ++i)
        {
            const float avg_height = (y[i - 1] + y[i]) * 0.5f;
            const float width = x[i] - x[i - 1];
            area += width * avg_height;
        }

        const float avg_height = (y[0] + y[count - 1]) * 0.5f;
        const float width = x[0] - x[count - 1];
        area += width * avg_height;

        return std::abs(area);
    }

    enum class ProjectionPlane
    {
        X,
        Y,
        Z,
    };

    static std::vector<uint32_t> triangulatePolygon(
        const std::vector<float32x3>& positions,
        const s32* positionIndices,
        size_t count)
    {
        std::array<float, 128> xs;
        std::array<float, 128> ys;
        std::array<float, 128> zs;

        for (size_t i = 0; i < count; ++i)
        {
            const float32x3& p = positions[positionIndices[i] - 1];
            xs[i] = p.x;
            ys[i] = p.y;
            zs[i] = p.z;
        }

        const float area_x = polygonArea2D(ys.data(), zs.data(), count);
        const float area_y = polygonArea2D(xs.data(), zs.data(), count);
        const float area_z = polygonArea2D(xs.data(), ys.data(), count);

        if (std::max({ area_x, area_y, area_z }) < FLT_MIN)
            return {};

        ProjectionPlane plane;

        if (area_x > area_y)
            plane = area_x > area_z ? ProjectionPlane::X : ProjectionPlane::Z;
        else
            plane = area_y > area_z ? ProjectionPlane::Y : ProjectionPlane::Z;

        std::vector<std::array<float, 2>> polygon;
        polygon.reserve(count);

        switch (plane)
        {
            case ProjectionPlane::X:
                for (size_t i = 0; i < count; ++i)
                    polygon.push_back({ ys[i], zs[i] });
                break;

            case ProjectionPlane::Y:
                for (size_t i = 0; i < count; ++i)
                    polygon.push_back({ xs[i], zs[i] });
                break;

            case ProjectionPlane::Z:
                for (size_t i = 0; i < count; ++i)
                    polygon.push_back({ xs[i], ys[i] });
                break;
        }

        std::array<std::vector<std::array<float, 2>>, 1> complex;
        complex[0] = std::move(polygon);

        std::vector<uint32_t> result = mapbox::earcut(complex);
        if (result.empty() || result.size() % 3 != 0)
            return {};

        for (size_t k = 0; k < result.size(); k += 3)
            std::swap(result[k], result[k + 1]);

        return result;
    }

} // namespace

namespace mango::import3d
{

    struct VertexOBJ
    {
        u32 position;
        u32 texcoord;
        u32 normal;
    };

    static inline
    bool operator == (const VertexOBJ& a, const VertexOBJ& b)
    {
        return std::memcmp(&a, &b, sizeof(VertexOBJ)) == 0;
    }

    struct CornerKey
    {
        VertexOBJ vertex;
        u32 smoothing_group = 0;
    };

    static inline
    bool operator == (const CornerKey& a, const CornerKey& b)
    {
        return a.smoothing_group == b.smoothing_group && a.vertex == b.vertex;
    }

    struct CornerHash
    {
        std::size_t operator () (const CornerKey& k) const
        {
            const VertexOBJ& v = k.vertex;
            return v.position ^ (v.texcoord << 10) ^ (v.normal << 20) ^ (k.smoothing_group << 30);
        }
    };

    struct FaceOBJ
    {
        VertexOBJ vertex[3];
        u32 smoothing_group = 0;
        u32 material = 0;
    };

    struct GroupOBJ
    {
        std::string name;
        std::vector<FaceOBJ> faces;
    };

    struct MaterialOBJ
    {
        std::string name;

        float ns = 0.0f; // specular exponent
        float ni = 1.0f; // optical density / refraction index
        float tr = 1.0f; // transparency (1.0 = opaque)
        float tf = 1.0f; // transmission filter (for transparency)

        u32 illum = 2; // illumination model

        float32x3 ka { 0.0f, 0.0f, 0.0f }; // ambient color
        float32x3 kd { 1.0f, 1.0f, 1.0f }; // diffuse color
        float32x3 ks { 0.0f, 0.0f, 0.0f }; // specular color
        float32x3 ke { 0.0f, 0.0f, 0.0f }; // emissive color

        float pr = 0.5f; // roughness (Exocortex PBR)
        float pm = 0.0f; // metallic
        bool has_pr = false;
        bool has_pm = false;
        bool has_d = false; // dissolve: Tr ignored when d is present

        // Exocortex PBR extension — http://exocortex.com/blog/extending_wavefront_mtl_to_support_pbr
        // pr/pm/maps above; sheen / clearcoat / anisotropy below.
        float ps = 0.0f;    // sheen strength
        float pc = 0.0f;    // clearcoat strength
        float pcr = 0.0f;   // clearcoat roughness
        float aniso = 0.0f;
        float anisor = 0.0f;
        bool has_ps = false;
        bool has_pc = false;
        bool has_pcr = false;
        bool has_aniso = false;
        bool has_anisor = false;

        std::string map_ka;    // ambient texture
        std::string map_kd;    // diffuse texture
        std::string map_ks;    // specular texture
        std::string map_ke;    // emissive texture
        std::string map_bump;  // normal texture
        std::string map_ns;    // specular exponent texture
        std::string map_d;     // alpha texture
        std::string map_disp;  // displacement texture
        std::string map_decal; // stencil texture
        std::string map_refl;  // reflection texture
        std::string map_pr;    // roughness texture
        std::string map_pm;    // metallic texture
        std::string map_norm;  // normal map (PBR; preferred over bump when present)
        std::string map_ps;    // sheen texture (Exocortex)
    };

    struct ObjectOBJ
    {
        std::string name;
        std::vector<GroupOBJ> groups;
    };

    struct ReaderOBJ
    {
        const filesystem::Path& m_path;

        std::vector<float32x3> positions;
        std::vector<float32x4> colors;
        std::vector<float32x3> normals;
        std::vector<float32x2> texcoords;

        bool has_vertex_colors = false;

        std::vector<ObjectOBJ> m_objects;
        std::vector<MaterialOBJ> m_materials;

        MaterialOBJ* m_current_material = nullptr;
        u32 m_face_material = 0;
        u32 m_smoothing_group = 0;

        ReaderOBJ(const filesystem::Path& path, const std::string& filename);

        void parse_mtl(const std::string_view& s);

        void parse_v(const std::string_view* tokens, size_t count);
        void parse_vn(const std::string_view* tokens, size_t count);
        void parse_vt(const std::string_view* tokens, size_t count);
        void parse_mtllib(const std::string_view* tokens, size_t count);
        void parse_usemtl(const std::string_view* tokens, size_t count);
        void parse_o(const std::string_view* tokens, size_t count);
        void parse_g(const std::string_view* tokens, size_t count);
        void parse_s(const std::string_view* tokens, size_t count);
        void parse_f(const std::string_view* tokens, size_t count);

        ObjectOBJ& getCurrentObject()
        {
            if (m_objects.empty())
            {
                ObjectOBJ object;
                object.name = "default";
                m_objects.push_back(object);
            }

            return m_objects.back();
        }

        GroupOBJ& getCurrentGroup()
        {
            ObjectOBJ& object = getCurrentObject();
            if (object.groups.empty())
            {
                GroupOBJ group;
                group.name = "default";
                object.groups.push_back(group);
            }

            return object.groups.back();
        }

        std::string map_filename(const std::string_view* tokens, size_t count) const
        {
            // skip parameters
            size_t index = count - 1;

            std::string filename(tokens[index]);
            replace(filename, "\\", "/");
            return filename;
        }

        float parse_float(const std::string_view* tokens, size_t count) const
        {
            if (count != 1)
            {
                // error
            }

            float value = parseFloat(tokens[0]);
            return value;
        }

        float32x3 parse_float32x3(const std::string_view* tokens, size_t count) const
        {
            if (count != 3)
            {
                // error
            }

            float value[3];

            for (size_t i = 0; i < count; ++i)
            {
                value[i] = parseFloat(tokens[i]);
            }

            return float32x3(value[0], value[1], value[2]);
        }
    };

    ReaderOBJ::ReaderOBJ(const filesystem::Path& path, const std::string& filename)
        : m_path(path)
    {
        filesystem::File file(path, filename);
        std::string_view s(reinterpret_cast<const char *>(file.data()), file.size());

        std::vector<std::string_view> tokens;

        size_t first = 0;

        while (first < s.size())
        {
            size_t second = s.find_first_of(" \t\n\r", first);

            if (first < second)
            {
                tokens.emplace_back(s.data() + first, second - first);

                char s0 = s[second + 0];
                char s1 = s[second + 1];

                if (s0 == '\n' || s0 == '\r' || s1 == '\n' || s1 == '\r')
                {
                    /*
                    printf("%s", std::string(id).c_str());
                    for (size_t i = 0; i < tokens.size(); ++i)
                    {
                        printf(" %s", std::string(tokens[i]).c_str());
                    }
                    printf("\n");
                    */

                    const std::string_view& id = tokens[0];
                    const std::string_view* data = tokens.data() + 1;
                    size_t count = tokens.size() - 1;

                    if (id == "#")
                    {
                        // comment
                    }
                    else if (id == "v")
                    {
                        parse_v(data, count);
                    }
                    else if (id == "vn")
                    {
                        parse_vn(data, count);
                    }
                    else if (id == "vt")
                    {
                        parse_vt(data, count);
                    }
                    else if (id == "mtllib")
                    {
                        parse_mtllib(data, count);
                    }
                    else if (id == "usemtl")
                    {
                        parse_usemtl(data, count);
                    }
                    else if (id == "o")
                    {
                        parse_o(data, count);
                    }
                    else if (id == "g")
                    {
                        parse_g(data, count);
                    }
                    else if (id == "s")
                    {
                        parse_s(data, count);
                    }
                    else if (id == "f")
                    {
                        parse_f(data, count);
                    }

                    tokens.clear();
                }
            }

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }
    }

    void ReaderOBJ::parse_mtl(const std::string_view& s)
    {
        std::vector<std::string_view> tokens;

        size_t first = 0;

        while (first < s.size())
        {
            size_t second = s.find_first_of(" \t\n\r", first);

            if (first != second)
            {
                tokens.emplace_back(s.data() + first, second - first);

                char s0 = s[second + 0];
                char s1 = s[second + 1];

                if (s0 == '\n' || s0 == '\r' || s1 == '\n' || s1 == '\r')
                {
                    const std::string_view& id = tokens[0];
                    const std::string_view* data = tokens.data() + 1;
                    size_t count = tokens.size() - 1;

                    if (id == "newmtl")
                    {
                        MaterialOBJ material;
                        material.name = std::string(data[0]);

                        m_materials.push_back(material);
                        m_current_material = &m_materials.back();
                    }
                    else if (m_current_material)
                    {
                        if (id == "#")
                        {
                            // ignore comment
                        }
                        else if (id == "Ns")
                        {
                            m_current_material->ns = parse_float(data, count);
                        }
                        else if (id == "Ni")
                        {
                            m_current_material->ni = parse_float(data, count);
                        }
                        else if (id == "d")
                        {
                            m_current_material->tr = parse_float(data, count);
                            m_current_material->has_d = true;
                        }
                        else if (id == "Tr")
                        {
                            if (!m_current_material->has_d)
                                m_current_material->tr = 1.0f - parse_float(data, count);
                        }
                        else if (id == "Tf")
                        {
                            m_current_material->tf = parse_float(data, count);
                        }
                        else if (id == "illum")
                        {
                            m_current_material->illum = u32(parse_float(data, count));
                        }
                        else if (id == "Ka")
                        {
                            m_current_material->ka = parse_float32x3(data, count);
                        }
                        else if (id == "Kd")
                        {
                            m_current_material->kd = parse_float32x3(data, count);
                        }
                        else if (id == "Ks")
                        {
                            m_current_material->ks = parse_float32x3(data, count);
                        }
                        else if (id == "Ke")
                        {
                            m_current_material->ke = parse_float32x3(data, count);
                        }
                        else if (id == "map_Ka")
                        {
                            m_current_material->map_ka = map_filename(data, count);
                        }
                        else if (id == "map_Kd")
                        {
                            m_current_material->map_kd = map_filename(data, count);
                        }
                        else if (id == "map_Ks")
                        {
                            m_current_material->map_ks = map_filename(data, count);
                        }
                        else if (id == "map_Ke")
                        {
                            m_current_material->map_ke = map_filename(data, count);
                        }
                        else if (id == "map_bump" || id == "map_Bump"|| id == "bump")
                        {
                            m_current_material->map_bump = map_filename(data, count);
                        }
                        else if (id == "map_Ns")
                        {
                            m_current_material->map_ns = map_filename(data, count);
                        }
                        else if (id == "map_d")
                        {
                            m_current_material->map_d = map_filename(data, count);
                        }
                        else if (id == "disp")
                        {
                            m_current_material->map_disp = map_filename(data, count);
                        }
                        else if (id == "decal")
                        {
                            m_current_material->map_decal = map_filename(data, count);
                        }
                        else if (id == "refl")
                        {
                            m_current_material->map_refl = map_filename(data, count);
                        }
                        else if (id == "Pr")
                        {
                            m_current_material->pr = parse_float(data, count);
                            m_current_material->has_pr = true;
                        }
                        else if (id == "Pcr")
                        {
                            m_current_material->pcr = parse_float(data, count);
                            m_current_material->has_pcr = true;
                        }
                        else if (id == "Pc")
                        {
                            m_current_material->pc = parse_float(data, count);
                            m_current_material->has_pc = true;
                        }
                        else if (id == "Pm")
                        {
                            m_current_material->pm = parse_float(data, count);
                            m_current_material->has_pm = true;
                        }
                        else if (id == "Ps")
                        {
                            m_current_material->ps = parse_float(data, count);
                            m_current_material->has_ps = true;
                        }
                        else if (id == "anisor")
                        {
                            m_current_material->anisor = parse_float(data, count);
                            m_current_material->has_anisor = true;
                        }
                        else if (id == "aniso")
                        {
                            m_current_material->aniso = parse_float(data, count);
                            m_current_material->has_aniso = true;
                        }
                        else if (id == "map_Pr")
                        {
                            m_current_material->map_pr = map_filename(data, count);
                        }
                        else if (id == "map_Pm")
                        {
                            m_current_material->map_pm = map_filename(data, count);
                        }
                        else if (id == "map_Ps")
                        {
                            m_current_material->map_ps = map_filename(data, count);
                        }
                        else if (id == "norm")
                        {
                            m_current_material->map_norm = map_filename(data, count);
                        }
                        else
                        {
                            //printLine(Print::Verbose, "TODO: {}", id);
                        }

                        //printLine(Print::Verbose, "token: {} : {}", i, tokens[0]);
                    }

                    tokens.clear();
                }
            }

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }
    }

    void ReaderOBJ::parse_v(const std::string_view* tokens, size_t count)
    {
        if (count < 3)
            return;

        const float x = parseFloat(tokens[0]);
        const float y = parseFloat(tokens[1]);
        const float z = parseFloat(tokens[2]);

        positions.emplace_back(x, y, -z);

        if (count >= 6)
        {
            const float r = parseFloat(tokens[3]);
            const float g = parseFloat(tokens[4]);
            const float b = parseFloat(tokens[5]);

            if (!has_vertex_colors)
            {
                has_vertex_colors = true;
                colors.resize(positions.size() - 1, float32x4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            colors.emplace_back(r, g, b, 1.0f);
        }
        else if (has_vertex_colors)
        {
            colors.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    void ReaderOBJ::parse_vn(const std::string_view* tokens, size_t count)
    {
        float32x3 value = parse_float32x3(tokens, count);
        normals.push_back(float32x3(value.x, value.y, -value.z));
    }

    void ReaderOBJ::parse_vt(const std::string_view* tokens, size_t count)
    {
        if (count < 2 || count > 3)
        {
            // error
        }

        float value[3];

        value[2] = 0.0;

        for (size_t i = 0; i < count; ++i)
        {
            value[i] = parseFloat(tokens[i]);
        }

        //printf("vt %f %f %f\n", value[0], value[1], value[2]);
        texcoords.emplace_back(value[0], value[1]);
    }

    void ReaderOBJ::parse_mtllib(const std::string_view* tokens, size_t count)
    {
        if (count < 1)
            return;

        for (size_t i = 0; i < count; ++i)
        {
            std::string filename(tokens[i]);
            replace(filename, "\\", "/");
            printLine(Print::Verbose, "mtllib: {}", filename);

            filesystem::File file(m_path, filename);

            std::string_view s(reinterpret_cast<const char *>(file.data()), file.size());
            parse_mtl(s);
        }
    }

    void ReaderOBJ::parse_usemtl(const std::string_view* tokens, size_t count)
    {
        if (count != 1)
        {
            // error
        }

        std::string name(tokens[0]);

        for (size_t index = 0; index < m_materials.size(); ++index)
        {
            if (m_materials[index].name == name)
            {
                m_face_material = u32(index);
                return;
            }
        }
    }

    void ReaderOBJ::parse_o(const std::string_view* tokens, size_t count)
    {
        if (count != 1)
        {
            // error
            return;
        }

        ObjectOBJ object;
        object.name = std::string(tokens[0]);
        m_objects.push_back(object);
    }

    void ReaderOBJ::parse_g(const std::string_view* tokens, size_t count)
    {
        if (count != 1)
        {
            // error
            return;
        }

        ObjectOBJ& object = getCurrentObject();

        GroupOBJ group;
        group.name = std::string(tokens[0]);
        object.groups.push_back(group);
    }

    void ReaderOBJ::parse_s(const std::string_view* tokens, size_t count)
    {
        if (count != 1)
        {
            return;
        }

        if (tokens[0] == "off")
        {
            m_smoothing_group = 0;
        }
        else
        {
            m_smoothing_group = u32(parseInt(tokens[0]));
        }
    }

    void ReaderOBJ::parse_f(const std::string_view* tokens, size_t count)
    {
        constexpr size_t maxVertexPerFace = 128;

        if (count < 3 || count > maxVertexPerFace)
        {
            // error
            return;
        }

        s32 positionIndex[maxVertexPerFace];
        s32 texcoordIndex[maxVertexPerFace];
        s32 normalIndex[maxVertexPerFace];

        const s32 bias[3] =
        {
            s32(positions.size() + 1),
            s32(texcoords.size() + 1),
            s32(normals.size() + 1),
        };

        for (size_t i = 0; i < count; ++i)
        {
            // "pos"
            // "pos/tex"
            // "pos/tex/nrm"
            // "pos//nrm"
            std::string_view s = tokens[i];

            int value[3] = { 0, 0, 0 };

            parseFaceCorner(s, value);

            // negative indices start from the last element
            if (value[0] < 0) value[0] += bias[0];
            if (value[1] < 0) value[1] += bias[1];
            if (value[2] < 0) value[2] += bias[2];

            positionIndex[i] = value[0];
            texcoordIndex[i] = value[1];
            normalIndex[i] = value[2];
        }

        GroupOBJ& group = getCurrentGroup();

        auto& faces = group.faces;

        auto emit_triangle = [&](size_t i0, size_t i1, size_t i2)
        {
            FaceOBJ face;

            face.smoothing_group = m_smoothing_group;
            face.material = m_face_material;

            // Z-reflect already yields CW front faces — keep file corner order.
            face.vertex[0].position = positionIndex[i0];
            face.vertex[0].texcoord = texcoordIndex[i0];
            face.vertex[0].normal   = normalIndex[i0];

            face.vertex[1].position = positionIndex[i1];
            face.vertex[1].texcoord = texcoordIndex[i1];
            face.vertex[1].normal   = normalIndex[i1];

            face.vertex[2].position = positionIndex[i2];
            face.vertex[2].texcoord = texcoordIndex[i2];
            face.vertex[2].normal   = normalIndex[i2];

            faces.push_back(face);
        };

        if (count == 3)
        {
            emit_triangle(0, 1, 2);
        }
        else if (count == 4)
        {
            const float32x3& p0 = positions[positionIndex[0] - 1];
            const float32x3& p1 = positions[positionIndex[1] - 1];
            const float32x3& p2 = positions[positionIndex[2] - 1];
            const float32x3& p3 = positions[positionIndex[3] - 1];

            const float32x3 e02 = p0 - p2;
            const float32x3 e13 = p1 - p3;

            if (square(e02) < square(e13))
            {
                emit_triangle(0, 1, 2);
                emit_triangle(0, 2, 3);
            }
            else
            {
                emit_triangle(0, 1, 3);
                emit_triangle(1, 2, 3);
            }
        }
        else
        {
            const std::vector<uint32_t> indices = triangulatePolygon(positions, positionIndex, count);

            if (indices.empty())
            {
                for (size_t i = 0; i < count - 2; ++i)
                    emit_triangle(0, i + 1, i + 2);
            }
            else
            {
                for (size_t i = 0; i < indices.size(); i += 3)
                    emit_triangle(indices[i], indices[i + 1], indices[i + 2]);
            }
        }
    }

    ImportOBJ::ImportOBJ(const filesystem::Path& path, const std::string& filename)
        : Scene(path)
    {

        u64 time0 = mango::Time::ms();

        ReaderOBJ reader(path, filename);

        u64 time1 = mango::Time::ms();

        printLine("Materials: {}", reader.m_materials.size());

        std::unordered_map<std::string, u32> imageIndexByKey;
        auto addFileImage = [&](const std::string& filename) -> u32
        {
            if (filename.empty())
                return ImageSample::none;

            auto it = imageIndexByKey.find(filename);
            if (it != imageIndexByKey.end())
                return it->second;

            u32 idx = u32(images.size());
            images.push_back(ImageSource::fromFile(filename));
            imageIndexByKey.emplace(filename, idx);
            return idx;
        };

        for (const MaterialOBJ& materialobj : reader.m_materials)
        {
            Material material;

            material.name = materialobj.name;

            material.baseColorFactor = float32x4(materialobj.kd, materialobj.tr);
            material.emissiveFactor = materialobj.ke;

            if (materialobj.has_pm)
                material.metallicFactor = materialobj.pm;
            else
                material.metallicFactor = 0.0f;

            if (materialobj.has_pr)
            {
                material.roughnessFactor = materialobj.pr;
            }
            else if (materialobj.ns > 0.0f)
            {
                // Blinn-Phong exponent → GGX roughness (common OBJ fallback).
                material.roughnessFactor = std::sqrt(2.0f / (materialobj.ns + 2.0f));
            }
            else
            {
                material.roughnessFactor = 0.5f;
            }

            const u32 kd = addFileImage(materialobj.map_kd);
            if (kd != ImageSample::none)
                material.baseColor = ImageSample::from(kd, ImageSwizzle::rgba(), ImageColorSpace::sRGB);

            const u32 ke = addFileImage(materialobj.map_ke);
            if (ke != ImageSample::none)
                material.emissive = ImageSample::from(ke, ImageSwizzle::rgb1(), ImageColorSpace::sRGB);

            const std::string& normalMap = !materialobj.map_norm.empty()
                ? materialobj.map_norm
                : materialobj.map_bump;
            const u32 normalTex = addFileImage(normalMap);
            if (normalTex != ImageSample::none)
                material.normal = ImageSample::from(normalTex, ImageSwizzle::rgb1(), ImageColorSpace::Linear);

            const u32 pr = addFileImage(materialobj.map_pr);
            if (pr != ImageSample::none)
                material.roughness = ImageSample::from(pr, ImageSwizzle::r(), ImageColorSpace::Linear);

            const u32 pm = addFileImage(materialobj.map_pm);
            if (pm != ImageSample::none)
                material.metallic = ImageSample::from(pm, ImageSwizzle::r(), ImageColorSpace::Linear);

            const u32 opacity = addFileImage(materialobj.map_d);
            if (opacity != ImageSample::none)
                material.opacity = ImageSample::from(opacity, ImageSwizzle::r(), ImageColorSpace::Linear);

            const u32 ka = addFileImage(materialobj.map_ka);
            if (ka != ImageSample::none)
                material.occlusion = ImageSample::from(ka, ImageSwizzle::r(), ImageColorSpace::Linear);

            if (materialobj.has_pc)
                material.clearcoatFactor = materialobj.pc;

            if (materialobj.has_pcr)
                material.clearcoatRoughnessFactor = materialobj.pcr;

            if (materialobj.has_ps)
                material.sheenColorFactor = float32x3(materialobj.ps, materialobj.ps, materialobj.ps);

            const u32 sheen = addFileImage(materialobj.map_ps);
            if (sheen != ImageSample::none)
                material.sheenColor = ImageSample::from(sheen, ImageSwizzle::rgb1(), ImageColorSpace::sRGB);

            if (materialobj.has_aniso)
                material.anisotropyStrength = materialobj.aniso;

            if (materialobj.has_anisor)
                material.anisotropyRotation = materialobj.anisor;

            if (materialobj.tr < 1.0f || material.opacity)
                material.alphaMode = Material::AlphaMode::Blend;

            materials.push_back(material);
        }

        if (materials.empty())
        {
            // create default material
            Material material;

            material.name = "default";

            material.baseColorFactor = float32x4(1.0f, 1.0f, 1.0f, 1.0f);
            material.emissiveFactor = 0.0f;
            material.metallicFactor = 0.0f;
            material.roughnessFactor = 0.5f;

            materials.push_back(material);
        }

        u64 time2 = mango::Time::ms();

        printLine("Objects: {}", reader.m_objects.size());

        for (const auto& object : reader.m_objects)
        {
            for (const auto& group : object.groups)
            {
                std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
                IndexedMesh& mesh = *ptr;

                mesh.flags = Vertex::Position | Vertex::Normal | Vertex::Texcoord;

                if (reader.has_vertex_colors)
                    mesh.flags |= Vertex::Color;

                std::unordered_map<CornerKey, u32, CornerHash> unique;

                for (const FaceOBJ& face : group.faces)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        u32 index;

                        CornerKey key { face.vertex[i], face.smoothing_group };

                        auto it = unique.find(key);
                        if (it != unique.end())
                        {
                            index = it->second;
                        }
                        else
                        {
                            index = u32(mesh.vertices.size());
                            unique[key] = index;

                            Vertex vertex;

                            u32 positionIndex = face.vertex[i].position;
                            u32 texcoordIndex = face.vertex[i].texcoord;
                            u32 normalIndex = face.vertex[i].normal;

                            if (positionIndex > reader.positions.size())
                            {
                                //printLine("positionIndex: {} > {}", positionIndex, reader.positions.size());
                            }

                            if (texcoordIndex != 0 && texcoordIndex > reader.texcoords.size())
                            {
                                //printLine("texcoordIndex: {} > {}", texcoordIndex, reader.texcoords.size());
                                texcoordIndex = 0;
                            }

                            if (normalIndex != 0 && normalIndex > reader.normals.size())
                            {
                                //printLine("normalIndex: {} > {}", normalIndex, reader.normals.size());
                                normalIndex = 0;
                            }

                            vertex.position = reader.positions[positionIndex - 1];

                            mesh.boundingBox.extend(vertex.position);

                            if (reader.has_vertex_colors && positionIndex > 0 && positionIndex <= reader.colors.size())
                                vertex.color = reader.colors[positionIndex - 1];

                            if (texcoordIndex)
                            {
                                vertex.texcoord = reader.texcoords[texcoordIndex - 1];
                                // OBJ UVs are bottom-left origin → Vulkan/glTF top-left (V down).
                                vertex.texcoord.y = 1.0f - vertex.texcoord.y;
                            }

                            if (normalIndex)
                            {
                                // File vn is outward; winding remap does not flip it.
                                vertex.normal = reader.normals[normalIndex - 1];
                            }

                            mesh.vertices.push_back(vertex);
                        }

                        mesh.indices.push_back(index);
                    }
                }

                // Fill any missing normals from CW face geometry (outward).
                {
                    const size_t vertexCount = mesh.vertices.size();
                    std::vector<float32x3> accumulated(vertexCount, float32x3(0.0f));
                    bool anyMissing = false;

                    for (const Vertex& vertex : mesh.vertices)
                    {
                        if (square(vertex.normal) < 1.0e-12f)
                        {
                            anyMissing = true;
                            break;
                        }
                    }

                    if (anyMissing)
                    {
                        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
                        {
                            const u32 i0 = mesh.indices[i + 0];
                            const u32 i1 = mesh.indices[i + 1];
                            const u32 i2 = mesh.indices[i + 2];

                            const float32x3& p0 = mesh.vertices[i0].position;
                            const float32x3& p1 = mesh.vertices[i1].position;
                            const float32x3& p2 = mesh.vertices[i2].position;

                            const float32x3 faceNormal = normalize(cross(p0 - p2, p0 - p1));
                            accumulated[i0] = accumulated[i0] + faceNormal;
                            accumulated[i1] = accumulated[i1] + faceNormal;
                            accumulated[i2] = accumulated[i2] + faceNormal;
                        }

                        for (size_t i = 0; i < vertexCount; ++i)
                        {
                            if (square(mesh.vertices[i].normal) < 1.0e-12f)
                            {
                                const float32x3& n = accumulated[i];
                                mesh.vertices[i].normal = square(n) > 1.0e-12f ? normalize(n) : float32x3(0.0f, 1.0f, 0.0f);
                            }
                        }
                    }
                }

                Mesh trimesh;
                trimesh.flags = mesh.flags;

                for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
                {
                    Triangle triangle;

                    triangle.vertex[0] = mesh.vertices[mesh.indices[i + 0]];
                    triangle.vertex[1] = mesh.vertices[mesh.indices[i + 1]];
                    triangle.vertex[2] = mesh.vertices[mesh.indices[i + 2]];

                    trimesh.triangles.push_back(triangle);
                }

                trimesh.computeTangents();

                for (size_t f = 0; f < trimesh.triangles.size(); ++f)
                {
                    const Triangle& triangle = trimesh.triangles[f];

                    for (int i = 0; i < 3; ++i)
                        mesh.vertices[mesh.indices[f * 3 + u32(i)]] = triangle.vertex[i];
                }

                mesh.flags = trimesh.flags;

                if (!group.faces.empty())
                {
                    size_t runStart = 0;
                    u32 material = group.faces[0].material;

                    for (size_t f = 1; f <= group.faces.size(); ++f)
                    {
                        if (f == group.faces.size() || group.faces[f].material != material)
                        {
                            Primitive primitive;

                            primitive.type = Primitive::Type::TriangleList;
                            primitive.start = u32(runStart * 3);
                            primitive.count = u32((f - runStart) * 3);
                            primitive.base = 0;
                            primitive.material = material;

                            mesh.primitives.push_back(primitive);

                            if (f < group.faces.size())
                            {
                                runStart = f;
                                material = group.faces[f].material;
                            }
                        }
                    }
                }

                ptr = std::make_unique<IndexedMesh>(std::move(mesh));

                Node node;

                node.name = object.name;
                node.transform = matrix4x4(1.0f);
                node.mesh = u32(meshes.size());

                nodes.push_back(node);
                meshes.push_back(std::move(ptr));
            } // groups
        } // objects

        printLine("Nodes: {}", nodes.size());

        // NOTE: we don't care about hierarchy in the .obj scene
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            u32 index = u32(i);
            roots.push_back(index);
        }

        u64 time3 = mango::Time::ms();

        printLine(Print::Verbose, "Reading: {} ms", time1 - time0);
        printLine(Print::Verbose, "Textures: {} ms", time2 - time1);
        printLine(Print::Verbose, "Conversion: {} ms", time3 - time2);
    }

} // namespace mango::import3d
