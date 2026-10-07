#include "stdafx.h"

#include <unordered_map>
#include "h1_ai_internal.h"

#include "../h1_log.h"
#include "../h1_map_loader.h"
#include "../h1_game_state.h"
#include <vector>

#include <stdarg.h>
#include <stdlib.h>

namespace h1_ai
{

/*
* The halo 1 engine the AI port calls, over carto: asserts and errors go to the halo 1 log, tags come from the halo 1 cache file,
* data arrays are halo 1's (memory/data.c: an identifier per slot, datum indices of identifier and absolute index).
*/

/* ---------- globals */

char temporary[1024];

/* ---------- asserts and errors */

void ai_assert_failed(const char* file, int32 line, const char* expression, const char* message)
{
	// a failing assert in an update would repeat every tick: logged a few times only
	static int32 s_count = 0;
	if (s_count < 64)
	{
		s_count++;
		const char* name = strrchr(file, '\\');
		h1_log("ai: assert %s:%d %s%s%s", name ? name + 1 : file, line, expression, message ? " " : "", message ? message : "");
	}
	return;
}

void error(int32 level, const char* format, ...)
{
	// each message's first few times (halo 1's debug checks repeat every tick)
	static std::unordered_map<const char*, int32> s_counts;
	if (++s_counts[format] > 3)
	{
		return;
	}
	char buffer[1024];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(buffer, sizeof(buffer), format, arguments);
	va_end(arguments);
	h1_log("ai: %s", buffer);
	return;
}

const char* csprintf(char* buffer, const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(buffer, sizeof(temporary), format, arguments);
	va_end(arguments);
	return buffer;
}

void console_printf(bool clear, const char* format, ...)
{
	char buffer[1024];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(buffer, sizeof(buffer), format, arguments);
	va_end(arguments);
	h1_log("ai: %s", buffer);
	return;
}

/* ---------- system and game */

uint32 system_milliseconds(void)
{
	return GetTickCount();
}

uint32 system_seconds(void)
{
	return GetTickCount() / 1000;
}

// a multiplayer game engine: halo 1's campaign AI runs without one
boolean game_engine_running(void)
{
	return FALSE;
}

/* ---------- tag access */

void* tag_block_get_element(const void* block, int32 index, int32 element_size)
{
	const tag_block* tag_block_pointer = (const tag_block*)block;
	if (!g_h1_cache_file || !tag_block_pointer || index < 0 || index >= tag_block_pointer->count)
	{
		ai_assert_failed(__FILE__, __LINE__, "VALID_INDEX(index, block->count)", NULL);
		return NULL;
	}
	return g_h1_cache_file->address_get(tag_block_pointer->address + (uint32)(element_size * index), (uint32)element_size);
}

c_void_pointer tag_get(tag group_tag, datum tag_index)
{
	return { g_h1_cache_file && tag_index != NONE ? g_h1_cache_file->tag_get(group_tag, tag_index) : NULL };
}

const char* tag_get_name(datum tag_index)
{
	const char* name = g_h1_cache_file && tag_index != NONE ? g_h1_cache_file->tag_name_get(tag_index) : NULL;
	return name ? name : "<none>";
}

const char* tag_name_strip_path(const char* name)
{
	const char* last = name ? strrchr(name, '\\') : NULL;
	return last ? last + 1 : name;
}

/* ---------- data arrays (data.c) */

static const uint32 k_data_signature = 'd@t@';

static inline int16* datum_identifier(data_array* data, int32 absolute_index)
{
	return (int16*)((uint8*)data->data + (size_t)absolute_index * data->size);
}

// game_state_malloc's and game_state_data_new's allocations: halo 1's game state of the AI, saved and restored with halo 2's
// checkpoints (allocated once, at ai_initialize, never freed)
struct s_game_state_allocation
{
	void* address;
	size_t size;
	std::vector<uint8> saved[2];
};
static std::vector<s_game_state_allocation>& game_state_allocations(void)
{
	static std::vector<s_game_state_allocation> allocations;
	return allocations;
}
static void* game_state_allocation_new(size_t size)
{
	void* address = calloc(1, size);
	game_state_allocations().push_back({ address, size });
	return address;
}
static void game_state_allocations_save(int32 slot)
{
	for (s_game_state_allocation& allocation : game_state_allocations())
	{
		allocation.saved[slot].assign((const uint8*)allocation.address, (const uint8*)allocation.address + allocation.size);
	}
	return;
}
static void game_state_allocations_restore(int32 slot)
{
	for (s_game_state_allocation& allocation : game_state_allocations())
	{
		if (allocation.saved[slot].size() == allocation.size)
		{
			csmemcpy(allocation.address, allocation.saved[slot].data(), allocation.size);
		}
	}
	return;
}
static c_h1_game_state_procedures g_game_state_allocations_game_state(game_state_allocations_save, game_state_allocations_restore);

data_array* game_state_data_new(const char* name, int32 maximum_count, int32 size)
{
	data_array* data = (data_array*)game_state_allocation_new(sizeof(data_array));
	strncpy(data->name, name, TAG_STRING_LENGTH);
	data->maximum_count = (int16)maximum_count;
	data->size = (int16)size;
	data->signature = k_data_signature;
	data->data = game_state_allocation_new((size_t)maximum_count * size);
	return data;
}

c_void_pointer game_state_malloc(const char* name, const char* type, int32 size)
{
	return { game_state_allocation_new(size) };
}

void data_delete_all(data_array* data)
{
	csmemset(data->data, 0, (size_t)data->maximum_count * data->size);
	data->first_free_absolute_index = 0;
	data->count = 0;
	data->actual_count = 0;
	data->next_identifier = (int16)0x8000;
	return;
}

void data_make_valid(data_array* data)
{
	data->valid = TRUE;
	data_delete_all(data);
	return;
}

void data_make_invalid(data_array* data)
{
	data->valid = FALSE;
	return;
}

int32 datum_new(data_array* data)
{
	if (!data->valid)
	{
		return NONE;
	}
	for (int32 absolute_index = data->first_free_absolute_index; absolute_index < data->maximum_count; absolute_index++)
	{
		int16* identifier = datum_identifier(data, absolute_index);
		if (*identifier != 0)
		{
			continue;
		}
		csmemset(identifier, 0, data->size);
		*identifier = data->next_identifier;
		data->next_identifier = (int16)(data->next_identifier + 1);
		if (data->next_identifier == 0)
		{
			data->next_identifier = (int16)0x8000;
		}
		data->actual_count++;
		data->first_free_absolute_index = (int16)(absolute_index + 1);
		if (absolute_index + 1 > data->count)
		{
			data->count = (int16)(absolute_index + 1);
		}
		return DATUM_INDEX_NEW(absolute_index, *identifier);
	}
	return NONE;
}

// data.c datum_new_at_index: the datum at a given index (and identifier) when it is free
int32 datum_new_at_index(data_array* data, int32 index)
{
	const int16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	const int16 identifier = DATUM_INDEX_TO_IDENTIFIER(index);
	if (!data->valid || absolute_index < 0 || absolute_index >= data->maximum_count || identifier == 0)
	{
		return NONE;
	}
	int16* header = datum_identifier(data, absolute_index);
	if (*header != 0)
	{
		return NONE;
	}
	data->actual_count++;
	if (absolute_index >= data->count)
	{
		data->count = (int16)(absolute_index + 1);
	}
	csmemset(header, 0, data->size);
	*header = identifier;
	return DATUM_INDEX_NEW(absolute_index, identifier);
}

void datum_delete(data_array* data, int32 index)
{
	int16* identifier = datum_get(data, index);
	if (!identifier)
	{
		return;
	}
	const int16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	*identifier = 0;
	data->actual_count--;
	if (absolute_index < data->first_free_absolute_index)
	{
		data->first_free_absolute_index = absolute_index;
	}
	while (data->count > 0 && *datum_identifier(data, data->count - 1) == 0)
	{
		data->count--;
	}
	return;
}

c_void_pointer datum_try_and_get(data_array* data, int32 index)
{
	if (!data || !data->valid || index == NONE)
	{
		return { NULL };
	}
	const int16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	if (absolute_index < 0 || absolute_index >= data->count)
	{
		return { NULL };
	}
	int16* identifier = datum_identifier(data, absolute_index);
	const int16 index_identifier = DATUM_INDEX_TO_IDENTIFIER(index);
	if (*identifier == 0 || (index_identifier != 0 && *identifier != index_identifier))
	{
		return { NULL };
	}
	return { identifier };
}

c_void_pointer datum_get(data_array* data, int32 index)
{
	c_void_pointer datum = datum_try_and_get(data, index);
	if (!datum)
	{
		char description[64];
		sprintf_s(description, "%s %08X from %08X", data ? data->name : "", (uint32)index, (uint32)((uintptr_t)_ReturnAddress() - (uintptr_t)GetModuleHandleA("xlive.dll")));
		ai_assert_failed(__FILE__, __LINE__, "datum_get: invalid index", description);
		// halo 1's release datum_get doesn't test the index: its code reads what's there and carries on (ai_communication.c's
		// reply filters ask for a non-actor target unit's actor). A zeroed datum, not NULL
		static uint64 s_invalid_datum[0x2000 / sizeof(uint64)];
		csmemset(s_invalid_datum, 0, sizeof(s_invalid_datum));
		*(int16*)s_invalid_datum = NONE;
		datum = { s_invalid_datum };
	}
	return datum;
}

void data_iterator_new(data_iterator* iterator, data_array* data)
{
	iterator->data = data;
	iterator->absolute_index = 0;
	iterator->datum_index = NONE;
	iterator->signature = k_data_signature;
	return;
}

c_void_pointer data_iterator_next(data_iterator* iterator)
{
	data_array* data = iterator->data;
	if (!data || !data->valid)
	{
		return { NULL };
	}
	while (iterator->absolute_index < data->count)
	{
		const int16 absolute_index = iterator->absolute_index++;
		int16* identifier = datum_identifier(data, absolute_index);
		if (*identifier != 0)
		{
			iterator->datum_index = DATUM_INDEX_NEW(absolute_index, *identifier);
			return { identifier };
		}
	}
	iterator->datum_index = NONE;
	return { NULL };
}

/* ---------- game */

boolean game_in_editor(void)
{
	return FALSE;
}

/* ---------- scenario and structure bsp */

scenario* global_scenario_get(void)
{
	return g_h1_cache_file ? (scenario*)g_h1_cache_file->scenario_get() : NULL;
}

scenario* global_scenario_try_and_get(void)
{
	return h1_maps_active() ? global_scenario_get() : NULL;
}

int16 global_structure_bsp_index_get(void)
{
	return h1_maps_structure_bsp_index();
}

structure_bsp* global_structure_bsp_get(void)
{
	return g_h1_cache_file ? (structure_bsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(global_structure_bsp_index_get())) : NULL;
}

collision_bsp* global_collision_bsp_get(void)
{
	structure_bsp* bsp = global_structure_bsp_get();
	return bsp ? TAG_BLOCK_GET_ELEMENT(&bsp->collision_bsp, 0, collision_bsp) : NULL;
}

// scenario.c: the collision bsp's 3d bsp (its first blocks)
struct bsp3d* global_bsp3d_get(void)
{
	return (struct bsp3d*)global_collision_bsp_get();
}

long scenario_leaf_index_from_point(const union real_point3d* point)
{
	return bsp3d_test_point(global_bsp3d_get(), 0, point);
}

/* ---------- collision_usage.c: the collision users (collision_log_initialize pushes the first) */

short global_current_collision_user_depth = 1;
short global_current_collision_users[MAXIMUM_COLLISION_USER_STACK_DEPTH];

/* ---------- structures.c: the clusters a search has visited */

static struct
{
	boolean cluster_marker_initialized;
	uint32 cluster_marker;
	uint32 cluster_magic_numbers[MAXIMUM_CLUSTERS_PER_STRUCTURE];
} structure_globals;

void structure_cluster_marker_begin(void)
{
	ASSERT(!structure_globals.cluster_marker_initialized);
	structure_globals.cluster_marker++;
	structure_globals.cluster_marker_initialized = TRUE;
	return;
}

void structure_cluster_marker_end(void)
{
	ASSERT(structure_globals.cluster_marker_initialized);
	structure_globals.cluster_marker_initialized = FALSE;
	return;
}

boolean structure_cluster_unmarked(short cluster_index)
{
	ASSERT(structure_globals.cluster_marker_initialized);
	ASSERT(cluster_index >= 0 && cluster_index < MAXIMUM_CLUSTERS_PER_STRUCTURE);
	return (boolean)(structure_globals.cluster_magic_numbers[cluster_index] != structure_globals.cluster_marker);
}

boolean structure_cluster_mark(short cluster_index)
{
	ASSERT(structure_globals.cluster_marker_initialized);
	ASSERT(cluster_index >= 0 && cluster_index < MAXIMUM_CLUSTERS_PER_STRUCTURE);
	if (structure_globals.cluster_magic_numbers[cluster_index] != structure_globals.cluster_marker)
	{
		structure_globals.cluster_magic_numbers[cluster_index] = structure_globals.cluster_marker;
		return TRUE;
	}
	return FALSE;
}

} // namespace h1_ai
