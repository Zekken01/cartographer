#include "stdafx.h"
#include "h1_cache_file.h"

#include "h1_log.h"

#include <zlib.h>

/* globals */

c_h1_cache_file* g_h1_cache_file = NULL;

/* public code */

bool h1_cache_file_read_header(const wchar_t* path, h1_cache_file_header* out_header)
{
	bool result = false;
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD read = 0;
		if (ReadFile(file, out_header, sizeof(h1_cache_file_header), &read, NULL) && read == sizeof(h1_cache_file_header))
		{
			result =
				out_header->header_signature == 'head' &&
				out_header->footer_signature == 'foot' &&
				out_header->version == k_h1_cache_file_version &&
				out_header->file_length > 0 &&
				out_header->tag_data_offset > 0 &&
				out_header->tag_data_size > 0 &&
				csstrnlen(out_header->name, sizeof(out_header->name)) < sizeof(out_header->name);
		}
		CloseHandle(file);
	}
	return result;
}

c_h1_cache_file::c_h1_cache_file(void) :
	m_path{},
	m_header{},
	m_data(NULL),
	m_size(0),
	m_tags_header(NULL),
	m_instances(NULL),
	m_regions{},
	m_region_count(0),
	m_structure_bsp_tags{},
	m_structure_bsp_count(0),
	m_active_structure_bsp_region(NONE)
{
	return;
}

c_h1_cache_file::~c_h1_cache_file(void)
{
	close();
	return;
}

bool c_h1_cache_file::open(const wchar_t* path)
{
	close();

	if (!h1_cache_file_read_header(path, &m_header))
	{
		h1_log("cache: %ws is not a halo 1 xbox cache file", path);
		return false;
	}

	wcsncpy_s(m_path, path, _TRUNCATE);

	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		h1_log("cache: failed to open %ws (%d)", path, GetLastError());
		return false;
	}

	LARGE_INTEGER file_size;
	GetFileSizeEx(file, &file_size);
	uint8* file_bytes = (uint8*)malloc((size_t)file_size.QuadPart);
	DWORD read = 0;
	bool read_ok = file_bytes && ReadFile(file, file_bytes, (DWORD)file_size.QuadPart, &read, NULL) && read == file_size.QuadPart;
	CloseHandle(file);

	if (!read_ok)
	{
		free(file_bytes);
		h1_log("cache: failed to read %ws", path);
		return false;
	}

	m_size = (uint32)m_header.file_length;
	if ((uint32)file_size.QuadPart >= m_size)
	{
		// uncompressed cache file
		m_data = file_bytes;
	}
	else
	{
		// Xbox cache files are zlib compressed after the header
		m_data = (uint8*)malloc(m_size);
		if (!m_data)
		{
			free(file_bytes);
			h1_log("cache: failed to allocate %u bytes for %ws", m_size, path);
			return false;
		}
		csmemcpy(m_data, file_bytes, sizeof(h1_cache_file_header));

		z_stream stream{};
		stream.next_in = file_bytes + sizeof(h1_cache_file_header);
		stream.avail_in = (uInt)(file_size.QuadPart - sizeof(h1_cache_file_header));
		stream.next_out = m_data + sizeof(h1_cache_file_header);
		stream.avail_out = m_size - sizeof(h1_cache_file_header);

		int status = inflateInit(&stream);
		if (status == Z_OK)
		{
			status = inflate(&stream, Z_FINISH);
			inflateEnd(&stream);
		}
		free(file_bytes);

		if (status != Z_STREAM_END)
		{
			h1_log("cache: decompression of %ws failed (%d), got %u of %u bytes", path, status, stream.total_out, m_size);
			free(m_data);
			m_data = NULL;
			return false;
		}
	}

	if ((uint32)(m_header.tag_data_offset + m_header.tag_data_size) > m_size)
	{
		h1_log("cache: tag data out of range in %ws", path);
		close();
		return false;
	}

	m_regions[0].base_address = k_h1_tag_cache_base_address;
	m_regions[0].file_offset = m_header.tag_data_offset;
	m_regions[0].size = m_header.tag_data_size;
	m_region_count = 1;

	m_tags_header = (h1_cache_file_tags_header*)(m_data + m_header.tag_data_offset);
	if (m_tags_header->signature != 'tags')
	{
		h1_log("cache: bad tags header signature in %ws", path);
		close();
		return false;
	}

	m_instances = (h1_cache_file_tag_instance*)address_get(m_tags_header->tag_instances_address, sizeof(h1_cache_file_tag_instance) * m_tags_header->tag_count);
	if (!m_instances)
	{
		h1_log("cache: bad tag instance table in %ws", path);
		close();
		return false;
	}

	// structure bsps live outside of the tag cache and are paged in on demand by Halo 1, map them all in at once
	h1_scnr* scenario = scenario_get();
	if (!scenario)
	{
		h1_log("cache: missing scenario in %ws", path);
		close();
		return false;
	}

	m_structure_bsp_count = 0;
	for (int32 i = 0; i < scenario->structure_bsps.count && m_region_count < NUMBEROF(m_regions); i++)
	{
		h1_scnr_structure_bsps* reference = block_get(scenario->structure_bsps, i);
		if ((uint32)(reference->structure_bsp_offset + reference->structure_bsp_size) > m_size)
		{
			h1_log("cache: structure bsp %d out of range", i);
			continue;
		}

		s_region* region = &m_regions[m_region_count++];
		region->base_address = reference->structure_bsp_address;
		region->file_offset = reference->structure_bsp_offset;
		region->size = reference->structure_bsp_size;

		h1_structure_bsp_header* bsp_header = (h1_structure_bsp_header*)(m_data + region->file_offset);
		if (bsp_header->signature != 'sbsp')
		{
			h1_log("cache: structure bsp %d has a bad header", i);
			--m_region_count;
			continue;
		}

		// the tag instance for the bsp has no base address until the bsp is loaded
		h1_cache_file_tag_instance* instance = (h1_cache_file_tag_instance*)tag_instance_get(reference->structure_bsp.index);
		if (instance)
		{
			instance->base_address = bsp_header->base_address;
		}
		m_structure_bsp_regions[m_structure_bsp_count] = m_region_count - 1;
		m_structure_bsp_tags[m_structure_bsp_count++] = reference->structure_bsp.index;
	}

	h1_log("cache: opened %ws [%s] build %s, %d tags, %d bsps", path, m_header.name, m_header.build, m_tags_header->tag_count, m_structure_bsp_count);
	return true;
}

