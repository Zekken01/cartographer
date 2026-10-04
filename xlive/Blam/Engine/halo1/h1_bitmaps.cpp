#include "stdafx.h"
#include "h1_bitmaps.h"

#include "h1_cache_file.h"
#include "h1_log.h"

#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"

#include <unordered_map>

/* constants */

enum e_h1_bitmap_type
{
	_h1_bitmap_type_2d = 0,
	_h1_bitmap_type_3d,
	_h1_bitmap_type_cube_map,
	_h1_bitmap_type_white,
};

enum e_h1_bitmap_format
{
	_h1_bitmap_format_a8 = 0,
	_h1_bitmap_format_y8 = 1,
	_h1_bitmap_format_ay8 = 2,
	_h1_bitmap_format_a8y8 = 3,
	_h1_bitmap_format_r5g6b5 = 6,
	_h1_bitmap_format_a1r5g5b5 = 8,
	_h1_bitmap_format_a4r4g4b4 = 9,
	_h1_bitmap_format_x8r8g8b8 = 10,
	_h1_bitmap_format_a8r8g8b8 = 11,
	_h1_bitmap_format_dxt1 = 14,
	_h1_bitmap_format_dxt3 = 15,
	_h1_bitmap_format_dxt5 = 16,
	_h1_bitmap_format_p8_bump = 17,
};

enum e_h1_bitmap_flags
{
	_h1_bitmap_flag_power_of_two_bit = 0,
	_h1_bitmap_flag_compressed_bit,
	_h1_bitmap_flag_palettized_bit,
	_h1_bitmap_flag_swizzled_bit,
};

/* globals */

static std::unordered_map<uint64, IDirect3DBaseTexture9*> g_h1_textures;

/* prototypes */

static IDirect3DBaseTexture9* h1_bitmap_texture_create(const h1_bitm_bitmaps* bitmap);

/* public code */

IDirect3DBaseTexture9* h1_bitmap_texture_get(datum bitmap_tag_index, int32 bitmap_index)
{
	if (!g_h1_cache_file || bitmap_tag_index == NONE)
	{
		return NULL;
	}

	const uint64 key = ((uint64)(uint32)bitmap_tag_index << 32) | (uint32)bitmap_index;
	auto found = g_h1_textures.find(key);
	if (found != g_h1_textures.end())
	{
		return found->second;
	}

	IDirect3DBaseTexture9* texture = NULL;
	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index);
	if (bitmap_group)
	{
		const h1_bitm_bitmaps* bitmap = g_h1_cache_file->block_get(bitmap_group->bitmaps, bitmap_index);
		if (bitmap)
		{
			texture = h1_bitmap_texture_create(bitmap);
			if (!texture)
			{
				h1_log("bitmaps: failed to create %s[%d] format %d type %d %dx%d", g_h1_cache_file->tag_name_get(bitmap_tag_index), bitmap_index, bitmap->format, bitmap->type, bitmap->width, bitmap->height);
			}
		}
	}

	g_h1_textures[key] = texture;
	return texture;
}

IDirect3DBaseTexture9* h1_bitmap_texture_get(const h1_tag_reference& reference, int32 bitmap_index)
{
	return reference.index == NONE ? NULL : h1_bitmap_texture_get(reference.index, bitmap_index);
}

void h1_bitmaps_dispose(void)
{
	for (auto& entry : g_h1_textures)
	{
		if (entry.second)
		{
			entry.second->Release();
		}
	}
	g_h1_textures.clear();
	return;
}

/* private code */

// Xbox textures are stored in Morton (z) order: x bits in the even positions, y bits in the odd ones,
// with the larger dimension's extra bits on top
static uint32 h1_swizzle_offset(uint32 x, uint32 y, uint32 width, uint32 height)
{
	uint32 result = 0;
	uint32 bit = 1;
	while (width > 1 || height > 1)
	{
		if (width > 1)
		{
			if (x & 1)
			{
				result |= bit;
			}
			x >>= 1;
			bit <<= 1;
			width >>= 1;
		}
		if (height > 1)
		{
			if (y & 1)
			{
				result |= bit;
			}
			y >>= 1;
			bit <<= 1;
			height >>= 1;
		}
	}
	return result;
}

