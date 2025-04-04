/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <cmath>
#include <mango/math/geometry.hpp>

namespace mango::math
{

    // ------------------------------------------------------------------
    // LineSegment
    // ------------------------------------------------------------------

    float32x3 LineSegment::closest(const float32x3& point) const
    {
        const float32x3 v = position[0] - position[1];
        float time = dot(v, point - position[0]);
        if (time <= 0)
            return position[0];

        const float sqrlen = square(v);
        if (time >= sqrlen)
            return position[1];

        return position[0] + (v * (time / sqrlen));
    }

    float LineSegment::distance(const float32x3& point) const
    {
        const float32x3 v = position[1] - position[0];
        const float32x3 u = point - position[0];

        const float c1 = dot(u, v);
        if (c1 <= 0)
            return length(u);

        const float c2 = dot(v, v);
        if (c2 <= c1)
            return length(point - position[1]);

        return length(point - (position[0] + v * (c1 / c2)));
    }

    // ------------------------------------------------------------------
    // Ray
    // ------------------------------------------------------------------

    float Ray::distance(const float32x3& point) const
    {
        const float32x3 u = point - origin;
        const float c1 = dot(u, direction);
        const float c2 = dot(direction, direction);

        return length(point - (origin + direction * (c1 / c2)));
    }

    // ------------------------------------------------------------------
    // FastRay
    // ------------------------------------------------------------------

    FastRay::FastRay(const Ray& ray)
        : Ray(ray.origin, ray.direction)
    {
        dot_od = dot(origin, direction);
        dot_oo = dot(origin, origin);
        invdir = 1.0f / direction;

        sign.x = invdir.x < 0;
        sign.y = invdir.y < 0;
        sign.z = invdir.z < 0;
    }

    // ------------------------------------------------------------------
    // Rectangle
    // ------------------------------------------------------------------

    float Rectangle::aspect() const
    {
        return size.x / size.y;
    }

    bool Rectangle::inside(const float32x2& point) const
    {
        const float32x2 d = position - point;

        if (d.x < 0 || d.x >= size.x)
            return false;

        if (d.y < 0 || d.y >= size.y)
            return false;

        return true;
    }

    // ------------------------------------------------------------------
    // Box
    // ------------------------------------------------------------------

    float32x3 Box::center() const
    {
        return (corner[0] + corner[1]) * 0.5f;
    }

    float32x3 Box::size() const
    {
        return corner[1] - corner[0];
    }

    bool Box::empty() const
    {
        return
            corner[0].x >= corner[1].x ||
            corner[0].y >= corner[1].y ||
            corner[0].z >= corner[1].z;
    }

    void Box::extend(const float32x3& point)
    {
        corner[0] = min(corner[0], point);
        corner[1] = max(corner[1], point);
    }

    void Box::extend(const Box& box)
    {
        corner[0] = min(corner[0], box.corner[0]);
        corner[1] = max(corner[1], box.corner[1]);
    }

    bool Box::inside(const float32x3& point) const
    {
        return corner[0].x <= point.x && corner[1].x > point.x &&
               corner[0].y <= point.y && corner[1].y > point.y &&
               corner[0].z <= point.z && corner[1].z > point.z;
    }

    float32x3 Box::vertex(size_t index) const
    {
        assert(index >= 0 && index < 8);
        float x = corner[(index >> 0) & 1].x;
        float y = corner[(index >> 1) & 1].y;
        float z = corner[(index >> 2) & 1].z;
        return float32x3(x, y, z);
    }

    void Box::vertices(float32x3 vertex[8]) const
    {
        vertex[0] = float32x3(corner[0].x, corner[0].y, corner[0].z);
        vertex[1] = float32x3(corner[1].x, corner[0].y, corner[0].z);
        vertex[2] = float32x3(corner[0].x, corner[1].y, corner[0].z);
        vertex[3] = float32x3(corner[1].x, corner[1].y, corner[0].z);
        vertex[4] = float32x3(corner[0].x, corner[0].y, corner[1].z);
        vertex[5] = float32x3(corner[1].x, corner[0].y, corner[1].z);
        vertex[6] = float32x3(corner[0].x, corner[1].y, corner[1].z);
        vertex[7] = float32x3(corner[1].x, corner[1].y, corner[1].z);
    }

