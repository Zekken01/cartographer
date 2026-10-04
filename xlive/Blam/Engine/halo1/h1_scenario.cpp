#include "stdafx.h"
#include "h1_scenario.h"

#include "h1_cache_file.h"
#include "h1_equipment.h"
#include "h1_log.h"
#include "h1_runtime.h"
#include "h1_structure_bsp.h"
#include "h2_tag_definitions_generated.h"

#include "cache/cache_files.h"
#include "scenario/scenario_definitions.h"
#include "scenario/scenario_fog.h"
#include "structures/structure_bsp_definitions.h"
#include "structures/structure_bsp_definitions.h"

/* constants */

enum e_h1_netgame_flag_type
{
	_h1_netgame_flag_ctf_flag = 0,
	_h1_netgame_flag_ctf_vehicle,
	_h1_netgame_flag_oddball_ball_spawn,
	_h1_netgame_flag_race_track,
	_h1_netgame_flag_race_vehicle,
	_h1_netgame_flag_vegas_bank,
	_h1_netgame_flag_teleporter_source,
	_h1_netgame_flag_teleporter_destination,
	_h1_netgame_flag_hill,
};

/* prototypes */

static e_item_spawn_game_type h1_game_type_convert(int16 h1_game_type);
static void h1_scenario_build_player_starting_locations(scenario* h2_scenario, const h1_scnr* h1_scenario);
static void h1_scenario_build_netgame_flags(scenario* h2_scenario, const h1_scnr* h1_scenario);
static bool h1_scenario_build_structure_bsps(scenario* h2_scenario, const h1_scnr* h1_scenario);
static void h1_scenario_clear_host_placements(scenario* h2_scenario);

/* public code */

bool h1_scenario_build(void)
{
	cache_file_tags_header* tags_header = cache_files_get_tags_header();
	scenario* h2_scenario = (scenario*)tag_get('scnr', tags_header->scenario_index);
	const h1_scnr* h1_scenario = g_h1_cache_file->scenario_get();

	h1_scenario_clear_host_placements(h2_scenario);
	h1_scenario_build_player_starting_locations(h2_scenario, h1_scenario);
	h1_scenario_build_netgame_flags(h2_scenario, h1_scenario);
	h1_equipment_build(h2_scenario, h1_scenario);
	return h1_scenario_build_structure_bsps(h2_scenario, h1_scenario);
}

/* private code */

static e_item_spawn_game_type h1_game_type_convert(int16 h1_game_type)
{
	// Halo 1: none, ctf, slayer, oddball, king, race, terminator, stub, ignored1-4, all, all except ctf, all except race and ctf
	switch (h1_game_type)
	{
	case 0: return item_spawn_game_type_game_type_none;
	case 1: return item_spawn_game_type_capture_the_flag;
	case 2: return item_spawn_game_type_slayer;
	case 3: return item_spawn_game_type_oddball;
	case 4: return item_spawn_game_type_king_of_the_hill;
	case 5: return item_spawn_game_type_race;
	case 6: return item_spawn_game_type_juggernaut;
	case 7: return item_spawn_game_type_stub;
	case 12: return item_spawn_game_type_all_game_types;
	case 13: return item_spawn_game_type_all_except_ctf;
	case 14: return item_spawn_game_type_all_except_ctf_race;
	default: return item_spawn_game_type_game_type_none;
	}
}

static void h1_scenario_build_player_starting_locations(scenario* h2_scenario, const h1_scnr* h1_scenario)
{
	const int32 count = h1_scenario->player_starting_locations.count;
	scenario_player* players = h1_runtime_block_new<scenario_player>(&h2_scenario->player_starting_locations, count);

	for (int32 i = 0; i < count; i++)
	{
		const h1_scnr_player_starting_locations* source = g_h1_cache_file->block_get(h1_scenario->player_starting_locations, i);
		scenario_player* player = &players[i];

		player->position = source->position;
		// Halo 1 stores facing in radians, Halo 2 in degrees
		player->facing_degrees = RADIANS_TO_DEGREES(source->facing);
		player->team_designator = (e_game_team)source->team_index;
		player->bsp_index = source->bsp_index;
		player->game_type_1 = h1_game_type_convert(source->type_0);
		player->game_type_2 = h1_game_type_convert(source->type_1);
		player->game_type_3 = h1_game_type_convert(source->type_2);
		player->game_type_4 = h1_game_type_convert(source->type_3);
		player->spawn_type_0 = spawn_type_both;
		player->spawn_type_1 = spawn_type_both;
		player->spawn_type_2 = spawn_type_both;
		player->spawn_type_3 = spawn_type_both;
		player->unused_name_0 = _string_id_empty_string;
		player->unused_name_1 = _string_id_empty_string;
		player->campaign_player_type = NONE;
	}

	h1_log("scenario: %d player starting locations", count);
	return;
}