struct s_h1_format_info
{
	D3DFORMAT d3d_format;
	uint32 source_bits_per_pixel;
	uint32 destination_bytes_per_pixel;
	bool block_compressed;
};

static bool h1_bitmap_format_info(int16 format, s_h1_format_info* out_info)
{
	switch (format)
	{
	// alpha only bitmaps are white on xbox (d3d9 A8 would sample black)
	case _h1_bitmap_format_a8:			*out_info = { D3DFMT_A8L8, 8, 2, false }; return true;
	case _h1_bitmap_format_y8:			*out_info = { D3DFMT_L8, 8, 1, false }; return true;
	case _h1_bitmap_format_ay8:			*out_info = { D3DFMT_A8L8, 8, 2, false }; return true;
	case _h1_bitmap_format_a8y8:		*out_info = { D3DFMT_A8L8, 16, 2, false }; return true;
	case _h1_bitmap_format_r5g6b5:		*out_info = { D3DFMT_R5G6B5, 16, 2, false }; return true;
	case _h1_bitmap_format_a1r5g5b5:	*out_info = { D3DFMT_A1R5G5B5, 16, 2, false }; return true;
	case _h1_bitmap_format_a4r4g4b4:	*out_info = { D3DFMT_A4R4G4B4, 16, 2, false }; return true;
	case _h1_bitmap_format_x8r8g8b8:	*out_info = { D3DFMT_X8R8G8B8, 32, 4, false }; return true;
	case _h1_bitmap_format_a8r8g8b8:	*out_info = { D3DFMT_A8R8G8B8, 32, 4, false }; return true;
	case _h1_bitmap_format_dxt1:		*out_info = { D3DFMT_DXT1, 4, 0, true }; return true;
	case _h1_bitmap_format_dxt3:		*out_info = { D3DFMT_DXT3, 8, 0, true }; return true;
	case _h1_bitmap_format_dxt5:		*out_info = { D3DFMT_DXT5, 8, 0, true }; return true;
	// palettized bump maps: the palette lives in the Xbox executable, these become flat normals
	case _h1_bitmap_format_p8_bump:		*out_info = { D3DFMT_A8R8G8B8, 8, 4, false }; return true;
	default:
		return false;
	}
}

static uint32 h1_bitmap_level_size(const s_h1_format_info* info, uint32 width, uint32 height)
{
	if (info->block_compressed)
	{
		const uint32 block_size = info->d3d_format == D3DFMT_DXT1 ? 8 : 16;
		return MAX(1u, (width + 3) / 4) * MAX(1u, (height + 3) / 4) * block_size;
	}
	return width * height * info->source_bits_per_pixel / 8;
}

