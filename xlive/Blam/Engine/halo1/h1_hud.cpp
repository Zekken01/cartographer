#include "stdafx.h"
#include "h1_hud.h"

#include "h1_bitmaps.h"
#include "h1_cache_file.h"
#include "h1_first_person_weapon.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_weapon_logic.h"
#include "h1_weapons.h"

#include "game/game_time.h"
#include "items/weapons.h"
#include "objects/objects.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

/* constants */

enum
{
	k_h1_hud_ticks_per_second = 30,
	k_h1_hud_window_height = 480,
	k_h1_maximum_weapon_hud_depth = 16,
	k_h1_weapon_hud_flash_references = 8,
	k_h1_crosshair_states = 19,
};

// hud_definitions.h
enum
{
	_h1_hud_anchor_top_left = 0,
	_h1_hud_anchor_top_right,
	_h1_hud_anchor_bottom_left,
	_h1_hud_anchor_bottom_right,
	_h1_hud_anchor_center,
};

// hud_draw.h, hud_draw.c
enum
{
	_h1_hud_draw_flashing_bit = 0,
	_h1_hud_draw_disabled_bit,
	_h1_hud_draw_in_multiplayer_bit,
};

enum
{
	_h1_hud_number_show_all_leading_zeros_bit = 0,
	_h1_hud_number_show_only_when_zoomed_bit,
	_h1_hud_number_show_trailing_m_bit,
};

enum
{
	_h1_hud_number_decimal_index = 10,
	_h1_hud_number_colon_index,
	_h1_hud_number_negative_sign_index,
	_h1_hud_number_meters_index,
	_h1_hud_number_kilometers_index,
};

enum
{
	_h1_hud_meter_switch_color_on_state_change_bit = 0,
	_h1_hud_meter_interpolates_between_min_max_bit,
	_h1_hud_meter_interpolate_in_hsv_space_bit,
	_h1_hud_meter_interpolate_along_farthest_hue_path_bit,
	_h1_hud_meter_invert_interpolation_value_bit,
};

enum
{
	_h1_hud_flash_reverse_colors_bit = 0,
};

enum
{
	_h1_hud_overlay_flashes_bit = 0,
	_h1_hud_overlay_runtime_invalid_bit,
};

// hud_weapon.c
enum
{
	_h1_crosshair_state_aim = 0,
	_h1_crosshair_state_zoom,
	_h1_crosshair_state_charge,
	_h1_crosshair_state_flash_ammo,
	_h1_crosshair_state_flash_heat,
	_h1_crosshair_state_flash_total_ammo,
	_h1_crosshair_state_flash_total_battery,
	_h1_crosshair_state_reload,
	_h1_crosshair_state_fired_with_no_ammo,
	_h1_crosshair_state_threw_with_no_grenade,
	_h1_crosshair_state_flash_ammo_none_for_reload,
	_h1_crosshair_state_flash_secondary_ammo,
	_h1_crosshair_state_flash_secondary_total_ammo,
	_h1_crosshair_state_secondary_reload,
	_h1_crosshair_state_fired_secondary_with_no_ammo,
	_h1_crosshair_state_flash_secondary_ammo_none_for_reload,
	_h1_crosshair_state_primary_trigger_ready,
	_h1_crosshair_state_secondary_trigger_ready,
	_h1_crosshair_state_flash_fired_battery_depleted,
};

enum
{
	_h1_hud_dont_scale_offset_bit = 0,
	_h1_hud_dont_scale_size_bit,
};

enum
{
	_h1_hud_crosshair_flashes_bit = 0,
	_h1_hud_crosshair_not_a_sprite_bit,
	_h1_hud_crosshair_not_on_default_zoom_bit,
	_h1_hud_crosshair_show_sniper_data_bit,
	_h1_hud_crosshair_hide_outside_area_bit,
	_h1_hud_crosshair_one_zoom_level_bit,
	_h1_hud_crosshair_only_on_default_zoom_bit,
	_h1_hud_crosshair_runtime_invalid_bit,
};

enum
{
	_h1_weapon_overlay_on_flashing_bit = 0,
	_h1_weapon_overlay_on_empty_bit,
	_h1_weapon_overlay_on_reload_bit,
	_h1_weapon_overlay_on_default_bit,
	_h1_weapon_overlay_on_always_bit,
};

enum
{
	_h1_bitmap_group_type_sprites = 3,
	_h1_bitmap_group_type_interface_bitmaps = 4,
};

// halo 2's unit control flags (unit +0x24)
enum : uint32
{
	k_h2_unit_control_primary_trigger_held = FLAG(18),
};

/* structures */

// hud_definitions.h, unit_hud_interface_definition.h, weapon_hud_interface_definition.h and hud_weapon.c
struct h1_hud_placement
{
	point2d offset;
	real_vector2d scale;
	int16 multiplayer_scaling_flags;
	int16 pad;
	int32 unused[5];
};
static_assert(sizeof(h1_hud_placement) == 0x24);

struct h1_hud_absolute_placement
{
	int16 corner;
	int16 pad;
	int32 unused[8];
};
static_assert(sizeof(h1_hud_absolute_placement) == 0x24);

struct h1_hud_color
{
	uint32 color;
	uint32 flash_color;
	real32 flash_period;
	real32 flash_delay;
	int16 number_of_flashes;
	uint16 flash_flags;
	real32 flash_length;
	uint32 disabled_color;
	int32 custom;
};
static_assert(sizeof(h1_hud_color) == 0x20);

struct h1_hud_static_element
{
	h1_hud_placement placement;
	h1_tag_reference interface_bitmap;
	h1_hud_color colors;
	int16 sequence_index;
	int16 pad;
	h1_tag_block<uint8> multitexture_overlays;
	int32 unused;
};
static_assert(sizeof(h1_hud_static_element) == 0x68);

struct h1_hud_meter_element
{
	h1_hud_placement placement;
	h1_tag_reference meter_bitmap;
	uint32 min_color;
	uint32 max_color;
	uint32 flash_color;
	uint32 empty_color;
	uint8 meter_flags;
	uint8 minimum_value;
	int16 sequence_index;
	uint8 alpha_multiplier;
	uint8 alpha_bias;
	int16 value_scale;
	real32 opacity;
	real32 fade;
	uint32 disabled_color;
	h1_tag_block<uint8> multitexture_overlays;
	int32 unused;
};
static_assert(sizeof(h1_hud_meter_element) == 0x68);

struct h1_hud_number_element
{
	h1_hud_placement placement;
	h1_hud_color colors;
	int8 digits;
	uint8 number_flags;
	int8 fractional_digits;
	uint8 pad;
	int32 unused[3];
};
static_assert(sizeof(h1_hud_number_element) == 0x54);

struct h1_wphi_element_header
{
	int16 state_type;
	int16 runtime_flags;
	int16 use_on_map_type;
	int16 pad;
	int32 unused[7];
};
static_assert(sizeof(h1_wphi_element_header) == 0x24);

struct h1_wphi_static
{
	h1_wphi_element_header header;
	h1_hud_static_element static_element;
	int32 unused[10];
};
static_assert(sizeof(h1_wphi_static) == 0xB4);

struct h1_wphi_meter
{
	h1_wphi_element_header header;
	h1_hud_meter_element meter_element;
	int32 unused[10];
};
static_assert(sizeof(h1_wphi_meter) == 0xB4);

struct h1_wphi_number
{
	h1_wphi_element_header header;
	h1_hud_number_element number_element;
	uint16 weapon_flags;
	int16 pad;
	int32 unused[9];
};
static_assert(sizeof(h1_wphi_number) == 0xA0);

struct h1_wphi_overlay_item
{
	h1_hud_placement placement;
	h1_hud_color colors;
	int16 frame_rate;
	int16 pad;
	int16 sequence_index;
	int16 type;
	int32 flags;
	int32 unused[14];
};
static_assert(sizeof(h1_wphi_overlay_item) == 0x88);

struct h1_wphi_overlays
{
	h1_wphi_element_header header;
	h1_tag_reference bitmap;
	h1_tag_block<h1_wphi_overlay_item> items;
	int32 unused[10];
};
static_assert(sizeof(h1_wphi_overlays) == 0x68);

struct h1_wphi_crosshair_item
{
	h1_hud_placement placement;
	h1_hud_color colors;
	int16 frame_rate;
	int16 sequence_index;
	uint32 flags;
	int32 unused[8];
};
static_assert(sizeof(h1_wphi_crosshair_item) == 0x6C);

struct h1_wphi_crosshairs
{
	int16 crosshair_type;
	int16 runtime_flags;
	int16 use_on_map_type;
	int16 pad;
	int32 unused[7];
	h1_tag_reference bitmap;
	h1_tag_block<h1_wphi_crosshair_item> items;
	int32 unused2[10];
};
static_assert(sizeof(h1_wphi_crosshairs) == 0x68);

struct h1_wphi
{
	h1_tag_reference parent_hud;
	int16 flash_flags;
	int16 flash_pad;
	int16 flash_total_ammo;
	int16 flash_loaded_ammo;
	int16 flash_heat;
	int16 flash_age;
	int32 flash_unused[8];
	h1_hud_absolute_placement absolute_placement;
	h1_tag_block<h1_wphi_static> statics;
	h1_tag_block<h1_wphi_meter> meters;
	h1_tag_block<h1_wphi_number> numbers;
	h1_tag_block<h1_wphi_crosshairs> crosshairs;
	h1_tag_block<h1_wphi_overlays> overlays;
	uint32 valid_crosshair_types_flags;
	h1_tag_block<uint8> warning_sounds;
	h1_tag_block<uint8> screen_effects;
	int32 unused1[33];
	uint8 messaging_icon[16];
	int32 unused2[12];
};
static_assert(sizeof(h1_wphi) == 0x17C);

