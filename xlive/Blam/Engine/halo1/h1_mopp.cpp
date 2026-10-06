#include "stdafx.h"
#include "h1_mopp.h"

#include "physics/collision_bsp_definition.h"

#include <algorithm>

/* constants */

enum
{
	k_mopp_header_size = 0x30,
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
		if (!m_leaves.empty())
		{
			node(0, (int32)m_leaves.size(), out_code);
		}
		out_code.push_back(0x00);
		return;
	}

private:
	std::vector<s_mopp_leaf>& m_leaves;

	static void terminal(uint32 key, std::vector<uint8>& out)
	{
		// terminals add the key to the running offset, which stays 0
		if (key < 32)
		{
			out.push_back((uint8)(0x30 + key));
		}
		else if (key < 0x100)
		{
			out.push_back(0x50);
			out.push_back((uint8)key);
		}
		else if (key < 0x10000)
		{
			out.push_back(0x51);
			out.push_back((uint8)((key >> 8) & 0xFF));
			out.push_back((uint8)(key & 0xFF));
		}
		else if (key < 0x1000000)
		{
			out.push_back(0x52);
			out.push_back((uint8)((key >> 16) & 0xFF));
			out.push_back((uint8)((key >> 8) & 0xFF));
			out.push_back((uint8)(key & 0xFF));
		}
		else
		{
			out.push_back(0x53);
			out.push_back((uint8)((key >> 24) & 0xFF));
			out.push_back((uint8)((key >> 16) & 0xFF));
			out.push_back((uint8)((key >> 8) & 0xFF));
			out.push_back((uint8)(key & 0xFF));
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
		else if (left.size() < 0x10000)
		{
			const uint32 second_jump = (uint32)left.size();
			out.push_back((uint8)(0x23 + axis));
			out.push_back(a);
			out.push_back(b);
			out.push_back(0);
			out.push_back(0);
			out.push_back((uint8)((second_jump >> 8) & 0xFF));
			out.push_back((uint8)(second_jump & 0xFF));
		}
		else
		{
			// the 16 bit jumps can't pass the first child: the second child's jump lands on a 24 bit jump (0x07) over it, the
			// first child's skips that
			const uint32 second_jump = (uint32)left.size();
			out.push_back((uint8)(0x23 + axis));
			out.push_back(a);
			out.push_back(b);
			out.push_back(0);
			out.push_back(4);
			out.push_back(0);
			out.push_back(0);
			out.push_back(0x07);
			out.push_back((uint8)((second_jump >> 16) & 0xFF));
			out.push_back((uint8)((second_jump >> 8) & 0xFF));
			out.push_back((uint8)(second_jump & 0xFF));
		}
		out.insert(out.end(), left.begin(), left.end());
		out.insert(out.end(), right.begin(), right.end());
		return;
	}
};

/* prototypes */

static int32 collision_surface_vertices_get(const collision_bsp* bsp, int32 surface_index, real_point3d* out_points, int32 max_points);

/* public code */

void h1_mopp_collect_surfaces(const collision_bsp* bsp, uint32 key_base, std::vector<s_h1_mopp_item>& items)
{
	for (int32 i = 0; i < bsp->surfaces.count; i++)
	{
		real_point3d points[64];
		const int32 point_count = collision_surface_vertices_get(bsp, i, points, NUMBEROF(points));
		if (point_count <= 0)
		{
			continue;
		}

		s_h1_mopp_item item;
		item.key = key_base | (uint32)i;
		item.bounds = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
		for (int32 j = 0; j < point_count; j++)
		{
			item.bounds.x0 = MIN(item.bounds.x0, points[j].x); item.bounds.x1 = MAX(item.bounds.x1, points[j].x);
			item.bounds.y0 = MIN(item.bounds.y0, points[j].y); item.bounds.y1 = MAX(item.bounds.y1, points[j].y);
			item.bounds.z0 = MIN(item.bounds.z0, points[j].z); item.bounds.z1 = MAX(item.bounds.z1, points[j].z);
		}
		items.push_back(item);
	}
	return;
}