// copies one mip level of a face into a locked surface
static void h1_bitmap_level_copy(const h1_bitm_bitmaps* bitmap, const s_h1_format_info* info, const uint8* source, uint32 width, uint32 height, uint8* destination, uint32 pitch)
{
	const bool swizzled = TEST_BIT(bitmap->flags, _h1_bitmap_flag_swizzled_bit) && !info->block_compressed;

	if (info->block_compressed)
	{
		const uint32 block_size = info->d3d_format == D3DFMT_DXT1 ? 8 : 16;
		const uint32 blocks_x = MAX(1u, (width + 3) / 4);
		const uint32 blocks_y = MAX(1u, (height + 3) / 4);
		for (uint32 y = 0; y < blocks_y; y++)
		{
			csmemcpy(destination + y * pitch, source + y * blocks_x * block_size, blocks_x * block_size);
		}
		return;
	}

	const uint32 source_bytes = info->source_bits_per_pixel / 8;
	for (uint32 y = 0; y < height; y++)
	{
		uint8* row = destination + y * pitch;
		for (uint32 x = 0; x < width; x++)
		{
			const uint32 texel = swizzled ? h1_swizzle_offset(x, y, width, height) : (y * width + x);
			const uint8* src = source + texel * source_bytes;
			uint8* dst = row + x * info->destination_bytes_per_pixel;

			switch (bitmap->format)
			{
			case _h1_bitmap_format_a8:
				dst[0] = 0xFF;
				dst[1] = src[0];
				break;
			case _h1_bitmap_format_ay8:
				dst[0] = src[0];
				dst[1] = src[0];
				break;
			case _h1_bitmap_format_p8_bump:
				dst[0] = 0xFF;
				dst[1] = 0x80;
				dst[2] = 0x80;
				dst[3] = 0xFF;
				break;
			default:
				csmemcpy(dst, src, source_bytes);
				break;
			}
		}
	}
	return;
}

static IDirect3DBaseTexture9* h1_bitmap_texture_create(const h1_bitm_bitmaps* bitmap)
{
	s_h1_format_info info;
	if (!h1_bitmap_format_info(bitmap->format, &info))
	{
		return NULL;
	}

	if (bitmap->type != _h1_bitmap_type_2d && bitmap->type != _h1_bitmap_type_cube_map)
	{
		return NULL;
	}

	const uint32 width = (uint32)bitmap->width;
	const uint32 height = (uint32)bitmap->height;
	uint32 level_count = (uint32)bitmap->mipmap_count + 1;
	if (info.block_compressed)
	{
		// xbox caches leave dxt mip levels below 4x4 empty (black), the hardware never samples them
		while (level_count > 1 && (MAX(1u, width >> (level_count - 1)) < 4 || MAX(1u, height >> (level_count - 1)) < 4))
		{
			level_count--;
		}
	}
	const uint32 face_count = bitmap->type == _h1_bitmap_type_cube_map ? 6 : 1;

	const uint8* file_data = g_h1_cache_file->file_data();
	if ((uint64)bitmap->pixels_offset + bitmap->pixels_size > g_h1_cache_file->file_size() || width == 0 || height == 0)
	{
		return NULL;
	}
	const uint8* pixels = file_data + bitmap->pixels_offset;
	const uint8* pixels_end = pixels + bitmap->pixels_size;

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const bool use_d3d9_ex = rasterizer_globals_get()->use_d3d9_ex;
	const D3DPOOL pool = use_d3d9_ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

	IDirect3DBaseTexture9* result = NULL;

	if (face_count == 1)
	{
		IDirect3DTexture9* texture = NULL;
		IDirect3DTexture9* staging = NULL;
		if (FAILED(device->CreateTexture(width, height, level_count, 0, info.d3d_format, pool, &texture, NULL)))
		{
			return NULL;
		}
		IDirect3DTexture9* target = texture;
		if (use_d3d9_ex)
		{
			if (FAILED(device->CreateTexture(width, height, level_count, 0, info.d3d_format, D3DPOOL_SYSTEMMEM, &staging, NULL)))
			{
				texture->Release();
				return NULL;
			}
			target = staging;
		}

		const uint8* source = pixels;
		for (uint32 level = 0; level < level_count; level++)
		{
			const uint32 level_width = MAX(1u, width >> level);
			const uint32 level_height = MAX(1u, height >> level);
			const uint32 level_size = h1_bitmap_level_size(&info, level_width, level_height);
			if (source + level_size > pixels_end)
			{
				break;
			}

			D3DLOCKED_RECT locked;
			if (SUCCEEDED(target->LockRect(level, &locked, NULL, 0)))
			{
				h1_bitmap_level_copy(bitmap, &info, source, level_width, level_height, (uint8*)locked.pBits, locked.Pitch);
				target->UnlockRect(level);
			}
			source += level_size;
		}

		if (staging)
		{
			device->UpdateTexture(staging, texture);
			staging->Release();
		}
		result = texture;
	}
	else
	{
		IDirect3DCubeTexture9* texture = NULL;
		IDirect3DCubeTexture9* staging = NULL;
		if (FAILED(device->CreateCubeTexture(width, level_count, 0, info.d3d_format, pool, &texture, NULL)))
		{
			return NULL;
		}
		IDirect3DCubeTexture9* target = texture;
		if (use_d3d9_ex)
		{
			if (FAILED(device->CreateCubeTexture(width, level_count, 0, info.d3d_format, D3DPOOL_SYSTEMMEM, &staging, NULL)))
			{
				texture->Release();
				return NULL;
			}
			target = staging;
		}

		// faces are stored one after another, each with its full mip chain
		const uint8* source = pixels;
		for (uint32 face = 0; face < face_count; face++)
		{
			for (uint32 level = 0; level < level_count; level++)
			{
				const uint32 level_size = MAX(1u, width >> level);
				const uint32 size = h1_bitmap_level_size(&info, level_size, level_size);
				if (source + size > pixels_end)
				{
					break;
				}

				D3DLOCKED_RECT locked;
				if (SUCCEEDED(target->LockRect((D3DCUBEMAP_FACES)face, level, &locked, NULL, 0)))
				{
					h1_bitmap_level_copy(bitmap, &info, source, level_size, level_size, (uint8*)locked.pBits, locked.Pitch);
					target->UnlockRect((D3DCUBEMAP_FACES)face, level);
				}
				source += size;
			}
		}

		if (staging)
		{
			device->UpdateTexture(staging, texture);
			staging->Release();
		}
		result = texture;
	}

	return result;
}