struct h1_hud_number_definition
{
	h1_tag_reference number_bitmap;
	int8 character_width;
	int8 screen_width;
	int8 x_offset;
	int8 y_offset;
	int8 decimal_point_width;
	int8 colon_width;
	int16 pad;
	int32 unused[19];
};
static_assert(sizeof(h1_hud_number_definition) == 0x64);

// unit_hud_interface_definition.h
struct h1_hud_metered_panel
{
	h1_hud_static_element background;
	h1_hud_meter_element meter;
	uint32 extras[4];	// shield: the overcharge colors, health: mid color, maximum and minimum cutoffs
	int32 unused[4];
};
static_assert(sizeof(h1_hud_metered_panel) == 0xF0);

struct h1_unhi
{
	h1_hud_absolute_placement absolute_placement;
	h1_hud_static_element background;
	h1_hud_metered_panel shield_meter;
	h1_hud_metered_panel health_meter;
	h1_hud_static_element motion_sensor_background;
	h1_hud_static_element motion_sensor_foreground;
	int32 motion_sensor_unused[8];
	h1_hud_placement blip_placement;
	// the auxilary panel and meters (the flashlight) aren't drawn
};
static_assert(offsetof(h1_unhi, shield_meter) == 0x8C);
static_assert(offsetof(h1_unhi, health_meter) == 0x17C);
static_assert(offsetof(h1_unhi, blip_placement) == 0x35C);

// hud_weapon.c grenade_hud_interface_definition
struct h1_grhi
{
	h1_hud_absolute_placement absolute_placement;
	h1_hud_static_element background;
	h1_hud_static_element count_background;
	h1_hud_number_element count_numbers;
	int16 flash_cutoff;
	int16 pad;
	h1_tag_reference overlay_bitmap;
	h1_tag_block<h1_wphi_overlay_item> overlay_items;
};
static_assert(offsetof(h1_grhi, count_numbers) == 0xF4);
static_assert(offsetof(h1_grhi, overlay_bitmap) == 0x14C);

// hud_unit.c unit_hud_state
struct s_h1_unit_hud_state
{
	real32 last_shield_vitality;
	real32 fade_time;
	int32 last_shield_hit_time;
	int32 last_shield_flash_time;
	int32 last_health_flash_time;
	int32 last_grenade_flash_time;
	datum last_unit_index;
};

// hud_weapon.c weapon_hud_state and crosshair_hud_state
struct s_h1_hud_state
{
	int32 last_weapon_flash_time[k_h1_weapon_hud_flash_references];
	datum last_weapon_index;
	int32 crosshair_states[k_h1_crosshair_states];
	uint32 render_flags;
};

// hud_draw.c rasterizer_meter_parameters
struct s_h1_meter_parameters
{
	uint32 gradient_min_color;
	uint32 gradient_max_color;
	uint32 background_color;
	uint32 flash_color;
	uint32 tint_color;
	bool flash_color_is_negative;
};

struct s_h1_hud_vertex
{
	real32 x, y, z, w;
	real32 u, v;
	D3DCOLOR color;
};

// the hud's window: halo 1's 480 high window, as wide as the screen's shape
struct s_h1_hud_window
{
	real32 x0, x1, y0, y1;
	real32 pixel_scale;		// screen pixels per hud pixel
	D3DVIEWPORT9 viewport;
};

/* globals */

static s_h1_hud_state g_h1_hud = { {}, NONE, {}, 0 };
static s_h1_unit_hud_state g_h1_unit_hud = { -1.f, -1.f, 0, NONE, NONE, NONE, NONE };
static s_h1_hud_window g_h1_hud_window;
static IDirect3DVertexShader9* g_h1_hud_vertex_shader = NULL;
static IDirect3DPixelShader9* g_h1_hud_pixel_shader = NULL;
static IDirect3DPixelShader9* g_h1_hud_meter_shader = NULL;
static IDirect3DVertexDeclaration9* g_h1_hud_vertex_declaration = NULL;

static const char k_h1_hud_vertex_shader[] = R"(
struct VS_INPUT { float4 position : POSITION; float2 texcoord : TEXCOORD0; float4 color : COLOR0; };
struct VS_OUTPUT { float4 position : POSITION; float2 texcoord : TEXCOORD0; float4 color : COLOR0; };
VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = input.position;
	output.texcoord = input.texcoord;
	output.color = input.color;
	return output;
}
)";

// rasterizer_xbox_dynavobgeom.c _rasterizer_psuedo_dynamic_screen_quad_draw, one map: the map times the vertex color
static const char k_h1_hud_pixel_shader[] = R"(
sampler2D map : register(s0);
float4 main(float2 texcoord : TEXCOORD0, float4 color : COLOR0) : COLOR0
{
	return tex2D(map, texcoord) * color;
}
)";

// rasterizer_xbox_dynavobgeom.c _rasterizer_psuedo_dynamic_screen_quad_draw with meter parameters: the four combiners
static const char k_h1_hud_meter_shader[] = R"(
sampler2D map : register(s0);
float4 gradient_minimum : register(c0);	// a: the meter's alpha (its value)
float4 gradient_maximum : register(c1);
float4 flash : register(c2);			// a: the meter's maximum alpha
float4 background : register(c3);
float4 tint : register(c4);
float4 settings : register(c5);			// x: the flash color is negative
float4 main(float2 texcoord : TEXCOORD0) : COLOR0
{
	float4 t0 = tex2D(map, texcoord);
	// alpha kill
	clip(t0.a - 0.5f / 255.0f);
	// combiner 0: r0.a = 4(c1.a t0.b + c1.a t0.b) with c1.a 1/8, r0.rgb = 4(c0.a - t0.rgb)
	float r0a = saturate(t0.b);
	float3 r0 = clamp(4.0f * (gradient_minimum.a - t0.rgb), -1.0f, 1.0f);
	// combiner 1: r0.a = -(2 r0.b - 1), r0.rgb = (1 - r0.a) c0 + r0.a c1
	float a1 = -(2.0f * saturate(r0.b) - 1.0f);
	float3 c1 = clamp((1.0f - r0a) * gradient_minimum.rgb + r0a * gradient_maximum.rgb, -1.0f, 1.0f);
	// combiner 2: r0.a = t0.b + (0.5 - c1.a), r0.rgb = r0.rgb + r0.a c1 (the flash at the meter's edge)
	float a2 = clamp(t0.b + (0.5f - saturate(flash.a)), -1.0f, 1.0f);
	float3 c2 = clamp(c1 + saturate(a1) * (settings.x > 0.5f ? -flash.rgb : flash.rgb), -1.0f, 1.0f);
	// combiner 3 (mux on r0.a): past the meter's value the background
	float alpha = a2 >= 0.5f ? background.a : tint.a;
	float3 color = a2 >= 0.5f ? background.rgb : c2;
	// final combiner: r0.rgb t0.a, r0.a
	return float4(saturate(color * t0.a), saturate(alpha));
}
)";

/* prototypes */

static bool h1_hud_initialize(void);
static int32 h1_hud_time(void);
static void h1_hud_update(datum unit_index, datum weapon_index, const h1_wphi* root_definition, const s_h1_weapon_interface_state* weapon_state);
static void h1_hud_crosshairs_draw(datum unit_index, datum weapon_index, datum hud_index, const s_h1_weapon_interface_state* weapon_state);
static void h1_hud_render_weapon(datum hud_index, const h1_weap* weapon_definition, const s_h1_weapon_interface_state* weapon_state,
	const int16* new_state_flags, const int16* new_overlay_flags, const int16* new_numbers);
static const h1_wphi* h1_wphi_get(datum hud_index);
static uint32 h1_hud_flash_color(const h1_hud_color* color, int32 reference_value);
static int32 h1_hud_flash_duration(const h1_hud_color* color);
static void h1_hud_calculate_point(const h1_hud_absolute_placement* absolute_placement, const h1_hud_placement* placement, real_point2d* result);
static void h1_hud_bitmap_bounds(int16 corner, const real_rectangle2d* clip, real32 width, real32 height, bool interface_bitmap, real_rectangle2d* bounds);
static bool h1_hud_bitmap_get(datum bitmap_tag_index, int16 sequence_index, int16 frame_index, int16* bitmap_index, const real_rectangle2d** clip, real_rectangle2d* clip_storage);
static void h1_hud_draw_bitmap(datum bitmap_tag_index, int16 bitmap_index, const real_point2d* point, int16 corner, const real_rectangle2d* clip,
	const real_vector2d* xy_scale, uint32 color, const s_h1_meter_parameters* meter);
static void h1_hud_draw_bitmap_placed(datum bitmap_tag_index, int16 sequence_index, int16 frame_index, const h1_hud_absolute_placement* absolute_placement,
	const h1_hud_placement* placement, real32 scale, uint32 color, const s_h1_meter_parameters* meter, bool use_sprite_clip);
static void h1_hud_draw_static(const h1_hud_absolute_placement* absolute_placement, const h1_hud_static_element* element, int16 draw_flags, int32 flash_reference_time);
static void h1_hud_draw_meter(const h1_hud_absolute_placement* absolute_placement, const h1_hud_meter_element* meter, uint8 min_value, uint8 max_value,
	int16 draw_flags, real32 reference_time, real32 reference_value);
static void h1_hud_draw_numbers(const h1_hud_absolute_placement* absolute_placement, const h1_hud_number_element* numbers, int16 value, int16 decimal_value,
	int16 draw_flags, int32 flash_reference_time);
