#pragma once

/*
* Halo 1 (Xbox, cache version 5) tag primitives.
* Pointers stored inside Halo 1 tag data are Xbox virtual addresses; they're
* resolved through the active c_h1_cache_file (see h1_cache_file.h).
*/

#pragma pack(push, 1)

template<typename T>
struct h1_tag_block
{
	int32 count;
	uint32 address;
	uint32 definition;
};
static_assert(sizeof(h1_tag_block<int32>) == 12);

struct h1_tag_reference
{
	uint32 group_tag;
	uint32 name_address;
	int32 name_length;
	datum index;
};
static_assert(sizeof(h1_tag_reference) == 16);

struct h1_tag_data
{
	int32 size;
	uint32 flags;
	uint32 file_offset;
	uint32 address;
	uint32 definition;
};
static_assert(sizeof(h1_tag_data) == 20);

#pragma pack(pop)

#include "h1_tag_definitions_generated.h"