    // ------------------------------------------------------------------
    // Sphere
    // ------------------------------------------------------------------

    void Sphere::circumscribe(const Box& box)
    {
        float32x3 size = box.size();
        center = box.center();
        radius = float(std::sqrt(dot(size, size)) * 0.5f);
    }

    bool Sphere::inside(const float32x3& point) const
    {
        return square(point - center) < (radius * radius);
    }

    // ------------------------------------------------------------------
    // Triangle
    // ------------------------------------------------------------------

    float32x3 Triangle::normal() const
    {
        const float32x3 n = cross(position[1] - position[0], position[2] - position[0]);
        return normalize(n);
    }

    bool Triangle::barycentric(float32x3& result, const float32x3& point) const
    {
        const float32x3 v0 = position[1] - position[0]; // tag. cache
        const float32x3 v1 = position[2] - position[0]; // tag. cache
        const float32x3 v2 = point - position[0];

        float d01 = dot(v0, v1); // tag. cache
        float d11 = dot(v1, v1); // tag. cache
        float d20 = dot(v2, v0);
        float d21 = dot(v2, v1);

        float x = d20 * d11 - d21 * d01;
        if (x < 0)
            return false;

        float d00 = dot(v0, v0); // tag. cache
        float y = d21 * d00 - d20 * d01;
        if (y < 0)
            return false;

        float denom = d00 * d11 - d01 * d01; // tag. cache
        const bool inside = (x + y) <= denom;
        if (inside)
        {
            denom = 1.0f / denom; // tag. cache
            float v = x * denom;
            float w = y * denom;
            float u = 1.0f - v - w;

            result.x = u;
            result.y = v;
            result.z = w;
        }

        return inside;
    }

    // ------------------------------------------------------------------
    // TexTriangle
    // ------------------------------------------------------------------

    Matrix3x3 TexTriangle::tbn() const
    {
        const float32x3 p1 = position[1] - position[0];
        const float32x3 p2 = position[2] - position[0];
        const float32x2 t1 = texcoord[1] - texcoord[0];
        const float32x2 t2 = texcoord[2] - texcoord[0];

        float s = t1.x * t2.y - t1.y * t2.x;
        if (s)
        {
            s = 1.0f / s;
        }

        const float t2y = t2.y * s;
        const float t1y = t1.y * s;
        const float t2x = t2.x * -s;
        const float t1x = t1.x * -s;

        Matrix3x3 tbn;

        tbn[0] = normalize(p1 * t2y - p2 * t1y); // tangent
        tbn[1] = normalize(p1 * t2x + p2 * t1x); // binormal
        tbn[2] = normalize(cross(p1, p2)); // normal

        return tbn;
    }

    // ------------------------------------------------------------------
    // Frustum
    // ------------------------------------------------------------------

    Frustum::Frustum(const Matrix4x4& viewProject)
    {
        const Matrix4x4 m = transpose(viewProject);

        clip[0] = m[3] + m[0];
        clip[1] = m[3] - m[0];
        clip[2] = m[3] + m[1];
        clip[3] = m[3] - m[1];
        clip[4] = m[3] + m[2];
        clip[5] = m[3] - m[2];
    }

    bool Frustum::isVisible(const Box& box) const
    {
        for (int i = 0; i < 6; ++i)
        {
            float32x4 position;

            position.x = clip[i].x > 0 ? box.corner[1].x : box.corner[0].x;
            position.y = clip[i].y > 0 ? box.corner[1].y : box.corner[0].y;
            position.z = clip[i].z > 0 ? box.corner[1].z : box.corner[0].z;
            position.w = 1.0f;

            float32x4 s = dot(position, clip[i]);
            if (s.x < 0)
                return false;
        }
        
        return true;
    }

