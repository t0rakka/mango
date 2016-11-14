/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cassert>
#include "math.hpp"

namespace mango
{

    // ------------------------------------------------------------------
    // Rectangle
    // ------------------------------------------------------------------

    struct Rectangle
    {
        float2 position;
        float2 size;

        Rectangle()
        {
        }

        Rectangle(float x, float y, float width, float height)
        : position(x, y), size(width, height)
        {
        }

        Rectangle(const float2& _position, const float2& _size)
        : position(_position), size(_size)
        {
        }

        ~Rectangle()
        {
        }

        float aspect() const;
        bool inside(const float2& point) const;
    };

    // ------------------------------------------------------------------
    // Plane
    // ------------------------------------------------------------------

    struct Plane
    {
        float3 normal;
        float dist;

        Plane()
        {
        }

        Plane(const float3& _normal, float _dist)
        : normal(_normal), dist(_dist)
        {
        }

        Plane(const float3& _normal, const float3& point)
        : normal(_normal)
        {
            dist = dot(normal, point);
        }

        Plane(const float3& point0, const float3& point1, const float3& point2)
        {
            normal = normalize(cross(point1 - point0, point2 - point0));
            dist = dot(point0, normal);
        }

        Plane(float x, float y, float z, float w)
        : normal(x, y, z), dist(w)
        {
        }

        ~Plane()
        {
        }

        operator float4 () const
        {
            return float4(normal, dist);
        }

        float distance(const float3& point) const
        {
            return dot(normal, point) - dist;
        }
    };

    // ------------------------------------------------------------------
    // Box
    // ------------------------------------------------------------------

    struct Box
    {
        float3 corner[2];

        Box()
        {
            const float s = std::numeric_limits<float>::max();
            corner[0] = float3(s, s, s);
            corner[1] = float3(-s, -s, -s);
        }

        Box(const float3& point0, const float3& point1)
        {
            corner[0] = min(point0, point1);
            corner[1] = max(point0, point1);
        }

        Box(const Box& box0, const Box& box1)
        {
            corner[0] = min(box0.corner[0], box1.corner[0]);
            corner[1] = max(box0.corner[1], box1.corner[1]);
        }

        ~Box()
        {
        }

        float3 center() const;
        float3 size() const;
        void extend(const float3& point);
        void extend(const Box& box);
        bool inside(const float3& point) const;
        float3 vertex(int index) const;
        void vertices(float3 vertex[]) const;
    };

    // ------------------------------------------------------------------
    // Sphere
    // ------------------------------------------------------------------

    struct Sphere
    {
        float3 center;
        float radius;

        Sphere()
        {
        }

        Sphere(const float3& _center, float _radius)
        : center(_center), radius(_radius)
        {
        }

        ~Sphere()
        {
        }

        void circumscribe(const Box& box);
        bool inside(const float3& point) const;
    };

    // ------------------------------------------------------------------
    // Cone
    // ------------------------------------------------------------------

    struct Cone
    {
        float3 origin;
        float3 target;
        float angle;

        Cone()
        {
        }

        Cone(const float3& _origin, const float3& _target, float _angle)
        : origin(_origin), target(_target), angle(_angle)
        {
        }

        ~Cone()
        {
        }
    };

    // ------------------------------------------------------------------
    // Line
    // ------------------------------------------------------------------

    struct Line
    {
        float3 position[2];

        Line()
        {
        }

        Line(const float3& position0, const float3& position1)
        {
            position[0] = position0;
            position[1] = position1;
        }

        ~Line()
        {
        }

        float3 direction() const
        {
            return position[1] - position[0];
        }

        float3 closest(const float3& point) const;
        float distance(const float3& point) const;
    };

    // ------------------------------------------------------------------
    // Triangle
    // ------------------------------------------------------------------

    struct Triangle
    {
        float3 position[3];

        Triangle()
        {
        }

        Triangle(const float3& point0, const float3& point1, const float3& point2)
        {
            position[0] = point0;
            position[1] = point1;
            position[2] = point2;
        }

        ~Triangle()
        {
        }

        float3 normal() const;
        bool barycentric(float3& result, const float3& point) const;
    };

    // ------------------------------------------------------------------
    // TexTriangle
    // ------------------------------------------------------------------

    struct TexTriangle : Triangle
    {
        float2 texcoord[3];

        TexTriangle()
        {
        }

        ~TexTriangle()
        {
        }

        float3x3 tbn() const;
    };

    // ------------------------------------------------------------------
    // Ray
    // ------------------------------------------------------------------

    struct Ray
    {
        float3 origin;
        float3 direction;

        Ray()
        {
        }

        Ray(const float3& point0, const float3& point1)
        : origin(point0), direction(point1 - point0)
        {
        }

        Ray(const Line& line)
        : origin(line.position[0]), direction(line.direction())
        {
        }

        ~Ray()
        {
        }

        float distance(const float3& point) const;
    };

    // ------------------------------------------------------------------
    // FastRay
    // ------------------------------------------------------------------

    struct FastRay : Ray
    {
        float dotod;
        float dotoo;
        float3 invdir;
        int3 sign;

        FastRay(const Ray& ray)
        {
            origin = ray.origin;
            direction = ray.direction;

            dotod = dot(origin, direction);
            dotoo = dot(origin, origin);

            for (int i = 0; i < 3; ++i)
            {
                float d = direction[i] ? direction[i] : 0.00001f;
                invdir[i] = 1.0f / d;
                sign[i] = invdir[i] < 0;
            }
        }

        ~FastRay()
        {
        }
    };

    // ------------------------------------------------------------------
    // Frustum
    // ------------------------------------------------------------------

    struct Frustum
    {
        float3 point[4]; // 0: top_left, 1: top_right, 2: bottom_left, 3: bottom_right
        float3 origin;

        Frustum() = default;
        Frustum(const float4x4& m);
        ~Frustum() = default;

        Ray ray(float x, float y) const;
    };

    // ------------------------------------------------------------------
    // Intersect
    // ------------------------------------------------------------------

    struct Intersect
    {
        float t0;

	    bool intersect(const Line& line, const Plane& plane);
	    bool intersect(const Ray& ray, const Plane& plane);
	    bool intersect(const Ray& ray, const Triangle& triangle);
	    bool intersect(const Ray& ray, const Sphere& sphere);
    };

    struct IntersectRange
    {
        float t0;
        float t1;

	    bool intersect(const Ray& ray, const Box& box);
	    bool intersect(const FastRay& ray, const Sphere& sphere);
	    bool intersect(const FastRay& ray, const Box& box);
    };

    struct IntersectBarycentric
    {
        float t0;
        float u, v;

        bool intersect(const Ray& ray, const Triangle& triangle);
        bool intersect_cull(const Ray& ray, const Triangle& triangle);
    };

    bool intersect(Rectangle& result, const Rectangle& rect0, const Rectangle& rect1);
    bool intersect(Ray& result, const Plane& plane0, const Plane& plane1);
    bool intersect(float3& result, const Plane& plane0, const Plane& plane1, const Plane& plane2);
    bool intersect(const Sphere& sphere, const Box& box);
    bool intersect(const Cone& cone, const Sphere& sphere);

} // namespace mango