static void h1_hud_draw_overlays(const h1_hud_absolute_placement* absolute_placement, const h1_wphi_overlays* overlays, int32 type_flags,
	int32 reference_time, int16 draw_flags);
static void h1_hud_render_unit(datum unit_index);
static void h1_hud_render_grenades(datum unit_index, const h1_weap* weapon_definition);
static void h1_pixel32_to_argb(uint32 color, real32* argb);
static uint32 h1_argb_to_pixel32(const real32* argb);

/* public code */

void h1_hud_dispose(void)
{
	if (g_h1_hud_vertex_shader) g_h1_hud_vertex_shader->Release();
	if (g_h1_hud_pixel_shader) g_h1_hud_pixel_shader->Release();
	if (g_h1_hud_meter_shader) g_h1_hud_meter_shader->Release();
	if (g_h1_hud_vertex_declaration) g_h1_hud_vertex_declaration->Release();
	g_h1_hud_vertex_shader = NULL;
	g_h1_hud_pixel_shader = NULL;
	g_h1_hud_meter_shader = NULL;
	g_h1_hud_vertex_declaration = NULL;
	return;
}

void h1_hud_render(void)
{
	if (!h1_maps_active() || !g_h1_cache_file)
	{
		return;
	}
	const datum unit_index = h1_first_person_weapon_unit_get();
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	if (!unit || unit->unit.weapon_indices[0] == NONE)
	{
		return;
	}
	const datum weapon_index = unit_inventory_get_weapon(unit_index, unit->unit.weapon_indices[0]);
	const weapon_datum* weapon = (const weapon_datum*)object_try_and_get_and_verify_type(weapon_index, _object_mask_weapon);
	const datum h1_weapon_index = weapon ? h1_weapon_h1_get(weapon->definition_index) : NONE;
	const h1_weap* definition = h1_weapon_index != NONE ? (const h1_weap*)g_h1_cache_file->tag_get('weap', h1_weapon_index) : NULL;
	s_h1_weapon_interface_state weapon_state;
	if (!definition || definition->hud_interface.index == NONE || !h1_wphi_get(definition->hud_interface.index) ||
		!h1_weapon_logic_interface_state(weapon_index, &weapon_state))
	{
		return;
	}
	if (!h1_hud_initialize())
	{
		return;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	IDirect3DStateBlock9* state_block = NULL;
	if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state_block)))
	{
		return;
	}

	// the hud's window: 480 high, as wide as the screen's shape
	device->GetViewport(&g_h1_hud_window.viewport);
	g_h1_hud_window.pixel_scale = (real32)g_h1_hud_window.viewport.Height / (real32)k_h1_hud_window_height;
	g_h1_hud_window.x0 = 0.f;
	g_h1_hud_window.y0 = 0.f;
	g_h1_hud_window.x1 = (real32)g_h1_hud_window.viewport.Width / g_h1_hud_window.pixel_scale;
	g_h1_hud_window.y1 = (real32)k_h1_hud_window_height;

	device->SetVertexDeclaration(g_h1_hud_vertex_declaration);
	device->SetVertexShader(g_h1_hud_vertex_shader);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

	// hud_update_weapon and hud_render_weapon_interface
	h1_hud_update(unit_index, weapon_index, h1_wphi_get(definition->hud_interface.index), &weapon_state);
	h1_hud_crosshairs_draw(unit_index, weapon_index, definition->hud_interface.index, &weapon_state);
	h1_hud_render_weapon(definition->hud_interface.index, definition, &weapon_state, NULL, NULL, NULL);
	h1_hud_render_grenades(unit_index, definition);
	g_h1_hud.last_weapon_index = weapon_index;
	h1_hud_render_unit(unit_index);

	state_block->Apply();
	state_block->Release();
	return;
}

/* private code */

static bool h1_hud_initialize(void)
{
	if (g_h1_hud_vertex_shader && g_h1_hud_pixel_shader && g_h1_hud_meter_shader && g_h1_hud_vertex_declaration)
	{
		return true;
	}
	h1_hud_dispose();
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	auto compile = [](const char* source, const char* profile) -> LPD3DXBUFFER
	{
		LPD3DXBUFFER code = NULL;
		LPD3DXBUFFER errors = NULL;
		if (FAILED(D3DXCompileShader(source, (UINT)strlen(source), NULL, NULL, "main", profile, 0, &code, &errors, NULL)))
		{
			h1_log("hud: shader error %s", errors ? (const char*)errors->GetBufferPointer() : "");
		}
		if (errors)
		{
			errors->Release();
		}
		return code;
	};
	LPD3DXBUFFER code = compile(k_h1_hud_vertex_shader, "vs_3_0");
	if (code)
	{
		device->CreateVertexShader((const DWORD*)code->GetBufferPointer(), &g_h1_hud_vertex_shader);
		code->Release();
	}
	code = compile(k_h1_hud_pixel_shader, "ps_3_0");
	if (code)
	{
		device->CreatePixelShader((const DWORD*)code->GetBufferPointer(), &g_h1_hud_pixel_shader);
		code->Release();
	}
	code = compile(k_h1_hud_meter_shader, "ps_3_0");
	if (code)
	{
		device->CreatePixelShader((const DWORD*)code->GetBufferPointer(), &g_h1_hud_meter_shader);
		code->Release();
	}
	const D3DVERTEXELEMENT9 elements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		D3DDECL_END()
	};
	device->CreateVertexDeclaration(elements, &g_h1_hud_vertex_declaration);
	return g_h1_hud_vertex_shader && g_h1_hud_pixel_shader && g_h1_hud_meter_shader && g_h1_hud_vertex_declaration;
}

// halo 1 ticks of the game time (the hud's flash references and frame rates)
static int32 h1_hud_time(void)
{
	return (int32)((real32)game_time_get() * game_tick_length() * k_h1_hud_ticks_per_second);
}

static const h1_wphi* h1_wphi_get(datum hud_index)
{
	return hud_index != NONE ? (const h1_wphi*)g_h1_cache_file->tag_get('wphi', hud_index) : NULL;
}

// hud_weapon.c hud_update_weapon_local_player: the crosshair states
static void h1_hud_update(datum unit_index, datum weapon_index, const h1_wphi* root_definition, const s_h1_weapon_interface_state* weapon_state)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	const uint32 unit_control = *(const uint32*)&unit->unit.control_flags;
	const bool primary_trigger = (unit_control & k_h2_unit_control_primary_trigger_held) != 0;

	uint32 valid_crosshair_types = root_definition->valid_crosshair_types_flags;
	const h1_wphi* definition = root_definition;
	for (int32 depth = 1; depth < k_h1_maximum_weapon_hud_depth && definition->parent_hud.index != NONE; depth++)
	{
		definition = h1_wphi_get(definition->parent_hud.index);
		if (!definition)
		{
			break;
		}
		valid_crosshair_types |= definition->valid_crosshair_types_flags;
	}

	uint32 render_flags = 0;
	for (int32 crosshair_index = 0; crosshair_index < k_h1_crosshair_states; crosshair_index++)
	{
		if (!TEST_BIT(valid_crosshair_types, crosshair_index))
		{
			continue;
		}
		int32* state = &g_h1_hud.crosshair_states[crosshair_index];
		int32 result = 0;
		const auto* magazine0 = &weapon_state->magazines[0];
		const auto* magazine1 = &weapon_state->magazines[1];
		switch (crosshair_index)
		{
		case _h1_crosshair_state_primary_trigger_ready: result = magazine0->can_fire; break;
		case _h1_crosshair_state_secondary_trigger_ready: result = magazine1->can_fire; break;
		case _h1_crosshair_state_aim: result = weapon_index != NONE && unit->unit.target_info.primary_auto_aim_level >= 1.f; break;
		case _h1_crosshair_state_zoom: result = unit->unit.current_zoom_level >= 0 ? unit->unit.current_zoom_level + 2 : 1; break;
		case _h1_crosshair_state_reload: result = magazine0->reloading; break;
		case _h1_crosshair_state_flash_ammo:
			result = (magazine0->rounds_remaining || magazine0->rounds_loaded) && magazine0->rounds_remaining &&
				magazine0->rounds_loaded <= root_definition->flash_loaded_ammo;
			break;
		case _h1_crosshair_state_flash_heat: result = weapon_state->heat * 100.f >= (real32)root_definition->flash_heat; break;
		case _h1_crosshair_state_flash_total_ammo: result = magazine0->rounds_remaining <= root_definition->flash_total_ammo && !magazine0->reloading; break;
		case _h1_crosshair_state_flash_total_battery: result = weapon_state->age < 1.f && (1.f - weapon_state->age) * 100.f <= (real32)root_definition->flash_age; break;
		case _h1_crosshair_state_charge: result = 0; break;
		case _h1_crosshair_state_flash_fired_battery_depleted: result = (weapon_state->age == 1.f && primary_trigger) || *state != NONE; break;
		case _h1_crosshair_state_fired_with_no_ammo:
			result = (!magazine0->rounds_loaded && !magazine0->rounds_remaining && primary_trigger) || *state != NONE;
			break;
		case _h1_crosshair_state_threw_with_no_grenade: result = *state != NONE; break;
		case _h1_crosshair_state_flash_ammo_none_for_reload:
			result = (magazine0->rounds_remaining || magazine0->rounds_loaded) && !magazine0->rounds_remaining &&
				magazine0->rounds_loaded <= root_definition->flash_loaded_ammo;
			break;
		case _h1_crosshair_state_flash_secondary_ammo:
			result = (magazine1->rounds_remaining || magazine1->rounds_loaded) && magazine1->rounds_remaining &&
				magazine1->rounds_loaded <= root_definition->flash_loaded_ammo;
			break;
		case _h1_crosshair_state_flash_secondary_total_ammo: result = magazine1->rounds_remaining <= root_definition->flash_total_ammo && !magazine1->reloading; break;
		case _h1_crosshair_state_secondary_reload: result = magazine1->reloading; break;
		case _h1_crosshair_state_fired_secondary_with_no_ammo:
			result = (!magazine1->rounds_loaded && !magazine1->rounds_remaining && primary_trigger) || *state != NONE;
			break;
		case _h1_crosshair_state_flash_secondary_ammo_none_for_reload:
			result = (magazine1->rounds_remaining || magazine1->rounds_loaded) && !magazine1->rounds_remaining &&
				magazine1->rounds_loaded <= root_definition->flash_loaded_ammo;
			break;
		}

		if (result > 0 || crosshair_index == _h1_crosshair_state_aim)
		{
			render_flags |= FLAG(crosshair_index);
		}
		switch (crosshair_index)
		{
		case _h1_crosshair_state_aim:
			*state = result;
			break;
		case _h1_crosshair_state_zoom:
			*state = result - 1;
			break;
		default:
			if (!result)
			{
				*state = NONE;
			}
			else if (*state == NONE)
			{
				*state = h1_hud_time();
			}
			break;
		}
	}
	g_h1_hud.render_flags = render_flags;
	return;
}

