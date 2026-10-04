#include "stdafx.h"
#include "h1_runtime.h"

#include "h1_log.h"

#include "cache/cache_files.h"
#include "tag_files/tag_loader/tag_injection.h"

/* globals */

static uint32 g_h1_runtime_used_size = 0;
static int32 g_h1_runtime_next_tag_slot = 0;

/* public code */

void h1_runtime_begin(void)
{
	g_h1_runtime_used_size = 0;

	// find the first free slot after the host map's own tags
	cache_file_tags_header* tags_header = cache_files_get_tags_header();
	int32 slot = 0;
	for (int32 i = 0; i < FIRST_SHARED_TAG_INSTANCE_INDEX && i < tags_header->tag_count; i++)
	{
		if (tags_header->tag_instances[i].tag_index != NONE)
		{
			slot = i + 1;
		}
	}
	g_h1_runtime_next_tag_slot = slot;
	h1_log("runtime: host has %d local tags, %d total instances", slot, tags_header->tag_count);
	return;
}

void* h1_runtime_allocate(uint32 size, uint32* out_offset)
{
	const uint32 aligned_size = (size + 15) & ~15u;
	uint32 offset = 0;

	// keep 16 byte alignment for everything we hand out
	uint32 probe_offset = 0;
	tag_injection_reserve_cache_memory(0, &probe_offset);
	const uint32 padding = ((probe_offset + 15) & ~15u) - probe_offset;
	if (padding)
	{
		tag_injection_reserve_cache_memory(padding, &probe_offset);
	}

	void* result = tag_injection_reserve_cache_memory(aligned_size, &offset);
	csmemset(result, 0, aligned_size);
	g_h1_runtime_used_size += aligned_size + padding;

	if (out_offset)
	{
		*out_offset = offset;
	}
	return result;
}

uint32 h1_runtime_offset_from_pointer(const void* pointer)
{
	return (uint32)((uintptr_t)pointer - (uintptr_t)cache_get_tag_data());
}

void* h1_runtime_block_allocate(s_tag_block* block, uint32 element_size, int32 count)
{
	if (count <= 0)
	{
		block->count = 0;
		block->data = 0;
		return NULL;
	}

	uint32 offset;
	void* result = h1_runtime_allocate(element_size * count, &offset);
	block->count = count;
	block->data = offset;
	return result;
}

void h1_runtime_data_set(tag_data* data, const void* source, int32 size)
{
	if (size <= 0)
	{
		data->size = 0;
		data->data = 0;
		return;
	}

	uint32 offset;
	void* buffer = h1_runtime_allocate(size, &offset);
	if (source)
	{
		csmemcpy(buffer, source, size);
	}
	data->size = size;
	data->data = offset;
	return;
}

datum h1_runtime_tag_new(tag_group group, const char* name, uint32 size, void** out_data)
{
	cache_file_tags_header* tags_header = cache_files_get_tags_header();

	while (g_h1_runtime_next_tag_slot < FIRST_SHARED_TAG_INSTANCE_INDEX &&
		tags_header->tag_instances[g_h1_runtime_next_tag_slot].tag_index != NONE)
	{
		g_h1_runtime_next_tag_slot++;
	}

	if (g_h1_runtime_next_tag_slot >= FIRST_SHARED_TAG_INSTANCE_INDEX)
	{
		h1_log("runtime: out of tag instance slots creating %s", name);
		return NONE;
	}

	const int32 absolute_index = g_h1_runtime_next_tag_slot++;
	const datum tag_index = DATUM_INDEX_NEW(absolute_index, (0xC000 | (absolute_index & 0x3FFF)));

	uint32 offset;
	void* data = h1_runtime_allocate(size, &offset);

	cache_file_tag_instance* instance = &tags_header->tag_instances[absolute_index];
	instance->group_tag = group;
	instance->tag_index = tag_index;
	instance->data_offset = offset;
	instance->size = size;

	tag_add_name(tag_index, name);

	if (out_data)
	{
		*out_data = data;
	}
	return tag_index;
}

datum h1_runtime_tag_find(tag_group group, const char* name)
{
	return tag_loaded(group, name);
}

void h1_runtime_reference_set(tag_reference* reference, tag_group group, datum index)
{
	reference->group = group;
	reference->index = index;
	return;
}

uint32 h1_runtime_used_size(void)
{
	return g_h1_runtime_used_size;
}
