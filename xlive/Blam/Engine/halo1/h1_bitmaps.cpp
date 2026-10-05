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

// normals of palettized bump maps (global_vector_palette in the xbox executable)
static const uint32 k_h1_vector_palette[256] =
{
	0xFF7A19CC, 0xFF7E19CC, 0xFF8019CC, 0xFF8119CC, 0xFF8519CC, 0xFF742FE2, 0xFF7A2FE2, 0xFF7E2FE2,
	0xFF802FE2, 0xFF812FE2, 0xFF852FE2, 0xFF8B2FE2, 0xFF6B42ED, 0xFF7442EE, 0xFF7A42EF, 0xFF7E42EF,
	0xFF8042EF, 0xFF8142EF, 0xFF8542EF, 0xFF8B42EE, 0xFF9442ED, 0xFF6052F2, 0xFF6B52F5, 0xFF7452F6,
	0xFF7A52F7, 0xFF7E52F7, 0xFF8052F7, 0xFF8152F7, 0xFF8552F7, 0xFF8B52F6, 0xFF9452F5, 0xFF9F52F2,
	0xFF5260F2, 0xFF6060F7, 0xFF6B60F9, 0xFF7460FB, 0xFF7A60FB, 0xFF7E60FB, 0xFF8060FB, 0xFF8160FB,
	0xFF8560FB, 0xFF8B60FB, 0xFF9460F9, 0xFF9F60F7, 0xFFAD60F2, 0xFF426BED, 0xFF526BF5, 0xFF606BF9,
	0xFF6B6BFC, 0xFF746BFD, 0xFF7A6BFD, 0xFF7E6BFD, 0xFF806BFD, 0xFF816BFD, 0xFF856BFD, 0xFF8B6BFD,
	0xFF946BFC, 0xFF9F6BF9, 0xFFAD6BF5, 0xFFBD6BED, 0xFF2F74E2, 0xFF4274EE, 0xFF5274F6, 0xFF6074FB,
	0xFF6B74FD, 0xFF7474FE, 0xFF7A74FE, 0xFF7E74FE, 0xFF8074FE, 0xFF8174FE, 0xFF8574FE, 0xFF8B74FE,
	0xFF9474FD, 0xFF9F74FB, 0xFFAD74F6, 0xFFBD74EE, 0xFFD074E2, 0xFF197ACC, 0xFF2F7AE2, 0xFF427AEF,
	0xFF527AF7, 0xFF607AFB, 0xFF6B7AFD, 0xFF747AFE, 0xFF7A7AFF, 0xFF7E7AFF, 0xFF807AFF, 0xFF817AFF,
	0xFF857AFF, 0xFF8B7AFE, 0xFF947AFD, 0xFF9F7AFB, 0xFFAD7AF7, 0xFFBD7AEF, 0xFFD07AE2, 0xFFE57ACC,
	0xFF197ECC, 0xFF2F7EE2, 0xFF427EEF, 0xFF527EF7, 0xFF607EFB, 0xFF6B7EFD, 0xFF747EFE, 0xFF7A7EFF,
	0xFF7E7EFF, 0xFF807EFF, 0xFF817EFF, 0xFF857EFF, 0xFF8B7EFE, 0xFF947EFD, 0xFF9F7EFB, 0xFFAD7EF7,
	0xFFBD7EEF, 0xFFD07EE2, 0xFFE57ECC, 0xFF1980CC, 0xFF2F80E2, 0xFF4280EF, 0xFF5280F7, 0xFF6080FB,
	0xFF6B80FD, 0xFF7480FE, 0xFF7A80FF, 0xFF7E80FF, 0xFF8080FF, 0xFF8180FF, 0xFF8580FF, 0xFF8B80FE,
	0xFF9480FD, 0xFF9F80FB, 0xFFAD80F7, 0xFFBD80EF, 0xFFD080E2, 0xFFE580CC, 0xFF1981CC, 0xFF2F81E2,
	0xFF4281EF, 0xFF5281F7, 0xFF6081FB, 0xFF6B81FD, 0xFF7481FE, 0xFF7A81FF, 0xFF7E81FF, 0xFF8081FF,
	0xFF8181FF, 0xFF8581FF, 0xFF8B81FE, 0xFF9481FD, 0xFF9F81FB, 0xFFAD81F7, 0xFFBD81EF, 0xFFD081E2,
	0xFFE581CC, 0xFF1985CC, 0xFF2F85E2, 0xFF4285EF, 0xFF5285F7, 0xFF6085FB, 0xFF6B85FD, 0xFF7485FE,
	0xFF7A85FF, 0xFF7E85FF, 0xFF8085FF, 0xFF8185FF, 0xFF8585FF, 0xFF8B85FE, 0xFF9485FD, 0xFF9F85FB,
	0xFFAD85F7, 0xFFBD85EF, 0xFFD085E2, 0xFFE585CC, 0xFF2F8BE2, 0xFF428BEE, 0xFF528BF6, 0xFF608BFB,
	0xFF6B8BFD, 0xFF748BFE, 0xFF7A8BFE, 0xFF7E8BFE, 0xFF808BFE, 0xFF818BFE, 0xFF858BFE, 0xFF8B8BFE,
	0xFF948BFD, 0xFF9F8BFB, 0xFFAD8BF6, 0xFFBD8BEE, 0xFFD08BE2, 0xFF4294ED, 0xFF5294F5, 0xFF6094F9,
	0xFF6B94FC, 0xFF7494FD, 0xFF7A94FD, 0xFF7E94FD, 0xFF8094FD, 0xFF8194FD, 0xFF8594FD, 0xFF8B94FD,
	0xFF9494FC, 0xFF9F94F9, 0xFFAD94F5, 0xFFBD94ED, 0xFF529FF2, 0xFF609FF7, 0xFF6B9FF9, 0xFF749FFB,
	0xFF7A9FFB, 0xFF7E9FFB, 0xFF809FFB, 0xFF819FFB, 0xFF859FFB, 0xFF8B9FFB, 0xFF949FF9, 0xFF9F9FF7,
	0xFFAD9FF2, 0xFF60ADF2, 0xFF6BADF5, 0xFF74ADF6, 0xFF7AADF7, 0xFF7EADF7, 0xFF80ADF7, 0xFF81ADF7,
	0xFF85ADF7, 0xFF8BADF6, 0xFF94ADF5, 0xFF9FADF2, 0xFF6BBDED, 0xFF74BDEE, 0xFF7ABDEF, 0xFF7EBDEF,
	0xFF80BDEF, 0xFF81BDEF, 0xFF85BDEF, 0xFF8BBDEE, 0xFF94BDED, 0xFF74D0E2, 0xFF7AD0E2, 0xFF7ED0E2,
	0xFF80D0E2, 0xFF81D0E2, 0xFF85D0E2, 0xFF8BD0E2, 0xFF7AE5CC, 0xFF7EE5CC, 0xFF80E5CC, 0xFF81E5CC,
	0xFF85E5CC, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x008080FF,
};

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
	// palettized bump maps index the xbox executable's normal palette
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
				*(uint32*)dst = k_h1_vector_palette[src[0]];
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

