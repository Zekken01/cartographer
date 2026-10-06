#include "stdafx.h"
#include "h1_map_loader.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_scenery.h"
#include "h1_projectile_logic.h"
#include "h1_weapon_logic.h"
#include "h1_hs.h"
#include "h1_items.h"
#include "h1_vehicle_physics.h"
#include "h1_devices.h"
#include "ce_ai/h1_ai.h"
#include "h1_log.h"
#include "h1_render.h"
#include "h1_runtime.h"
#include "h1_scenario.h"
#include "h1_sound.h"

#include "cache/cache_files.h"
#include "effects/player_effects.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/game_time.h"
#include "game/game_options.h"
#include "game/players.h"
#include "main/main.h"
#include "objects/objects.h"
#include "structures/structure_bsp_definitions.h"
#include "main/main_game.h"
#include "main/map_repository.h"
#include "multithreading/synchronization.h"
#include "saved_games/game_variant.h"
#include "text/unicode.h"

/* constants */

// Halo 2 multiplayer map opened in place of the Halo 1 cache file; it supplies globals, multiplayer globals
// and the shared resource database every Halo 2 multiplayer map relies on
#define k_h1_host_map_path L".\\maps\\coagulation.map"

// additional tag cache memory reserved for tags built from the Halo 1 cache file
#define k_h1_tag_memory_size (96 * 1024 * 1024)

#define k_h1_autolaunch_file L"h1_autolaunch.txt"

#define k_h1_maps_folder L".\\maps\\ce\\"

struct s_h1_map_description
{
	const char* file_name;
	const wchar_t* display_name;
	const wchar_t* description;
};

static const s_h1_map_description k_h1_map_descriptions[] =
{
	{ "beavercreek",	L"Battle Creek",		L"Halo: Combat Evolved. Two bases face each other across a creek. 2-8 players." },
	{ "bloodgulch",		L"Blood Gulch",			L"Halo: Combat Evolved. The quick and the dead. 4-16 players." },
	{ "boardingaction",	L"Boarding Action",		L"Halo: Combat Evolved. Two ships, one goal. 4-16 players." },
	{ "carousel",		L"Derelict",			L"Halo: Combat Evolved. Watch your step. 2-8 players." },
	{ "chillout",		L"Chill Out",			L"Halo: Combat Evolved. Keep your cool. 2-8 players." },
	{ "damnation",		L"Damnation",			L"Halo: Combat Evolved. Covenant hydro-processing center. 2-12 players." },
	{ "hangemhigh",		L"Hang 'Em High",		L"Halo: Combat Evolved. A tombstone for every man. 2-12 players." },
	{ "longest",		L"Longest",				L"Halo: Combat Evolved. Hope you have a long rifle. 2-8 players." },
	{ "prisoner",		L"Prisoner",			L"Halo: Combat Evolved. Clinical and cold. 2-8 players." },
	{ "putput",			L"Chiron TL-34",		L"Halo: Combat Evolved. Spartan clone training complex. 2-8 players." },
	{ "ratrace",		L"Rat Race",			L"Halo: Combat Evolved. Run of the mill. 2-8 players." },
	{ "sidewinder",		L"Sidewinder",			L"Halo: Combat Evolved. Snow and cold. 4-16 players." },
	{ "wizard",			L"Wizard",				L"Halo: Combat Evolved. Round and round. 2-8 players." },
	{ "a10",			L"The Pillar of Autumn",L"Halo: Combat Evolved campaign." },
	{ "a30",			L"Halo",				L"Halo: Combat Evolved campaign." },
	{ "a50",			L"Truth and Reconciliation", L"Halo: Combat Evolved campaign." },
	{ "b30",			L"The Silent Cartographer", L"Halo: Combat Evolved campaign." },
	{ "b40",			L"Assault on the Control Room", L"Halo: Combat Evolved campaign." },
	{ "c10",			L"343 Guilty Spark",	L"Halo: Combat Evolved campaign." },
	{ "c20",			L"The Library",			L"Halo: Combat Evolved campaign." },
	{ "c40",			L"Two Betrayals",		L"Halo: Combat Evolved campaign." },
	{ "d20",			L"Keyes",				L"Halo: Combat Evolved campaign." },
	{ "d40",			L"The Maw",				L"Halo: Combat Evolved campaign." },
};

