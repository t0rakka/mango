/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <string_view>
#include <mango/filesystem/filesystem.hpp>
#include <mango/import3d/import_obj.hpp>
#include "../../external/fast_float/fast_float.h"

namespace mango::import3d
{

// https://en.wikipedia.org/wiki/Wavefront_.obj_file

    struct FaceOBJ
    {
        int position[3];
        int normal[3];
        int texcoord[3];
    };

    struct MaterialOBJ
    {
        std::string name;

        float ns;
        float ni;
        float d;
        float tr;
        float tf;

        float illum;

        float32x3 ka;
        float32x3 kd;
        float32x3 ks;
        float32x3 ke;

        std::string map_ka;
        std::string map_kd;
        std::string map_ks;
        std::string map_ke;
        std::string map_bump;
    };

    struct ObjectOBJ
    {
        u32 material;
        std::string name;
        std::vector<FaceOBJ> faces;
    };

    struct ReaderOBJ
    {
        const filesystem::Path& m_path;

        std::vector<float32x3> positions;
        std::vector<float32x3> normals;
        std::vector<float32x2> texcoords;

        std::vector<ObjectOBJ> m_objects;
        std::vector<MaterialOBJ> m_materials;

        ObjectOBJ* m_current_object = nullptr;
        MaterialOBJ* m_current_material = nullptr;

        ReaderOBJ(const filesystem::Path& path, const std::string& filename);

        void parse_mtl(const std::string_view& s);

        void parse_v(const std::vector<std::string_view>& tokens);
        void parse_vn(const std::vector<std::string_view>& tokens);
        void parse_vt(const std::vector<std::string_view>& tokens);
        void parse_mtllib(const std::vector<std::string_view>& tokens);
        void parse_usemtl(const std::vector<std::string_view>& tokens);
        void parse_o(const std::vector<std::string_view>& tokens);
        void parse_g(const std::vector<std::string_view>& tokens);
        void parse_s(const std::vector<std::string_view>& tokens);
        void parse_f(const std::vector<std::string_view>& tokens);

        float parse_float(std::string_view s) const
        {
            float value;
            fast_float::from_chars(s.data(), s.data() + s.size(), value);
            return value;
        }

        float parse_float(const std::vector<std::string_view>& tokens) const
        {
            if (tokens.size() != 1)
            {
                // error
            }

            double value = parse_float(tokens[0]);
            return value;
        }

        float32x3 parse_float32x3(const std::vector<std::string_view>& tokens) const
        {
            if (tokens.size() != 3)
            {
                // error
            }

            double value[3];

            for (size_t i = 0; i < tokens.size(); ++i)
            {
                value[i] = parse_float(tokens[i]);
            }

            return float32x3(value[0], value[1], value[2]);
        }
    };

