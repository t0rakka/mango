/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <mango/core/core.hpp>
#include <mango/math/quaternion.hpp>
#include <mango/import3d/import_bvh.hpp>

namespace
{
    using namespace mango;
    using namespace mango::import3d;
    using namespace mango::math;

    enum class BvhChannel : u8
    {
        Xposition,
        Yposition,
        Zposition,
        Xrotation,
        Yrotation,
        Zrotation,
    };

    struct BvhJointParse
    {
        std::string name;
        float32x3 offset { 0.0f, 0.0f, 0.0f };
        std::vector<BvhChannel> channels;
        int parent = -1;
    };

    static std::string toLowerCopy(std::string s)
    {
        for (char& c : s)
            c = char(std::tolower(u8(c)));
        return s;
    }

    static bool parseChannel(const std::string& token, BvhChannel& out)
    {
        const std::string t = toLowerCopy(token);
        if (t == "xposition") { out = BvhChannel::Xposition; return true; }
        if (t == "yposition") { out = BvhChannel::Yposition; return true; }
        if (t == "zposition") { out = BvhChannel::Zposition; return true; }
        if (t == "xrotation") { out = BvhChannel::Xrotation; return true; }
        if (t == "yrotation") { out = BvhChannel::Yrotation; return true; }
        if (t == "zrotation") { out = BvhChannel::Zrotation; return true; }
        return false;
    }

    static bool isPosition(BvhChannel c)
    {
        return c == BvhChannel::Xposition || c == BvhChannel::Yposition || c == BvhChannel::Zposition;
    }

    static bool isRotation(BvhChannel c)
    {
        return c == BvhChannel::Xrotation || c == BvhChannel::Yrotation || c == BvhChannel::Zrotation;
    }

    static const matrix4x4 axisReflect = matrix4x4::scale(1.0f, 1.0f, -1.0f);

    static float32x3 toOursPosition(float32x3 p)
    {
        return float32x3(p.x, p.y, -p.z);
    }

    static float32x4 toOursRotation(const Quaternion& qIn)
    {
        matrix4x4 R(qIn);
        matrix4x4 Rp = axisReflect * R * axisReflect;
        Quaternion q(Rp);
        if (q.w < 0.0f)
        {
            q.x = -q.x; q.y = -q.y; q.z = -q.z; q.w = -q.w;
        }
        return float32x4(q.x, q.y, q.z, q.w);
    }

    struct TokenReader
    {
        std::vector<std::string> tokens;
        size_t i = 0;

        explicit TokenReader(std::string_view text)
        {
            tokens.reserve(text.size() / 4);
            std::string cur;
            auto flush = [&] {
                if (!cur.empty())
                {
                    tokens.push_back(std::move(cur));
                    cur.clear();
                }
            };
            for (char c : text)
            {
                if (std::isspace(u8(c)))
                    flush();
                else
                    cur.push_back(c);
            }
            flush();
        }

        bool empty() const { return i >= tokens.size(); }
        const std::string& peek() const { return tokens[i]; }
        std::string next() { return tokens[i++]; }

        bool accept(const std::string& word)
        {
            if (!empty() && toLowerCopy(peek()) == toLowerCopy(word))
            {
                ++i;
                return true;
            }
            return false;
        }

        void expect(const std::string& word)
        {
            if (!accept(word))
                MANGO_EXCEPTION("[ImportBVH] Expected '{}'.", word);
        }

        float nextFloat()
        {
            if (empty())
                MANGO_EXCEPTION("[ImportBVH] Unexpected end of file (float).");
            return float(std::atof(next().c_str()));
        }

        int nextInt()
        {
            if (empty())
                MANGO_EXCEPTION("[ImportBVH] Unexpected end of file (int).");
            return int(std::atoi(next().c_str()));
        }
    };

