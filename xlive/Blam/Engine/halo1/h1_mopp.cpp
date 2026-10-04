#include "stdafx.h"
#include "h1_mopp.h"

#include "physics/collision_bsp_definition.h"

#include <algorithm>

/* constants */

enum
{
	k_mopp_header_size = 0x30,
	k_mopp_structure_surface_key = 0x20000000,
	k_mopp_maximum_coordinate = 0xFE,
};

/* structures */

struct s_mopp_leaf
{
	uint32 key;
	uint8 bounds_min[3];
	uint8 bounds_max[3];
	real32 center[3];
};

/* classes */

class c_mopp_builder
{
public:
	c_mopp_builder(std::vector<s_mopp_leaf>& leaves) : m_leaves(leaves) {}

	void build(std::vector<uint8>& out_code)
	{
		out_code.clear();
		// absolute key offset for every terminal below
		out_code.push_back(0x0B);
		const uint32 key_offset = k_mopp_structure_surface_key;
		out_code.push_back((uint8)((key_offset >> 24) & 0xFF));
		out_code.push_back((uint8)((key_offset >> 16) & 0xFF));
		out_code.push_back((uint8)((key_offset >> 8) & 0xFF));
		out_code.push_back((uint8)(key_offset & 0xFF));

		std::vector<uint8> tree;
		if (!m_leaves.empty())
		{
			node(0, (int32)m_leaves.size(), tree);
		}
		out_code.insert(out_code.end(), tree.begin(), tree.end());
		out_code.push_back(0x00);
		return;
	}

private:
	std::vector<s_mopp_leaf>& m_leaves;

	static void terminal(uint32 key, std::vector<uint8>& out)
	{
		if (key < 32)
		{
			out.push_back((uint8)(0x30 + key));
		}
		else if (key < 256)
		{
			out.push_back(0x50);
			out.push_back((uint8)key);
		}
		else if (key < 65536)
		{
			out.push_back(0x51);
			out.push_back((uint8)(key >> 8));
			out.push_back((uint8)key);
		}
		else
		{
			out.push_back(0x52);
			out.push_back((uint8)(key >> 16));
			out.push_back((uint8)(key >> 8));
			out.push_back((uint8)key);
		}
		return;
	}

	void node(int32 first, int32 count, std::vector<uint8>& out)
	{
		if (count == 1)
		{
			terminal(m_leaves[first].key, out);
			return;
		}

		// split on the axis with the widest spread of centers
		real32 spread[3];
		for (int32 axis = 0; axis < 3; axis++)
		{
			real32 lo = FLT_MAX, hi = -FLT_MAX;
			for (int32 i = first; i < first + count; i++)
			{
				lo = MIN(lo, m_leaves[i].center[axis]);
				hi = MAX(hi, m_leaves[i].center[axis]);
			}
			spread[axis] = hi - lo;
		}
		const int32 axis = spread[0] >= spread[1] && spread[0] >= spread[2] ? 0 : (spread[1] >= spread[2] ? 1 : 2);

		std::sort(m_leaves.begin() + first, m_leaves.begin() + first + count,
			[axis](const s_mopp_leaf& a, const s_mopp_leaf& b) { return a.center[axis] < b.center[axis]; });

		const int32 left_count = count / 2;
		int32 first_child_max = 0;
		int32 second_child_min = 0xFF;
		for (int32 i = first; i < first + left_count; i++)
		{
			first_child_max = MAX(first_child_max, (int32)m_leaves[i].bounds_max[axis]);
		}
		for (int32 i = first + left_count; i < first + count; i++)
		{
			second_child_min = MIN(second_child_min, (int32)m_leaves[i].bounds_min[axis]);
		}

		std::vector<uint8> left, right;
		node(first, left_count, left);
		node(first + left_count, count - left_count, right);

		// first child entered while query.min < A, second while B < query.max
		const uint8 a = (uint8)(first_child_max + 1);
		const uint8 b = (uint8)second_child_min;
		if (left.size() < 256)
		{
			out.push_back((uint8)(0x10 + axis));
			out.push_back(a);
			out.push_back(b);
			out.push_back((uint8)left.size());
		}
		else
		{
			const uint32 second_jump = (uint32)left.size();
			out.push_back((uint8)(0x23 + axis));
			out.push_back(a);
			out.push_back(b);
			out.push_back(0);
			out.push_back(0);
			out.push_back((uint8)(second_jump >> 8));
			out.push_back((uint8)second_jump);
		}
		out.insert(out.end(), left.begin(), left.end());
		out.insert(out.end(), right.begin(), right.end());
		return;
	}
};

/* prototypes */

static int32 collision_surface_vertices_get(const collision_bsp* bsp, int32 surface_index, real_point3d* out_points, int32 max_points);

/* public code */

