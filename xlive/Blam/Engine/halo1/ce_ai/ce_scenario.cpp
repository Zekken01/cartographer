#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* scenario.c: the functions of it the AI calls.
*/

namespace h1_ai
{

boolean scenario_location_deafening(
	const struct location *location)
{
	struct structure_cluster *cluster = TAG_BLOCK_GET_ELEMENT(
		&global_structure_bsp_get()->clusters,
		location->cluster_index,
		struct structure_cluster);
	boolean deafening = FALSE;

	if (cluster->background_sound_palette_index != NONE &&
		cluster->background_sound_palette_index < global_structure_bsp_get()->background_sound_palette.count)
	{
		struct structure_background_sound_palette_entry *background_sound = TAG_BLOCK_GET_ELEMENT(
			&global_structure_bsp_get()->background_sound_palette,
			cluster->background_sound_palette_index,
			struct structure_background_sound_palette_entry);

		if (background_sound->background_sound.index != NONE)
			deafening = TEST_FLAG(
				looping_sound_definition_get(background_sound->background_sound.index)->flags,
				0);
	}

	return deafening;
}

short scenario_get_fog_region_index(
	const struct location *location,
	const real_point3d *position)
{
	short result = NONE;
	short fog_reference;
	real plane_distance;
	struct structure_bsp *structure_bsp;
	struct structure_cluster *cluster;
	struct structure_fog_plane *fog_plane;
	struct fog_definition *fog;
	long fog_index;

	if (location->cluster_index != NONE)
	{
		structure_bsp = global_structure_bsp_get();
		cluster = TAG_BLOCK_GET_ELEMENT(
			&structure_bsp->clusters,
			location->cluster_index,
			struct structure_cluster);
		fog_reference = cluster->fog_reference;
		if (fog_reference != NONE)
		{
			if (TEST_FLAG((word)fog_reference, 15))
			{
				fog_plane = TAG_BLOCK_GET_ELEMENT(
					&structure_bsp->fog_planes,
					fog_reference & SHORT_MAX,
					struct structure_fog_plane);
				fog_index = scenario_fog_region_get_fog_index(fog_plane->region_index);
				plane_distance = 0.0f;
				if (fog_index != NONE)
				{
					fog = fog_definition_get(fog_index);
					if (TEST_FLAG(fog->flags, 0))
						plane_distance = fog->plane_distance;
				}

				if (!position || plane3d_distance_to_point(&fog_plane->plane, position) + plane_distance < 0.0f)
					result = fog_plane->region_index;
			}
			else
			{
				result = fog_reference & SHORT_MAX;
			}
		}
	}

	return result;
}

boolean scenario_location_underwater(
	const struct location *location,
	const real_point3d *position,
	short *optional_weather_palette_index)
{
	boolean result = FALSE;
	short fog_region_index;
	short weather_palette_index;
	long fog_index;
	struct structure_bsp *structure_bsp = global_structure_bsp_get();
	struct structure_fog_region *fog_region;
	struct structure_cluster *cluster;

	fog_region_index = scenario_get_fog_region_index(location, position);
	weather_palette_index = NONE;
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 0x258, location);
	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 0x259, position);

	if (fog_region_index != NONE)
	{
		fog_region = TAG_BLOCK_GET_ELEMENT(
			&structure_bsp->fog_regions,
			fog_region_index,
			struct structure_fog_region);
		fog_index = scenario_fog_region_get_fog_index(fog_region_index);
		if (fog_index != NONE)
			result = TEST_FLAG(fog_definition_get(fog_index)->flags, 0);
		weather_palette_index = fog_region->weather_palette_index;
	}

	if (weather_palette_index == NONE && location->cluster_index != NONE)
	{
		cluster = TAG_BLOCK_GET_ELEMENT(
			&structure_bsp->clusters,
			location->cluster_index,
			struct structure_cluster);
		weather_palette_index = cluster->weather_palette_index;
	}

	if (optional_weather_palette_index)
		*optional_weather_palette_index = weather_palette_index;

	return result;
}

real scenario_fog_at_point(
	const struct location *viewer_location,
	const real_point3d *viewer_point,
	const real_point3d *point)
{
	return 0.0f;
}

boolean scenario_test_pvs(
	short cluster_index0,
	short cluster_index1)
{
	struct structure_bsp *structure_bsp = global_structure_bsp_get();
	unsigned long *pvs = structure_bsp_get_cluster_pvs(structure_bsp, cluster_index0);

	match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 468,
		cluster_index1>=0 && cluster_index1<structure_bsp->clusters.count);

	return BIT_VECTOR_TEST_FLAG(pvs, cluster_index1);
}

long scenario_fog_region_get_fog_index(
	short fog_region_index)
{
	struct structure_bsp *structure_bsp = global_structure_bsp_get();

	if (fog_region_index != NONE)
	{
		struct structure_fog_region *fog_region = TAG_BLOCK_GET_ELEMENT(
			&structure_bsp->fog_regions,
			fog_region_index,
			struct structure_fog_region);

		if (fog_region->fog_palette_index != NONE)
		{
			struct structure_fog_palette_entry *fog_palette_entry = TAG_BLOCK_GET_ELEMENT(
				&structure_bsp->fog_palette,
				fog_region->fog_palette_index,
				struct structure_fog_palette_entry);

			if (fog_palette_entry->fog.index != NONE)
				return fog_palette_entry->fog.index;
		}
	}

	return NONE;
}

} // namespace h1_ai