    RayFrustum::RayFrustum(const Matrix4x4& viewProject)
    {
        const Matrix4x4 m = transpose(viewProject);

        float32x4 clip[4];

        clip[0] = m[3] + m[0];
        clip[1] = m[3] - m[0];
        clip[2] = m[3] + m[1];
        clip[3] = m[3] - m[1];

        const float32x3 v0 = clip[0].xyz;
        const float32x3 v1 = clip[1].xyz;
        const float32x3 v2 = clip[2].xyz;
        const float32x3 v3 = clip[3].xyz;

        point[0] = cross(v0, v3);
        point[1] = cross(v3, v1);
        point[2] = cross(v2, v0);
        point[3] = cross(v1, v2);

        const float s = -1.0f / dot(v0, point[1]);
        const float32x3 temp = cross(v1, v0);

        const float d0 = m[3][3] - m[0][3];
        const float d1 = m[3][3] + m[0][3];
        const float d2 = m[3][3] - m[1][3];

        origin = (point[0] * d0 + point[1] * d1 + temp * d2) * s;
    }

    Ray RayFrustum::ray(float x, float y) const
    {
        const float32x3 x0 = lerp(point[0], point[2], y);
        const float32x3 x1 = lerp(point[1], point[3], y);
        const float32x3 p = lerp(x0, x1, x);
        return Ray(origin, normalize(p - origin));
    }

    // ------------------------------------------------------------------
    // Intersect
    // ------------------------------------------------------------------

    bool Intersect::intersect(const Ray& ray, const Plane& plane)
    {
        const float ndot = plane.dist - dot(ray.origin, plane.normal);
        if (ndot > 0)
            return false; // culled

        const float vdot = dot(ray.direction, plane.normal);
        if (vdot == 0)
            return false; // parallel

        t0 = ndot / vdot;
        return t0 > 0;
    }

    bool Intersect::intersect(const Ray& ray, const Sphere& sphere)
    {
        const float32x3 dep = ray.origin - sphere.center;
        const float b = dot(dep, ray.direction);
        const float c = square(dep) - sphere.radius * sphere.radius;

        // culling
        if (c > 0 && b > 0)
            return false;

        float d = b * b - c;
        if (d < 0)
            return false;

        t0 = -b - std::sqrt(d);
        return t0 >= 0;
    }

    bool Intersect::intersect(const Ray& ray, const Triangle& triangle)
    {
        const float32x3* vertex = triangle.position;

        // Intersect the ray with plane
        Plane plane(vertex[0], vertex[1], vertex[2]);
        Intersect is;
        if (!is.intersect(ray, plane))
            return false;

        // Intersection point
        float32x3 point = ray.origin + ray.direction * is.t0;

        // Test if intersection point is inside the triangle
        float32x3 normal;

        // edge 1
        normal = cross(ray.origin - vertex[1], ray.origin - vertex[0]);
        if (dot(point, normal) < dot(ray.origin, normal))
            return false;

        // edge 2
        normal = cross(ray.origin - vertex[2], ray.origin - vertex[1]);
        if (dot(point, normal) < dot(ray.origin, normal))
            return false;

        // edge 3
        normal = cross(ray.origin - vertex[0], ray.origin - vertex[2]);
        if (dot(point, normal) < dot(ray.origin, normal))
            return false;

        t0 = is.t0;
        return true;
    }

    // ------------------------------------------------------------------
    // IntersectRange
    // ------------------------------------------------------------------

    bool IntersectRange::intersect(const Ray& ray, const Box& box)
    {
        float32x3 s0 = (box.corner[0] - ray.origin) / ray.direction;
        float32x3 s1 = (box.corner[1] - ray.origin) / ray.direction;

        float tmin = std::min(s0.x, s1.x);
        float tmax = std::max(s0.x, s1.x);

        tmin = std::max(tmin, std::min({ s0.y, s1.y, tmax }));
        tmax = std::min(tmax, std::max({ s0.y, s1.y, tmin }));

        t0 = std::max(tmin, std::min({ s0.z, s1.z, tmax }));
        t1 = std::min(tmax, std::max({ s0.z, s1.z, tmin }));

        return t1 > std::max(t0, 0.0f);
    }