bool h1_mopp_collision_bsp_bounds(const collision_bsp* bsp, const real_matrix4x3* matrix, real_rectangle3d* out_bounds)
{
	*out_bounds = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
	for (int32 i = 0; i < bsp->vertices.count; i++)
	{
		const collision_vertex* vertex = (const collision_vertex*)tag_block_get_element_with_size(&bsp->vertices, i, sizeof(collision_vertex));
		real_point3d p = vertex->point;
		if (matrix)
		{
			const real_point3d v = p;
			p.x = (v.x * matrix->n[0][0] + v.y * matrix->n[1][0] + v.z * matrix->n[2][0]) * matrix->scale + matrix->n[3][0];
			p.y = (v.x * matrix->n[0][1] + v.y * matrix->n[1][1] + v.z * matrix->n[2][1]) * matrix->scale + matrix->n[3][1];
			p.z = (v.x * matrix->n[0][2] + v.y * matrix->n[1][2] + v.z * matrix->n[2][2]) * matrix->scale + matrix->n[3][2];
		}
		out_bounds->x0 = MIN(out_bounds->x0, p.x); out_bounds->x1 = MAX(out_bounds->x1, p.x);
		out_bounds->y0 = MIN(out_bounds->y0, p.y); out_bounds->y1 = MAX(out_bounds->y1, p.y);
		out_bounds->z0 = MIN(out_bounds->z0, p.z); out_bounds->z1 = MAX(out_bounds->z1, p.z);
	}
	return bsp->vertices.count > 0;
}

bool h1_mopp_build(const std::vector<s_h1_mopp_item>& items, uint8** out_blob, uint32* out_size, real_point3d* out_bounds_min, real_point3d* out_bounds_max)
{
	if (items.empty())
	{
		return false;
	}

	real_point3d bounds_min = { FLT_MAX, FLT_MAX, FLT_MAX };
	real_point3d bounds_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const s_h1_mopp_item& item : items)
	{
		bounds_min.x = MIN(bounds_min.x, item.bounds.x0); bounds_max.x = MAX(bounds_max.x, item.bounds.x1);
		bounds_min.y = MIN(bounds_min.y, item.bounds.y0); bounds_max.y = MAX(bounds_max.y, item.bounds.y1);
		bounds_min.z = MIN(bounds_min.z, item.bounds.z0); bounds_max.z = MAX(bounds_max.z, item.bounds.z1);
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
	leaves.reserve(items.size());
	for (const s_h1_mopp_item& item : items)
	{
		s_mopp_leaf leaf;
		leaf.key = item.key;
		leaf.bounds_min[0] = quantize(item.bounds.x0, 0); leaf.bounds_max[0] = quantize(item.bounds.x1, 0);
		leaf.bounds_min[1] = quantize(item.bounds.y0, 1); leaf.bounds_max[1] = quantize(item.bounds.y1, 1);
		leaf.bounds_min[2] = quantize(item.bounds.z0, 2); leaf.bounds_max[2] = quantize(item.bounds.z1, 2);
		leaf.center[0] = (item.bounds.x0 + item.bounds.x1) * 0.5f;
		leaf.center[1] = (item.bounds.y0 + item.bounds.y1) * 0.5f;
		leaf.center[2] = (item.bounds.z0 + item.bounds.z1) * 0.5f;
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
	*(uint32*)(blob + 0x20) = total_size - 4;
	*(uint32*)(blob + 0x24) = 1;
	csmemcpy(blob + k_mopp_header_size, code.data(), code.size());

	*out_blob = blob;
	*out_size = total_size;
	*out_bounds_min = bounds_min;
	*out_bounds_max = bounds_max;
	return true;
}

int32 h1_mopp_surface_vertices_get(const collision_bsp* bsp, int32 surface_index, real_point3d* out_points, int32 max_points)
{
	return collision_surface_vertices_get(bsp, surface_index, out_points, max_points);
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
