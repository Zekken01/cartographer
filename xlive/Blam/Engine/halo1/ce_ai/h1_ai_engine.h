#pragma once

/*
* The halo 1 AI port (halo-ce-universal source/ai, namespace h1_ai) runs on Carto: this header stands in for the halo 1 engine
* headers it included. Halo 1's types, tag blocks, data arrays and asserts are reproduced; the engine functions it calls (units,
* objects, collisions, scenario) are implemented over halo 2's objects and the halo 1 cache file in h1_ai_engine.cpp.
*/

// halo 1's code as it was written: its implicit conversions, shadowed names and boolean arithmetic aren't errors here
#pragma warning(disable: 4018 4100 4101 4127 4146 4189 4201 4244 4245 4267 4302 4305 4310 4311 4312 4334 4389 4456 4457 4458 4459 4701 4702 4703 4706 4800 4806 4996)

#include "../h1_cache_file.h"
#include "../h1_tag_definitions.h"


#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace h1_ai
{

/* ---------- types */

typedef unsigned char boolean;
typedef uint8 byte;
typedef uint16 word;
typedef uint32 dword;
typedef real32 real;
typedef real32 angle;
typedef real32 real_fraction;
#define TAG_STRING_LENGTH 31
typedef char tag_string[TAG_STRING_LENGTH + 1];
typedef uint32 tag;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// cseries.h
enum
{
	UNSIGNED_LONG_MAX_VALUE = 0xFFFFFFFF,
	LONG_BITS_BITS = 5,
	UNSIGNED_SHORT_MAX = 65535,
	SHORT_MAX = 32767,
	SHORT_MIN = -32768,
	SHORT_BITS_BITS = 4,
	UNSIGNED_CHAR_MAX = 255,
	CHAR_BITS_BITS = 3,
};

enum
{
	_x = 0,
	_y,
	_z,
	NUMBER_OF_RECTANGLE2D_COMPONENTS = 4,
	NUMBER_OF_RECTANGLE3D_COMPONENTS = 6,
	NUMBER_OF_VERTICES_PER_LINE = 2,
	NUMBER_OF_VERTICES_PER_TRIANGLE = 3,
	NUMBER_OF_VERTICES_PER_QUADRALATERAL = 4,
	NUMBER_OF_VERTICES_PER_QUADRILATERAL = 4,
	NUMBER_OF_VERTICES_PER_HEXAGON = 6,
	NUMBER_OF_VERTICES_PER_PYRAMID = 5,
	NUMBER_OF_VERTICES_PER_CUBE = 8,
	NUMBER_OF_TRIANGLES_PER_QUADRILATERAL = 2,
	NUMBER_OF_EDGES_PER_TRIANGLE = 3,
	NUMBER_OF_EDGES_PER_QUADRALATERAL = 4,
	NUMBER_OF_EDGES_PER_HEXAGON = 6,
	NUMBER_OF_FACES_PER_CUBE = 6,
};

// halo 1 tag primitives, as the cache file holds them (pointers are xbox addresses)
struct tag_block
{
	int32 count;
	uint32 address;
	uint32 definition;
};
static_assert(sizeof(tag_block) == 12);

struct tag_reference
{
	tag group_tag;
	uint32 name;
	int32 name_length;
	datum index;
};
static_assert(sizeof(tag_reference) == 16);

struct tag_data
{
	int32 size;
	uint32 flags;
	uint32 file_offset;
	uint32 address;
	uint32 definition;
};
static_assert(sizeof(tag_data) == 20);

struct short_bounds
{
	int16 lower;
	int16 upper;
};

struct real_bounds
{
	real32 lower;
	real32 upper;
};




// halo 1's c converts void * to any pointer: results the port assigns to typed pointers (allocations, data and tags) are this
struct c_void_pointer
{
	void* pointer;
	template<typename t_type> operator t_type*() const { return (t_type*)pointer; }
	explicit operator bool() const { return pointer != NULL; }
	bool operator==(decltype(nullptr)) const { return pointer == NULL; }
	bool operator!=(decltype(nullptr)) const { return pointer != NULL; }
	bool operator==(int null) const { return pointer == (void*)(intptr_t)null; }
	bool operator!=(int null) const { return pointer != (void*)(intptr_t)null; }
};

// path.h's flee routines
#define PATH_EXTERNAL_FLEE_ROUTINES

/* ---------- datum indices */

#undef DATUM_INDEX_TO_ABSOLUTE_INDEX
#undef DATUM_INDEX_TO_IDENTIFIER
#undef DATUM_INDEX_NEW
#define DATUM_INDEX_TO_ABSOLUTE_INDEX(index) ((int16)((index) & 0xFFFF))
#define DATUM_INDEX_TO_IDENTIFIER(index) ((int16)(((uint32)(index)) >> 16))
#define DATUM_INDEX_NEW(absolute_index, identifier) ((int32)(((uint32)(uint16)(identifier) << 16) | (uint16)(absolute_index)))

/* ---------- flags */

#undef FLAG
#undef TEST_FLAG
#undef BIT_VECTOR_SIZE_IN_LONGS
#undef BIT_VECTOR_TEST_FLAG
#undef BIT_VECTOR_SET_FLAG
#undef SET_FLAG
#undef NUMBEROF
#define FLAG(bit) (1u << (bit))
#define TEST_FLAG(flags, bit) (((flags) & FLAG(bit)) != 0)
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define BIT_VECTOR_SIZE_IN_LONGS(count) (((count) + 31) >> 5)
#define BIT_VECTOR_TEST_FLAG(vector, bit) (((vector)[(bit) >> 5] & FLAG((bit) & 31)) != 0)
#define BIT_VECTOR_SET_FLAG(vector, bit, value) ((value) ? ((vector)[(bit) >> 5] |= FLAG((bit) & 31)) : ((vector)[(bit) >> 5] &= ~FLAG((bit) & 31)))
#define NUMBEROF(array) ((int32)(sizeof(array) / sizeof((array)[0])))

/* ---------- asserts and errors */

enum
{
	_error_silent = 0,
	_error_warning,
	_error_status,
	_error_message,
	_error_critical,
};

void ai_assert_failed(const char* file, int32 line, const char* expression, const char* message);
void error(int32 level, const char* format, ...);
const char* csprintf(char* buffer, const char* format, ...);
extern char temporary[1024];

// as halo 1's release builds: a failed assert is logged and the game goes on (statements ending in a brace, as halo 1's, which
// some uses rely on)
#undef assert
#undef vassert
#undef halt
#undef error
#define match_assert(file, line, expression) if (!(expression)) { ai_assert_failed(file, line, #expression, NULL); }
#define match_vassert(file, line, expression, message) if (!(expression)) { ai_assert_failed(file, line, #expression, NULL); }
#define match_warn(file, line, expression) if (!(expression)) { }
#define match_vwarn(file, line, expression, message) if (!(expression)) { }
#define assert(expression) if (!(expression)) { ai_assert_failed(__FILE__, __LINE__, #expression, NULL); }
#define vassert(expression, message) if (!(expression)) { ai_assert_failed(__FILE__, __LINE__, #expression, NULL); }

#define match_dassert(file, line, expression, diagnostic) match_vassert(file, line, expression, diagnostic)
#define match_dwarn(file, line, expression, diagnostic) match_vwarn(file, line, expression, diagnostic)

/* ---------- memory and strings */

#define match_malloc(file, line, size) (h1_ai::c_void_pointer{ malloc(size) })
#define match_free(file, line, pointer) free(pointer)
#define debug_free(pointer, file, line) free(pointer)
#undef SHORT_FIXED_TO_LONG
#define SHORT_FIXED_TO_LONG(f) ((f) >> 8)

#define csmemset memset
#define csmemcpy memcpy
#define csmemmove memmove
#define csstrcpy strcpy
#define csstrcat strcat
#define csstrncpy strncpy
#define csstrlen strlen
#define csstrcmp strcmp

/* ---------- tag access */

void* tag_block_get_element(const void* block, int32 index, int32 element_size);
c_void_pointer tag_get(tag group_tag, datum tag_index);
const char* tag_get_name(datum tag_index);
const char* tag_name_strip_path(const char* name);

tag tag_get_group_tag(datum tag_index);
// a tag block's elements (its xbox address resolved)
c_void_pointer tag_block_address(const tag_block* block);

#undef TAG_BLOCK_GET_ELEMENT
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type*)tag_block_get_element((block), (index), (int32)sizeof(type)))