    bool IntersectRange::intersect(const FastRay& ray, const Box& box)
    {
        // Implementation based on a paper by:
        // Amy Williams, Steve Barrus, R. Keith Morley and Peter Shirley

        float tmin = (box.corner[0 + ray.sign.x].x - ray.origin.x) * ray.invdir.x;
        float ymax = (box.corner[1 - ray.sign.y].y - ray.origin.y) * ray.invdir.y;
        if (tmin > ymax)
            return false;

        float tmax = (box.corner[1 - ray.sign.x].x - ray.origin.x) * ray.invdir.x;
        float ymin = (box.corner[0 + ray.sign.y].y - ray.origin.y) * ray.invdir.y;
        if (tmax < ymin)
            return false;

        tmin = std::max(tmin, ymin);
        tmax = std::min(tmax, ymax);

        float zmin = (box.corner[0 + ray.sign.z].z - ray.origin.z) * ray.invdir.z;
        if (tmax < zmin)
            return false;

        float zmax = (box.corner[1 - ray.sign.z].z - ray.origin.z) * ray.invdir.z;
        if (tmin > zmax)
            return false;

        tmin = std::max(tmin, zmin);
        tmax = std::min(tmax, zmax);

        t0 = tmin;
        t1 = tmax;
        return t1 > std::max(t0, 0.0f);
    }

    bool IntersectRange::intersect(const FastRay& ray, const Sphere& sphere)
    {
        float b = -(ray.dot_od - dot(sphere.center, ray.direction));
        float c = ray.dot_oo + dot(sphere.center, sphere.center) - 2.0f * dot(ray.origin, sphere.center) - sphere.radius * sphere.radius;

        const float det = b * b - c;

        if (det >= 0)
        {
            const float sd = float(std::sqrt(det));
            t0 = b + sd;
            t1 = b - sd;

            if (t0 > t1)
            {
                std::swap(t0, t1);
            }

            return t1 > std::max(t0, 0.0f);
        }

        return false;
    }

    // ------------------------------------------------------------------
    // IntersectSolid
    // ------------------------------------------------------------------

    bool IntersectSolid::intersect(const Ray& ray, const Box& box)
    {
        IntersectRange is;
        bool s = is.intersect(ray, box);
        if (s)
            t0 = std::max(is.t0, 0.0f);
        return s;
    }

    bool IntersectSolid::intersect(const FastRay& ray, const Box& box)
    {
        IntersectRange is;
        bool s = is.intersect(ray, box);
        if (s)
            t0 = std::max(is.t0, 0.0f);
        return s;
    }

    bool IntersectSolid::intersect(const FastRay& ray, const Sphere& sphere)
    {
        IntersectRange is;
        bool s = is.intersect(ray, sphere);
        if (s)
            t0 = std::max(is.t0, 0.0f);
        return s;
    }

    // ------------------------------------------------------------------
    // IntersectBarycentric
    // ------------------------------------------------------------------

    bool IntersectBarycentric::intersect(const Ray& ray, const Triangle& triangle)
    {
        // Based on article by Tomas Möller
        // Fast, Minimum Storage Ray-Triangle Intersection

        float32x3 edge1 = triangle.position[1] - triangle.position[0];
        float32x3 edge2 = triangle.position[2] - triangle.position[0];

        float32x3 pvec = cross(ray.direction, edge2);
        float det = dot(edge1, pvec);

        const float epsilon = 0.000001f;
        if (det < epsilon)
            return false;

        float32x3 tvec = ray.origin - triangle.position[0];
        v = dot(tvec, pvec);
        if (v < 0 || v > det)
            return false;

        float32x3 qvec = cross(tvec, edge1);
        w = dot(ray.direction, qvec);
        if (w < 0 || (v + w) > det)
            return false;

        det = 1.0f / det;

        t0 = dot(edge2, qvec) * det;
        w *= det;
        v *= det;
        u = 1.0f - w - v;

        return true;
    }

