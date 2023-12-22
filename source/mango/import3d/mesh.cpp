/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

// Torus knot generation
// written by Jari Komppa aka Sol / Trauma
// Based on:
// http://www.blackpawn.com/texts/pqtorus/default.html

static
constexpr float pi2 = float(math::pi * 2.0);

Torusknot::Torusknot(Parameters params)
{
	params.thickness *= params.scale;

    // generate indices
    std::vector<int> stripIndices((params.steps + 1) * params.facets * 2);

	for (int j = 0; j < params.facets; j++)
	{
		for (int i = 0; i < params.steps + 1; i++)
		{
			stripIndices[i * 2 + 0 + j * (params.steps + 1) * 2] = (j + 1 + i * (params.facets + 1));
			stripIndices[i * 2 + 1 + j * (params.steps + 1) * 2] = (j + 0 + i * (params.facets + 1));
		}
	}

    // convert triangle strip into triangles
    for (size_t i = 2; i < stripIndices.size(); ++i)
    {
        int s = i & 1; // swap triangle winding-order
        indices.push_back(stripIndices[i]);
        indices.push_back(stripIndices[i - 1 - s]);
        indices.push_back(stripIndices[i - 2 + s]);
    }

    // generate vertices
    vertices.resize((params.steps + 1) * (params.facets + 1) + 1);

    float32x3 centerpoint;
    float Pp = params.p * 0 * pi2 / params.steps;
    float Qp = params.q * 0 * pi2 / params.steps;
    float r = (.5f * (2 + (float)sin(Qp))) * params.scale;
    centerpoint.x = r * (float)cos(Pp);
    centerpoint.y = r * (float)cos(Qp);
    centerpoint.z = r * (float)sin(Pp);

	for (int i = 0; i < params.steps; i++)
	{
		float32x3 nextpoint;
		Pp = params.p * (i + 1) * pi2 / params.steps;
		Qp = params.q * (i + 1) * pi2 / params.steps;
		r = (.5f * (2 + (float)sin(Qp))) * params.scale;
		nextpoint.x = r * (float)cos(Pp);
		nextpoint.y = r * (float)cos(Qp);
		nextpoint.z = r * (float)sin(Pp);

        float32x3 T = nextpoint - centerpoint;
        float32x3 N = nextpoint + centerpoint;
		float32x3 B = cross(T, N);
        N = cross(B, T);
        B = normalize(B);
        N = normalize(N);

		for (int j = 0; j < params.facets; j++)
		{
			float pointx = (float)sin(j * pi2 / params.facets) * params.thickness * (((float)sin(params.clumpOffset + params.clumps * i * pi2 / params.steps) * params.clumpScale) + 1);
			float pointy = (float)cos(j * pi2 / params.facets) * params.thickness * (((float)cos(params.clumpOffset + params.clumps * i * pi2 / params.steps) * params.clumpScale) + 1);

			const int offset = i * (params.facets + 1) + j;
			vertices[offset].position = N * pointx + B * pointy + centerpoint;
			vertices[offset].normal = normalize(vertices[offset].position - centerpoint);
			vertices[offset].texcoord = float32x2((float(j) / params.facets) * params.uscale, (float(i) / params.steps) * params.vscale);
		}

		// create duplicate vertex for sideways wrapping
		// otherwise identical to first vertex in the 'ring' except for the U coordinate
		vertices[i * (params.facets + 1) + params.facets].position = vertices[i * (params.facets + 1)].position;
		vertices[i * (params.facets + 1) + params.facets].normal = vertices[i * (params.facets + 1)].normal;
		vertices[i * (params.facets + 1) + params.facets].texcoord.x = params.uscale;
		vertices[i * (params.facets + 1) + params.facets].texcoord.y = vertices[i * (params.facets + 1)].texcoord.y;
        
        centerpoint = nextpoint;
	}

	// create duplicate ring of vertices for longways wrapping
	// otherwise identical to first 'ring' in the knot except for the V coordinate
	for (int j = 0; j < params.facets; j++)
	{
		vertices[params.steps * (params.facets + 1) + j].position = vertices[j].position;
		vertices[params.steps * (params.facets + 1) + j].normal = vertices[j].normal;

		vertices[params.steps * (params.facets + 1) + j].texcoord.x = vertices[j].texcoord.x;
		vertices[params.steps * (params.facets + 1) + j].texcoord.y = params.vscale;
	}

	// finally, there's one vertex that needs to be duplicated due to both U and V coordinate.
	vertices[params.steps * (params.facets + 1) + params.facets].position = vertices[0].position;
	vertices[params.steps * (params.facets + 1) + params.facets].normal = vertices[0].normal;
	vertices[params.steps * (params.facets + 1) + params.facets].texcoord = float32x2(params.uscale, params.vscale);
}