static void h1_scenario_build_netgame_flags(scenario* h2_scenario, const h1_scnr* h1_scenario)
{
	// count first, ctf flags become a spawn and a return point
	int32 count = 0;
	for (int32 i = 0; i < h1_scenario->netgame_flags.count; i++)
	{
		const h1_scnr_netgame_flags* source = g_h1_cache_file->block_get(h1_scenario->netgame_flags, i);
		switch (source->type)
		{
		case _h1_netgame_flag_ctf_flag: count += 4; break;	// flag spawn, flag return, bomb spawn, bomb return
		case _h1_netgame_flag_oddball_ball_spawn:
		case _h1_netgame_flag_race_track:
		case _h1_netgame_flag_teleporter_source:
		case _h1_netgame_flag_teleporter_destination:
		case _h1_netgame_flag_hill:
			count++;
			break;
		default:
			break;
		}
	}

	h2x_scnr_netgame_flags* points = (h2x_scnr_netgame_flags*)h1_runtime_block_allocate(&h2_scenario->netgame_flags, sizeof(h2x_scnr_netgame_flags), count);
	int32 point_index = 0;

	auto add_point = [&](const h1_scnr_netgame_flags* source, e_netpoint_type type, int16 team) -> void
	{
		h2x_scnr_netgame_flags* point = &points[point_index++];
		point->position = source->position;
		point->facing = RADIANS_TO_DEGREES(source->facing);
		point->type = (int16)type;
		point->team_designator = team;
		// teleporters pair by identifier, hills and race checkpoints are ordered by it
		point->identifier = source->usage_id;
		point->flags = 0;
		point->spawn_object_name = _string_id_empty_string;
		point->spawn_marker_name = _string_id_empty_string;
	};

	for (int32 i = 0; i < h1_scenario->netgame_flags.count; i++)
	{
		const h1_scnr_netgame_flags* source = g_h1_cache_file->block_get(h1_scenario->netgame_flags, i);
		const int16 usage = source->usage_id;
		switch (source->type)
		{
		case _h1_netgame_flag_ctf_flag:
			add_point(source, netpoint_type_ctf_flag_spawn, usage);
			add_point(source, netpoint_type_ctf_flag_return, usage);
			add_point(source, netpoint_type_assault_bomb_spawn, usage);
			add_point(source, netpoint_type_assault_bomb_return, usage);
			break;
		case _h1_netgame_flag_oddball_ball_spawn:
			add_point(source, netpoint_type_oddball_spawn, 8);
			break;
		case _h1_netgame_flag_race_track:
			// race checkpoints are ordered by the identifier
			add_point(source, netpoint_type_race_checkpoint, usage);
			break;
		case _h1_netgame_flag_teleporter_source:
			add_point(source, netpoint_type_teleporter_src, usage);
			break;
		case _h1_netgame_flag_teleporter_destination:
			add_point(source, netpoint_type_teleporter_dest, usage);
			break;
		case _h1_netgame_flag_hill:
			// Halo 1 hills are a set of points sharing a hill index
			add_point(source, (e_netpoint_type)(netpoint_type_king_hill_0 + PIN(usage, 0, 7)), 8);
			break;
		default:
			break;
		}
	}

	h1_log("scenario: %d netgame flags from %d halo 1 flags", count, h1_scenario->netgame_flags.count);
	return;
}

static void h1_scenario_clear_host_placements(scenario* h2_scenario)
{
	// objects and decoration placed for the host's level
	s_tag_block* blocks[] =
	{
		&h2_scenario->scenery, &h2_scenario->scenery_palette,
		&h2_scenario->sound_scenery, &h2_scenario->sound_scenery_palette,
		&h2_scenario->light_volumes, &h2_scenario->light_volumes_palette,
		&h2_scenario->crates, &h2_scenario->crates_palette,
		&h2_scenario->decorators, &h2_scenario->decorators_palette,
		&h2_scenario->decals, &h2_scenario->decals_palette,
		&h2_scenario->structure_bsp_lighting,
	};
	for (int32 i = 0; i < NUMBEROF(blocks); i++)
	{
		blocks[i]->count = 0;
		blocks[i]->data = 0;
	}
	return;
}