// hud_weapon.c crosshairs_draw
static void h1_hud_crosshairs_draw(datum unit_index, datum weapon_index, datum hud_index, const s_h1_weapon_interface_state* weapon_state)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	const uint32 unit_control = *(const uint32*)&unit->unit.control_flags;
	const bool primary_trigger = (unit_control & k_h2_unit_control_primary_trigger_held) != 0;
	const int16 map_type_flags = FLAG(0) | FLAG(1);
	const h1_hud_absolute_placement absolute_placement = { _h1_hud_anchor_center };

	const h1_wphi* definitions[k_h1_maximum_weapon_hud_depth];
	int32 definition_count = 0;
	for (datum index = hud_index; index != NONE && definition_count < k_h1_maximum_weapon_hud_depth; )
	{
		const h1_wphi* definition = h1_wphi_get(index);
		if (!definition)
		{
			break;
		}
		definitions[definition_count++] = definition;
		index = definition->parent_hud.index;
	}

	for (int32 definition_index = 0; definition_index < definition_count; definition_index++)
	{
		const h1_wphi* definition = definitions[definition_index];
		for (int32 crosshair_index = 0; crosshair_index < definition->crosshairs.count; crosshair_index++)
		{
			const h1_wphi_crosshairs* element = g_h1_cache_file->block_get(definition->crosshairs, crosshair_index);
			const int16 state_index = element->crosshair_type;
			if (!VALID_INDEX(state_index, k_h1_crosshair_states) || !TEST_BIT(g_h1_hud.render_flags, state_index) ||
				!TEST_BIT(map_type_flags, element->use_on_map_type))
			{
				continue;
			}
			int32* state = &g_h1_hud.crosshair_states[state_index];
			const h1_bitm* bitmap_group = element->bitmap.index != NONE ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', element->bitmap.index) : NULL;
			if (!bitmap_group)
			{
				continue;
			}
			for (int32 item_index = 0; item_index < element->items.count; item_index++)
			{
				const h1_wphi_crosshair_item* item = g_h1_cache_file->block_get(element->items, item_index);
				const int32 zoom = g_h1_hud.crosshair_states[_h1_crosshair_state_zoom];
				if (TEST_BIT(item->flags, _h1_hud_crosshair_runtime_invalid_bit) ||
					(TEST_BIT(item->flags, _h1_hud_crosshair_not_on_default_zoom_bit) && zoom <= 0) ||
					(TEST_BIT(item->flags, _h1_hud_crosshair_only_on_default_zoom_bit) && zoom != 0))
				{
					continue;
				}
				const h1_bitm_sequences* sequence = !TEST_BIT(item->flags, _h1_hud_crosshair_not_a_sprite_bit) &&
					VALID_INDEX(item->sequence_index, bitmap_group->sequences.count) ? g_h1_cache_file->block_get(bitmap_group->sequences, item->sequence_index) : NULL;
				int32 frame_index = 0;
				uint32 color = item->colors.color;
				bool flash = false;

				switch (state_index)
				{
				case _h1_crosshair_state_aim:
					if (TEST_BIT(item->flags, _h1_hud_crosshair_flashes_bit))
					{
						frame_index = 0;
						flash = *state > 0;
						if (flash)
						{
							color = h1_hud_flash_color(&item->colors, 0);
						}
					}
					else
					{
						frame_index = *state;
					}
					break;
				case _h1_crosshair_state_zoom:
					if (TEST_BIT(item->flags, _h1_hud_crosshair_one_zoom_level_bit))
					{
						if (!*state)
						{
							continue;
						}
						frame_index = 0;
					}
					else
					{
						frame_index = *state - (TEST_BIT(item->flags, _h1_hud_crosshair_not_on_default_zoom_bit) ? 1 : 0);
					}
					if (TEST_BIT(item->flags, _h1_hud_crosshair_flashes_bit) && g_h1_hud.crosshair_states[_h1_crosshair_state_aim] > 0)
					{
						color = h1_hud_flash_color(&item->colors, 0);
					}
					break;
				default:
					if (state_index == _h1_crosshair_state_fired_with_no_ammo || state_index == _h1_crosshair_state_threw_with_no_grenade ||
						state_index == _h1_crosshair_state_fired_secondary_with_no_ammo || state_index == _h1_crosshair_state_flash_fired_battery_depleted)
					{
						bool firing_active = false;
						if (state_index == _h1_crosshair_state_flash_fired_battery_depleted)
						{
							firing_active = weapon_state->age == 0.f && primary_trigger;
						}
						else if (state_index == _h1_crosshair_state_fired_with_no_ammo)
						{
							firing_active = !weapon_state->magazines[0].rounds_loaded && !weapon_state->magazines[0].rounds_remaining && primary_trigger;
						}
						if (!firing_active && h1_hud_time() - *state >= h1_hud_flash_duration(&item->colors))
						{
							*state = NONE;
						}
						if (*state == NONE)
						{
							continue;
						}
					}
					frame_index = item->frame_rate > 0 && sequence && sequence->sprites.count > 0 ?
						((h1_hud_time() - *state) / item->frame_rate / k_h1_hud_ticks_per_second) % sequence->sprites.count : 0;
					if (TEST_BIT(item->flags, _h1_hud_crosshair_flashes_bit) && *state != NONE)
					{
						color = h1_hud_flash_color(&item->colors, *state);
					}
					break;
				}

				if (frame_index < 0 || (sequence && frame_index >= sequence->sprites.count))
				{
					continue;
				}
				int16 bitmap_index;
				real_rectangle2d clip_storage;
				const real_rectangle2d* clip = NULL;
				if (sequence)
				{
					if (!h1_hud_bitmap_get(element->bitmap.index, item->sequence_index, (int16)frame_index, &bitmap_index, &clip, &clip_storage))
					{
						continue;
					}
				}
				else
				{
					bitmap_index = item->sequence_index;
				}
				if (!VALID_INDEX(bitmap_index, bitmap_group->bitmaps.count))
				{
					continue;
				}
				real_point2d point;
				h1_hud_calculate_point(&absolute_placement, &item->placement, &point);
				const real_vector2d xy_scale = { item->placement.scale.i, item->placement.scale.j };
				h1_hud_draw_bitmap(element->bitmap.index, bitmap_index, &point, absolute_placement.corner, clip, &xy_scale, color, NULL);
			}
		}
	}
	return;
}

