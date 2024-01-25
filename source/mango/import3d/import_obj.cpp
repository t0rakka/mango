/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <string_view>
#include <mango/core/core.hpp>
#include <mango/import3d/import_obj.hpp>

namespace mango::import3d
{

    // https://en.wikipedia.org/wiki/Wavefront_.obj_file

    struct FaceOBJ
    {
        s32 position[3];
        s32 normal[3];
        s32 texcoord[3];
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

        /* PBR:
        Pr / map_Pr     # roughness
        Pm / map_Pm     # metallic
        Ps / map_Ps     # sheen
        Pc              # clearcoat thickness
        Pcr             # clearcoat roughness
        Ke / map_Ke     # emissive
        aniso           # anisotropy
        anisor          # anisotropy rotation
        norm            # normal map (RGB components represent XYZ components of the surface normal)
        */
    };

    struct ObjectOBJ
    {
        u32 material = 0;
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

        std::string map_filename(std::string_view filename) const
        {
            std::string s(filename);
            replace(s, "\\", "/");
            return s;
        }

        float parse_float(std::string_view s) const
        {
            return parseFloat(s);
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

        std::string_view s(reinterpret_cast<const char *>(file.data()), file.size());

        // ---------------------

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
#if 0
                    printf("%s", std::string(id).c_str());
                    for (size_t i = 0; i < tokens.size(); ++i)
                    {
                        printf(" %s", std::string(tokens[i]).c_str());
                    }
                    printf("\n");
#endif

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
                            m_current_material->tr = parse_float(tokens);
                        }
                        else if (id == "Tr")
                        {
                            m_current_material->tr = 1.0f - parse_float(tokens);
                        }
                        else if (id == "Tf")
                        {
                            m_current_material->tf = parse_float(tokens);
                        }
                        else if (id == "illum")
                        {
                            m_current_material->illum = u32(parse_float(tokens));
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
                            m_current_material->map_ka = map_filename(tokens[0]);
                        }
                        else if (id == "map_Kd")
                        {
                            m_current_material->map_kd = map_filename(tokens[0]);
                        }
                        else if (id == "map_Ks")
                        {
                            m_current_material->map_ks = map_filename(tokens[0]);
                        }
                        else if (id == "map_Ke")
                        {
                            m_current_material->map_ke = map_filename(tokens[0]);
                        }
                        else if (id == "map_bump" || id == "map_Bump"|| id == "bump")
                        {
                            size_t filenameIndex = 0;
                            if (tokens.size() > 1)
                            {
                                // there could be tokens here that we skip, example: "-bm <float>"
                                filenameIndex = tokens.size() - 1;
                            }
                            m_current_material->map_bump = map_filename(tokens[filenameIndex]);
                        }
                        else if (id == "map_Ns")
                        {
                            m_current_material->map_ns = map_filename(tokens[0]);
                        }
                        else if (id == "map_d")
                        {
                            m_current_material->map_d = map_filename(tokens[0]);
                        }
                        else if (id == "disp")
                        {
                            m_current_material->map_disp = map_filename(tokens[0]);
                        }
                        else if (id == "decal")
                        {
                            m_current_material->map_decal = map_filename(tokens[0]);
                        }
                        else if (id == "refl")
                        {
                            m_current_material->map_refl = map_filename(tokens[0]);
                        }
                        else
                        {
                            //printLine(Print::Verbose, "TODO: {}", id);
                        }

                        //printLine(Print::Verbose, "token: {} : {}", i, tokens[0]);
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
        printLine(Print::Verbose, "mtllib: {}", filename);

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
        constexpr size_t maxVertexPerFace = 128;

        if (tokens.size() < 3 || tokens.size() > maxVertexPerFace)
        {
            // error
            return;
        }

        int positionIndex[maxVertexPerFace];
        int texcoordIndex[maxVertexPerFace];
        int normalIndex[maxVertexPerFace];

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

            // negative indices mean index from the end of the array
            positionIndex[i] = value[0] < 0 ? value[0] + positions.size() + 1 : value[0];
            texcoordIndex[i] = value[1] < 0 ? value[1] + texcoords.size() + 1 : value[1];
            normalIndex[i] = value[2] < 0 ? value[2] + normals.size() + 1 : value[2];
        }

        if (m_objects.empty())
        {
            // create default object
            m_objects.push_back(ObjectOBJ());
            m_current_object = &m_objects.back();
            m_current_object->name = "default";
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

    ImportOBJ::ImportOBJ(const filesystem::Path& path, const std::string& filename)
    {
        u64 time0 = mango::Time::ms();

        ReaderOBJ reader(path, filename);

        u64 time1 = mango::Time::ms();

        for (const MaterialOBJ& materialobj : reader.m_materials)
        {
            Material material;

            material.name = materialobj.name;

            material.baseColorFactor = float32x4(materialobj.kd, materialobj.tr);
            material.emissiveFactor = materialobj.ke;

            material.baseColorTexture = createTexture(path, materialobj.map_kd);
            material.emissiveTexture = createTexture(path, materialobj.map_ke);
            material.normalTexture = createTexture(path, materialobj.map_bump);
            material.occlusionTexture = createTexture(path, materialobj.map_ka);

            materials.push_back(material);
        }

        if (materials.empty())
        {
            // create default material
            Material material;

            material.name = "default";

            material.baseColorFactor = float32x4(1.0f, 1.0f, 1.0f, 1.0f);
            material.emissiveFactor = 1.0f;

            materials.push_back(material);
        }

        u64 time2 = mango::Time::ms();

        for (const auto& object : reader.m_objects)
        {
            Mesh trimesh;

            trimesh.triangles.resize(object.faces.size());

            for (size_t faceIndex = 0; faceIndex < object.faces.size(); ++faceIndex)
            {
                const FaceOBJ& face = object.faces[faceIndex];
                Triangle& triangle = trimesh.triangles[faceIndex];

                triangle.material = object.material;

                for (int i = 0; i < 3; ++i)
                {
                    Vertex& vertex = triangle.vertex[i];

                    u32 positionIndex = face.position[i];
                    u32 texcoordIndex = face.texcoord[i];
                    u32 normalIndex = face.normal[i];

                    /*
                    if (positionIndex > reader.positions.size())
                    {
                        printLine("positionIndex: {} > {}", positionIndex, reader.positions.size());
                        continue;
                    }

                    if (texcoordIndex != 0 && texcoordIndex > reader.texcoords.size())
                    {
                        printLine("texcoordIndex: {} > {}", texcoordIndex, reader.texcoords.size());
                        continue;
                    }

                    if (normalIndex != 0 && normalIndex > reader.normals.size())
                    {
                        printLine("normalIndex: {} > {}", normalIndex, reader.normals.size());
                        continue;
                    }
                    */

                    vertex.position = reader.positions[positionIndex - 1];

                    if (texcoordIndex)
                    {
                        vertex.texcoord = reader.texcoords[texcoordIndex - 1];
                        vertex.texcoord.y = -vertex.texcoord.y;
                    }

                    if (normalIndex)
                    {
                        vertex.normal = reader.normals[normalIndex - 1];
                    }
                }
            }

            //computeTangents(trimesh);
            IndexedMesh mesh = convertMesh(trimesh);

            Node node;

            node.name = object.name;
            node.transform = matrix4x4(1.0f);
            node.mesh = u32(meshes.size());

            nodes.push_back(node);
            meshes.push_back(mesh);
        }

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
