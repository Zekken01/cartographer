#include "stdafx.h"
#include "h1_ai_internal.h"

#include "../h1_log.h"
#include "../h1_map_loader.h"

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

data_array* game_state_data_new(const char* name, int32 maximum_count, int32 size)
{
	data_array* data = (data_array*)calloc(1, sizeof(data_array));
	strncpy(data->name, name, TAG_STRING_LENGTH);
	data->maximum_count = (int16)maximum_count;
	data->size = (int16)size;
	data->signature = k_data_signature;
	data->data = calloc(maximum_count, size);
	return data;
}

c_void_pointer game_state_malloc(const char* name, const char* type, int32 size)
{
	return { calloc(1, size) };
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
		ai_assert_failed(__FILE__, __LINE__, "datum_get: invalid index", data ? data->name : NULL);
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

} // namespace h1_ai