    ReaderOBJ::ReaderOBJ(const filesystem::Path& path, const std::string& filename)
        : m_path(path)
    {
        filesystem::File file(path, filename);
        printf("size: %d KB\n", int(file.size() / 1024));

        std::string_view s(reinterpret_cast<const char *>(file.data()), file.size());

        // ---------------------
#if 0
        const u8* data = file.data();
        const u8* end = file.data() + file.size();

        for ( ; data < end; )
        {
            const u8* p = mango::memchr(data, '\n', end - data);
            //if (!p) break;
            std::string_view line((const char *)data, p - data);
            //printf("%s\n", std::string(line).c_str());

            std::vector<std::string_view> v = split(line, " \t");

            if (!v.empty())
            {
                std::string_view id = v[0];

                std::vector<std::string_view> tokens(v.begin() + 1, v.end());

                if (id == "#")
                {
                    // comment
                }
                else if (id == "v")
                {
                    parse_v(tokens);
                }
                else if (id == "vn")
                {
                    parse_vn(tokens);
                }
                else if (id == "vt")
                {
                    parse_vt(tokens);
                }
                else if (id == "mtllib")
                {
                    //parse_mtllib(tokens);
                }
                else if (id == "usemtl")
                {
                    parse_usemtl(tokens);
                }
                else if (id == "o")
                {
                    parse_o(tokens);
                }
                else if (id == "g")
                {
                    parse_g(tokens);
                }
                else if (id == "s")
                {
                    parse_s(tokens);
                }
                else if (id == "f")
                {
                    parse_f(tokens);
                }

            }

            data = p + 1;
        }

#else

        std::string_view id;
        std::vector<std::string_view> tokens;

        size_t first = 0;

        while (first < s.size())
        {
            size_t second = s.find_first_of(" \t\n\r", first);

            if (first != second)
            {
                std::string_view token = s.substr(first, second - first);

                if (id.empty())
                {
                    id = token;
                }
                else
                {
                    tokens.push_back(token);
                }

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

                    if (id == "#")
                    {
                        // comment
                    }
                    else if (id == "v")
                    {
                        parse_v(tokens);
                    }
                    else if (id == "vn")
                    {
                        parse_vn(tokens);
                    }
                    else if (id == "vt")
                    {
                        parse_vt(tokens);
                    }
                    else if (id == "mtllib")
                    {
                        parse_mtllib(tokens);
                    }
                    else if (id == "usemtl")
                    {
                        parse_usemtl(tokens);
                    }
                    else if (id == "o")
                    {
                        parse_o(tokens);
                    }
                    else if (id == "g")
                    {
                        parse_g(tokens);
                    }
                    else if (id == "s")
                    {
                        parse_s(tokens);
                    }
                    else if (id == "f")
                    {
                        parse_f(tokens);
                    }

                    id = std::string_view();
                    tokens.clear();
                }
            }

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }
#endif
    }

    void ReaderOBJ::parse_mtl(const std::string_view& s)
    {
        std::string_view id;
        std::vector<std::string_view> tokens;

        size_t first = 0;

        while (first < s.size())
        {
            size_t second = s.find_first_of(" \t\n\r", first);

            if (first != second)
            {
                std::string_view token = s.substr(first, second - first);

                if (id.empty())
                {
                    id = token;
                }
                else
                {
                    tokens.push_back(token);
                }

                char s0 = s[second + 0];
                char s1 = s[second + 1];

                if (s0 == '\n' || s0 == '\r' || s1 == '\n' || s1 == '\r')
                {
                    if (id == "newmtl")
                    {
                        MaterialOBJ material;
                        material.name = std::string(tokens[0]);

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
                            m_current_material->ns = parse_float(tokens);
                        }
                        else if (id == "Ni")
                        {
                            m_current_material->ni = parse_float(tokens);
                        }
                        else if (id == "d")
                        {
                            m_current_material->d = parse_float(tokens);
                        }
                        else if (id == "Tr")
                        {
                            m_current_material->tr = parse_float(tokens);
                        }
                        else if (id == "illum")
                        {
                            m_current_material->illum = parse_float(tokens);
                        }
                        else if (id == "Ka")
                        {
                            m_current_material->ka = parse_float32x3(tokens);
                        }
                        else if (id == "Kd")
                        {
                            m_current_material->kd = parse_float32x3(tokens);
                        }
                        else if (id == "Ks")
                        {
                            m_current_material->ks = parse_float32x3(tokens);
                        }
                        else if (id == "Ke")
                        {
                            m_current_material->ke = parse_float32x3(tokens);
                        }
                        else if (id == "map_Ka")
                        {
                            m_current_material->map_ka = std::string(tokens[0]);
                        }
                        else if (id == "map_Kd")
                        {
                            m_current_material->map_kd = std::string(tokens[0]);
                        }
                        else if (id == "map_Ks")
                        {
                            m_current_material->map_ks = std::string(tokens[0]);
                        }
                        else if (id == "map_Ke")
                        {
                            m_current_material->map_ke = std::string(tokens[0]);
                        }
                        else if (id == "map_bump")
                        {
                            m_current_material->map_bump = std::string(tokens[0]);
                        }
                    }

                    id = std::string_view();
                    tokens.clear();
                }
            }

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }
    }

    void ReaderOBJ::parse_v(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() < 3 || tokens.size() > 4)
        {
            // error
        }

        double value[4];

        value[3] = 1.0;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            value[i] = parse_float(tokens[i]);
        }

        //printf("v %f %f %f %f\n", value[0], value[1], value[2], value[3]);
        positions.emplace_back(value[0], value[1], value[2]);
    }

    void ReaderOBJ::parse_vn(const std::vector<std::string_view>& tokens)
    {
        float32x3 value = parse_float32x3(tokens);
        normals.push_back(value);
    }

    void ReaderOBJ::parse_vt(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() < 2 || tokens.size() > 3)
        {
            // error
        }

        double value[3];

        value[2] = 0.0;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            value[i] = parse_float(tokens[i]);
        }

        //printf("vt %f %f %f\n", value[0], value[1], value[2]);
        texcoords.emplace_back(value[0], value[1]);
    }

    void ReaderOBJ::parse_mtllib(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 1)
        {
            // error
        }

        std::string filename(tokens[0]);
        printf("mtllib: %s\n", filename.c_str());

        filesystem::File file(m_path, filename);

        std::string_view s(reinterpret_cast<const char *>(file.data()), file.size());
        parse_mtl(s);
    }

    void ReaderOBJ::parse_usemtl(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 1)
        {
            // error
        }

        std::string name = std::string(tokens[0]);

        if (m_current_object)
        {
            // NOTE: brute-force search
            for (size_t index = 0; index < m_materials.size(); ++index)
            {
                if (m_materials[index].name == name)
                {
                    m_current_object->material = u32(index);
                }
            }
        }
    }

    void ReaderOBJ::parse_o(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 1)
        {
            // error
        }

        std::string name = std::string(tokens[0]);

        m_objects.push_back(ObjectOBJ());
        m_current_object = &m_objects.back();

        m_current_object->name = name;
    }

    void ReaderOBJ::parse_g(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 1)
        {
            // error
        }

        //std::string name = std::string(tokens[0]);
    }

    void ReaderOBJ::parse_s(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 1)
        {
            // error
        }

        // TODO
        /*
        s 1
        s off
        */
    }

    void ReaderOBJ::parse_f(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() < 3 || tokens.size() > 4)
        {
            // error
        }

        int positionIndex[4];
        int texcoordIndex[4];
        int normalIndex[4];

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            // "pos"
            // "pos/tex"
            // "pos/tex/nrm"
            // "pos//nrm"
            std::string_view s = tokens[i];

            int value[3] = { 0, 0, 0 };
            size_t index = 0;

            size_t first = 0;

            while (first < s.size() && index < 3)
            {
                value[index++] = std::atoi(s.data() + first);

                size_t second = s.find_first_of("/", first);

                if (second == std::string_view::npos)
                    break;

                first = second + 1;
            }

            positionIndex[i] = value[0];
            texcoordIndex[i] = value[1];
            normalIndex[i] = value[2];
        }

        auto& faces = m_objects.back().faces;

        for (size_t i = 0; i < tokens.size() - 2; ++i)
        {
            FaceOBJ face;

            face.position[0] = positionIndex[0];
            face.position[1] = positionIndex[i + 1];
            face.position[2] = positionIndex[i + 2];

            face.texcoord[0] = texcoordIndex[0];
            face.texcoord[1] = texcoordIndex[i + 1];
            face.texcoord[2] = texcoordIndex[i + 2];

            face.normal[0] = normalIndex[0];
            face.normal[1] = normalIndex[i + 1];
            face.normal[2] = normalIndex[i + 2];

            faces.push_back(face);
        }
    }

    /*
    ImportOBJ::ImportOBJ(const filesystem::Path& path, const std::string& filename)
    {
        ReaderOBJ reader(path, filename);

        u64 time0 = mango::Time::ms();

        materials = reader.m_materials;

        for (const auto& object : reader.m_objects)
        {
            Mesh mesh;

            mesh.material = object.material;
            mesh.triangles.resize(object.faces.size());

            for (size_t faceIndex = 0; faceIndex < object.faces.size(); ++faceIndex)
            {
                const FaceOBJ& face = object.faces[faceIndex];
                Triangle& triangle = mesh.triangles[faceIndex];

                for (int i = 0; i < 3; ++i)
                {
                    Vertex& vertex = triangle.vertex[i];

                    size_t positionIndex = face.position[i];
                    size_t texcoordIndex = face.texcoord[i];
                    size_t normalIndex = face.normal[i];
                    // TODO: bound check indices

                    vertex.position = reader.positions[positionIndex - 1];
                    vertex.texcoord = texcoordIndex ? reader.texcoords[texcoordIndex - 1] : float32x2(0.0f, 0.0f);
                    vertex.normal = normalIndex ? reader.normals[normalIndex - 1] : float32x3(0.0f, 0.0f, 0.0f);

                    vertex.texcoord.y = -vertex.texcoord.y; // GL flip
                }
            }

            Object o;

            o.name = object.name;
            o.parent = ~0;
            o.transform = Matrix4(1.0f);
            o.meshes.push_back(u32(meshes.size()));

            objects.push_back(o);

            meshes.push_back(mesh);
        }

        u64 time1 = mango::Time::ms();
        printf("Conversion: %d ms\n", int(time1 - time0));
    }
    */

} // namespace mango::import3d