/* globals */

static c_h1_cache_file g_h1_loading_cache_file;
static bool g_h1_active = false;
static bool g_h1_autolaunch_done = false;

/* prototypes */

static bool __cdecl h1_custom_map_cache_file_open(const wchar_t* path, cache_file_header* header, int32 flags);
static bool __cdecl h1_custom_map_hash_verify(const uint8* expected_hash, const uint8* file_hash);
static const s_h1_map_description* h1_map_description_get(const char* file_name);
static bool h1_file_hash(const wchar_t* path, uint8* out_hash);
static bool __cdecl h1_player_starting_location_clear(datum player_index, datum unit_definition_index, const void* starting_location);

/* public code */

void h1_maps_apply_patches(void)
{
	h1_cache_file_reserve_memory();
	// custom map cache file open inside the custom map load (FUN_00464a01)
	PatchCall(Memory::GetAddress(0x64B68), h1_custom_map_cache_file_open);
	// the custom map load compares a hash of the file against the map id, Halo 1 entries carry our own hash
	PatchCall(Memory::GetAddress(0x64B44), h1_custom_map_hash_verify);
	// the starting location choice (FUN_0045373f) skips locations a player's pill would collide at: halo 1 has no such test, its
	// players start where the scenario says (a30's start is inside the crashed lifepod)
	PatchCall(Memory::GetAddress(0x53820), h1_player_starting_location_clear);
	h1_effects_apply_patches();
	h1_scenery_apply_patches();
	h1_items_apply_patches();
	h1_vehicle_physics_apply_patches();
	h1_devices_apply_patches();
	h1_weapon_logic_apply_patches();
	h1_projectile_logic_apply_patches();
	return;
}

static bool __cdecl h1_player_starting_location_clear(datum player_index, datum unit_definition_index, const void* starting_location)
{
	if (h1_maps_active())
	{
		return true;
	}
	return Memory::GetAddress<bool(__cdecl*)(datum, datum, const void*)>(0xC7529)(player_index, unit_definition_index, starting_location);
}

bool h1_maps_file_is_halo1(const wchar_t* path)
{
	h1_cache_file_header header;
	return h1_cache_file_read_header(path, &header);
}

void h1_maps_register_folder(c_map_manager* map_manager)
{
	WIN32_FIND_DATAW find_data;
	HANDLE find = FindFirstFileW(k_h1_maps_folder L"*.map", &find_data);
	if (find == INVALID_HANDLE_VALUE)
	{
		return;
	}

	int32 count = 0;
	do
	{
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		wchar_t relative_path[MAX_PATH];
		swprintf_s(relative_path, k_h1_maps_folder L"%s", find_data.cFileName);

		s_custom_map_entry entry;
		csmemset(&entry, 0, sizeof(entry));
		GetFullPathNameW(relative_path, k_map_file_path_size, entry.file_path, NULL);
		entry.file_time = find_data.ftLastWriteTime;

		if (h1_maps_custom_map_entry_fill(&entry))
		{
			if (map_manager->remove_duplicates_and_add_entry(&entry))
			{
				count++;
			}
			else
			{
				h1_log("maps: custom map list rejected %ws", entry.file_path);
			}
		}
	} while (FindNextFileW(find, &find_data));
	FindClose(find);

	h1_log("maps: %d halo 1 maps in the custom map list", count);
	return;
}

