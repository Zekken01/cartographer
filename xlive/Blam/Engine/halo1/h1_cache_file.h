#pragma once
#include "h1_tag_definitions.h"

/* constants */

enum
{
	k_h1_cache_file_version = 5,
	k_h1_maximum_structure_bsps = 32,
};

constexpr uint32 k_h1_tag_cache_base_address = 0x803A6000;

/* structures */

#pragma pack(push, 1)
struct h1_cache_file_header
{
	uint32 header_signature;		// 'head'
	int32 version;					// 5 on Xbox
	int32 file_length;				// decompressed length
	int32 compressed_padding;
	int32 tag_data_offset;
	int32 tag_data_size;
	int8 pad_18[8];
	char name[32];
	char build[32];
	int16 type;						// 0 solo, 1 multiplayer, 2 main menu
	int16 pad_62;
	uint32 checksum;
	int8 pad_68[0x794];
	uint32 footer_signature;		// 'foot'
};
static_assert(sizeof(h1_cache_file_header) == 0x800);

struct h1_cache_file_tag_instance
{
	uint32 group_tag;
	uint32 parent_group_tags[2];
	datum tag_index;
	uint32 name_address;
	uint32 base_address;
	uint32 unused[2];
};
static_assert(sizeof(h1_cache_file_tag_instance) == 32);

struct h1_cache_file_tags_header
{
	uint32 tag_instances_address;
	datum scenario_index;
	uint32 checksum;
	int32 tag_count;
	int32 vertex_buffer_count;
	uint32 vertex_buffers_address;
	int32 index_buffer_count;
	uint32 index_buffers_address;
	uint32 signature;				// 'tags'
};
static_assert(sizeof(h1_cache_file_tags_header) == 36);

struct h1_structure_bsp_header
{
	uint32 base_address;
	int32 vertex_buffer_count;
	uint32 vertex_buffers;
	int32 index_buffer_count;
	uint32 index_buffers;
	uint32 signature;				// 'sbsp'
};
static_assert(sizeof(h1_structure_bsp_header) == 24);
#pragma pack(pop)

/* classes */

class c_h1_cache_file
{
public:
	c_h1_cache_file(void);
	~c_h1_cache_file(void);

	bool open(const wchar_t* path);
	void close(void);
	bool is_open(void) const { return m_data != NULL; }

	const h1_cache_file_header* header(void) const { return &m_header; }
	const wchar_t* path(void) const { return m_path; }
	const char* name(void) const { return m_header.name; }

	// resolves an Xbox address inside the tag cache or a loaded structure bsp
	void* address_get(uint32 address, uint32 size = 1) const;
	const char* string_get(uint32 address) const;

	template<typename T>
	T* block_get(const h1_tag_block<T>& block, int32 index) const
	{
		if (index < 0 || index >= block.count)
		{
			return NULL;
		}
		return (T*)address_get(block.address + sizeof(T) * index, sizeof(T));
	}

	void* data_get(const h1_tag_data& data) const;

	int32 tag_count(void) const;
	const h1_cache_file_tag_instance* tag_instance_get(datum tag_index) const;
	const h1_cache_file_tag_instance* tag_instance_get_by_absolute_index(int32 absolute_index) const;
	// returns NULL if the tag doesn't match the expected group (or one of its parents)
	void* tag_get(uint32 group_tag, datum tag_index) const;
	void* tag_get(const h1_tag_reference& reference) const { return tag_get(reference.group_tag, reference.index); }
	const char* tag_name_get(datum tag_index) const;
	datum tag_find(uint32 group_tag, const char* name) const;

	datum scenario_index(void) const { return m_tags_header->scenario_index; }
	h1_scnr* scenario_get(void) const;
	int32 structure_bsp_count(void) const { return m_structure_bsp_count; }
	datum structure_bsp_tag_get(int32 bsp_index) const;

	// raw file bytes (decompressed), used for texture and sound data that lives outside of the tag cache
	const uint8* file_data(void) const { return m_data; }
	uint32 file_size(void) const { return m_size; }

private:
	struct s_region
	{
		uint32 base_address;
		uint32 file_offset;
		uint32 size;
	};

	wchar_t m_path[MAX_PATH];
	h1_cache_file_header m_header;
	uint8* m_data;
	uint32 m_size;
	h1_cache_file_tags_header* m_tags_header;
	h1_cache_file_tag_instance* m_instances;

	s_region m_regions[k_h1_maximum_structure_bsps + 1];
	int32 m_region_count;

	datum m_structure_bsp_tags[k_h1_maximum_structure_bsps];
	int32 m_structure_bsp_count;
};

/* prototypes */

// Peeks at the header of a file to decide if it's a Halo 1 Xbox cache file
bool h1_cache_file_read_header(const wchar_t* path, h1_cache_file_header* out_header);

/* globals */

// the Halo 1 cache file backing the currently loaded game, or NULL
extern c_h1_cache_file* g_h1_cache_file;