bool h1_bitmap_sample(datum bitmap_tag_index, int32 bitmap_index, real32 u, real32 v, real_rgb_color* out_color)
{
	if (!g_h1_cache_file || bitmap_tag_index == NONE)
	{
		return false;
	}

	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index);
	const h1_bitm_bitmaps* bitmap = bitmap_group ? g_h1_cache_file->block_get(bitmap_group->bitmaps, bitmap_index) : NULL;
	if (!bitmap || bitmap->type != _h1_bitmap_type_2d || bitmap->width <= 0 || bitmap->height <= 0)
	{
		return false;
	}

	const uint32 width = (uint32)bitmap->width;
	const uint32 height = (uint32)bitmap->height;
	const uint32 x = (uint32)PIN((int32)(u * width), 0, (int32)width - 1);
	const uint32 y = (uint32)PIN((int32)(v * height), 0, (int32)height - 1);
	const bool swizzled = TEST_BIT(bitmap->flags, _h1_bitmap_flag_swizzled_bit);
	const uint32 texel = swizzled ? h1_swizzle_offset(x, y, width, height) : y * width + x;

	const uint8* pixels = g_h1_cache_file->file_data() + bitmap->pixels_offset;
	switch (bitmap->format)
	{
	case _h1_bitmap_format_r5g6b5:
	{
		if ((texel + 1) * 2 > (uint32)bitmap->pixels_size) return false;
		const uint16 value = *(const uint16*)(pixels + texel * 2);
		out_color->red = (real32)((value >> 11) & 0x1F) / 31.f;
		out_color->green = (real32)((value >> 5) & 0x3F) / 63.f;
		out_color->blue = (real32)(value & 0x1F) / 31.f;
		return true;
	}
	case _h1_bitmap_format_x8r8g8b8:
	case _h1_bitmap_format_a8r8g8b8:
	{
		if ((texel + 1) * 4 > (uint32)bitmap->pixels_size) return false;
		const uint8* value = pixels + texel * 4;
		out_color->red = (real32)value[2] / 255.f;
		out_color->green = (real32)value[1] / 255.f;
		out_color->blue = (real32)value[0] / 255.f;
		return true;
	}
	default:
		return false;
	}
}