bool h1_mopp_build_for_collision_bsp(const collision_bsp* bsp, uint8** out_blob, uint32* out_size, real_point3d* out_bounds_min, real_point3d* out_bounds_max)
{
	const int32 surface_count = bsp->surfaces.count;
	if (surface_count <= 0)
	{
		return false;
	}

	std::vector<real_rectangle3d> surface_bounds(surface_count);
	real_point3d bounds_min = { FLT_MAX, FLT_MAX, FLT_MAX };
	real_point3d bounds_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (int32 i = 0; i < surface_count; i++)
	{
		real_point3d points[64];
		const int32 point_count = collision_surface_vertices_get(bsp, i, points, NUMBEROF(points));
		real_rectangle3d* bounds = &surface_bounds[i];
		*bounds = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
		for (int32 j = 0; j < point_count; j++)
		{
			bounds->x0 = MIN(bounds->x0, points[j].x); bounds->x1 = MAX(bounds->x1, points[j].x);
			bounds->y0 = MIN(bounds->y0, points[j].y); bounds->y1 = MAX(bounds->y1, points[j].y);
			bounds->z0 = MIN(bounds->z0, points[j].z); bounds->z1 = MAX(bounds->z1, points[j].z);
		}
		if (point_count > 0)
		{
			bounds_min.x = MIN(bounds_min.x, bounds->x0); bounds_max.x = MAX(bounds_max.x, bounds->x1);
			bounds_min.y = MIN(bounds_min.y, bounds->y0); bounds_max.y = MAX(bounds_max.y, bounds->y1);
			bounds_min.z = MIN(bounds_min.z, bounds->z0); bounds_max.z = MAX(bounds_max.z, bounds->z1);
		}
	}

	const real32 margin = 0.05f;
	const real32 origin[3] = { bounds_min.x - margin, bounds_min.y - margin, bounds_min.z - margin };
	const real32 extent = MAX(MAX(bounds_max.x - bounds_min.x, bounds_max.y - bounds_min.y), bounds_max.z - bounds_min.z) + 2.f * margin;
	// keeps every 8 bit coordinate at or below 0xFE so split limits always fit in a byte
	const real32 scale = (real32)0xFE0000 / extent;

	auto quantize = [&](real32 value, int32 axis) -> uint8
	{
		const int32 q = ((int32)roundf((value - origin[axis]) * scale) - 1) >> 16;
		return (uint8)PIN(q, 0, k_mopp_maximum_coordinate);
	};

	std::vector<s_mopp_leaf> leaves;
	leaves.reserve(surface_count);
	for (int32 i = 0; i < surface_count; i++)
	{
		const real_rectangle3d* bounds = &surface_bounds[i];
		if (bounds->x0 > bounds->x1)
		{
			continue;
		}

		s_mopp_leaf leaf;
		leaf.key = (uint32)i;
		leaf.bounds_min[0] = quantize(bounds->x0, 0); leaf.bounds_max[0] = quantize(bounds->x1, 0);
		leaf.bounds_min[1] = quantize(bounds->y0, 1); leaf.bounds_max[1] = quantize(bounds->y1, 1);
		leaf.bounds_min[2] = quantize(bounds->z0, 2); leaf.bounds_max[2] = quantize(bounds->z1, 2);
		leaf.center[0] = (bounds->x0 + bounds->x1) * 0.5f;
		leaf.center[1] = (bounds->y0 + bounds->y1) * 0.5f;
		leaf.center[2] = (bounds->z0 + bounds->z1) * 0.5f;
		leaves.push_back(leaf);
	}

	std::vector<uint8> code;
	c_mopp_builder builder(leaves);
	builder.build(code);

	const uint32 total_size = (k_mopp_header_size + (uint32)code.size() + 15) & ~15u;
	uint8* blob = (uint8*)calloc(1, total_size);
	real32* header = (real32*)blob;
	header[0] = origin[0];
	header[1] = origin[1];
	header[2] = origin[2];
	header[3] = scale;
	blob[0x10] = 0xFF;
	*(uint32*)(blob + 0x20) = total_size - 5;
	*(uint32*)(blob + 0x24) = 1;
	csmemcpy(blob + k_mopp_header_size, code.data(), code.size());

	*out_blob = blob;
	*out_size = total_size;
	*out_bounds_min = bounds_min;
	*out_bounds_max = bounds_max;
	return true;
}

void h1_mopp_free(uint8* blob)
{
	free(blob);
	return;
}

/* private code */

static int32 collision_surface_vertices_get(const collision_bsp* bsp, int32 surface_index, real_point3d* out_points, int32 max_points)
{
	const collision_surface* surface = (const collision_surface*)tag_block_get_element_with_size(&bsp->surfaces, surface_index, sizeof(collision_surface));
	const int32 first_edge = surface->first_edge_index;
	int32 edge_index = first_edge;
	int32 count = 0;

	do
	{
		const collision_edge* edge = (const collision_edge*)tag_block_get_element_with_size(&bsp->edges, edge_index, sizeof(collision_edge));
		int32 vertex_index;
		if (edge->surface_indices[0] == surface_index)
		{
			vertex_index = edge->vertex_indices[0];
			edge_index = edge->edge_indices[0];
		}
		else
		{
			vertex_index = edge->vertex_indices[1];
			edge_index = edge->edge_indices[1];
		}

		const collision_vertex* vertex = (const collision_vertex*)tag_block_get_element_with_size(&bsp->vertices, vertex_index, sizeof(collision_vertex));
		out_points[count++] = vertex->point;
	} while (edge_index != first_edge && count < max_points);

	return count;
}
