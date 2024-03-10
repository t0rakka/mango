/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/import3d/import_3ds.hpp>

/*
    Autodesk 3D Studio R4 importer
*/

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    struct Texture3DS
    {
        std::string filename;
    };

    struct Material3DS
    {
        std::string name;

        float32x4 ambient;
        float32x4 diffuse;
        float32x4 specular;

        Texture3DS texture_map1;
        Texture3DS texture_map2;
        Texture3DS texture_opacity;
        Texture3DS texture_bump;
        Texture3DS texture_specular;
        Texture3DS texture_shininess;
        Texture3DS texture_self_illum;
        Texture3DS texture_reflection;

        bool twosided = false;
        bool additive = false;
        bool wireframe = false;

        float shininess;
        float shininess2;
        float transparency;
    };

    struct Face3DS
    {
        u16 index[3];
        u16 flags = 0;
        u32 smoothing = 0;
    };

    struct Primitive3DS
    {
        u32 material;
        size_t start;
        size_t end;
    };

    struct Mesh3DS
    {
        std::vector<float32x3> positions;
        std::vector<float32x2> mappings;
        std::vector<Face3DS> faces;

        std::vector<u16> face_materials;
        std::vector<Primitive3DS> primitives;

        bool visible = true;

        float32x3 computeNormal(const Face3DS& face) const
        {
            float32x3 p0 = positions[face.index[0]];
            float32x3 p1 = positions[face.index[1]];
            float32x3 p2 = positions[face.index[2]];
            return cross(p0 - p1, p0 - p2);
        }
    };

    struct Hermite3DS
    {
        u16 time;
        float tension;
        float continuity;
        float bias;
        float easeto;
        float easefrom;
    };

    class Reader3DS
    {
    protected:
        int level = 0;
        bool debug = true;

        float32x4 color;
        float percent;
        std::string text;
        Texture3DS* texture = nullptr;

        u32 getMaterialIndex(const std::string& name) const
        {
            // brute-force linear search
            for (size_t i = 0; i < materials.size(); ++i)
            {
                if (materials[i].name == name)
                    return u32(i);
            }
            return 0;
        }

        Material3DS& getCurrentMaterial()
        {
            assert(!materials.empty());
            return materials.back();
        }

        Mesh3DS& getCurrentMesh()
        {
            assert(!meshes.empty());
            return meshes.back();
        }

    public:
        std::vector<Material3DS> materials;
        std::vector<Mesh3DS> meshes;

        Reader3DS(ConstMemory memory)
        {
            LittleEndianConstPointer p = memory.address;
            parse_chunks(p);
        }

        ~Reader3DS()
        {
        }

        std::string read_string(LittleEndianConstPointer& p) const
        {
            const u8* ptr = p;
            const char* text = reinterpret_cast<const char*>(ptr);
            p += strlen(text) + 1;
            return text;
        }

        float32x3 read_vec3(LittleEndianConstPointer& p) const
        {
            float x = p.read32f();
            float y = p.read32f();
            float z = p.read32f();
            return float32x3(x, y, z);
        }

        float32x2 read_vec2(LittleEndianConstPointer& p) const
        {
            float x = p.read32f();
            float y = p.read32f();
            return float32x2(x, y);
        }

        Hermite3DS read_hermite(LittleEndianConstPointer& p) const
        {
            u16 time = p.read16();
            u32 mask = p.read32();

            Hermite3DS h;

            h.time       = time;
            h.tension    = (mask & 0x0001) ? p.read32f() : 0.0f;
            h.continuity = (mask & 0x0002) ? p.read32f() : 0.0f;
            h.bias       = (mask & 0x0004) ? p.read32f() : 0.0f;
            h.easeto     = (mask & 0x0008) ? p.read32f() : 25.0f;
            h.easefrom   = (mask & 0x0010) ? p.read32f() : 25.0f;

            return h;
        }

        // data chunks

        void chunk_color_f32(LittleEndianConstPointer& p)
        {
            float r = p.read32f() / 255.0f;
            float g = p.read32f() / 255.0f;
            float b = p.read32f() / 255.0f;
            color = float32x4(r, g, b, 1.0f);
        }

        void chunk_color_u8(LittleEndianConstPointer& p)
        {
            float r = float(p[0]) / 255.0f;
            float g = float(p[1]) / 255.0f;
            float b = float(p[2]) / 255.0f;
            color = float32x4(r, g, b, 1.0f);
            p += 3;
        }

        void chunk_percent_u16(LittleEndianConstPointer& p)
        {
            percent = p.read16() * 0.01f;
        }

        void chunk_percent_f32(LittleEndianConstPointer& p)
        {
            percent = p.read32f() * 0.01f;
        }

        void chunk_master_scale(LittleEndianConstPointer& p)
        {
            float master_scale = p.read32f();
            printLine(Print::Verbose, level * 2, "master scale: {}", master_scale);
        }

        // main chunks

        void chunk_main(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[main]");
        }

        void chunk_version(LittleEndianConstPointer& p)
        {
            u32 version = p.read32();
            if (version > 3)
            {
                MANGO_EXCEPTION("[Import3DS] Incorrect version: {}", version);
            }

            printLine(Print::Verbose, level * 2, "version: {}", version);
        }

        // editor chunks

        void chunk_editor(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[editor]");
        }

        void chunk_mesh_version(LittleEndianConstPointer& p)
        {
            u32 mesh_version = p.read32();
            printLine(Print::Verbose, level * 2, "mesh version: {}", mesh_version);
        }

        // material chunks

        void chunk_material(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[material]");

            materials.emplace_back();
        }

        void chunk_material_name(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.name = read_string(p);
            printLine(Print::Verbose, level * 2, "name: \"{}\"", material.name);
        }

        void chunk_material_ambient_color(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.ambient = color;
            printLine(Print::Verbose, level * 2, "ambient color: {}, {}, {}", color[0], color[1], color[2]);
        }

        void chunk_material_diffuse_color(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.diffuse = color;
            printLine(Print::Verbose, level * 2, "diffuse color: {}, {}, {}", color[0], color[1], color[2]);
        }

        void chunk_material_specular_color(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.specular = color;
            printLine(Print::Verbose, level * 2, "specular color: {}, {}, {}", color[0], color[1], color[2]);
        }

        void chunk_material_shininess(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.shininess = percent;
            printLine(Print::Verbose, level * 2, "shininess: {}", percent);
        }

        void chunk_material_shininess2(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.shininess2 = percent;
            printLine(Print::Verbose, level * 2, "shininess2: {}", percent);
        }

        void chunk_material_transparency(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.transparency = percent;
            printLine(Print::Verbose, level * 2, "transparency: {}", percent);
        }

        void chunk_material_xpfall(LittleEndianConstPointer& p)
        {
            parse_chunks(p);
            printLine(Print::Verbose, level * 2, "xpfall: {}", percent);
        }

        void chunk_material_refblur(LittleEndianConstPointer& p)
        {
            parse_chunks(p);
            printLine(Print::Verbose, level * 2, "refblur: {}", percent);
        }

        void chunk_material_twosided(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.twosided = true;
            printLine(Print::Verbose, level * 2, "+ twosided");
        }

        void chunk_material_additive(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.additive = true;
            printLine(Print::Verbose, level * 2, "+ additive");
        }

        void chunk_material_self_illum_pct(LittleEndianConstPointer& p)
        {
            parse_chunks(p);
            printLine(Print::Verbose, level * 2, "self illum: {}", percent);
        }

        void chunk_material_wireframe(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.wireframe = true;
            printLine(Print::Verbose, level * 2, "+ wireframe");
        }

        void chunk_material_wireframe_size(LittleEndianConstPointer& p)
        {
            float wireframe_size = p.read32f();
            printLine(Print::Verbose, level * 2, "wireframe size: {}", percent);
            MANGO_UNREFERENCED(wireframe_size);
        }

        void chunk_material_shading(LittleEndianConstPointer& p)
        {
            // 0 - wireframe
            // 1 - flat
            // 2 - gouraud
            // 3 - phong
            // 4 - metal
            u16 shading = p.read16();
            printLine(Print::Verbose, level * 2, "shading: {}", shading);
        }

        // texture chunks

        void chunk_texture_map1(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:map1]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_map1;
        }

        void chunk_texture_map2(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:map2]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_map2;
        }

        void chunk_texture_opacity(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:opacity]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_opacity;
        }

        void chunk_texture_bump(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:bump]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_bump;
        }

        void chunk_texture_specular(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:specular]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_specular;
        }

        void chunk_texture_shininess(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:shininess]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_shininess;
        }

        void chunk_texture_self_illum(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:self-illum]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_self_illum;
        }

        void chunk_texture_reflection(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[texture:reflection]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_reflection;
        }

        void chunk_texture_map_name(LittleEndianConstPointer& p)
        {
            assert(texture != nullptr);
            texture->filename = read_string(p);
            printLine(Print::Verbose, level * 2, "filename: \"{}\"", texture->filename);
        }

        void chunk_texture_map_tiling(LittleEndianConstPointer& p)
        {
            // 1 - tile
            // 2 - decal
            u16 tiling_flags = p.read16();
            printLine(Print::Verbose, level * 2, "tiling: {:#x}", tiling_flags);
        }

        void chunk_texture_map_blur(LittleEndianConstPointer& p)
        {
            float blur = p.read32f();
            printLine(Print::Verbose, level * 2, "blur: {}", blur);
        }

        void chunk_texture_map_uscale(LittleEndianConstPointer& p)
        {
            float uscale = p.read32f();
            printLine(Print::Verbose, level * 2, "uscale: {}", uscale);
        }

        void chunk_texture_map_vscale(LittleEndianConstPointer& p)
        {
            float vscale = p.read32f();
            printLine(Print::Verbose, level * 2, "vscale: {}", vscale);
        }

        void chunk_texture_map_uoffset(LittleEndianConstPointer& p)
        {
            float uoffset = p.read32f();
            printLine(Print::Verbose, level * 2, "uoffset: {}", uoffset);
        }

        void chunk_texture_map_voffset(LittleEndianConstPointer& p)
        {
            float voffset = p.read32f();
            printLine(Print::Verbose, level * 2, "voffset: {}", voffset);
        }

        void chunk_texture_map_angle(LittleEndianConstPointer& p)
        {
            float angle = p.read32f();
            printLine(Print::Verbose, level * 2, "angle: {}", angle);
        }

        // object chunks

        void chunk_object(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[object]");

            std::string name = read_string(p);
            printLine(Print::Verbose, level * 2, "  name: \"{}\"", name);
        }

        // mesh chunks

        void chunk_mesh(LittleEndianConstPointer& p)
        {
            printLine(Print::Verbose, level * 2, "[mesh]");
            meshes.emplace_back();
        }

        void chunk_mesh_vertex_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            printLine(Print::Verbose, level * 2, "vertex.list: {}", count);

            Mesh3DS& mesh = getCurrentMesh();
            mesh.positions.resize(count);

            for (int i = 0; i < count; ++i)
            {
                float32x3 v = read_vec3(p);
                mesh.positions[i] = float32x3(-v.x, v.z, -v.y);
            }
        }

        void chunk_mesh_flag_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            printLine(Print::Verbose, level * 2, "flag.list: {}", count);

            for (int i = 0; i < count; ++i)
            {
                u16 flags = p.read16();
                MANGO_UNREFERENCED(flags);
            }
        }

        void chunk_mesh_face_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            printLine(Print::Verbose, level * 2, "face.list: {}", count);

            Mesh3DS& mesh = getCurrentMesh();
            mesh.faces.resize(count);

            for (int i = 0; i < count; ++i)
            {
                Face3DS& face = mesh.faces[i];
                face.index[2] = p.read16();
                face.index[1] = p.read16();
                face.index[0] = p.read16();
                face.flags = p.read16();
            }
        }

        void chunk_mesh_material_list(LittleEndianConstPointer& p)
        {
            std::string name = read_string(p);
            size_t count = p.read16();
            printLine(Print::Verbose, level * 2, "material.list: {}, name: \"{}\"", count, name);

            Mesh3DS& mesh = getCurrentMesh();

            Primitive3DS primitive;

            primitive.material = getMaterialIndex(name);
            primitive.start = mesh.face_materials.size();
            primitive.end = mesh.face_materials.size() + count;

            mesh.primitives.push_back(primitive);

            for (size_t i = 0; i < count; ++i)
            {
                u16 index = p.read16();
                mesh.face_materials.push_back(index);
            }
        }

        void chunk_mesh_mapping_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            printLine(Print::Verbose, level * 2, "mapping.list: {}", count);

            Mesh3DS& mesh = getCurrentMesh();
            mesh.mappings.resize(count);

            for (int i = 0; i < count; ++i)
            {
                float32x2 v = read_vec2(p);
                mesh.mappings[i] =  float32x2(v.x, -v.y);
            }
        }

        void chunk_mesh_smoothing_list(LittleEndianConstPointer& p)
        {
            Mesh3DS& mesh = getCurrentMesh();
            size_t count = mesh.faces.size();

            printLine(Print::Verbose, level * 2, "smoothing.list: {}", count);

            for (size_t i = 0; i < count; ++i)
            {
                mesh.faces[i].smoothing = p.read32();
            }
        }

        void chunk_mesh_local_axis(LittleEndianConstPointer& p)
        {
            for (int i = 0; i < 4; ++i)
            {
                float x = p.read32f();
                float y = p.read32f();
                float z = p.read32f();

                printLine(Print::Verbose, level * 2, "{} {} {}", x, y, z);

                /*
                if (i == 3)
                {
                    float32x3 offset = float32x3(-x, z, -y);

                    Mesh3DS& mesh = getCurrentMesh();

                    for (auto& position : mesh.positions)
                    {
                        position -= offset;
                    }
                }
                */
            }
        }

        void chunk_mesh_visible(LittleEndianConstPointer& p)
        {
            u8 visible = *p++;
            Mesh3DS& mesh = getCurrentMesh();
            mesh.visible = visible != 0;
        }

        // camera chunks

        void chunk_camera(LittleEndianConstPointer& p)
        {
            float32x3 position = read_vec3(p);
            float32x3 target = read_vec3(p);
            float bank = p.read32f();
            float lense = p.read32f();
            MANGO_UNREFERENCED(position);
            MANGO_UNREFERENCED(target);
            MANGO_UNREFERENCED(bank);
            MANGO_UNREFERENCED(lense);
        }

        void chunk_camera_range(LittleEndianConstPointer& p)
        {
            float inner_range = p.read32f();
            float outer_range = p.read32f();
            MANGO_UNREFERENCED(inner_range);
            MANGO_UNREFERENCED(outer_range);
        }

        // light chunks

        void chunk_light(LittleEndianConstPointer& p)
        {
            float32x3 position = read_vec3(p);
            MANGO_UNREFERENCED(position);
        }

        void chunk_light_spot(LittleEndianConstPointer& p)
        {
            //bool spotlight = true;
            float32x3 target = read_vec3(p);
            float hotspot = p.read32f();
            float falloff = p.read32f();
            MANGO_UNREFERENCED(target);
            MANGO_UNREFERENCED(hotspot);
            MANGO_UNREFERENCED(falloff);
        }

        void chunk_light_off(LittleEndianConstPointer& p)
        {
            // enable = false
        }

        void chunk_light_spot_roll(LittleEndianConstPointer& p)
        {
            float roll = p.read32f();
            MANGO_UNREFERENCED(roll);
        }

        void chunk_light_inner_range(LittleEndianConstPointer& p)
        {
            float inner_range = p.read32f();
            MANGO_UNREFERENCED(inner_range);
        }

        void chunk_light_outer_range(LittleEndianConstPointer& p)
        {
            float outer_range = p.read32f();
            MANGO_UNREFERENCED(outer_range);

        }

        void chunk_light_brightness(LittleEndianConstPointer& p)
        {
            float brightness = p.read32f();
            MANGO_UNREFERENCED(brightness);
        }

        // keyframer chunks

        void chunk_keyframer(LittleEndianConstPointer& p)
        {
            //printLine(Print::Verbose, level * 2, "[keyframer]");
        }

        void chunk_key_ambient(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.ambient]");
        }

        void chunk_key_object(LittleEndianConstPointer& p)
        {
            //printLine(Print::Verbose, level * 2, "[key.object]");
        }

        void chunk_key_camera(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.camera]");
        }

        void chunk_key_camera_target(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.camera.target]");
        }

        void chunk_key_light(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.light]");
        }

        void chunk_key_light_target(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.light.target]");
        }

        void chunk_key_spotlight(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.spotlight]");
        }

        void chunk_key_segment(LittleEndianConstPointer& p)
        {
            u32 min_frame = p.read32();
            u32 max_frame = p.read32();
            MANGO_UNREFERENCED(min_frame);
            MANGO_UNREFERENCED(max_frame);
        }

        void chunk_key_current_time(LittleEndianConstPointer& p)
        {
            // TODO
            p += 4;
        }

        void chunk_key_header(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.header]");
            p += size;
        }

        void chunk_key_node_header(LittleEndianConstPointer& p)
        {
            std::string name = read_string(p);
            u32 unused = p.read32();
            u16 parent = p.read16();
            MANGO_UNREFERENCED(name);
            MANGO_UNREFERENCED(unused);
            MANGO_UNREFERENCED(parent);
        }

        void chunk_key_dummy_object(LittleEndianConstPointer& p)
        {
            std::string name = read_string(p);
            MANGO_UNREFERENCED(name);
        }

        void chunk_key_pivot(LittleEndianConstPointer& p)
        {
            float32x3 pivot = read_vec3(p);
            //printLine(Print::Verbose, level * 2, "pivot: {} {} {}", pivot[0], pivot[1], pivot[2]);
            MANGO_UNREFERENCED(pivot);
        }

        void chunk_key_boundbox(LittleEndianConstPointer& p)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[boundbox]");
            p += 24;
        }

        void chunk_key_morph_smooth(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[key.morph.smooth]");
            p += size;
        }

        void chunk_key_pos_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[pos.track]");
            p += size;
        }

        void chunk_key_rot_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[rot.track]");
            p += size;
        }

        void chunk_key_scale_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[scale.track]");
            p += size;
        }

        void chunk_key_fov_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[fov.track]");
            p += size;
        }

        void chunk_key_roll_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[roll.track]");
            p += size;
        }

        void chunk_key_color_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[color.track]");
            p += size;
        }

        void chunk_key_morph_track(LittleEndianConstPointer& p, u32 size)
        {
            // TODO
            //printLine(Print::Verbose, level * 2, "[morph.track]");
            p += size;
        }

        void chunk_key_node_id(LittleEndianConstPointer& p)
        {
            u16 id = p.read16();
            MANGO_UNREFERENCED(id);
        }

        // parser

        void parse_chunks(LittleEndianConstPointer& p)
        {
            u16 id = p.read16();
            u32 size = p.read32() - 6;

            const u8* end = p + size;

            ++level;

            switch (id)
            {
                case 0x0010: chunk_color_f32(p); break;
                case 0x0011: chunk_color_u8(p); break;
                case 0x0012: chunk_color_u8(p); break;
                case 0x0013: chunk_color_f32(p); break;
                case 0x0030: chunk_percent_u16(p); break;
                case 0x0031: chunk_percent_f32(p); break;
                case 0x0100: chunk_master_scale(p); break;

                case 0x4d4d: chunk_main(p); break;
                case 0x0002: chunk_version(p); break;

                case 0x3d3d: chunk_editor(p); break;
                case 0x3d3e: chunk_mesh_version(p); break;

                case 0xafff: chunk_material(p); break;
                case 0xa000: chunk_material_name(p); break;
                case 0xa010: chunk_material_ambient_color(p); break;
                case 0xa020: chunk_material_diffuse_color(p); break;
                case 0xa030: chunk_material_specular_color(p); break;
                case 0xa040: chunk_material_shininess(p); break;
                case 0xa041: chunk_material_shininess2(p); break;
                case 0xa050: chunk_material_transparency(p); break;
                case 0xa052: chunk_material_xpfall(p); break;
                case 0xa053: chunk_material_refblur(p); break;
                case 0xa081: chunk_material_twosided(p); break;
                case 0xa083: chunk_material_additive(p); break;
                case 0xa084: chunk_material_self_illum_pct(p); break;
                case 0xa085: chunk_material_wireframe(p); break;
                case 0xa087: chunk_material_wireframe_size(p); break;
                case 0xa100: chunk_material_shading(p); break;

                case 0xa200: chunk_texture_map1(p); break;
                case 0xa33a: chunk_texture_map2(p); break;
                case 0xa210: chunk_texture_opacity(p); break;
                case 0xa230: chunk_texture_bump(p); break;
                case 0xa204: chunk_texture_specular(p); break;
                case 0xa33c: chunk_texture_shininess(p); break;
                case 0xa33d: chunk_texture_self_illum(p); break;
                case 0xa220: chunk_texture_reflection(p); break;

                case 0xa300: chunk_texture_map_name(p); break;
                case 0xa351: chunk_texture_map_tiling(p); break;
                case 0xa353: chunk_texture_map_blur(p); break;
                case 0xa354: chunk_texture_map_uscale(p); break;
                case 0xa356: chunk_texture_map_vscale(p); break;
                case 0xa358: chunk_texture_map_uoffset(p); break;
                case 0xa35a: chunk_texture_map_voffset(p); break;
                case 0xa35c: chunk_texture_map_angle(p); break;

                case 0x4000: chunk_object(p); break;
                case 0x4100: chunk_mesh(p); break;
                case 0x4110: chunk_mesh_vertex_list(p); break;
                case 0x4111: chunk_mesh_flag_list(p); break;
                case 0x4120: chunk_mesh_face_list(p); break;
                case 0x4130: chunk_mesh_material_list(p); break;
                case 0x4140: chunk_mesh_mapping_list(p); break;
                case 0x4150: chunk_mesh_smoothing_list(p); break;
                case 0x4160: chunk_mesh_local_axis(p); break;
                case 0x4165: chunk_mesh_visible(p); break;

                case 0x4700: chunk_camera(p); break;
                case 0x4720: chunk_camera_range(p); break;

                case 0x4600: chunk_light(p); break;
                case 0x4610: chunk_light_spot(p); break;
                case 0x4620: chunk_light_off(p); break;
                case 0x4656: chunk_light_spot_roll(p); break;
                case 0x4659: chunk_light_inner_range(p); break;
                case 0x465a: chunk_light_outer_range(p); break;
                case 0x465b: chunk_light_brightness(p); break;

                /*
                case 0xb000: chunk_keyframer(p); break;
                case 0xb001: chunk_key_ambient(p); break;
                case 0xb002: chunk_key_object(p); break;
                case 0xb003: chunk_key_camera(p); break;
                case 0xb004: chunk_key_camera_target(p); break;
                case 0xb005: chunk_key_light(p); break;
                case 0xb006: chunk_key_light_target(p); break;
                case 0xb007: chunk_key_spotlight(p); break;
                case 0xb008: chunk_key_segment(p); break;
                case 0xb009: chunk_key_current_time(p); break;
                case 0xb00a: chunk_key_header(p, size); break;
                case 0xb010: chunk_key_node_header(p); break;
                case 0xb011: chunk_key_dummy_object(p); break;
                case 0xb013: chunk_key_pivot(p); break;
                case 0xb014: chunk_key_boundbox(p); break;
                case 0xb015: chunk_key_morph_smooth(p, size); break;
                case 0xb020: chunk_key_pos_track(p, size); break;
                case 0xb021: chunk_key_rot_track(p, size); break;
                case 0xb022: chunk_key_scale_track(p, size); break;
                case 0xb023: chunk_key_fov_track(p, size); break;
                case 0xb024: chunk_key_roll_track(p, size); break;
                case 0xb025: chunk_key_color_track(p, size); break;
                case 0xb026: chunk_key_morph_track(p, size); break;
                case 0xb030: chunk_key_node_id(p); break;
                */

                default:
                    p += size;
                    printLine(Print::Verbose, level * 2, "[* {:#06x} *] {} bytes", id, size);
                    break;
            }

            while (p < end)
            {
                parse_chunks(p);
            }

            --level;
        }
    };

    void unwrapTexcoord(float& a, float& b, float& c)
    {
        float s0 = std::min({a, b, c});
        float s1 = std::max({a, b, c});
        float d = s1 - s0;
        if (d > 0.8f)
        {
            d = std::ceil(d);
            a += (a < 0.5f) * d;
            b += (b < 0.5f) * d;
            c += (c < 0.5f) * d;
        }
    }

    void fixTexcoordWrapping(Triangle& triangle, u32 flags)
    {
        enum
        {
            WRAP_U = 0x0008,
            WRAP_V = 0x0010,
        };

        if (flags & WRAP_U)
        {
            float& x0 = triangle.vertex[0].texcoord.x;
            float& x1 = triangle.vertex[1].texcoord.x;
            float& x2 = triangle.vertex[2].texcoord.x;
            unwrapTexcoord(x0, x1, x2);
        }

        if (flags & WRAP_V)
        {
            float& y0 = triangle.vertex[0].texcoord.y;
            float& y1 = triangle.vertex[1].texcoord.y;
            float& y2 = triangle.vertex[2].texcoord.y;
            unwrapTexcoord(y0, y1, y2);
        }
    }

} // namespace