// bitmaps.c bitmap_2d_get_pixel: the nearest texel of the mip level the level of detail picks (lod 1: the largest), any 2d
// format the textures decode
bool h1_bitmap_sample_lod(datum bitmap_tag_index, int32 bitmap_index, real32 u, real32 v, real32 lod, real_rgb_color* out_color)
{
	if (!g_h1_cache_file || bitmap_tag_index == NONE)
	{
		return false;
	}
	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index);
	const h1_bitm_bitmaps* bitmap = bitmap_group && VALID_INDEX(bitmap_index, bitmap_group->bitmaps.count) ?
		g_h1_cache_file->block_get(bitmap_group->bitmaps, bitmap_index) : NULL;
	s_h1_format_info info;
	if (!bitmap || bitmap->type != _h1_bitmap_type_2d || bitmap->width <= 0 || bitmap->height <= 0 || !h1_bitmap_format_info(bitmap->format, &info))
	{
		return false;
	}

	int32 mipmap_index = lod < 1.f && bitmap->mipmap_count > 0 ? (int32)((1.f - lod) * bitmap->mipmap_count) : 0;
	mipmap_index = PIN(mipmap_index, 0, (int32)bitmap->mipmap_count);
	uint32 width = (uint32)bitmap->width;
	uint32 height = (uint32)bitmap->height;
	uint32 offset = 0;
	for (int32 level = 0; level < mipmap_index; level++)
	{
		offset += h1_bitmap_level_size(&info, width, height);
		width = MAX(1u, width / 2);
		height = MAX(1u, height / 2);
	}
	// xbox caches leave dxt levels below 4x4 empty: sample the 4x4 one
	if (info.block_compressed && (width < 4 || height < 4) && mipmap_index > 0)
	{
		return h1_bitmap_sample_lod(bitmap_tag_index, bitmap_index, u, v, lod + 1.f / MAX((real32)bitmap->mipmap_count, 1.f), out_color);
	}

	auto wrap = [](int32 value, uint32 size) -> uint32
	{
		return (size & (size - 1)) == 0 ? (uint32)value & (size - 1) : (uint32)(((value % (int32)size) + (int32)size) % (int32)size);
	};
	const uint32 x = wrap((int32)floorf((real32)width * u - 0.5f + 0.5f), width);
	const uint32 y = wrap((int32)floorf((real32)height * v - 0.5f + 0.5f), height);
	const uint8* pixels = g_h1_cache_file->file_data() + bitmap->pixels_offset + offset;
	const uint32 level_size = h1_bitmap_level_size(&info, width, height);
	if (offset + level_size > (uint32)bitmap->pixels_size)
	{
		return false;
	}

	if (info.block_compressed)
	{
		const uint32 block_size = info.d3d_format == D3DFMT_DXT1 ? 8 : 16;
		const uint32 blocks_x = MAX(1u, (width + 3) / 4);
		const uint8* block = pixels + ((y / 4) * blocks_x + (x / 4)) * block_size;
		const uint8* color_block = block + (block_size == 16 ? 8 : 0);
		const uint16 c0 = *(const uint16*)color_block;
		const uint16 c1 = *(const uint16*)(color_block + 2);
		const uint32 selectors = *(const uint32*)(color_block + 4);
		const uint32 selector = (selectors >> (((y & 3) * 4 + (x & 3)) * 2)) & 3;
		real32 colors[4][3];
		auto expand = [](uint16 c, real32* out)
		{
			out[0] = (real32)((c >> 11) & 0x1F) / 31.f;
			out[1] = (real32)((c >> 5) & 0x3F) / 63.f;
			out[2] = (real32)(c & 0x1F) / 31.f;
		};
		expand(c0, colors[0]);
		expand(c1, colors[1]);
		for (int32 i = 0; i < 3; i++)
		{
			if (c0 > c1 || block_size == 16)
			{
				colors[2][i] = (2.f * colors[0][i] + colors[1][i]) / 3.f;
				colors[3][i] = (colors[0][i] + 2.f * colors[1][i]) / 3.f;
			}
			else
			{
				colors[2][i] = (colors[0][i] + colors[1][i]) / 2.f;
				colors[3][i] = 0.f;
			}
		}
		out_color->red = colors[selector][0];
		out_color->green = colors[selector][1];
		out_color->blue = colors[selector][2];
		return true;
	}

	const bool swizzled = TEST_BIT(bitmap->flags, _h1_bitmap_flag_swizzled_bit);
	const uint32 texel = swizzled ? h1_swizzle_offset(x, y, width, height) : y * width + x;
	switch (bitmap->format)
	{
	case _h1_bitmap_format_r5g6b5:
	{
		const uint16 value = *(const uint16*)(pixels + texel * 2);
		out_color->red = (real32)((value >> 11) & 0x1F) / 31.f;
		out_color->green = (real32)((value >> 5) & 0x3F) / 63.f;
		out_color->blue = (real32)(value & 0x1F) / 31.f;
		return true;
	}
	case _h1_bitmap_format_a1r5g5b5:
	{
		const uint16 value = *(const uint16*)(pixels + texel * 2);
		out_color->red = (real32)((value >> 10) & 0x1F) / 31.f;
		out_color->green = (real32)((value >> 5) & 0x1F) / 31.f;
		out_color->blue = (real32)(value & 0x1F) / 31.f;
		return true;
	}
	case _h1_bitmap_format_a4r4g4b4:
	{
		const uint16 value = *(const uint16*)(pixels + texel * 2);
		out_color->red = (real32)((value >> 8) & 0xF) / 15.f;
		out_color->green = (real32)((value >> 4) & 0xF) / 15.f;
		out_color->blue = (real32)(value & 0xF) / 15.f;
		return true;
	}
	case _h1_bitmap_format_x8r8g8b8:
	case _h1_bitmap_format_a8r8g8b8:
	{
		const uint8* value = pixels + texel * 4;
		out_color->red = (real32)value[2] / 255.f;
		out_color->green = (real32)value[1] / 255.f;
		out_color->blue = (real32)value[0] / 255.f;
		return true;
	}
	case _h1_bitmap_format_y8:
	case _h1_bitmap_format_ay8:
	{
		const real32 value = (real32)pixels[texel] / 255.f;
		*out_color = { value, value, value };
		return true;
	}
	default:
		return false;
	}
}
