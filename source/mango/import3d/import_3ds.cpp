/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/import3d/import_3ds.hpp>

#define debugPrint3DS(...) \
    printf("%*s", level * 2, " "); \
    debugPrintLine(__VA_ARGS__);

namespace
{
    using namespace mango;
    using namespace mango::math;

    struct Texture3DS
    {
        std::string filename;
    };

    struct Material3DS
    {
        std::string name;

        //ucolor ambient;
        //ucolor diffuse;
        //ucolor specular;

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
        enum
        {
            UWRAP = 0x0008,
            VWRAP = 0x0800,
        };

        u16 index[3];
        u16 flags = 0;
        u32 smoothing = 0;
        u32 material = 0;
    };

    struct Mesh3DS
    {
        std::vector<float32x3> positions;
        std::vector<float32x2> mappings;
        std::vector<Face3DS> faces;

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

        //ucolor color;
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
            float r = p.read32f();
            float g = p.read32f();
            float b = p.read32f();
            //color = ucolor(r, g, b, 0xff);
        }

        void chunk_color_u8(LittleEndianConstPointer& p)
        {
            //color = ucolor(p[0], p[1], p[2], 0xff);
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
            debugPrint3DS("master scale: %f", master_scale);
        }

        // main chunks

        void chunk_main(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[main]");
            parse_chunks(p);
        }

        void chunk_version(LittleEndianConstPointer& p)
        {
            u32 version = p.read32();
            if (version > 3)
            {
                MANGO_EXCEPTION("[Import3DS] Incorrect version: %d", version);
            }

            debugPrint3DS("version: %d", version);
        }

        // editor chunks

        void chunk_editor(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[editor]");
            parse_chunks(p);
        }

        void chunk_mesh_version(LittleEndianConstPointer& p)
        {
            u32 mesh_version = p.read32();
            debugPrint3DS("mesh version: %d", mesh_version);
        }

        // material chunks

        void chunk_material(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[material]");

            materials.emplace_back();
            parse_chunks(p);
        }

        void chunk_material_name(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.name = read_string(p);
            debugPrint3DS("name: \"%s\"", material.name.c_str());
        }

        void chunk_material_ambient_color(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            //material.ambient = color;
            //debugPrint3DS("ambient color: %d, %d, %d", color[0], color[1], color[2]);
        }

        void chunk_material_diffuse_color(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            //material.diffuse = color;
            //debugPrint3DS("diffuse color: %d, %d, %d", color[0], color[1], color[2]);
        }

        void chunk_material_specular_color(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            //material.specular = color;
            //debugPrint3DS("specular color: %d, %d, %d", color[0], color[1], color[2]);
        }

        void chunk_material_shininess(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.shininess = percent;
            debugPrint3DS("shininess: %f", percent);
        }

        void chunk_material_shininess2(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.shininess2 = percent;
            debugPrint3DS("shininess2: %f", percent);
        }

        void chunk_material_transparency(LittleEndianConstPointer& p)
        {
            parse_chunks(p);

            Material3DS& material = getCurrentMaterial();
            material.transparency = percent;
            debugPrint3DS("transparency: %f", percent);
        }

        void chunk_material_xpfall(LittleEndianConstPointer& p)
        {
            parse_chunks(p);
            debugPrint3DS("xpfall: %f", percent);
        }

        void chunk_material_refblur(LittleEndianConstPointer& p)
        {
            parse_chunks(p);
            debugPrint3DS("refblur: %f", percent);
        }

        void chunk_material_twosided(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.twosided = true;
            debugPrint3DS("+ twosided");
        }

        void chunk_material_additive(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.additive = true;
            debugPrint3DS("+ additive");
        }

        void chunk_material_self_illum_pct(LittleEndianConstPointer& p)
        {
            parse_chunks(p);
            debugPrint3DS("self illum: %f", percent);
        }

        void chunk_material_wireframe(LittleEndianConstPointer& p)
        {
            Material3DS& material = getCurrentMaterial();
            material.wireframe = true;
            debugPrint3DS("+ wireframe");
        }

        void chunk_material_wireframe_size(LittleEndianConstPointer& p)
        {
            float wireframe_size = p.read32f();
            debugPrint3DS("wireframe size: %f", percent);
        }

        void chunk_material_shading(LittleEndianConstPointer& p)
        {
            // 0 - wireframe
            // 1 - flat
            // 2 - gouraud
            // 3 - phong
            // 4 - metal
            u16 shading = p.read16();
            debugPrint3DS("shading: %d", shading);
        }

        // texture chunks