// hud_weapon.c render_weapon_hud
static void h1_hud_render_weapon(datum hud_index, const h1_weap* weapon_definition, const s_h1_weapon_interface_state* weapon_state,
	const int16* new_state_flags, const int16* new_overlay_flags, const int16* new_numbers)
{
	const h1_wphi* definition = h1_wphi_get(hud_index);
	if (!definition)
	{
		return;
	}
	int16 state_flags[k_h1_weapon_hud_flash_references] = {};
	int16 overlay_flags[k_h1_weapon_hud_flash_references] = {};
	int16 number_values[k_h1_weapon_hud_flash_references] = {};

	if (TEST_BIT(definition->flash_flags, 0) && new_state_flags && new_overlay_flags && new_numbers)
	{
		// the parent's flash parameters
		csmemcpy(state_flags, new_state_flags, sizeof(state_flags));
		csmemcpy(overlay_flags, new_overlay_flags, sizeof(overlay_flags));
		csmemcpy(number_values, new_numbers, sizeof(number_values));
	}
	else
	{
		const auto* magazine0 = &weapon_state->magazines[0];
		const auto* magazine1 = &weapon_state->magazines[1];
		const int32 age_percentage = PIN((int32)(weapon_state->age * 100.f), 0, 100);
		auto flags = [](bool flashing, bool disabled) -> int16
		{
			return (int16)((flashing ? FLAG(_h1_hud_draw_flashing_bit) : 0) | (disabled ? FLAG(_h1_hud_draw_disabled_bit) : 0));
		};
		state_flags[0] = flags(magazine0->rounds_remaining <= definition->flash_total_ammo, magazine0->rounds_remaining == 0);
		state_flags[1] = flags(magazine0->rounds_loaded <= definition->flash_loaded_ammo && !magazine0->reloading, false);
		state_flags[2] = flags(weapon_state->heat * 100.f >= definition->flash_heat, false);
		state_flags[3] = flags((1.f - weapon_state->age) * 100.f <= definition->flash_age, 100 - age_percentage == 0);
		state_flags[4] = flags(magazine1->rounds_remaining <= definition->flash_total_ammo, magazine1->rounds_remaining == 0);
		state_flags[5] = flags(magazine1->rounds_loaded <= definition->flash_loaded_ammo && !magazine1->reloading, false);

		for (int32 i = 0; i < k_h1_weapon_hud_flash_references; i++)
		{
			if (TEST_BIT(state_flags[i], _h1_hud_draw_flashing_bit))
			{
				if (g_h1_hud.last_weapon_flash_time[i] == NONE || g_h1_hud.last_weapon_flash_time[i] == 0)
				{
					g_h1_hud.last_weapon_flash_time[i] = h1_hud_time();
				}
			}
			else
			{
				g_h1_hud.last_weapon_flash_time[i] = NONE;
			}
		}

		auto overlay = [](bool flashing, bool reload, bool empty) -> int16
		{
			int16 result = (int16)((flashing ? FLAG(_h1_weapon_overlay_on_flashing_bit) : 0) | (reload ? FLAG(_h1_weapon_overlay_on_reload_bit) : 0) |
				(empty ? FLAG(_h1_weapon_overlay_on_empty_bit) : 0));
			if (!result)
			{
				result |= FLAG(_h1_weapon_overlay_on_default_bit);
			}
			return (int16)(result | FLAG(_h1_weapon_overlay_on_always_bit));
		};
		overlay_flags[2] = overlay(weapon_state->heat * 100.f >= definition->flash_heat, weapon_state->overheated, 100 - age_percentage == 0);
		overlay_flags[3] = overlay((1.f - weapon_state->age) * 100.f <= definition->flash_age, weapon_state->overheated, 100 - age_percentage == 0);
		// (the secondary magazine's flags overwrite the first two, first-party)
		overlay_flags[0] = overlay(magazine1->rounds_remaining <= definition->flash_total_ammo && !magazine1->reloading, magazine1->reloading,
			magazine1->rounds_remaining == 0);
		overlay_flags[1] = overlay(magazine1->rounds_loaded <= definition->flash_loaded_ammo, magazine1->reloading, magazine1->rounds_loaded == 0);
		if (weapon_state->magazine_count < 2)
		{
			overlay_flags[0] = overlay(magazine0->rounds_remaining <= definition->flash_total_ammo && !magazine0->reloading, magazine0->reloading,
				magazine0->rounds_remaining == 0);
			overlay_flags[1] = overlay(magazine0->rounds_loaded <= definition->flash_loaded_ammo, magazine0->reloading, magazine0->rounds_loaded == 0);
		}

		number_values[0] = magazine0->rounds_remaining;
		number_values[1] = magazine0->rounds_loaded;
		number_values[2] = (int16)(weapon_state->heat * 255.f);
		number_values[3] = (int16)((1.f - weapon_state->age) * 100.f);
		number_values[4] = magazine1->rounds_remaining;
		number_values[5] = magazine1->rounds_loaded;
	}

	if (definition->parent_hud.index != NONE)
	{
		h1_hud_render_weapon(definition->parent_hud.index, weapon_definition, weapon_state, state_flags, overlay_flags, number_values);
	}

	const int16 map_type_flags = FLAG(0) | FLAG(1);
	for (int32 i = 0; i < definition->statics.count; i++)
	{
		const h1_wphi_static* element = g_h1_cache_file->block_get(definition->statics, i);
		const int16 state_index = element->header.state_type;
		if (!TEST_BIT(element->header.runtime_flags, 0) && TEST_BIT(map_type_flags, element->header.use_on_map_type) &&
			VALID_INDEX(state_index, k_h1_weapon_hud_flash_references))
		{
			h1_hud_draw_static(&definition->absolute_placement, &element->static_element, state_flags[state_index], g_h1_hud.last_weapon_flash_time[state_index]);
		}
	}
	for (int32 i = 0; i < definition->meters.count; i++)
	{
		const h1_wphi_meter* element = g_h1_cache_file->block_get(definition->meters, i);
		const int16 state_index = element->header.state_type;
		if (!TEST_BIT(element->header.runtime_flags, 0) && TEST_BIT(map_type_flags, element->header.use_on_map_type) &&
			VALID_INDEX(state_index, k_h1_weapon_hud_flash_references))
		{
			h1_hud_draw_meter(&definition->absolute_placement, &element->meter_element, (uint8)number_values[state_index], (uint8)number_values[state_index],
				state_flags[state_index], (real32)g_h1_hud.last_weapon_flash_time[state_index], 0.f);
		}
	}
	for (int32 i = 0; i < definition->numbers.count; i++)
	{
		const h1_wphi_number* element = g_h1_cache_file->block_get(definition->numbers, i);
		const int16 state_index = element->header.state_type;
		if (TEST_BIT(element->header.runtime_flags, 0) || !TEST_BIT(map_type_flags, element->header.use_on_map_type) ||
			!VALID_INDEX(state_index, k_h1_weapon_hud_flash_references))
		{
			continue;
		}
		if (element->number_element.fractional_digits)
		{
			// the range finder's distances need the aim assist target: none
			continue;
		}
		int16 magazine_size = 1;
		if (TEST_BIT(element->weapon_flags, 0) && weapon_definition->magazines.count > 0)
		{
			magazine_size = MAX(g_h1_cache_file->block_get(weapon_definition->magazines, 0)->rounds_loaded_maximum, (int16)1);
		}
		h1_hud_draw_numbers(&definition->absolute_placement, &element->number_element, (int16)(number_values[state_index] / magazine_size), NONE,
			state_flags[state_index], g_h1_hud.last_weapon_flash_time[state_index]);
	}
	for (int32 i = 0; i < definition->overlays.count; i++)
	{
		const h1_wphi_overlays* element = g_h1_cache_file->block_get(definition->overlays, i);
		const int16 state_index = element->header.state_type;
		if (!TEST_BIT(element->header.runtime_flags, 0) && TEST_BIT(map_type_flags, element->header.use_on_map_type) &&
			VALID_INDEX(state_index, k_h1_weapon_hud_flash_references))
		{
			h1_hud_draw_overlays(&definition->absolute_placement, element, overlay_flags[state_index], g_h1_hud.last_weapon_flash_time[state_index],
				state_flags[state_index]);
		}
	}
	return;
}

// hud_draw.c get_flash_duration
static int32 h1_hud_flash_duration(const h1_hud_color* color)
{
	return (int32)(color->flash_period * 30.f);
}

// hud_draw.c get_flash_color
static uint32 h1_hud_flash_color(const h1_hud_color* hud_color, int32 reference_value)
{
	real32 base_color[4];
	real32 flash_color[4];
	real32 result[4];
	h1_pixel32_to_argb(hud_color->color, base_color);
	h1_pixel32_to_argb(hud_color->flash_color, flash_color);
	const bool reverse = TEST_BIT(hud_color->flash_flags, _h1_hud_flash_reverse_colors_bit);
	real32 flash_phase = hud_color->flash_period > 0.f ?
		fmodf((real32)(h1_hud_time() - reference_value) * (1.f / k_h1_hud_ticks_per_second), hud_color->flash_period) : 0.f;
	if (flash_phase < hud_color->number_of_flashes * (hud_color->flash_delay + hud_color->flash_length))
	{
		const real32 cycle = hud_color->flash_delay + hud_color->flash_length;
		flash_phase = cycle > 0.f ? fmodf(flash_phase, cycle) : 0.f;
		if (!reference_value)
		{
			csmemcpy(result, reverse ? base_color : flash_color, sizeof(result));
		}
		else if (flash_phase < hud_color->flash_length)
		{
			const real32 fraction = sqrtf(PIN(1.f - (cosf(flash_phase / hud_color->flash_length * 6.283f) + 1.f) * 0.5f, 0.f, 1.f));
			const real32* from = reverse ? flash_color : base_color;
			const real32* to = reverse ? base_color : flash_color;
			for (int32 i = 0; i < 4; i++)
			{
				result[i] = from[i] + (to[i] - from[i]) * fraction;
			}
		}
		else
		{
			csmemcpy(result, reverse ? flash_color : base_color, sizeof(result));
		}
	}
	else
	{
		csmemcpy(result, base_color, sizeof(result));
	}
	return h1_argb_to_pixel32(result);
}

// hud_draw.c hud_calculate_point (no bitmap registration point)
static void h1_hud_calculate_point(const h1_hud_absolute_placement* absolute_placement, const h1_hud_placement* placement, real_point2d* result)
{
	const real32 scale = 1.f;
	const int16 corner = absolute_placement->corner;
	if (corner < _h1_hud_anchor_center)
	{
		const bool right = TEST_BIT(corner, 0);
		const bool bottom = TEST_BIT(corner, 1);
		result->x = placement->offset.x * (right ? -1.f : 1.f) * scale + (right ? g_h1_hud_window.x1 : g_h1_hud_window.x0);
		result->y = placement->offset.y * (bottom ? -1.f : 1.f) * scale + (bottom ? g_h1_hud_window.y1 : g_h1_hud_window.y0);
	}
	else
	{
		result->x = (real32)(int32)((g_h1_hud_window.x1 + g_h1_hud_window.x0) / 2.f) + placement->offset.x * scale;
		result->y = (real32)(int32)((g_h1_hud_window.y1 + g_h1_hud_window.y0) / 2.f) + placement->offset.y * scale;
	}
	result->x = (real32)(int32)result->x;
	result->y = (real32)(int32)result->y;
	return;
}