void c_h1_cache_file::close(void)
{
	if (m_data)
	{
		free(m_data);
	}
	m_data = NULL;
	m_size = 0;
	m_tags_header = NULL;
	m_instances = NULL;
	m_region_count = 0;
	m_structure_bsp_count = 0;
	m_path[0] = L'\0';
	return;
}

void* c_h1_cache_file::address_get(uint32 address, uint32 size) const
{
	// the tags, the active structure bsp, then the others
	auto resolve = [&](int32 region_index) -> void*
	{
		const s_region* region = &m_regions[region_index];
		return address >= region->base_address && address - region->base_address + size <= region->size ?
			(void*)(m_data + region->file_offset + (address - region->base_address)) : NULL;
	};
	if (m_region_count > 0)
	{
		if (void* result = resolve(0))
		{
			return result;
		}
	}
	if (VALID_INDEX(m_active_structure_bsp_region, m_region_count))
	{
		if (void* result = resolve(m_active_structure_bsp_region))
		{
			return result;
		}
	}
	for (int32 i = 1; i < m_region_count; i++)
	{
		if (void* result = resolve(i))
		{
			return result;
		}
	}
	return NULL;
}

const char* c_h1_cache_file::string_get(uint32 address) const
{
	const char* result = (const char*)address_get(address);
	return result ? result : "";
}

void* c_h1_cache_file::data_get(const h1_tag_data& data) const
{
	if (data.size <= 0)
	{
		return NULL;
	}
	return address_get(data.address, data.size);
}

int32 c_h1_cache_file::tag_count(void) const
{
	return m_tags_header ? m_tags_header->tag_count : 0;
}

const h1_cache_file_tag_instance* c_h1_cache_file::tag_instance_get(datum tag_index) const
{
	if (tag_index == NONE || !m_instances)
	{
		return NULL;
	}

	const int32 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);
	if (absolute_index >= m_tags_header->tag_count)
	{
		return NULL;
	}

	const h1_cache_file_tag_instance* instance = &m_instances[absolute_index];
	return instance->tag_index == tag_index ? instance : NULL;
}

const h1_cache_file_tag_instance* c_h1_cache_file::tag_instance_get_by_absolute_index(int32 absolute_index) const
{
	if (!m_instances || absolute_index < 0 || absolute_index >= m_tags_header->tag_count)
	{
		return NULL;
	}
	return &m_instances[absolute_index];
}

void* c_h1_cache_file::tag_get(uint32 group_tag, datum tag_index) const
{
	const h1_cache_file_tag_instance* instance = tag_instance_get(tag_index);
	if (!instance || !instance->base_address)
	{
		return NULL;
	}

	if (group_tag != NONE &&
		instance->group_tag != group_tag &&
		instance->parent_group_tags[0] != group_tag &&
		instance->parent_group_tags[1] != group_tag)
	{
		return NULL;
	}

	// a structure bsp becomes the active one
	if (instance->group_tag == 'sbsp')
	{
		for (int32 i = 0; i < m_structure_bsp_count; i++)
		{
			if (m_structure_bsp_tags[i] == tag_index)
			{
				m_active_structure_bsp_region = m_structure_bsp_regions[i];
				break;
			}
		}
	}
	return address_get(instance->base_address);
}

const char* c_h1_cache_file::tag_name_get(datum tag_index) const
{
	const h1_cache_file_tag_instance* instance = tag_instance_get(tag_index);
	return instance ? string_get(instance->name_address) : "";
}

datum c_h1_cache_file::tag_find(uint32 group_tag, const char* name) const
{
	for (int32 i = 0; i < tag_count(); i++)
	{
		const h1_cache_file_tag_instance* instance = &m_instances[i];
		if (instance->group_tag == group_tag && !csstricmp(string_get(instance->name_address), name))
		{
			return instance->tag_index;
		}
	}
	return NONE;
}

h1_scnr* c_h1_cache_file::scenario_get(void) const
{
	return (h1_scnr*)tag_get('scnr', m_tags_header->scenario_index);
}

datum c_h1_cache_file::structure_bsp_tag_get(int32 bsp_index) const
{
	return VALID_INDEX(bsp_index, m_structure_bsp_count) ? m_structure_bsp_tags[bsp_index] : NONE;
}