Cube::Cube(float size)
{
	const float s = size * 0.5f;

	vertices =
	{
		{ float32x3( s, s,-s), float32x3( 0,-1, 0), float32x2(0, 0) },
		{ float32x3(-s, s,-s), float32x3( 0,-1, 0), float32x2(1, 0) },
		{ float32x3(-s, s, s), float32x3( 0,-1, 0), float32x2(1, 1) },
		{ float32x3( s, s, s), float32x3( 0,-1, 0), float32x2(0, 1) },

		{ float32x3( s,-s, s), float32x3( 0, 1, 0), float32x2(0, 0) },
		{ float32x3(-s,-s, s), float32x3( 0, 1, 0), float32x2(1, 0) },
		{ float32x3(-s,-s,-s), float32x3( 0, 1, 0), float32x2(1, 1) },
		{ float32x3( s,-s,-s), float32x3( 0, 1, 0), float32x2(0, 1) },

		{ float32x3( s, s, s), float32x3( 0, 0,-1), float32x2(0, 0) },
		{ float32x3(-s, s, s), float32x3( 0, 0,-1), float32x2(1, 0) },
		{ float32x3(-s,-s, s), float32x3( 0, 0,-1), float32x2(1, 1) },
		{ float32x3( s,-s, s), float32x3( 0, 0,-1), float32x2(0, 1) },

		{ float32x3( s,-s,-s), float32x3( 0, 0, 1), float32x2(0, 0) },
		{ float32x3(-s,-s,-s), float32x3( 0, 0, 1), float32x2(1, 0) },
		{ float32x3(-s, s,-s), float32x3( 0, 0, 1), float32x2(1, 1) },
		{ float32x3( s, s,-s), float32x3( 0, 0, 1), float32x2(0, 1) },

		{ float32x3(-s, s, s), float32x3( 1, 0, 0), float32x2(0, 0) },
		{ float32x3(-s, s,-s), float32x3( 1, 0, 0), float32x2(1, 0) },
		{ float32x3(-s,-s,-s), float32x3( 1, 0, 0), float32x2(1, 1) },
		{ float32x3(-s,-s, s), float32x3( 1, 0, 0), float32x2(0, 1) },

		{ float32x3( s, s,-s), float32x3(-1, 0, 0), float32x2(0, 0) },
		{ float32x3( s, s, s), float32x3(-1, 0, 0), float32x2(1, 0) },
		{ float32x3( s,-s, s), float32x3(-1, 0, 0), float32x2(1, 1) },
		{ float32x3( s,-s,-s), float32x3(-1, 0, 0), float32x2(0, 1) },
	};

	indices =
	{
		0, 2, 1, 0, 3, 2,
		4, 6, 5, 4, 7, 6,
		8, 10, 9, 8, 11, 10,
		12, 14, 13, 12, 15, 14,
		16, 18, 17, 16, 19, 18,
		20, 22, 21, 20, 23, 22,
	};
}

} // namespace mango::import3d
