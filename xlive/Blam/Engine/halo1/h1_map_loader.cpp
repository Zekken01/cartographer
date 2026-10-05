#include "stdafx.h"
#include "h1_map_loader.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_scenery.h"
#include "h1_projectile_logic.h"
#include "h1_weapon_logic.h"
#include "h1_log.h"
#include "h1_render.h"
#include "h1_runtime.h"
#include "h1_scenario.h"
#include "h1_sound.h"

#include "cache/cache_files.h"
#include "game/game.h"
#include "game/game_options.h"
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

/* public code */

void h1_maps_apply_patches(void)
{
	// custom map cache file open inside the custom map load (FUN_00464a01)
	PatchCall(Memory::GetAddress(0x64B68), h1_custom_map_cache_file_open);
	// the custom map load compares a hash of the file against the map id, Halo 1 entries carry our own hash
	PatchCall(Memory::GetAddress(0x64B44), h1_custom_map_hash_verify);
	h1_effects_apply_patches();
	h1_scenery_apply_patches();
	h1_weapon_logic_apply_patches();
	h1_projectile_logic_apply_patches();
	return;
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

	// only multiplayer maps are playable for now
	if (header.type != 1)
	{
		h1_log("maps: skipping %ws (%s), scenario type %d is not multiplayer", entry->file_path, header.name, header.type);
		return false;
	}

	const s_h1_map_description* description = h1_map_description_get(header.name);

	wchar_t display_name[k_custom_map_name_length];
	if (description)
	{
		swprintf_s(display_name, L"%s (CE)", description->display_name);
	}
	else
	{
		wchar_t name[32];
		utf8_string_to_wchar_string(header.name, name, NUMBEROF(name));
		swprintf_s(display_name, L"%s (CE)", name);
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
	}
	return result;
}

void h1_maps_update(void)
{
	h1_sound_update();

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
		h1_log("autolaunch: no custom map entry for %s", map_name);
		return;
	}

	h1_log("autolaunch: launching %ws (%s)", found->file_path, variant_name);

	game_options_new(&g_main_game_launch_options);
	main_game_launch_set_multiplayer_variant(variant_name);
	g_main_game_launch_options.game_mode = _game_mode_multiplayer;
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
