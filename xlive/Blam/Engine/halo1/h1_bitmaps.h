#pragma once

/*
* Direct3D 9 textures for Halo 1 Xbox bitmaps. Pixel data lives in the (decompressed) cache file,
* non block compressed formats are swizzled on Xbox and get unswizzled here.
*/

// texture for a bitmap tag's bitmap, NULL when it can't be decoded. Cached until h1_bitmaps_dispose
IDirect3DBaseTexture9* h1_bitmap_texture_get(datum bitmap_tag_index, int32 bitmap_index);

// texture for a bitmap referenced by a shader (the first bitmap of the tag)
IDirect3DBaseTexture9* h1_bitmap_texture_get(const struct h1_tag_reference& reference, int32 bitmap_index = 0);

// releases every texture
void h1_bitmaps_dispose(void);

// samples level 0 of a 16/32 bit bitmap on the cpu (lightmaps), returns false for unsupported formats
bool h1_bitmap_sample(datum bitmap_tag_index, int32 bitmap_index, real32 u, real32 v, real_rgb_color* out_color);