// hud_draw.c hud_calculate_bitmap_bounds
static void h1_hud_bitmap_bounds(int16 corner, const real_rectangle2d* clip, real32 width, real32 height, bool interface_bitmap, real_rectangle2d* bounds)
{
	const real32 w = (clip->x1 - clip->x0) * (interface_bitmap ? 1.f : width);
	const real32 h = (clip->y1 - clip->y0) * (interface_bitmap ? 1.f : height);
	switch (corner)
	{
	case _h1_hud_anchor_top_left: *bounds = { 0.f, w, 0.f, h }; break;
	case _h1_hud_anchor_top_right: *bounds = { -w, 0.f, 0.f, h }; break;
	case _h1_hud_anchor_bottom_left: *bounds = { 0.f, w, -h, 0.f }; break;
	case _h1_hud_anchor_bottom_right: *bounds = { -w, 0.f, -h, 0.f }; break;
	default: *bounds = { w * -0.5f, w * 0.5f, h * -0.5f, h * 0.5f }; break;
	}
	return;
}

// hud_draw.c hud_retrieve_bitmap_and_bounding_rect: a sequence frame's bitmap and its sprite's bounds (none without sprites)
static bool h1_hud_bitmap_get(datum bitmap_tag_index, int16 sequence_index, int16 frame_index, int16* bitmap_index, const real_rectangle2d** clip,
	real_rectangle2d* clip_storage)
{
	const h1_bitm* group = bitmap_tag_index != NONE ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index) : NULL;
	*clip = NULL;
	if (!group || !VALID_INDEX(sequence_index, group->sequences.count))
	{
		return false;
	}
	const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(group->sequences, sequence_index);
	frame_index &= 0x7FFF;
	if (sequence->sprites.count > 0)
	{
		const h1_bitm_sequences_sprites* sprite = g_h1_cache_file->block_get(sequence->sprites, frame_index % sequence->sprites.count);
		*bitmap_index = sprite->bitmap_index;
		*clip_storage = { sprite->left, sprite->right, sprite->top, sprite->bottom };
		*clip = clip_storage;
	}
	else
	{
		*bitmap_index = (int16)(sequence->first_bitmap_index + (sequence->bitmap_count > 0 ? frame_index % sequence->bitmap_count : 0));
	}
	return VALID_INDEX(*bitmap_index, group->bitmaps.count);
}

// hud_draw.c hud_draw_bitmap_internal and rasterizer_xbox_dynavobgeom.c _rasterizer_psuedo_dynamic_screen_quad_draw
static void h1_hud_draw_bitmap(datum bitmap_tag_index, int16 bitmap_index, const real_point2d* point, int16 corner, const real_rectangle2d* clip,
	const real_vector2d* xy_scale, uint32 color, const s_h1_meter_parameters* meter)
{
	const h1_bitm* group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index);
	if (!group || !VALID_INDEX(bitmap_index, group->bitmaps.count))
	{
		return;
	}
	IDirect3DBaseTexture9* texture = h1_bitmap_texture_get(bitmap_tag_index, bitmap_index);
	if (!texture)
	{
		return;
	}
	const h1_bitm_bitmaps* bitmap = g_h1_cache_file->block_get(group->bitmaps, bitmap_index);
	const bool interface_bitmap = group->type == _h1_bitmap_group_type_interface_bitmaps;
	const real_rectangle2d default_clip = { 0.f, interface_bitmap ? (real32)bitmap->width : 1.f, 0.f, interface_bitmap ? (real32)bitmap->height : 1.f };
	if (!clip)
	{
		clip = &default_clip;
	}
	real_rectangle2d bounds;
	h1_hud_bitmap_bounds(corner, clip, (real32)bitmap->width, (real32)bitmap->height, interface_bitmap, &bounds);

	// interface bitmaps are addressed in texels
	const real32 texel_u = interface_bitmap ? 1.f / MAX((real32)bitmap->width, 1.f) : 1.f;
	const real32 texel_v = interface_bitmap ? 1.f / MAX((real32)bitmap->height, 1.f) : 1.f;
	const D3DVIEWPORT9& viewport = g_h1_hud_window.viewport;
	const real32 pixel_scale = g_h1_hud_window.pixel_scale;
	s_h1_hud_vertex vertices[4];
	for (int32 i = 0; i < 4; i++)
	{
		const real32 texture_x = ((i + 1) & 2) ? clip->x1 : clip->x0;
		const real32 texture_y = i > 1 ? clip->y1 : clip->y0;
		const real32 bound_x = ((i + 1) & 2) ? bounds.x1 : bounds.x0;
		const real32 bound_y = i > 1 ? bounds.y1 : bounds.y0;
		const real32 x = (point->x + (real32)(int32)(bound_x * xy_scale->i)) * pixel_scale;
		const real32 y = (point->y + (real32)(int32)(bound_y * xy_scale->j)) * pixel_scale;
		vertices[i].x = (x - 0.5f) * 2.f / (real32)viewport.Width - 1.f;
		vertices[i].y = 1.f - (y - 0.5f) * 2.f / (real32)viewport.Height;
		vertices[i].z = 0.f;
		vertices[i].w = 1.f;
		vertices[i].u = texture_x * texel_u;
		vertices[i].v = texture_y * texel_v;
		vertices[i].color = color;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetTexture(0, texture);
	if (meter)
	{
		// the meter: the source times the tint color plus the destination times the source's alpha
		real32 constants[6][4];
		h1_pixel32_to_argb(meter->gradient_min_color, constants[0]);
		h1_pixel32_to_argb(meter->gradient_max_color, constants[1]);
		h1_pixel32_to_argb(meter->flash_color, constants[2]);
		h1_pixel32_to_argb(meter->background_color, constants[3]);
		h1_pixel32_to_argb(meter->tint_color, constants[4]);
		real32 shader_constants[6][4];
		for (int32 i = 0; i < 5; i++)
		{
			// argb to rgba
			shader_constants[i][0] = constants[i][1];
			shader_constants[i][1] = constants[i][2];
			shader_constants[i][2] = constants[i][3];
			shader_constants[i][3] = constants[i][0];
		}
		shader_constants[5][0] = meter->flash_color_is_negative ? 1.f : 0.f;
		shader_constants[5][1] = shader_constants[5][2] = shader_constants[5][3] = 0.f;
		device->SetPixelShader(g_h1_hud_meter_shader);
		device->SetPixelShaderConstantF(0, &shader_constants[0][0], 6);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_BLENDFACTOR, meter->tint_color);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
	}
	else
	{
		// alpha multiply add
		device->SetPixelShader(g_h1_hud_pixel_shader);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	}
	device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(s_h1_hud_vertex));
	return;
}

// hud_draw.c hud_draw_bitmap_with_meter: a sequence's frame at its placement
static void h1_hud_draw_bitmap_placed(datum bitmap_tag_index, int16 sequence_index, int16 frame_index, const h1_hud_absolute_placement* absolute_placement,
	const h1_hud_placement* placement, real32 scale, uint32 color, const s_h1_meter_parameters* meter, bool use_sprite_clip)
{
	int16 bitmap_index;
	real_rectangle2d clip_storage;
	const real_rectangle2d* clip;
	if (!h1_hud_bitmap_get(bitmap_tag_index, sequence_index, frame_index, &bitmap_index, &clip, &clip_storage))
	{
		return;
	}
	real_point2d point;
	h1_hud_calculate_point(absolute_placement, placement, &point);
	const real_vector2d xy_scale = { placement->scale.i * scale, placement->scale.j * scale };
	h1_hud_draw_bitmap(bitmap_tag_index, bitmap_index, &point, absolute_placement->corner, use_sprite_clip ? clip : NULL, &xy_scale, color, meter);
	return;
}

// hud_draw.c hud_draw_static_element (the multitexture overlays aren't drawn)
static void h1_hud_draw_static(const h1_hud_absolute_placement* absolute_placement, const h1_hud_static_element* element, int16 draw_flags, int32 flash_reference_time)
{
	uint32 color;
	if (TEST_BIT(draw_flags, _h1_hud_draw_disabled_bit))
	{
		color = element->colors.disabled_color;
	}
	else if (TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit))
	{
		color = h1_hud_flash_color(&element->colors, flash_reference_time);
	}
	else
	{
		color = element->colors.color;
	}
	h1_hud_draw_bitmap_placed(element->interface_bitmap.index, element->sequence_index, 0, absolute_placement, &element->placement, 1.f, color, NULL, true);
	return;
}