/* ---------- data arrays (memory/data.c) */

struct data_array
{
	char name[TAG_STRING_LENGTH + 1];
	int16 maximum_count;
	int16 size;
	boolean valid;
	boolean identifier_zero_invalid;
	uint32 signature;
	int16 first_free_absolute_index;
	int16 count;			// slots up to the last one in use
	int16 actual_count;		// slots in use
	int16 next_identifier;
	void* data;
};

struct data_iterator
{
	data_array* data;
	int16 absolute_index;
	int16 pad;
	int32 datum_index;
	uint32 signature;
};

data_array* game_state_data_new(const char* name, int32 maximum_count, int32 size);
c_void_pointer game_state_malloc(const char* name, const char* type, int32 size);
void data_make_valid(data_array* data);
void data_make_invalid(data_array* data);
void data_delete_all(data_array* data);
int32 datum_new(data_array* data);
void datum_delete(data_array* data, int32 index);
c_void_pointer datum_get(data_array* data, int32 index);
c_void_pointer datum_try_and_get(data_array* data, int32 index);
void data_iterator_new(data_iterator* iterator, data_array* data);
c_void_pointer data_iterator_next(data_iterator* iterator);

/* ---------- scenario and structure bsp (halo 1's, the fields the AI reads at their cache file offsets) */