    static int parseJoint(TokenReader& r, std::vector<BvhJointParse>& joints, int parent)
    {
        const int index = int(joints.size());
        BvhJointParse joint;
        joint.parent = parent;
        joint.name = r.next();
        joints.push_back(joint);

        r.expect("{");
        while (!r.empty() && !r.accept("}"))
        {
            if (r.accept("OFFSET"))
            {
                joints[index].offset.x = r.nextFloat();
                joints[index].offset.y = r.nextFloat();
                joints[index].offset.z = r.nextFloat();
            }
            else if (r.accept("CHANNELS"))
            {
                const int count = r.nextInt();
                joints[index].channels.reserve(size_t(count));
                for (int c = 0; c < count; ++c)
                {
                    BvhChannel ch;
                    const std::string token = r.next();
                    if (!parseChannel(token, ch))
                        MANGO_EXCEPTION("[ImportBVH] Unknown channel '{}'.", token);
                    joints[index].channels.push_back(ch);
                }
            }
            else if (r.accept("JOINT") || r.accept("ROOT"))
            {
                parseJoint(r, joints, index);
            }
            else if (r.accept("End"))
            {
                r.accept("Site");
                r.expect("{");
                while (!r.empty() && !r.accept("}"))
                {
                    if (r.accept("OFFSET"))
                    {
                        r.nextFloat();
                        r.nextFloat();
                        r.nextFloat();
                    }
                    else
                    {
                        r.next();
                    }
                }
            }
            else
            {
                r.next();
            }
        }
        return index;
    }

    static Animation buildAnimation(const std::string& name,
                                    const std::vector<BvhJointParse>& joints,
                                    int frameCount, float frameTime,
                                    const std::vector<float>& motion)
    {
        Animation animation;
        animation.name = name;
        animation.duration = frameCount > 0 ? frameTime * float(frameCount - 1) : 0.0f;

        size_t channelsPerFrame = 0;
        for (const BvhJointParse& j : joints)
            channelsPerFrame += j.channels.size();

        if (channelsPerFrame == 0 || frameCount <= 0)
            return animation;

        if (motion.size() < size_t(frameCount) * channelsPerFrame)
        {
            MANGO_EXCEPTION("[ImportBVH] Motion data truncated (need {} floats, got {}).",
                size_t(frameCount) * channelsPerFrame, motion.size());
        }

        std::vector<float> frameTimes;
        frameTimes.resize(size_t(frameCount));
        for (int f = 0; f < frameCount; ++f)
            frameTimes[size_t(f)] = frameTime * float(f);

        std::vector<size_t> jointMotionOffset(joints.size(), 0);
        {
            size_t cursor = 0;
            for (size_t j = 0; j < joints.size(); ++j)
            {
                jointMotionOffset[j] = cursor;
                cursor += joints[j].channels.size();
            }
        }

        constexpr float degToRad = 0.017453292519943295769f;

        for (size_t ji = 0; ji < joints.size(); ++ji)
        {
            const BvhJointParse& joint = joints[ji];

            bool hasPos = false;
            bool hasRot = false;
            for (BvhChannel ch : joint.channels)
            {
                hasPos = hasPos || isPosition(ch);
                hasRot = hasRot || isRotation(ch);
            }

            if (hasRot)
            {
                AnimationSampler sampler;
                sampler.interpolation = AnimationInterpolation::Linear;
                sampler.components = 4;
                sampler.times = frameTimes;
                sampler.values.resize(size_t(frameCount) * 4);

                for (int f = 0; f < frameCount; ++f)
                {
                    size_t c = size_t(f) * channelsPerFrame + jointMotionOffset[ji];
                    Quaternion q = Quaternion::identity();
                    for (BvhChannel ch : joint.channels)
                    {
                        const float value = motion[c++];
                        if (!isRotation(ch))
                            continue;
                        const float angle = value * degToRad;
                        Quaternion r;
                        switch (ch)
                        {
                            case BvhChannel::Xrotation: r = Quaternion::rotateX(angle); break;
                            case BvhChannel::Yrotation: r = Quaternion::rotateY(angle); break;
                            case BvhChannel::Zrotation: r = Quaternion::rotateZ(angle); break;
                            default: r = Quaternion::identity(); break;
                        }
                        q = q * r;
                    }

                    const float32x4 qOurs = toOursRotation(q);
                    sampler.values[size_t(f) * 4 + 0] = qOurs.x;
                    sampler.values[size_t(f) * 4 + 1] = qOurs.y;
                    sampler.values[size_t(f) * 4 + 2] = qOurs.z;
                    sampler.values[size_t(f) * 4 + 3] = qOurs.w;
                }

                AnimationChannel channel;
                channel.sampler = u32(animation.samplers.size());
                channel.targetName = joint.name;
                channel.path = AnimationPath::Rotation;
                animation.samplers.push_back(std::move(sampler));
                animation.channels.push_back(std::move(channel));
            }

            if (hasPos)
            {
                AnimationSampler sampler;
                sampler.interpolation = AnimationInterpolation::Linear;
                sampler.components = 3;
                sampler.times = frameTimes;
                sampler.values.resize(size_t(frameCount) * 3);

                for (int f = 0; f < frameCount; ++f)
                {
                    size_t c = size_t(f) * channelsPerFrame + jointMotionOffset[ji];
                    // Motion positions are absolute in file space; start from zero then fill.
                    float32x3 p(0.0f, 0.0f, 0.0f);
                    for (BvhChannel ch : joint.channels)
                    {
                        const float value = motion[c++];
                        switch (ch)
                        {
                            case BvhChannel::Xposition: p.x = value; break;
                            case BvhChannel::Yposition: p.y = value; break;
                            case BvhChannel::Zposition: p.z = value; break;
                            default: break;
                        }
                    }
                    p = toOursPosition(p);
                    sampler.values[size_t(f) * 3 + 0] = p.x;
                    sampler.values[size_t(f) * 3 + 1] = p.y;
                    sampler.values[size_t(f) * 3 + 2] = p.z;
                }

                AnimationChannel channel;
                channel.sampler = u32(animation.samplers.size());
                channel.targetName = joint.name;
                channel.path = AnimationPath::Translation;
                animation.samplers.push_back(std::move(sampler));
                animation.channels.push_back(std::move(channel));
            }
        }

        return animation;
    }