bool h1_maps_custom_map_entry_fill(s_custom_map_entry* entry)
{
	h1_cache_file_header header;
	if (!h1_cache_file_read_header(entry->file_path, &header))
	{
		return false;
	}

	// solo (0) and multiplayer (1) scenarios; the user interface's (2) isn't a level
	if (header.type != 0 && header.type != 1)
	{
		h1_log("maps: skipping %ws (%s), scenario type %d isn't a level", entry->file_path, header.name, header.type);
		return false;
	}

	const s_h1_map_description* description = h1_map_description_get(header.name);

	wchar_t display_name[k_custom_map_name_length];
	if (description)
	{
		_snwprintf_s(display_name, _TRUNCATE, L"%s (CE)", description->display_name);
	}
	else
	{
		wchar_t name[32];
		utf8_string_to_wchar_string(header.name, name, NUMBEROF(name));
		_snwprintf_s(display_name, _TRUNCATE, L"%s (CE)", name);
	}
	wcsncpy_s(entry->map_name, display_name, _TRUNCATE);

	for (int32 i = 0; i < NUMBEROF(entry->field_60); i++)
	{
		wcsncpy_s(entry->field_60[i], description ? description->description : L"Halo: Combat Evolved", _TRUNCATE);
	}

	if (!h1_file_hash(entry->file_path, entry->hash))
	{
		return false;
	}

	h1_log("maps: registered %ws as \"%ws\"", entry->file_path, entry->map_name);
	return true;
}

bool h1_maps_active(void)
{
	return g_h1_active;
}

uint32 h1_maps_tag_memory_required(void)
{
	return g_h1_loading_cache_file.is_open() ? k_h1_tag_memory_size : 0;
}

bool h1_maps_scenario_tags_loaded(bool custom_map)
{
	// direct3d resources and sounds of the previous halo 1 map
	h1_render_dispose();
	h1_sound_dispose();

	// built-in maps don't go through the custom map open, drop whatever was loaded before
	if (!custom_map)
	{
		g_h1_loading_cache_file.close();
	}

	if (!g_h1_loading_cache_file.is_open())
	{
		g_h1_active = false;
		g_h1_cache_file = NULL;
		return true;
	}

	g_h1_cache_file = &g_h1_loading_cache_file;
	g_h1_active = true;

	h1_runtime_begin();
	const bool result = h1_scenario_build();
	h1_log("maps: built %s, %u bytes of tag data", g_h1_cache_file->name(), h1_runtime_used_size());
	if (result)
	{
		h1_sound_begin();
		// halo 1's crosshair is at the middle of the screen: halo 2's vehicle cameras and aim assist use the player control's
		// crosshair location (the first person camera centers its own)
		s_game_globals_player_control* player_control = TAG_BLOCK_GET_ELEMENT(&scenario_get_game_globals()->player_control, 0, s_game_globals_player_control);
		if (player_control)
		{
			player_control->crosshair_location = { 0.f, 0.f };
		}
	}
	return result;
}

int16 h1_maps_structure_bsp_index(void)
{
	const int16 bsp_index = get_global_structure_bsp_index();
	return g_h1_cache_file && VALID_INDEX(bsp_index, g_h1_cache_file->structure_bsp_count()) ? bsp_index : 0;
}

// the cluster of a halo 1 structure bsp a point is in (its collision bsp's leaf), NONE outside its open space
static int32 h1_maps_structure_bsp_cluster_get(int32 bsp_index, const real_point3d* point)
{
	int32 leaf_index;
	return h1_maps_structure_bsp_leaf_get(bsp_index, point, &leaf_index);
}

int32 h1_maps_structure_bsp_leaf_get(int32 bsp_index, const real_point3d* point, int32* out_leaf_index)
{
	*out_leaf_index = NONE;
	const h1_sbsp* bsp = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(bsp_index));
	const h1_sbsp_collision_bsp* collision = bsp ? g_h1_cache_file->block_get(bsp->collision_bsp, 0) : NULL;
	if (!collision || collision->bsp3d_nodes.count <= 0 ||
		point->x < bsp->world_bounds_x.lower || point->x > bsp->world_bounds_x.upper ||
		point->y < bsp->world_bounds_y.lower || point->y > bsp->world_bounds_y.upper ||
		point->z < bsp->world_bounds_z.lower || point->z > bsp->world_bounds_z.upper)
	{
		return NONE;
	}
	int32 node_index = 0;
	for (int32 guard = 0; node_index >= 0 && guard < collision->bsp3d_nodes.count; guard++)
	{
		const h1_sbsp_collision_bsp_bsp3d_nodes* node = g_h1_cache_file->block_get(collision->bsp3d_nodes, node_index);
		const h1_sbsp_collision_bsp_planes* plane = node ? g_h1_cache_file->block_get(collision->planes, node->plane) : NULL;
		if (!plane)
		{
			return NONE;
		}
		const real32 side = plane->plane.n.i * point->x + plane->plane.n.j * point->y + plane->plane.n.k * point->z - plane->plane.d;
		node_index = side >= 0.f ? node->front_child : node->back_child;
	}
	if (node_index == NONE || node_index >= 0)
	{
		return NONE;
	}
	const h1_sbsp_leaves* leaf = g_h1_cache_file->block_get(bsp->leaves, node_index & 0x7FFFFFFF);
	*out_leaf_index = node_index & 0x7FFFFFFF;
	return leaf ? leaf->cluster : NONE;
}