// hud_draw.c hud_draw_meter
static void h1_hud_draw_meter(const h1_hud_absolute_placement* absolute_placement, const h1_hud_meter_element* meter, uint8 min_value, uint8 max_value,
	int16 draw_flags, real32 reference_time, real32 reference_value)
{
	const int32 alpha = MAX((int32)meter->minimum_value, PIN((int32)(meter->alpha_multiplier * min_value + meter->alpha_bias), 0, 255));
	const int32 max_alpha = MAX((int32)meter->minimum_value, PIN((int32)(meter->alpha_multiplier * max_value + meter->alpha_bias), 0, 255));
	s_h1_meter_parameters parameters = {};
	if (TEST_BIT(draw_flags, _h1_hud_draw_disabled_bit))
	{
		parameters.gradient_min_color = 0;
		parameters.flash_color = 0;
		parameters.gradient_max_color = 0;
	}
	else if (!TEST_BIT(meter->meter_flags, _h1_hud_meter_switch_color_on_state_change_bit))
	{
		const real32 fade = reference_time < 0.f ? 0.f : PIN(1.f - reference_time, 0.f, 1.f);
		real32 flash[4];
		h1_pixel32_to_argb(meter->flash_color, flash);
		flash[1] *= fade;
		flash[2] *= fade;
		flash[3] *= fade;
		flash[0] = 0.f;
		parameters.gradient_min_color = (meter->min_color & 0xFFFFFF) | (alpha << 24);
		parameters.gradient_max_color = meter->max_color & 0xFFFFFF;
		parameters.flash_color = (h1_argb_to_pixel32(flash) & 0xFFFFFF) | (max_alpha << 24);
	}
	else if (TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit))
	{
		if (TEST_BIT(meter->meter_flags, _h1_hud_meter_interpolates_between_min_max_bit))
		{
			real32 minimum[4], maximum[4], color[4];
			h1_pixel32_to_argb(meter->min_color, minimum);
			h1_pixel32_to_argb(meter->max_color, maximum);
			const real32 t = TEST_BIT(meter->meter_flags, _h1_hud_meter_invert_interpolation_value_bit) ? 1.f - reference_value : reference_value;
			for (int32 i = 0; i < 4; i++)
			{
				color[i] = minimum[i] + (maximum[i] - minimum[i]) * t;
			}
			const uint32 pixel = h1_argb_to_pixel32(color) & 0xFFFFFF;
			parameters.gradient_min_color = pixel | (alpha << 24);
			parameters.gradient_max_color = pixel;
			parameters.flash_color = alpha << 24;
		}
		else
		{
			parameters.gradient_min_color = (meter->max_color & 0xFFFFFF) | (alpha << 24);
			parameters.flash_color = alpha << 24;
			parameters.gradient_max_color = meter->max_color & 0xFFFFFF;
		}
	}
	else
	{
		parameters.gradient_min_color = (alpha << 24) | (meter->min_color & 0xFFFFFF);
		parameters.flash_color = alpha << 24;
		parameters.gradient_max_color = meter->min_color & 0xFFFFFF;
	}
	parameters.background_color = ((255 - (meter->empty_color >> 24)) << 24) | (meter->empty_color & 0xFFFFFF);
	// real_alpha_intensity_to_pixel32(fade, 1 - opacity)
	const real32 tint[4] = { PIN(meter->fade, 0.f, 1.f), PIN(1.f - meter->opacity, 0.f, 1.f), PIN(1.f - meter->opacity, 0.f, 1.f), PIN(1.f - meter->opacity, 0.f, 1.f) };
	parameters.tint_color = h1_argb_to_pixel32(tint);
	parameters.flash_color_is_negative = false;
	h1_hud_draw_bitmap_placed(meter->meter_bitmap.index, meter->sequence_index, 0, absolute_placement, &meter->placement, 1.f, 0xFFFFFFFF, &parameters, true);
	return;
}

// hud_draw.c hud_draw_numbers (the hud digits definition of the globals)
static void h1_hud_draw_numbers(const h1_hud_absolute_placement* absolute_placement, const h1_hud_number_element* numbers, int16 value, int16 decimal_value,
	int16 draw_flags, int32 flash_reference_time)
{
	const datum globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* globals = globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', globals_index) : NULL;
	const h1_matg_interface_bitmaps* interface_bitmaps = globals && globals->interface_bitmaps.count > 0 ?
		g_h1_cache_file->block_get(globals->interface_bitmaps, 0) : NULL;
	const h1_hud_number_definition* hud_number = interface_bitmaps && interface_bitmaps->hud_digits_definition.index != NONE ?
		(const h1_hud_number_definition*)g_h1_cache_file->tag_get('hud#', interface_bitmaps->hud_digits_definition.index) : NULL;
	if (!hud_number || hud_number->number_bitmap.index == NONE)
	{
		return;
	}
	const datum number_bitmap = hud_number->number_bitmap.index;
	const bool negative = value < 0;
	real32 digit_count = (real32)numbers->digits;
	const real32 scale = 1.f;
	if (TEST_BIT(numbers->number_flags, _h1_hud_number_show_trailing_m_bit))
	{
		digit_count += 1.f;
	}
	value = (int16)abs(value);

	real_point2d origin;
	h1_hud_calculate_point(absolute_placement, &numbers->placement, &origin);
	real32 cursor_x;
	switch (absolute_placement->corner)
	{
	case _h1_hud_anchor_top_left:
	case _h1_hud_anchor_bottom_left:
		cursor_x = (real32)(int32)((digit_count - 2.f) * hud_number->screen_width * scale + origin.x);
		break;
	case _h1_hud_anchor_center:
		cursor_x = (real32)(int32)((digit_count - 1.f) * hud_number->screen_width * scale * 0.5f + origin.x);
		break;
	default:
		cursor_x = origin.x;
		break;
	}

	uint32 color;
	if (TEST_BIT(draw_flags, _h1_hud_draw_disabled_bit))
	{
		color = numbers->colors.disabled_color;
	}
	else if (TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit))
	{
		color = h1_hud_flash_color(&numbers->colors, flash_reference_time);
	}
	else
	{
		color = numbers->colors.color;
	}

	const real_vector2d xy_scale = { scale, scale };
	auto draw_character = [&](int16 character)
	{
		int16 bitmap_index;
		real_rectangle2d clip_storage;
		const real_rectangle2d* clip;
		if (h1_hud_bitmap_get(number_bitmap, 0, character, &bitmap_index, &clip, &clip_storage))
		{
			const real_point2d point = { cursor_x, origin.y };
			h1_hud_draw_bitmap(number_bitmap, bitmap_index, &point, absolute_placement->corner, clip, &xy_scale, color, NULL);
		}
	};

	if (TEST_BIT(numbers->number_flags, _h1_hud_number_show_trailing_m_bit))
	{
		draw_character(_h1_hud_number_meters_index);
		cursor_x = (real32)(int32)(cursor_x - hud_number->screen_width * scale);
	}
	for (int32 digit_index = 0; digit_index < numbers->digits; digit_index++)
	{
		if (!value && !TEST_BIT(numbers->number_flags, _h1_hud_number_show_all_leading_zeros_bit))
		{
			break;
		}
		draw_character((int16)(value % 10));
		cursor_x = (real32)(int32)(cursor_x - hud_number->screen_width * scale);
		value /= 10;
	}
	if (negative)
	{
		draw_character(_h1_hud_number_negative_sign_index);
	}
	(void)decimal_value;
	return;
}

// hud_draw.c hud_draw_weapon_overlays
static void h1_hud_draw_overlays(const h1_hud_absolute_placement* absolute_placement, const h1_wphi_overlays* overlays, int32 type_flags,
	int32 reference_time, int16 draw_flags)
{
	const h1_bitm* group = overlays->bitmap.index != NONE ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', overlays->bitmap.index) : NULL;
	if (!group)
	{
		return;
	}
	for (int32 item_index = 0; item_index < overlays->items.count; item_index++)
	{
		const h1_wphi_overlay_item* item = g_h1_cache_file->block_get(overlays->items, item_index);
		if (TEST_BIT(item->flags, _h1_hud_overlay_runtime_invalid_bit) || !(item->type & type_flags) ||
			!VALID_INDEX(item->sequence_index, group->sequences.count))
		{
			continue;
		}
		const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(group->sequences, item->sequence_index);
		const bool flashing = TEST_BIT(item->flags, _h1_hud_overlay_flashes_bit) && TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit);
		const uint32 color = flashing ? h1_hud_flash_color(&item->colors, reference_time) : item->colors.color;
		int16 frame_index = 0;
		if (flashing && item->frame_rate > 0 && sequence->sprites.count > 0)
		{
			frame_index = (int16)(((h1_hud_time() - reference_time) / item->frame_rate / k_h1_hud_ticks_per_second) % sequence->sprites.count);
		}
		h1_hud_draw_bitmap_placed(overlays->bitmap.index, item->sequence_index, frame_index, absolute_placement, &item->placement, 1.f, color, NULL, true);
	}
	return;
}

// pixel32 is argb, a byte each
static void h1_pixel32_to_argb(uint32 color, real32* argb)
{
	argb[0] = (real32)((color >> 24) & 0xFF) / 255.f;
	argb[1] = (real32)((color >> 16) & 0xFF) / 255.f;
	argb[2] = (real32)((color >> 8) & 0xFF) / 255.f;
	argb[3] = (real32)(color & 0xFF) / 255.f;
	return;
}

static uint32 h1_argb_to_pixel32(const real32* argb)
{
	uint32 result = 0;
	for (int32 i = 0; i < 4; i++)
	{
		result = (result << 8) | (uint32)(PIN(argb[i], 0.f, 1.f) * 255.f + 0.5f);
	}
	return result;
}