        void chunk_texture_map1(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:map1]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_map1;
        }

        void chunk_texture_map2(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:map2]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_map2;
        }

        void chunk_texture_opacity(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:opacity]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_opacity;
        }

        void chunk_texture_bump(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:bump]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_bump;
        }

        void chunk_texture_specular(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:specular]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_specular;
        }

        void chunk_texture_shininess(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:shininess]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_shininess;
        }

        void chunk_texture_self_illum(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:self-illum]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_self_illum;
        }

        void chunk_texture_reflection(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[texture:reflection]");
            Material3DS& material = getCurrentMaterial();
            texture = &material.texture_reflection;
        }

        void chunk_texture_map_name(LittleEndianConstPointer& p)
        {
            assert(texture != nullptr);
            texture->filename = read_string(p);
            debugPrint3DS("filename: \"%s\"", texture->filename.c_str());
        }

        void chunk_texture_map_tiling(LittleEndianConstPointer& p)
        {
            // 1 - tile
            // 2 - decal
            u16 tiling_flags = p.read16();
            debugPrint3DS("tiling: 0x%x", tiling_flags);
        }

        void chunk_texture_map_blur(LittleEndianConstPointer& p)
        {
            float blur = p.read32f();
            debugPrint3DS("blur: %f", blur);
        }

        void chunk_texture_map_uscale(LittleEndianConstPointer& p)
        {
            float uscale = p.read32f();
            debugPrint3DS("uscale: %f", uscale);
        }

        void chunk_texture_map_vscale(LittleEndianConstPointer& p)
        {
            float vscale = p.read32f();
            debugPrint3DS("vscale: %f", vscale);
        }

        void chunk_texture_map_uoffset(LittleEndianConstPointer& p)
        {
            float uoffset = p.read32f();
            debugPrint3DS("uoffset: %f", uoffset);
        }

        void chunk_texture_map_voffset(LittleEndianConstPointer& p)
        {
            float voffset = p.read32f();
            debugPrint3DS("voffset: %f", voffset);
        }

        void chunk_texture_map_angle(LittleEndianConstPointer& p)
        {
            float angle = p.read32f();
            debugPrint3DS("angle: %f", angle);
        }

        // object chunks

        void chunk_object(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[object]");
            std::string name = read_string(p);
            debugPrint3DS("  name: \"%s\"", name.c_str());

            parse_chunks(p);
        }

        // mesh chunks

        void chunk_mesh(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[mesh]");

            meshes.emplace_back();
            parse_chunks(p);
        }

        void chunk_mesh_vertex_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            debugPrint3DS("vertex.list: %d", count);

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
            debugPrint3DS("flag.list: %d", count);

            for (int i = 0; i < count; ++i)
            {
                u16 flags = p.read16();
                MANGO_UNREFERENCED(flags);
                // TODO: xxx
            }
        }

        void chunk_mesh_face_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            debugPrint3DS("face.list: %d", count);

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
            u32 material = getMaterialIndex(name);

            int count = p.read16();
            debugPrint3DS("material.list: %d, name: \"%s\"", count, name.c_str());

            Mesh3DS& mesh = getCurrentMesh();

            for (int i = 0; i < count; ++i)
            {
                u16 index = p.read16();
                mesh.faces[index].material = material;
            }
        }

        void chunk_mesh_mapping_list(LittleEndianConstPointer& p)
        {
            int count = p.read16();
            debugPrint3DS("mapping.list: %d", count);

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

            debugPrint3DS("smoothing.list: %d", count);

            for (size_t i = 0; i < count; ++i)
            {
                mesh.faces[i].smoothing = p.read32();
            }
        }

        void chunk_mesh_local_axis(LittleEndianConstPointer& p)
        {
            float xform[4][4];

            for (int i = 0; i < 4; ++i)
            {
                xform[0][i] = p.read32f();
                xform[1][i] = p.read32f();
                xform[2][i] = p.read32f();
            }

            // TODO: xxx
        }

        void chunk_mesh_visible(LittleEndianConstPointer& p)
        {
            u8 visible = *p++;
            MANGO_UNREFERENCED(visible);
            // TODO: xxx
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

            parse_chunks(p);
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
            parse_chunks(p);
            //ucolor color = color;
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

        void chunk_light_brightness(LittleEndianConstPointer& p)
        {
            float brightness = p.read32f();
            MANGO_UNREFERENCED(brightness);
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

        // keyframer chunks

        void chunk_keyframer(LittleEndianConstPointer& p)
        {
            debugPrint3DS("[keyframer]");
            parse_chunks(p);
        }

        void chunk_key_ambient(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_mesh(LittleEndianConstPointer& p)
        {
            // TODO
            // 173 bytes
        }

        void chunk_key_camera(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_target(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_light(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_light_target(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_spotlight(LittleEndianConstPointer& p)
        {
            // TODO
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
        }

        void chunk_key_header(LittleEndianConstPointer& p)
        {
            // TODO
            // 7 bytes
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
            MANGO_UNREFERENCED(pivot);
        }

        void chunk_key_boundbox(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_morph_smooth(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_pos_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_rot_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_scale_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_fov_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_roll_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_color_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_morph_track(LittleEndianConstPointer& p)
        {
            // TODO
        }

        void chunk_key_node_id(LittleEndianConstPointer& p)
        {
            u16 id = p.read32();
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
                case 0x465b: chunk_light_brightness(p); break;
                case 0x4659: chunk_light_inner_range(p); break;
                case 0x465a: chunk_light_outer_range(p); break;

                case 0xb000: chunk_keyframer(p); break;
                /*
                case 0xb001: chunk_key_ambient(p); break;
                case 0xb002: chunk_key_mesh(p); break;
                case 0xb003: chunk_key_camera(p); break;
                case 0xb004: chunk_key_target(p); break;
                case 0xb005: chunk_key_light(p); break;
                case 0xb006: chunk_key_light_target(p); break;
                case 0xb007: chunk_key_spotlight(p); break;
                */
                case 0xb008: chunk_key_segment(p); break;
                /*
                case 0xb009: chunk_key_current_time(p); break;
                case 0xb00a: chunk_key_header(p); break;
                */
                case 0xb010: chunk_key_node_header(p); break;
                case 0xb011: chunk_key_dummy_object(p); break;
                case 0xb013: chunk_key_pivot(p); break;
                /*
                case 0xb014: chunk_key_boundbox(p); break;
                case 0xb015: chunk_key_morph_smooth(p); break;
                case 0xb020: chunk_key_pos_track(p); break;
                case 0xb021: chunk_key_rot_track(p); break;
                case 0xb022: chunk_key_scale_track(p); break;
                case 0xb023: chunk_key_fov_track(p); break;
                case 0xb024: chunk_key_roll_track(p); break;
                case 0xb025: chunk_key_color_track(p); break;
                case 0xb026: chunk_key_morph_track(p); break;
                */
                case 0xb030: chunk_key_node_id(p); break;

                default:
                    p += size;
                    debugPrint3DS("[* 0x%.4x *] %d bytes", id, size);
                    break;
            }

            while (p < end)
            {
                parse_chunks(p);
            }

            --level;
        }
    };

    void texcoord_unwrap(float& a, float& b, float& c)
    {
        float smin = std::min(std::min(a, b), c);
        float smax = std::max(std::max(a, b), c);
        float d = smax - smin;
        if (d > 0.8f)
        {
            d = std::ceil(d);
            a += (a < 0.5f) * d;
            b += (b < 0.5f) * d;
            c += (c < 0.5f) * d;
        }
    }

    void texcoord_wrapping(import3d::Triangle& triangle, u32 flags)
    {
        if (flags & Face3DS::UWRAP)
        {
            texcoord_unwrap(triangle.vertex[0].texcoord.x,
                            triangle.vertex[1].texcoord.x,
                            triangle.vertex[2].texcoord.x);
        }

        if (flags & Face3DS::VWRAP)
        {
            texcoord_unwrap(triangle.vertex[0].texcoord.y,
                            triangle.vertex[1].texcoord.y,
                            triangle.vertex[2].texcoord.y);
        }
    }

} // namespace

namespace mango::import3d
{

    Import3DS::Import3DS(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        Reader3DS reader(file);

        debugPrintLine("materials: %d", int(reader.materials.size()));
        debugPrintLine("meshes: %d", int(reader.meshes.size()));

        /*
        for (auto& material3ds : reader.materials)
        {
            Material material;

            material.ambient = material3ds.ambient;
            material.diffuse = material3ds.diffuse;
            material.specular = material3ds.specular;
            material.texture = material3ds.texture_map1.filename;

            materials.push_back(material);
        }

        if (materials.empty())
        {
            Material material;

            material.ambient = ucolor(255, 255, 255, 255);
            material.diffuse = ucolor(255, 255, 255, 255);
            material.specular = ucolor(255, 255, 255, 255);

            materials.push_back(material);
        }
        */

        for (auto& mesh3ds : reader.meshes)
        {
            Mesh mesh;

            // NOTE: we must use the actual vertex positions and not the indices
            //       because meshes do not always share indices for shared positions
            struct compare
            {
                bool operator () (const float32x3& a, const float32x3& b) const
                {
                    return std::memcmp(&a, &b, sizeof(float32x3)) < 0;
                }
            };

            std::multimap<float32x3, u32, compare> sharing;

            // resolve vertex position sharing between faces
            for (size_t i = 0; i < mesh3ds.faces.size(); ++i)
            {
                Face3DS& face = mesh3ds.faces[i];

                // store face index into sharing map for all the positions
                sharing.emplace(mesh3ds.positions[face.index[0]], i);
                sharing.emplace(mesh3ds.positions[face.index[1]], i);
                sharing.emplace(mesh3ds.positions[face.index[2]], i);
            }

            // compute vertex normals
            for (size_t i = 0; i < mesh3ds.faces.size(); ++i)
            {
                Face3DS& face = mesh3ds.faces[i];

                import3d::Triangle triangle;

                //triangle.material = face.material;

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
                        Face3DS& shared = mesh3ds.faces[s->second];
                        if (!face.smoothing || (face.smoothing & shared.smoothing))
                        {
                            normal += mesh3ds.computeNormal(shared);
                        }
                    }

                    vertex.normal = normalize(normal);
                }

                // fix texcoord wrapping
                texcoord_wrapping(triangle, face.flags);

                mesh.triangles.push_back(triangle);
            }

            meshes.push_back(mesh);
        }
    }

} // namespace mango::import3d
