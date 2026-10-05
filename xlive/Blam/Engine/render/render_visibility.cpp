#include "stdafx.h"
#include "render_visibility.h"

#include "halo1/h1_map_loader.h"

/* constants */

enum
{
	k_visibility_cluster_count_offset = 0xA6C,
	k_visibility_cluster_entry_offset = 0xA6E,
	k_visibility_cluster_entry_size = 0x1A,
};

/* prototypes */

static void render_decal_visibility_halo1_clusters(draw_proc_t draw_proc, int32 a2);

/* public code */

void __cdecl render_decal_visibility(draw_proc_t draw_proc, int32 a2)
{
	INVOKE(0x19DC1E, 0x0, render_decal_visibility, draw_proc, a2);
	if (h1_maps_active())
	{
		render_decal_visibility_halo1_clusters(draw_proc, a2);
	}
	return;
}

/* private code */

// decals draw for the clusters in the visible geometry list, and a cluster only gets into that list with halo 2 render geometry:
// halo 1 clusters have none (the halo 1 renderer draws them), so their decals draw for the visible clusters instead
// (a decal drawn twice in one frame is skipped by the draw proc)
static void render_decal_visibility_halo1_clusters(draw_proc_t draw_proc, int32 a2)
{
	const uint8* visibility = *Memory::GetAddress<uint8**>(0x4D2D60);
	if (!visibility)
	{
		return;
	}

	const int16 cluster_count = *(const int16*)(visibility + k_visibility_cluster_count_offset);
	for (int16 i = 0; i < cluster_count; i++)
	{
		const int16 cluster_index = *(const int16*)(visibility + k_visibility_cluster_entry_offset + i * k_visibility_cluster_entry_size);
		draw_proc((e_decal_layer)cluster_index, (int16)a2);
	}
	return;
}