// hud_unit.c hud_update_unit_local_player and hud_render_unit_interface: the halo 1 player's unit hud (the background, the
// shield meter and its overcharge, the health meter) for halo 2's unit
static void h1_hud_render_unit(datum unit_index)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	const datum globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* globals = globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', globals_index) : NULL;
	const h1_matg_player_information* player_information = globals && globals->player_information.count > 0 ?
		g_h1_cache_file->block_get(globals->player_information, 0) : NULL;
	// every halo 1 unit definition keeps the unit fields after the object's, the huds at 0x2a8
	const h1_vehi* player_unit = player_information && player_information->unit.index != NONE ?
		(const h1_vehi*)g_h1_cache_file->tag_get('bipd', player_information->unit.index) : NULL;
	const datum hud_index = player_unit && player_unit->new_hud_interfaces.count > 0 ?
		g_h1_cache_file->block_get(player_unit->new_hud_interfaces, 0)->unit_hud_interface.index : NONE;
	const h1_unhi* hud = hud_index != NONE ? (const h1_unhi*)g_h1_cache_file->tag_get('unhi', hud_index) : NULL;
	if (!unit || !hud)
	{
		return;
	}
	const int32 time = h1_hud_time();
	const real32 shield_vitality = unit->object.shield_vitality;
	const real32 body_vitality = unit->object.body_vitality;
	const bool dead = unit->object.object_damage_flags.test(_object_is_dead_bit);

	// hud_update_unit_local_player: the shield's fade after a hit
	s_h1_unit_hud_state* state = &g_h1_unit_hud;
	if (state->last_unit_index != unit_index)
	{
		state->last_shield_vitality = shield_vitality;
		state->fade_time = -1.f;
		state->last_shield_hit_time = time;
		state->last_shield_flash_time = NONE;
		state->last_health_flash_time = NONE;
		state->last_unit_index = unit_index;
	}
	if (state->last_shield_vitality > shield_vitality)
	{
		if (state->fade_time < 0.f || state->fade_time > 1.f)
		{
			state->last_shield_hit_time = time;
		}
		if (time - state->last_shield_hit_time < 15)
		{
			state->fade_time = 0.f;
		}
		else
		{
			state->last_shield_vitality = shield_vitality;
			state->fade_time += (real32)(time - state->last_shield_hit_time) * (1.f / k_h1_hud_ticks_per_second);
			state->last_shield_hit_time = time;
		}
	}
	else if (state->last_shield_vitality < shield_vitality)
	{
		state->last_shield_vitality = shield_vitality;
		state->fade_time = -1.f;
		state->last_shield_hit_time = time;
	}
	else
	{
		if (state->fade_time > 0.f)
		{
			state->fade_time += (real32)(time - state->last_shield_hit_time) * (1.f / k_h1_hud_ticks_per_second);
		}
		state->last_shield_hit_time = time;
	}

	if (hud->background.interface_bitmap.index != NONE)
	{
		h1_hud_draw_static(&hud->absolute_placement, &hud->background, (int16)(dead ? FLAG(_h1_hud_draw_disabled_bit) : 0), NONE);
	}

	// the shield
	{
		const int16 draw_flags = (int16)((shield_vitality < 0.25f ? FLAG(_h1_hud_draw_flashing_bit) : 0) | (dead ? FLAG(_h1_hud_draw_disabled_bit) : 0));
		if (TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit))
		{
			if (state->last_shield_flash_time == NONE)
			{
				state->last_shield_flash_time = time;
			}
		}
		else
		{
			state->last_shield_flash_time = NONE;
		}
		const h1_hud_meter_element* meter = &hud->shield_meter.meter;
		if (meter->meter_bitmap.index != NONE)
		{
			const int16 value_scale = meter->value_scale ? meter->value_scale : 255;
			h1_hud_meter_element overcharge_meter = *meter;
			const uint32 colors[5] = { 0, 0x00FF0000, 0x0000FF00, 0x00FFFF00, 0x007F00FF };
			for (int32 overcharge_index = 0; overcharge_index <= 4; overcharge_index++)
			{
				const real32 vitality = PIN(shield_vitality - (real32)overcharge_index, 0.f, 1.f);
				const real32 last_vitality = PIN(state->last_shield_vitality - (real32)overcharge_index, 0.f, 1.f);
				const bool fading = last_vitality > vitality;
				const real32 maximum_vitality = fading ? last_vitality : vitality;
				if (vitality <= 0.f && maximum_vitality <= 0.f)
				{
					break;
				}
				overcharge_meter.min_color = colors[overcharge_index];
				overcharge_meter.max_color = colors[overcharge_index];
				h1_hud_draw_meter(&hud->absolute_placement, overcharge_index == 0 ? meter : &overcharge_meter,
					(uint8)PIN((int32)((real32)value_scale * vitality), 0, 255), (uint8)PIN((int32)((real32)value_scale * maximum_vitality), 0, 255),
					draw_flags, fading ? state->fade_time : -1.f, vitality);
			}
		}
		if (hud->shield_meter.background.interface_bitmap.index != NONE)
		{
			h1_hud_draw_static(&hud->absolute_placement, &hud->shield_meter.background, draw_flags, state->last_shield_flash_time);
		}
	}

	// the health
	{
		const int16 draw_flags = (int16)((shield_vitality <= 0.f ? FLAG(_h1_hud_draw_flashing_bit) : 0) | (dead ? FLAG(_h1_hud_draw_disabled_bit) : 0));
		if (TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit))
		{
			if (state->last_health_flash_time == NONE)
			{
				state->last_health_flash_time = time;
			}
		}
		else
		{
			state->last_health_flash_time = NONE;
		}
		if (hud->health_meter.meter.meter_bitmap.index != NONE)
		{
			const int16 value_scale = hud->health_meter.meter.value_scale ? hud->health_meter.meter.value_scale : 8;
			h1_hud_meter_element health_meter = hud->health_meter.meter;
			const uint32 mid_color = hud->health_meter.extras[0];
			real32 max_cutoff;
			real32 min_cutoff;
			csmemcpy(&max_cutoff, &hud->health_meter.extras[1], sizeof(real32));
			csmemcpy(&min_cutoff, &hud->health_meter.extras[2], sizeof(real32));
			if (body_vitality >= max_cutoff)
			{
				health_meter.min_color = health_meter.max_color;
			}
			else if (body_vitality <= min_cutoff)
			{
				health_meter.max_color = health_meter.min_color;
			}
			else
			{
				health_meter.max_color = mid_color;
				health_meter.min_color = mid_color;
			}
			const uint8 value = (uint8)PIN((int32)((real32)value_scale * body_vitality), 0, 255);
			h1_hud_draw_meter(&hud->absolute_placement, &health_meter, value, value, draw_flags, -1.f, body_vitality);
		}
		if (hud->health_meter.background.interface_bitmap.index != NONE)
		{
			h1_hud_draw_static(&hud->absolute_placement, &hud->health_meter.background, draw_flags, state->last_health_flash_time);
		}
	}
	return;
}

// hud_weapon.c render_grenade_hud: the current grenade's count panel
static void h1_hud_render_grenades(datum unit_index, const h1_weap* weapon_definition)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	// weapons.c weapon_prevents_grenade_throwing
	if (!unit || TEST_BIT(weapon_definition->flags_3, 6) || unit->unit.current_grenade_index < 0)
	{
		return;
	}
	const datum globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* globals = globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', globals_index) : NULL;
	const int32 grenade_type = unit->unit.current_grenade_index;
	if (!globals || !VALID_INDEX(grenade_type, globals->grenades.count) || !VALID_INDEX(grenade_type, (int32)k_unit_grenade_types_count))
	{
		return;
	}
	const datum hud_index = g_h1_cache_file->block_get(globals->grenades, grenade_type)->hud_interface.index;
	const h1_grhi* definition = hud_index != NONE ? (const h1_grhi*)g_h1_cache_file->tag_get('grhi', hud_index) : NULL;
	if (!definition)
	{
		return;
	}
	const int32 count = unit->unit.grenade_counts[grenade_type];
	const int16 draw_flags = (int16)((count <= definition->flash_cutoff ? FLAG(_h1_hud_draw_flashing_bit) : 0) | (count == 0 ? FLAG(_h1_hud_draw_disabled_bit) : 0));
	if (TEST_BIT(draw_flags, _h1_hud_draw_flashing_bit))
	{
		if (g_h1_unit_hud.last_grenade_flash_time == NONE)
		{
			g_h1_unit_hud.last_grenade_flash_time = h1_hud_time();
		}
	}
	else
	{
		g_h1_unit_hud.last_grenade_flash_time = NONE;
	}
	if (definition->background.interface_bitmap.index != NONE)
	{
		h1_hud_draw_static(&definition->absolute_placement, &definition->background, draw_flags, g_h1_unit_hud.last_grenade_flash_time);
	}
	if (definition->count_background.interface_bitmap.index != NONE)
	{
		h1_hud_draw_static(&definition->absolute_placement, &definition->count_background, draw_flags, g_h1_unit_hud.last_grenade_flash_time);
	}
	if (definition->count_numbers.digits)
	{
		h1_hud_draw_numbers(&definition->absolute_placement, &definition->count_numbers, (int16)count, NONE, draw_flags, g_h1_unit_hud.last_grenade_flash_time);
	}
	if (definition->overlay_bitmap.index != NONE)
	{
		// grenade overlays: flashing, empty, default, always
		int32 overlay_flags = count <= definition->flash_cutoff ? FLAG(0) : 0;
		if (count == 0)
		{
			overlay_flags |= FLAG(1);
		}
		if (!overlay_flags)
		{
			overlay_flags |= FLAG(2);
		}
		overlay_flags |= FLAG(3);
		h1_wphi_overlays overlays = {};
		overlays.bitmap = definition->overlay_bitmap;
		overlays.items = definition->overlay_items;
		h1_hud_draw_overlays(&definition->absolute_placement, &overlays, overlay_flags, g_h1_unit_hud.last_grenade_flash_time, draw_flags);
	}
	return;
}

bool h1_hud_hides_halo2_widget(string_id name)
{
	if (!h1_maps_active())
	{
		return false;
	}
	// halo 1's unit hud shows the shield, health and grenades: halo 2's go
	const char* text = string_id_get_string_const(name);
	if (!text)
	{
		return false;
	}
	return strncmp(text, "shield", 6) == 0 || strncmp(text, "frag", 4) == 0 || strncmp(text, "plasma", 6) == 0 ||
		strstr(text, "grenade") != NULL || strncmp(text, "health", 6) == 0;
}