// scenario.c scenario_trigger_volume_test_point: inside the trigger volume's box (oriented by its forward and up)
bool h1_maps_trigger_volume_test_point(int16 trigger_volume_index, const real_point3d* point)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_trigger_volumes* volume = scenario && VALID_INDEX(trigger_volume_index, scenario->trigger_volumes.count) ?
		g_h1_cache_file->block_get(scenario->trigger_volumes, trigger_volume_index) : NULL;
	if (!volume)
	{
		return false;
	}
	const real_vector3d offset = { point->x - volume->position.x, point->y - volume->position.y, point->z - volume->position.z };
	real_vector3d left;
	cross_product3d(&volume->up, &volume->forward, &left);
	const real32 x = dot_product3d(&offset, &volume->forward);
	const real32 y = dot_product3d(&offset, &left);
	const real32 z = dot_product3d(&offset, &volume->up);
	return x > 0.f && y > 0.f && z > 0.f && x < volume->extents.x && y < volume->extents.y && z < volume->extents.z;
}

// until halo 1's scripts switch the structure bsps (switch_bsp), the player's leaving the current one for another switches to it
static void h1_maps_structure_bsp_follow_player(void)
{
	if (!h1_maps_active() || !g_h1_cache_file || g_h1_cache_file->structure_bsp_count() < 2 || !game_in_progress())
	{
		return;
	}
	const datum player_index = player_index_from_user_index(0);
	const player_datum* player = player_index != NONE ? player_get(player_index) : NULL;
	if (!player || player->unit_index == NONE || !object_try_and_get(player->unit_index))
	{
		return;
	}
	real_point3d position;
	object_get_origin(player->unit_index, &position, false);
	position.z += 0.3f;
	const int16 current = h1_maps_structure_bsp_index();

	// the scenario's bsp switch trigger volumes: the player in one from the current structure bsp switches to its destination
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const real_point3d center = ((const object_datum*)object_get(player->unit_index))->object.center;
	for (int32 i = 0; scenario && i < scenario->bsp_switch_trigger_volumes.count; i++)
	{
		const h1_scnr_bsp_switch_trigger_volumes* bsp_switch = g_h1_cache_file->block_get(scenario->bsp_switch_trigger_volumes, i);
		if (bsp_switch->source_index != current || !VALID_INDEX(bsp_switch->destination_index, g_h1_cache_file->structure_bsp_count()) ||
			!h1_maps_trigger_volume_test_point(bsp_switch->trigger_volume_index, &center))
		{
			continue;
		}
		h1_log("maps: the player entered the switch from structure bsp %d to %d", current, bsp_switch->destination_index);
		main_switch_structure_bsp_request(bsp_switch->destination_index);
		return;
	}

	// without scripts (switch_bsp), the player's leaving the current structure bsp for another
	if (h1_hs_running() || h1_maps_structure_bsp_cluster_get(current, &position) != NONE)
	{
		return;
	}
	for (int16 bsp_index = 0; bsp_index < g_h1_cache_file->structure_bsp_count(); bsp_index++)
	{
		if (bsp_index != current && h1_maps_structure_bsp_cluster_get(bsp_index, &position) != NONE)
		{
			h1_log("maps: the player left structure bsp %d for %d", current, bsp_index);
			main_switch_structure_bsp_request(bsp_index);
			break;
		}
	}
	// (the current structure bsp's addresses again)
	g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(current));
	return;
}