namespace mango::import3d
{

    Import3DS::Import3DS(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        Reader3DS reader(file);

        for (auto& material3ds : reader.materials)
        {
            Material material;

            material.name = material3ds.name;

            material.baseColorFactor = material3ds.diffuse;
            material.twosided = material3ds.twosided;

            material.baseColorTexture = createTexture(path, material3ds.texture_map1.filename);
            material.emissiveTexture = createTexture(path, material3ds.texture_self_illum.filename);

            materials.push_back(material);
        }

        if (materials.empty())
        {
            Material material;
            materials.push_back(material);
        }

        for (const auto& mesh3ds : reader.meshes)
        {
            struct PositionCompare
            {
                bool operator () (const float32x3& a, const float32x3& b) const
                {
                    return std::memcmp(&a, &b, sizeof(float32x3)) < 0;
                }
            };

            std::multimap<float32x3, u32, PositionCompare> sharing;

            std::vector<Triangle> triangles;

            // resolve vertex position sharing between faces
            for (size_t i = 0; i < mesh3ds.faces.size(); ++i)
            {
                const Face3DS& face = mesh3ds.faces[i];

                // store face index into sharing map for all the positions
                sharing.emplace(mesh3ds.positions[face.index[0]], u32(i));
                sharing.emplace(mesh3ds.positions[face.index[1]], u32(i));
                sharing.emplace(mesh3ds.positions[face.index[2]], u32(i));
            }

            // compute vertex normals
            for (size_t i = 0; i < mesh3ds.faces.size(); ++i)
            {
                const Face3DS& face = mesh3ds.faces[i];

                Triangle triangle;

                for (int j = 0; j < 3; ++j)
                {
                    u32 index = face.index[j];

                    Vertex& vertex = triangle.vertex[j];

                    vertex.position = mesh3ds.positions[index];
                    vertex.texcoord = mesh3ds.mappings.size() > index ? mesh3ds.mappings[index] : 0;

                    float32x3 normal = 0;

                    // lookup all faces sharing the position
                    auto share = sharing.equal_range(vertex.position);

                    for (auto s = share.first; s != share.second; ++s)
                    {
                        const Face3DS& shared = mesh3ds.faces[s->second];
                        if (!face.smoothing || (face.smoothing & shared.smoothing))
                        {
                            normal += mesh3ds.computeNormal(shared);
                        }
                    }

                    vertex.normal = normalize(normal);
                }

                // fix texcoord wrapping
                fixTexcoordWrapping(triangle, face.flags);

                triangles.push_back(triangle);
            }

            // process material lists

            std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
            IndexedMesh& mesh = *ptr;

            for (const auto& primitive3ds : mesh3ds.primitives)
            {
                Mesh trimesh;

                trimesh.flags = Vertex::POSITION | Vertex::NORMAL | Vertex::TEXCOORD;

                for (size_t i = primitive3ds.start; i < primitive3ds.end; ++i)
                {
                    u16 idx = mesh3ds.face_materials[i];
                    trimesh.triangles.push_back(triangles[idx]);
                }

                mesh.append(trimesh, primitive3ds.material);
            }

            meshes.push_back(std::move(ptr));
        }

        // NOTE: we don't care about hierarchy in the .3ds scene

        Node root;

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            u32 index = u32(i);

            Node node;
            node.mesh = index;

            nodes.push_back(node);
            root.children.push_back(index);
        }

        nodes.push_back(root);
        roots.push_back(u32(meshes.size()));
    }

} // namespace mango::import3d