struct scenario
{
	uint8 pad_0[0x204];
	tag_block object_names;				// 0x204
	uint8 pad_210[0x420 - 0x210];
	tag_block ai_actor_palette;			// 0x420
	tag_block ai_encounters;			// 0x42C
	tag_block ai_command_lists;			// 0x438
	tag_block ai_animation_references;	// 0x444
	tag_block ai_script_references;		// 0x450
	tag_block ai_recording_references;	// 0x45C
	tag_block ai_conversations;			// 0x468
	uint8 pad_474[0x5A4 - 0x474];
	tag_block structure_bsps;			// 0x5A4
};
static_assert(offsetof(scenario, ai_encounters) == 0x42C && offsetof(scenario, structure_bsps) == 0x5A4);

struct structure_bsp
{
	uint8 pad_0[0xB0];
	tag_block collision_bsp;			// 0xB0
	uint8 pad_bc[0x134 - 0xBC];
	tag_block clusters;					// 0x134
	uint8 pad_140[0x1E4 - 0x140];
	tag_block pathfinding_surfaces;		// 0x1E4
	tag_block pathfinding_edges;		// 0x1F0
};
static_assert(offsetof(structure_bsp, pathfinding_edges) == 0x1F0);

struct collision_bsp;

scenario* global_scenario_get(void);
scenario* global_scenario_try_and_get(void);
structure_bsp* global_structure_bsp_get(void);
collision_bsp* global_collision_bsp_get(void);
int16 global_structure_bsp_index_get(void);

/* ---------- system and game */

// halo 1 ticks at 30 a second: the AI updates on halo 1 ticks and game_time_get is their count
constexpr int32 TICKS_PER_SECOND = 30;
constexpr int32 ACTUAL_TICKS_PER_SECOND = 30;
enum
{
	MAXIMUM_CLUSTERS_PER_STRUCTURE = 512,
	// halo_port_capacity.h (the native ports' object capacity)
	HALO_PORT_MAXIMUM_OBJECTS_PER_MAP = 8192,
	HALO_PORT_OBJECT_MEMORY_POOL_SIZE = 0x800000,
	HALO_PORT_MAXIMUM_RENDERED_OBJECTS = 1024,
	// interface.h
	NUMBER_OF_INTERFACE_TAGS = 16,
};

#define global_structure_bsp_index (global_structure_bsp_index_get())
boolean game_in_editor(void);

/* ---------- players (players.h): the AI's mirror of carto's players */

struct player_datum
{
	int16 identifier;
	int16 local_player_index;
	int32 team_index;
	int32 unit_index;
	int32 dead_unit_index;
	int16 cluster_index;
	int16 pad;
	int32 aim_assist_unit_index;
	int32 aim_assist_timestamp;
};

extern data_array* player_data;
#undef player_get
#undef player_try_and_get
#define player_get(index) ((struct player_datum*)datum_get(player_data, (index)))
#define player_try_and_get(index) ((struct player_datum*)datum_try_and_get(player_data, (index)))
const uint32* players_get_combined_pvs(void);

uint32 system_milliseconds(void);
uint32 system_seconds(void);
boolean game_engine_running(void);

/* ---------- console */

void console_printf(bool clear, const char* format, ...);

} // namespace h1_ai

// halo 1's math (its types, random numbers, 2d geometry and the rest), in the namespace
#undef vectors_interpolate
#include "integer_math.h"
#include "real_math.h"
#include "real_math_planes.h"
#include "real_math_cones.h"
#include "geometry.h"