void h1_maps_update(void)
{
	// the structure bsp's sounds (its background sounds and clusters) when it switches
	static int16 s_sound_bsp_index = NONE;
	if (h1_maps_active() && g_h1_cache_file)
	{
		const int16 bsp_index = h1_maps_structure_bsp_index();
		if (s_sound_bsp_index != NONE && s_sound_bsp_index != bsp_index)
		{
			h1_sound_begin();
		}
		s_sound_bsp_index = bsp_index;
	}
	else
	{
		s_sound_bsp_index = NONE;
	}
	h1_maps_structure_bsp_follow_player();
	h1_sound_update();

	// halo 1's scripts run in campaign games, from the game's first tick (a halo 2 campaign game starts faded to black for them to
	// fade in; a map whose scripts don't load fades in by itself)
	static bool s_campaign_started = false;
	if (!h1_maps_active() || !game_in_progress() || !game_is_campaign())
	{
		if (s_campaign_started)
		{
			h1_hs_dispose_from_old_map();
			h1_ai_dispose_from_old_map();
		}
		s_campaign_started = false;
	}
	else if (!s_campaign_started && game_time_get() > 0)
	{
		s_campaign_started = true;
		// game_initialize_for_new_map: the AI, the scripts and the objects they place, then the encounters made at the start
		h1_ai_initialize_for_new_map();
		h1_hs_initialize_for_new_map();
		h1_ai_place();
		if (!h1_hs_running())
		{
			scripted_player_effect_screen_fade_in(0.f, 0.f, 0.f, 30);
		}
	}
	h1_ai_update();
	h1_hs_update();
	h1_devices_update();

	if (g_h1_autolaunch_done)
	{
		return;
	}

	if (!game_in_progress() || !game_is_ui_shell())
	{
		return;
	}

	g_h1_autolaunch_done = true;
	h1_log("maps: custom map folder is \"%ws\", %d custom maps", get_custom_map_folder_path(), map_manager_get()->m_map_count);

	// h1_autolaunch.txt: "<map file name> [variant]", used for development
	FILE* file = _wfsopen(k_h1_autolaunch_file, L"r", _SH_DENYNO);
	if (!file)
	{
		return;
	}

	char map_name[64] = {};
	char variant_name[64] = "slayer";
	fscanf_s(file, "%63s %63s", map_name, (unsigned)sizeof(map_name), variant_name, (unsigned)sizeof(variant_name));
	fclose(file);

	c_map_manager* map_manager = map_manager_get();
	s_custom_map_entry* found = NULL;
	{
		c_critical_section_scope lock(map_manager->m_lock);
		for (uint16 i = 0; i < map_manager->m_map_count; i++)
		{
			s_custom_map_entry* entry = &map_manager->m_new_custom_map_entry_list_buffer[i];
			const wchar_t* file_name = wcsrchr(entry->file_path, L'\\');
			file_name = file_name ? file_name + 1 : entry->file_path;

			char file_name_utf8[MAX_PATH];
			wchar_string_to_utf8_string(file_name, file_name_utf8, NUMBEROF(file_name_utf8));
			char* extension = strrchr(file_name_utf8, '.');
			if (extension)
			{
				*extension = '\0';
			}

			if (!csstricmp(file_name_utf8, map_name))
			{
				found = entry;
				break;
			}
		}
	}

	if (!found)
	{
		// a stock halo 2 multiplayer map by its scenario path
		h1_log("autolaunch: no custom map entry for %s, launching scenarios\\multi\\%s", map_name, map_name);
		game_options_new(&g_main_game_launch_options);
		main_game_launch_set_multiplayer_variant(variant_name);
		g_main_game_launch_options.game_mode = _game_mode_multiplayer;
		g_main_game_launch_options.game_simulation = _game_simulation_local;
		swprintf_s(g_main_game_launch_options.scenario_path, L"scenarios\\multi\\%S\\%S", map_name, map_name);
		game_options_setup_default_players(1, &g_main_game_launch_options);
		main_game_change(&g_main_game_launch_options);
		return;
	}

	h1_log("autolaunch: launching %ws (%s)", found->file_path, variant_name);

	game_options_new(&g_main_game_launch_options);
	if (!csstricmp(variant_name, "campaign"))
	{
		// "<map> campaign": a campaign game on normal
		main_game_launch_set_difficulty(1);
		g_main_game_launch_options.campaign_id = 1;
		g_main_game_launch_options.coop = false;
	}
	else
	{
		main_game_launch_set_multiplayer_variant(variant_name);
		g_main_game_launch_options.game_mode = _game_mode_multiplayer;
	}
	g_main_game_launch_options.game_simulation = _game_simulation_local;
	g_main_game_launch_options.is_custom_map = true;
	csmemcpy(g_main_game_launch_options.custom_map_id.hash, found->hash, sizeof(found->hash));
	wcsncpy_s(g_main_game_launch_options.custom_map_id.map_name, found->map_name, _TRUNCATE);
	wcsncpy_s(g_main_game_launch_options.scenario_path, found->file_path, _TRUNCATE);
	game_options_setup_default_players(1, &g_main_game_launch_options);
	main_game_change(&g_main_game_launch_options);
	return;
}