    bool IntersectBarycentricTwosided::intersect(const Ray& ray, const Triangle& triangle)
    {
        // Based on article by Tomas Möller
        // Fast, Minimum Storage Ray-Triangle Intersection

        float32x3 edge1 = triangle.position[1] - triangle.position[0];
        float32x3 edge2 = triangle.position[2] - triangle.position[0];

        float32x3 pvec = cross(ray.direction, edge2);
        float det = dot(edge1, pvec);

        const float epsilon = 0.000001f;
        if (std::fabs(det) < epsilon)
            return false;

        det = 1.0f / det;

        float32x3 tvec = ray.origin - triangle.position[0];
        v = dot(tvec, pvec) * det;
        if (v < 0 || v > 1.0f)
            return false;

        float32x3 qvec = cross(tvec, edge1);
        w = dot(ray.direction, qvec) * det;
        if (w < 0 || (v + w) > 1.0f)
            return false;

        t0 = dot(edge2, qvec) * det;
        u = 1.0f - (v + w);

        return true;
    }

    // ------------------------------------------------------------------
    // intersect()
    // ------------------------------------------------------------------

    bool intersect(Rectangle& result, const Rectangle& rect0, const Rectangle& rect1)
    {
        // Trivial reject
        const float32x2 pos0 = rect0.position + rect0.size;
        if (pos0.x <= rect1.position.x || pos0.y <= rect1.position.y)
            return false;

        const float32x2 pos1 = rect1.position + rect1.size;
        if (pos1.x <= rect0.position.x || pos1.y <= rect0.position.y)
            return false;

        // Compute intersection
        const float32x2 p0 = max(rect0.position, rect1.position);
        const float32x2 p1 = min(pos0, pos1);
        if (p0.x >= p1.x || p0.y >= p1.y)
            return false;

        result.position = p0;
        result.size = p1 - p0;

        return true;
    }

    bool intersect(Ray& result, const Plane& plane0, const Plane& plane1)
    {
        float32x3 direction = cross(plane0.normal, plane1.normal);

        // Parallel planes
        float s = square(direction);
        if (s < 0.000001f)
            return false;

        // Compute intersection
        result.origin = (plane0.dist * cross(plane1.normal, direction) + plane1.dist * cross(direction, plane0.normal)) / s;
        result.direction = direction / std::sqrt(s);

        return true;
    }

    bool intersect(float32x3& result, const Plane& plane0, const Plane& plane1, const Plane& plane2)
    {
        // Determinant
        float32x3 cp01 = cross(plane0.normal, plane1.normal);

        // Parallel planes
        float det = dot(cp01, plane2.normal);
        if (det < 0.001f)
            return false;

        // Compute intersection
        float32x3 cp12 = cross(plane1.normal, plane2.normal) * plane0.dist;
        float32x3 cp20 = cross(plane2.normal, plane0.normal) * plane1.dist;

        result = (cp12 + cp20 + plane2.dist * cp01) / det;

        return true;
    }

    bool intersect(const Sphere& sphere, const Box& box)
    {
        float s = 0;

        for (int i = 0; i < 3; ++i)
        {
            const float c = sphere.center[i];

            if (c < box.corner[0][i])
            {
                float delta = c - box.corner[0][i];
                s += delta * delta;
            }
            else if (c > box.corner[1][i])
            {
                float delta = c - box.corner[1][i];
                s += delta * delta;
            }
        }

        return s <= sphere.radius * sphere.radius;
    }

    bool intersect(const Cone& cone, const Sphere& sphere)
    {
        // Test if cone vertex is in sphere
        float32x3 diff = sphere.center - cone.origin;

        float r2 = sphere.radius * sphere.radius;
        float len2 = dot(diff, diff);
        float len2mr2 = len2 - r2;
        if (len2mr2 <= 0)
            return true;

        // Test if sphere center is in cone
        float dot1 = dot(cone.target, diff);
        float dot2 = dot1 * dot1;
        float cs = float(std::cos(cone.angle));
        float cs2 = cs * cs;
        if (dot2 >= len2 * cs2)
            return true;

        float ulen  = float(std::sqrt(std::abs(len2 - dot2)));
        float sn    = float(std::sin(cone.angle));
        float test  = cs * dot1 + sn * ulen;
        float discr = test * test - len2mr2;

        return discr >= 0 && test >= 0;
    }

} // namespace mango::math
