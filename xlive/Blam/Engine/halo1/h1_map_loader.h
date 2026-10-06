#pragma once

/*
* Halo 1 (Xbox) cache file support.
*
* Halo 1 maps show up in the custom map list like any other map. When one is
* loaded the engine opens a Halo 2 multiplayer map as the "host" cache (for
* globals, multiplayer globals, the shared resource database etc.) and the
* Halo 1 cache file is read into memory. Once the host tags are loaded the Halo 1
* scenario, structure bsp and their dependencies are built into the tag cache
* and replace the host's scenario.
*/

struct s_custom_map_entry;
struct cache_file_header;

/* prototypes */

void h1_maps_apply_patches(void);

// Fills a custom map list entry if the file is a Halo 1 map, returns false otherwise
bool h1_maps_custom_map_entry_fill(s_custom_map_entry* entry);

bool h1_maps_file_is_halo1(const wchar_t* path);

// adds every Halo 1 map in the game's maps\ce folder to the custom map list
void h1_maps_register_folder(class c_map_manager* map_manager);

// true while the loaded game is backed by a Halo 1 cache file
bool h1_maps_active(void);

// additional tag cache memory to allocate for the Halo 1 data
uint32 h1_maps_tag_memory_required(void);

// called by scenario_tags_load once the host cache file's tags are in memory
bool h1_maps_scenario_tags_loaded(bool custom_map);

// called every main loop iteration
void h1_maps_update(void);

// the halo 1 structure bsp of halo 2's current structure bsp (they're built in the same order), 0 without one
int16 h1_maps_structure_bsp_index(void);
// the cluster of a structure bsp a point is in (NONE outside its open space), the leaf of its collision bsp in out_leaf_index
int32 h1_maps_structure_bsp_leaf_get(int32 bsp_index, const real_point3d* point, int32* out_leaf_index);

// whether a point is inside one of the halo 1 scenario's trigger volumes
bool h1_maps_trigger_volume_test_point(int16 trigger_volume_index, const real_point3d* point);