/* private code */

static bool __cdecl h1_custom_map_cache_file_open(const wchar_t* path, cache_file_header* header, int32 flags)
{
	typedef bool(__cdecl* t_custom_map_cache_file_open)(const wchar_t*, cache_file_header*, int32);
	t_custom_map_cache_file_open original = Memory::GetAddress<t_custom_map_cache_file_open>(0x648ED);

	g_h1_active = false;
	g_h1_cache_file = NULL;
	g_h1_loading_cache_file.close();

	if (!h1_maps_file_is_halo1(path))
	{
		return original(path, header, flags);
	}

	h1_log("open: %ws is a halo 1 cache file, hosting it in %ws", path, k_h1_host_map_path);
	if (!g_h1_loading_cache_file.open(path))
	{
		h1_log("open: failed to load %ws", path);
		return false;
	}

	const bool result = original(k_h1_host_map_path, header, flags);
	if (!result)
	{
		h1_log("open: failed to open the host map %ws", k_h1_host_map_path);
		g_h1_loading_cache_file.close();
	}
	return result;
}

static bool __cdecl h1_custom_map_hash_verify(const uint8* expected_hash, const uint8* file_hash)
{
	typedef bool(__cdecl* t_custom_map_hash_verify)(const uint8*, const uint8*);
	t_custom_map_hash_verify original = Memory::GetAddress<t_custom_map_hash_verify>(0x8F914);

	// entry being loaded by the custom map load (FUN_00464a01)
	const s_custom_map_entry* entry = *Memory::GetAddress<s_custom_map_entry**>(0x4AE874);
	if (entry && h1_maps_file_is_halo1(entry->file_path))
	{
		uint8 hash[k_sha256_hash_size_bytes];
		return h1_file_hash(entry->file_path, hash) && !memcmp(hash, expected_hash, sizeof(hash));
	}
	return original(expected_hash, file_hash);
}

static const s_h1_map_description* h1_map_description_get(const char* file_name)
{
	for (int32 i = 0; i < NUMBEROF(k_h1_map_descriptions); i++)
	{
		if (!csstricmp(k_h1_map_descriptions[i].file_name, file_name))
		{
			return &k_h1_map_descriptions[i];
		}
	}
	return NULL;
}

// hash of the header and the first megabyte of the (compressed) file, identifies the map between peers
static bool h1_file_hash(const wchar_t* path, uint8* out_hash)
{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	const uint32 k_hash_size = 1024 * 1024;
	uint8* buffer = (uint8*)malloc(k_hash_size + sizeof(LARGE_INTEGER));
	DWORD read = 0;
	bool result = false;
	if (buffer && ReadFile(file, buffer, k_hash_size, &read, NULL))
	{
		LARGE_INTEGER size;
		GetFileSizeEx(file, &size);
		csmemcpy(buffer + read, &size, sizeof(size));

		s_sha256_hash hash;
		result = crypto_windows_sha256_hash_data(buffer, read + sizeof(size), &hash);
		csmemcpy(out_hash, hash.data, k_sha256_hash_size_bytes);
	}
	free(buffer);
	CloseHandle(file);
	return result;
}