    static void parseBVH(ConstMemory memory, const std::string& name, Animation& outAnimation,
                         std::vector<import3d::BvhJoint>& outJoints)
    {
        std::string_view text(reinterpret_cast<const char*>(memory.address), memory.size);
        TokenReader r(text);

        std::vector<BvhJointParse> joints;
        r.expect("HIERARCHY");
        r.expect("ROOT");
        parseJoint(r, joints, -1);

        r.expect("MOTION");

        // Frames: N
        if (!r.accept("Frames:") && !r.accept("Frames"))
            MANGO_EXCEPTION("[ImportBVH] Expected 'Frames:'.");
        if (!r.empty() && r.peek() == ":")
            r.next();
        const int frameCount = r.nextInt();

        // Frame Time: dt
        if (!r.accept("Frame") && !r.accept("Frame:"))
            MANGO_EXCEPTION("[ImportBVH] Expected 'Frame Time:'.");
        r.accept("Time:");
        r.accept("Time");
        if (!r.empty() && r.peek() == ":")
            r.next();
        const float frameTime = r.nextFloat();

        std::vector<float> motion;
        const size_t motionReserve = frameCount > 0 ? size_t(frameCount) * 64 : 64;
        motion.reserve(motionReserve);
        while (!r.empty())
            motion.push_back(r.nextFloat());

        outAnimation = buildAnimation(name, joints, frameCount, frameTime, motion);

        outJoints.clear();
        outJoints.reserve(joints.size());
        for (const BvhJointParse& j : joints)
        {
            import3d::BvhJoint out;
            out.name = j.name;
            out.parent = j.parent;
            out.offset = toOursPosition(j.offset);
            outJoints.push_back(std::move(out));
        }

        printLine(Print::Info, "[BVH] \"{}\" joints={} frames={} dt={:.4f}s duration={:.3f}s channels={}",
            name, outJoints.size(), frameCount, frameTime, outAnimation.duration, outAnimation.channels.size());
    }
} // namespace

namespace mango::import3d
{

    ImportBVH::ImportBVH(ConstMemory memory, const std::string& name)
    {
        parseBVH(memory, name, animation, joints);
        jointNames.clear();
        jointNames.reserve(joints.size());
        for (const BvhJoint& j : joints)
            jointNames.push_back(j.name);
    }

    ImportBVH::ImportBVH(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        parseBVH(file, filesystem::removeExtension(filename), animation, joints);
        jointNames.clear();
        jointNames.reserve(joints.size());
        for (const BvhJoint& j : joints)
            jointNames.push_back(j.name);
    }

    Animation importAnimation(const std::string& filename)
    {
        return importBvhClip(filename).animation;
    }

    BvhClip importBvhClip(const std::string& filename)
    {
        std::string dir = filesystem::getPath(filename);
        if (dir.empty())
            dir = "./";

        const std::string file = filesystem::removePath(filename);
        const std::string ext = toLower(filesystem::getExtension(filename));
        const filesystem::Path path(dir);

        if (ext != ".bvh")
            MANGO_EXCEPTION("[import3d] Unsupported animation format '{}' (expected .bvh).", ext);

        ImportBVH bvh(path, file);
        BvhClip clip;
        clip.animation = std::move(bvh.animation);
        clip.joints = std::move(bvh.joints);
        return clip;
    }

} // namespace mango::import3d