static bool h1_scenario_build_structure_bsps(scenario* h2_scenario, const h1_scnr* h1_scenario)
{
	if (h2_scenario->structure_bsp_references.count < 1 || g_h1_cache_file->structure_bsp_count() < 1)
	{
		h1_log("scenario: no structure bsp to build");
		return false;
	}

	// multiplayer maps have a single structure bsp, it replaces the host's
	scenario_structure_bsp_reference* reference = TAG_BLOCK_GET_ELEMENT(&h2_scenario->structure_bsp_references, 0, scenario_structure_bsp_reference);
	h2_scenario->structure_bsp_references.count = 1;
	if (!h1_structure_bsp_build(0, reference->structure_bsp.index, reference->structure_lightmap.index))
	{
		return false;
	}

	structure_bsp* bsp = (structure_bsp*)tag_get('sbsp', reference->structure_bsp.index);
	const int32 cluster_count = bsp->clusters.count;

	// halo 1 outdoor fog (from the sky) becomes halo 2 atmospheric fog for every cluster that sees the sky
	bool outdoor_fog = false;
	const h1_scnr_skies* sky_reference = g_h1_cache_file->block_get(h1_scenario->skies, 0);
	const h1_sky* sky = sky_reference ? (const h1_sky*)g_h1_cache_file->tag_get(sky_reference->sky) : NULL;
	if (sky && sky->outdoor_fog_maximum_density > 0.f && sky->outdoor_fog_opaque_distance > sky->outdoor_fog_start_distance)
	{
		s_scenario_atmospheric_fog_palette_entry* fog_entry = h1_runtime_block_new<s_scenario_atmospheric_fog_palette_entry>(&h2_scenario->atmospheric_fog_palette, 1);
		fog_entry->name = _string_id_empty_string;
		fog_entry->color = sky->outdoor_fog_color;
		fog_entry->spread_distance_world_units = 1.f;
		fog_entry->maximum_density = sky->outdoor_fog_maximum_density;
		fog_entry->start_distance_world_units = sky->outdoor_fog_start_distance;
		fog_entry->opaque_distance_world_units = sky->outdoor_fog_opaque_distance;
		fog_entry->secondary_fog_color = sky->outdoor_fog_color;
		fog_entry->patchy_fog.group = (tag_group)NONE;
		fog_entry->patchy_fog.index = NONE;
		outdoor_fog = true;

		const h1_sbsp* h1_bsp = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(0));
		for (int32 i = 0; i < cluster_count; i++)
		{
			const h1_sbsp_clusters* h1_cluster = g_h1_cache_file->block_get(h1_bsp->clusters, i);
			structure_cluster* cluster = (structure_cluster*)tag_block_get_element_with_size(&bsp->clusters, i, sizeof(structure_cluster));
			if (h1_cluster && h1_cluster->sky != NONE)
			{
				cluster->scenario_atmospheric_fog_index = 0;
			}
		}
	}

	// per cluster scenario data
	if (h2_scenario->scenario_cluster_data.count > 0)
	{
		h2_scenario->scenario_cluster_data.count = 1;
		h2x_scnr_scenario_cluster_data* cluster_data = TAG_BLOCK_GET_ELEMENT(&h2_scenario->scenario_cluster_data, 0, h2x_scnr_scenario_cluster_data);
		h2x_scnr_scenario_cluster_data_background_sounds* sounds = h1_runtime_block_new(&cluster_data->background_sounds, cluster_count);
		h2x_scnr_scenario_cluster_data_sound_environments* environments = h1_runtime_block_new(&cluster_data->sound_environments, cluster_count);
		h2x_scnr_scenario_cluster_data_cluster_centroids* centroids = h1_runtime_block_new(&cluster_data->cluster_centroids, cluster_count);
		h2x_scnr_scenario_cluster_data_weather_properties* weather = h1_runtime_block_new(&cluster_data->weather_properties, cluster_count);
		h2x_scnr_scenario_cluster_data_atmospheric_fog_properties* fog = h1_runtime_block_new(&cluster_data->atmospheric_fog_properties, cluster_count);
		for (int32 i = 0; i < cluster_count; i++)
		{
			const structure_cluster* cluster = (const structure_cluster*)tag_block_get_element_with_size(&bsp->clusters, i, sizeof(structure_cluster));
			sounds[i].palette_index = NONE;
			environments[i].palette_index = NONE;
			weather[i].palette_index = NONE;
			fog[i].palette_index = outdoor_fog && cluster->scenario_atmospheric_fog_index != 0xFF ? 0 : NONE;
			centroids[i].centroid.x = (cluster->bounds.x0 + cluster->bounds.x1) * 0.5f;
			centroids[i].centroid.y = (cluster->bounds.y0 + cluster->bounds.y1) * 0.5f;
			centroids[i].centroid.z = (cluster->bounds.z0 + cluster->bounds.z1) * 0.5f;
		}
	}

	return true;
}
