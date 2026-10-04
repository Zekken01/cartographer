#pragma once
#include "h1_cache_file.h"

#include "tag_files/tag_block.h"

/*
* Helpers for building Halo 2 tag data at runtime from Halo 1 tags.
* All memory comes from the tag cache (so tag block offsets resolve normally)
* and new tags are placed in the unused instance slots between the host map's
* own tags and the shared tag instances, so the engine iterates them like any
* other tag of the loaded map.
*/

/* prototypes */

// resets the runtime allocator, called once the host tags are loaded
void h1_runtime_begin(void);

// allocates zeroed, 16 byte aligned tag cache memory
void* h1_runtime_allocate(uint32 size, uint32* out_offset = NULL);

uint32 h1_runtime_offset_from_pointer(const void* pointer);

// allocates a new block and assigns it to the tag block
void* h1_runtime_block_allocate(s_tag_block* block, uint32 element_size, int32 count);

template<typename T>
T* h1_runtime_block_new(s_tag_block* block, int32 count)
{
	return (T*)h1_runtime_block_allocate(block, sizeof(T), count);
}

template<typename T>
T* h1_runtime_block_new(tag_block<T>* block, int32 count)
{
	return (T*)h1_runtime_block_allocate((s_tag_block*)block, sizeof(T), count);
}

void h1_runtime_data_set(tag_data* data, const void* source, int32 size);

// creates a new tag instance with zeroed data of the given size
datum h1_runtime_tag_new(tag_group group, const char* name, uint32 size, void** out_data);

template<typename T>
datum h1_runtime_tag_new(tag_group group, const char* name, T** out_data)
{
	return h1_runtime_tag_new(group, name, sizeof(T), (void**)out_data);
}

// looks up a tag in the loaded (host + shared) tag cache by name
datum h1_runtime_tag_find(tag_group group, const char* name);

void h1_runtime_reference_set(tag_reference* reference, tag_group group, datum index);

uint32 h1_runtime_used_size(void);

// halo 1 placement rotation (yaw, pitch, roll in radians) and position to a halo matrix
void h1_matrix_from_euler(const real_euler_angles3d* angles, const real_point3d* position, real_matrix4x3* out);
