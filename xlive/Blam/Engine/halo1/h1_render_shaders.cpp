#include "stdafx.h"
#include "h1_render_shaders.h"

#include "h1_bitmaps.h"
#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_fog.h"
#include "h1_log.h"

#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "render/render.h"

#include <string>
#include <unordered_map>

/* constants */

enum
{
	k_h1_maximum_generic_stages = 7,
	k_h1_maximum_shader_maps = 4,
};

enum e_h1_framebuffer_blend_function
{
	_h1_framebuffer_blend_alpha_blend = 0,
	_h1_framebuffer_blend_multiply,
	_h1_framebuffer_blend_double_multiply,
	_h1_framebuffer_blend_add,
	_h1_framebuffer_blend_subtract,
	_h1_framebuffer_blend_component_min,
	_h1_framebuffer_blend_component_max,
	_h1_framebuffer_blend_alpha_multiply_add,
};

/* shader sources */

static const char k_h1_vertex_shader[] = R"(
float4x4 world_view_projection : register(c0);
float4 view_forward : register(c4);		// object space to view depth (x row of world to view)
float4 depth_scale : register(c5);		// x: 1 / (far - near), y: sky (push to the far plane)
float4 object_to_world[3] : register(c6);	// rotation rows (for normals) and translation in w
float4 camera_position : register(c9);	// object space camera position

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
	float2 lightmap_texcoord : TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float2 lightmap_texcoord : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float depth : TEXCOORD3;
	float3 view : TEXCOORD4;
	float3 world_normal : TEXCOORD5;
	float3 world : TEXCOORD6;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 position = float4(input.position, 1.0f);
	output.position = mul(position, world_view_projection);
	if (depth_scale.y > 0.5f)
		output.position.z = output.position.w * 0.99999f;
	output.texcoord = input.texcoord;
	output.lightmap_texcoord = input.lightmap_texcoord;
	output.normal = input.normal;
	output.world_normal = float3(dot(input.normal, object_to_world[0].xyz), dot(input.normal, object_to_world[1].xyz), dot(input.normal, object_to_world[2].xyz));
	output.view = camera_position.xyz - input.position;
	output.world = float3(dot(position.xyz, object_to_world[0].xyz) + object_to_world[0].w, dot(position.xyz, object_to_world[1].xyz) + object_to_world[1].w, dot(position.xyz, object_to_world[2].xyz) + object_to_world[2].w);
	output.depth = depth_scale.y > 0.5f ? 1.0f : dot(position, view_forward) * depth_scale.x;
	return output;
}
)";

#define H1_PIXEL_SHADER_COMMON R"(
struct PS_INPUT
{
	float2 texcoord : TEXCOORD0;
	float2 lightmap_texcoord : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float depth : TEXCOORD3;
	float3 view : TEXCOORD4;
	float3 world_normal : TEXCOORD5;
	float3 world : TEXCOORD6;
};

struct PS_OUTPUT
{
	float4 color : COLOR0;
	float4 depth : COLOR1;
};

// halo 1 fog (h1_fog.cpp): the vertex shader fog constants c[-88] to c[-85], then the model fog colors
float4 fog_atmospheric : register(c100);	// (world position, 1) . this: atmospheric fog distance fraction
float4 fog_planar_depth : register(c101);	// depth below the fog plane, a fraction of the planar fog's opaque depth
float4 fog_planar_distance : register(c102);	// view distance, a fraction of the planar fog's opaque distance
float4 fog_settings : register(c103);		// atmospheric maximum density, camera planar density, planar maximum density, mode (1 model, 2 transparent)
float4 fog_model_atmospheric : register(c104);	// atmospheric color times the object's fog density, w: the object's fog density
float4 fog_model_planar : register(c105);	// the color planar fog blends models to

// the vertex fog of the xbox model and transparent vertex shaders
float fog_atmospheric_density(float3 world)
{
	return saturate(dot(world, fog_atmospheric.xyz) + fog_atmospheric.w) * fog_settings.x;
}

float fog_planar_density(float3 world)
{
	float depth = dot(world, fog_planar_depth.xyz) + fog_planar_depth.w;
	float distance = dot(world, fog_planar_distance.xyz) + fog_planar_distance.w;
	float a = min(pow(max(1.0f - depth, 0.0f), 2.0f), 1.0f);
	float b = min(pow(max(1.0f - distance, 0.0f), 2.0f), 1.0f);
	float x = 1.0f - min(a + b, 1.0f);
	float y = 1.0f - b;
	x *= x;
	y *= y;
	return (x + fog_settings.y * (y - x)) * fog_settings.z;
}

// how much of a transparent surface shows through the fog
float fog_transmittance(float3 world)
{
	return fog_settings.w > 1.5f ? (1.0f - fog_atmospheric_density(world)) * (1.0f - fog_planar_density(world)) : 1.0f;
}

// the final combiner of the xbox model shaders: the object's atmospheric fog, the planar fog per pixel
float3 fog_model(float3 color, float3 world)
{
	if (fog_settings.w < 0.5f || fog_settings.w > 1.5f)
		return color;
	float planar = saturate(fog_planar_density(world));
	return color * (1.0f - fog_model_atmospheric.w) * (1.0f - planar) + planar * fog_model_planar.rgb + fog_model_atmospheric.rgb;
}

// halo 1 dynamic lights (h1_effects.cpp): per light the position and 1 / radius, then the color
float4 dynamic_lights[16] : register(c110);

// rasterizer_xbox_environment light pass: the 3d distance attenuation texture times n.l times the light color
float3 dynamic_light(float3 world, float3 normal)
{
	float3 result = float3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < 8; i++)
	{
		float3 to_light = dynamic_lights[2 * i].xyz - world;
		float distance_squared = dot(to_light, to_light);
		float r = saturate(sqrt(distance_squared) * dynamic_lights[2 * i].w);
		float attenuation = saturate((((-1.8124f * r + 5.1325f) * r - 4.4503f) * r + 0.1303f) * r + 0.999f);
		float n_dot_l = saturate(dot(normal, to_light * rsqrt(distance_squared + 1e-6f)));
		result += dynamic_lights[2 * i + 1].rgb * (attenuation * n_dot_l);
	}
	return result;
}

// halo 2 reads view depth packed as r + g / 256 + b / 65536
float4 pack_depth(float depth)
{
	float d = saturate(depth) * 255.0f;
	return float4(floor(d) / 255.0f, floor(frac(d) * 256.0f) / 255.0f, frac(frac(d) * 256.0f), 1.0f);
}

float3 apply_detail(float3 color, float3 detail, float function)
{
	if (function < 0.5f)
		return saturate(2.0f * color * detail);
	if (function < 1.5f)
		return saturate(color * detail);
	return saturate(color + 2.0f * detail - 1.0f);
}
)"

// shader_environment diffuse pass (Xbox combiners 0-2) multiplied by the lightmap
static const char k_h1_environment_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D base_map : register(s0);
sampler2D primary_detail_map : register(s1);
sampler2D secondary_detail_map : register(s2);
sampler2D micro_detail_map : register(s3);
sampler2D lightmap : register(s4);
sampler2D self_illumination_map : register(s5);

float4 detail_scales : register(c0);	// primary, secondary, micro
float4 modes : register(c1);			// type, detail function, micro detail function, alpha tested
float4 ambient : register(c2);			// lightmap missing ambient color, w: has lightmap
float4 debug_mode : register(c3);		// x: 1 flat color, 2 base map only, 3 lightmap only, 4 detail only
float4 texture_animation : register(c4);	// u and v offsets (texture scrolling), self-illumination map scale, has self-illumination
float4 primary_color : register(c5);		// the animated primary and secondary self-illumination colors
float4 secondary_color : register(c6);
float4 plasma_on_color : register(c7);		// w: the plasma animation value
float4 plasma_off_color : register(c8);

// rasterizer_xbox_environment.c lightmap pass combiners 0-5: the map's red and green light the primary and secondary colors,
// a band of its alpha around the plasma value lights the plasma color through its blue
float3 self_illumination(float2 texcoord)
{
	float4 map = tex2D(self_illumination_map, texcoord * texture_animation.z);
	float d = plasma_on_color.w - map.a;
	float a = clamp(0.5f + d, -1.0f, 1.0f);
	float b = clamp(0.5f - d, -1.0f, 1.0f);
	float q = clamp(4.0f * (a >= 0.5f ? b * b : a * a), -1.0f, 1.0f);
	float q2 = q * q;
	float e = 2.0f * q2 - 1.0f;
	float plasma = q2 >= 0.5f ? clamp(e * e, -1.0f, 1.0f) : 0.0f;
	float3 plasma_color = clamp(plasma_on_color.rgb * plasma + plasma_off_color.rgb, -1.0f, 1.0f);
	float3 colors = clamp(map.r * primary_color.rgb + map.g * secondary_color.rgb, -1.0f, 1.0f);
	return saturate(plasma_color * map.b + colors);
}

PS_OUTPUT main(PS_INPUT input)
{
	// shader_environment_texture_animation_evaluate offsets the texture coordinates of every map
	float2 texcoord = input.texcoord + texture_animation.xy;
	float4 base = tex2D(base_map, texcoord);
	float4 primary = tex2D(primary_detail_map, texcoord * detail_scales.x);
	float4 secondary = tex2D(secondary_detail_map, texcoord * detail_scales.y);
	float4 micro = tex2D(micro_detail_map, texcoord * detail_scales.z);

	// normal: secondary alpha blends the detail maps, blended types use the base map alpha
	float blend = modes.x < 0.5f ? secondary.a : base.a;
	float3 detail = lerp(secondary.rgb, primary.rgb, blend);

	float3 color = apply_detail(base.rgb, detail, modes.y);
	color = apply_detail(color, micro.rgb, modes.z);

	if (modes.w > 0.5f)
		clip(base.a - 0.5f);

	float3 light = ambient.w > 0.5f ? tex2D(lightmap, input.lightmap_texcoord).rgb : ambient.rgb;
	light += dynamic_light(input.world, normalize(input.world_normal));
	if (texture_animation.w > 0.5f)
		light += self_illumination(texcoord);

	PS_OUTPUT output;
	output.color = float4(fog_model(color * light, input.world), 1.0f);
	if (debug_mode.x > 0.5f && debug_mode.x < 1.5f) output.color = float4(1.0f, 0.0f, 1.0f, 1.0f);
	else if (debug_mode.x > 1.5f && debug_mode.x < 2.5f) output.color = float4(base.rgb, 1.0f);
	else if (debug_mode.x > 2.5f && debug_mode.x < 3.5f) output.color = float4(light, 1.0f);
	else if (debug_mode.x > 3.5f && debug_mode.x < 4.5f) output.color = float4(detail, 1.0f);
	else if (debug_mode.x > 4.5f && debug_mode.x < 5.5f) output.color = float4(micro.rgb, 1.0f);
	else if (debug_mode.x > 5.5f) output.color = float4(debug_mode.yzw, 1.0f);
	output.depth = pack_depth(input.depth);
	return output;
}
)";

// halo 1 particles (shader_effect, rasterizer_xbox_transparent_geometry.c): the sprite texture tinted by the particle color,
// then the stage appended for the framebuffer blend function faded by fog (transparent effect vertex shader) and the fade
static const char k_h1_particle_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D map0 : register(s0);
sampler2D map1 : register(s1);

float4 particle_settings : register(c0);	// framebuffer blend function, nonlinear tint, has secondary map, mode (0 particle, 1 decal, 2 lens flare)
float4 secondary_transform[2] : register(c1);

PS_OUTPUT main(PS_INPUT input)
{
	// the particle color comes in the normal, its alpha (fade) in the lightmap coordinates
	float3 tint = input.normal;
	float vertex_alpha = input.lightmap_texcoord.x;
	float4 t0 = tex2D(map0, input.texcoord);

	// rasterizer_xbox_widgets internal sprite: the texture times the color, alpha brightness, added
	if (particle_settings.w > 1.5f)
	{
		PS_OUTPUT flare;
		flare.color = saturate(float4(t0.rgb * tint, t0.a * vertex_alpha));
		flare.depth = float4(0.0f, 0.0f, 0.0f, 0.0f);
		return flare;
	}

	float3 color;
	if (particle_settings.w > 0.5f)
	{
		// rasterizer_xbox_decals: the multiplying decals blend from the neutral color to the map by the decal color
		float b = particle_settings.x;
		if ((b > 0.5f && b < 1.5f) || abs(b - 5.0f) < 0.5f)
			color = lerp(float3(1.0f, 1.0f, 1.0f), t0.rgb, tint);
		else if (b > 1.5f && b < 2.5f)
			color = lerp(float3(0.5f, 0.5f, 0.5f), t0.rgb, tint);
		else
			color = t0.rgb * tint;
	}
	else if (particle_settings.y > 0.5f)
	{
		float3 t4 = t0.rgb * t0.rgb;
		t4 *= t4;
		color = (1.0f - tint) * t4 + tint * t0.rgb;
	}
	else
	{
		color = t0.rgb * tint;
	}
	float alpha = t0.a;

	if (particle_settings.z > 0.5f)
	{
		float2 uv = input.texcoord * secondary_transform[0].xy + secondary_transform[0].zw;
		float4 t1 = tex2D(map1, uv);
		color *= t1.rgb;
		alpha *= t1.a;
	}

	float4 result = float4(color, alpha);
	float f = saturate(fog_transmittance(input.world) * vertex_alpha);
	float blend = particle_settings.x;
	if (blend < 0.5f)
		result.a = result.a * f;
	else if (blend < 1.5f || abs(blend - 5.0f) < 0.5f)
		result.rgb = result.rgb * f + (1.0f - f);
	else if (blend < 2.5f)
		result.rgb = result.rgb * f + 0.5f * (1.0f - f);
	else if (blend < 6.5f)
		result.rgb = result.rgb * f;
	else
	{
		result.rgb = result.rgb * f;
		result.a = result.a * f;
	}

	PS_OUTPUT output;
	output.color = saturate(result);
	output.depth = float4(0.0f, 0.0f, 0.0f, 0.0f);
	return output;
}
)";

// halo 1 environment fog: a pass over the opaque structure, blended one / inverse source alpha where the depth is equal
static const char k_h1_environment_fog_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D atmospheric_fog_density : register(s0);
sampler2D planar_fog_density : register(s1);

float4 eye : register(c106);				// atmospheric eye density, planar eye density, atmospheric maximum density, planar maximum density
float4 atmospheric_color : register(c107);
float4 planar_color : register(c108);

PS_OUTPUT main(PS_INPUT input)
{
	// the environment fog vertex shader's texture coordinates
	float2 t0_coord = float2(dot(input.world, fog_atmospheric.xyz) + fog_atmospheric.w, 0.0f);
	float2 t1_coord = float2(dot(input.world, fog_planar_distance.xyz) + fog_planar_distance.w, dot(input.world, fog_planar_depth.xyz) + fog_planar_depth.w);
	float4 t0 = tex2D(atmospheric_fog_density, t0_coord);
	float4 t1 = tex2D(planar_fog_density, t1_coord);

	// stage 0
	float atmospheric = eye.z * t0.a;
	float eye_atmospheric = atmospheric * eye.x;
	float planar = saturate((1.0f - eye.y) * eye.w * t1.a + eye.y * eye.w * t1.b);
	// stage 1
	float3 r0 = atmospheric_color.rgb * atmospheric;
	float3 r1 = planar_color.rgb * planar;
	float r1_alpha = (1.0f - eye.x) * planar;
	float r0_alpha = (1.0f - atmospheric) * (1.0f - planar);
	// final combiner
	PS_OUTPUT output;
	output.color = float4(saturate(r0 * (1.0f - r1_alpha) + r1 * (1.0f - eye_atmospheric)), 1.0f - r0_alpha);
	output.depth = float4(0.0f, 0.0f, 0.0f, 0.0f);
	return output;
}
)";

// shader_model: base map, detail map, multipurpose map (self illumination in green), object lighting
static const char k_h1_model_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D base_map : register(s0);
sampler2D multipurpose_map : register(s1);	// red reflection mask, green self-illumination mask, alpha change color mask
sampler2D detail_map : register(s2);
samplerCUBE reflection_map : register(s4);

float4 map_scale : register(c0);		// base u, base v, detail u, detail v
float4 modes : register(c1);			// detail function, alpha tested, detail mask, detail after reflection
float4 ambient : register(c2);
float4 light0_direction : register(c3);
float4 light0_color : register(c4);
float4 light1_direction : register(c5);
float4 light1_color : register(c6);
float4 self_illumination_color : register(c7);
float4 change_color : register(c8);
float4 perpendicular : register(c9);	// reflection tint, w: brightness (with the reflection falloff)
float4 parallel : register(c10);
float4 model_settings : register(c11);	// x: has a reflection cube map, yz: base map offset, w: translucency
float4 camera_world : register(c12);

float3 detail_combine(float3 color, float3 detail, float function)
{
	if (function < 0.5f)
		return 2.0f * color * detail;	// double/biased multiply
	if (function < 1.5f)
		return color * detail;			// multiply
	return color + 2.0f * detail - 1.0f;	// double/biased add
}

PS_OUTPUT main(PS_INPUT input)
{
	float2 texcoord = input.texcoord * map_scale.xy + model_settings.yz;
	float4 base = tex2D(base_map, texcoord);
	float4 multipurpose = tex2D(multipurpose_map, texcoord);
	float4 detail = tex2D(detail_map, input.texcoord * map_scale.zw);

	if (modes.y > 0.5f)
		clip(base.a - 0.5f);

	// combiner 3: the detail map fades to neutral outside its mask
	float mask = 1.0f;
	float detail_mask = modes.z;
	if (detail_mask > 0.5f && detail_mask < 1.5f) mask = 1.0f - multipurpose.r;
	else if (detail_mask > 1.5f && detail_mask < 2.5f) mask = multipurpose.r;
	else if (detail_mask > 2.5f) mask = fmod(detail_mask, 2.0f) > 0.5f ? 1.0f - multipurpose.a : multipurpose.a;
	float neutral = modes.x > 0.5f && modes.x < 1.5f ? 1.0f : 0.5f;
	float3 masked_detail = lerp(float3(neutral, neutral, neutral), detail.rgb, mask);

	// the vertex lighting plus the self-illumination, tinted by the change color
	float3 normal = normalize(input.world_normal);
	// model vertex shader (reflection permutation): the first light reaches the back by the translucency
	float light0 = dot(normal, -light0_direction.xyz);
	float3 light = ambient.rgb +
		max(max(light0, -light0 * model_settings.w), 0.0f) * light0_color.rgb +
		saturate(dot(normal, -light1_direction.xyz)) * light1_color.rgb +
		dynamic_light(input.world, normal);
	light = saturate(saturate(light) + multipurpose.g * self_illumination_color.rgb);
	light *= lerp(float3(1.0f, 1.0f, 1.0f), change_color.rgb, multipurpose.a);

	// the reflection: the cube map tinted between the parallel and perpendicular colors by the view angle, masked
	// model vertex shader (reflection permutation): the reflection vector 2(n.e)n - e of the eye vector, and vertex color 1 the
	// parallel color plus the cosine of the view angle times the perpendicular less the parallel, clamped as a vertex color
	float3 eye = camera_world.xyz - input.world;
	float3 view = normalize(eye);
	float facing = dot(normal, view);
	float4 tint = saturate(parallel + facing * (perpendicular - parallel));
	float3 reflection_vector = 2.0f * dot(eye, normal) * normal - eye;
	float3 reflection = model_settings.x > 0.5f ? texCUBE(reflection_map, reflection_vector).rgb * tint.rgb : float3(0.0f, 0.0f, 0.0f);
	float specular = multipurpose.r * tint.a;

	float3 color;
	if (modes.w > 0.5f)
		color = detail_combine(saturate(base.rgb * light + reflection * specular), masked_detail, modes.x);
	else
		color = saturate(detail_combine(base.rgb, masked_detail, modes.x)) * light + reflection * specular;

	PS_OUTPUT output;
	output.color = float4(fog_model(saturate(color), input.world), base.a);
	output.depth = pack_depth(input.depth);
	return output;
}
)";

// shader_transparent_meter (rasterizer_xbox_transparent_geometry.c): the map's blue against the meter value picks the meter
// (the gradient between its colors by the map's blue, plus the flash past the flash extension) or the background, times the map's
// alpha; the frame buffer keeps its color tinted (tint mode 2: by the meter's alpha) and the meter adds at its brightness
static const char k_h1_meter_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D meter_map : register(s0);

float4 flash : register(c0);				// flash color times flash brightness, w: flash extension
float4 gradient_minimum : register(c1);		// w: meter value
float4 gradient_maximum : register(c2);		// w: flash alpha
float4 background : register(c3);			// w: background alpha
float4 meter_settings : register(c4);		// x: flash color is negative, y: source scale (brightness, or 1 in tint mode 2)

PS_OUTPUT main(PS_INPUT input)
{
	float4 t0 = tex2D(meter_map, input.texcoord);
	// combiner 0
	float flash_mask = clamp(4.0f * 2.0f * gradient_maximum.w * t0.b, -1.0f, 1.0f);
	float flash_distance = clamp(4.0f * (flash.w - t0.b), -1.0f, 1.0f);
	// combiner 1
	float flash_fraction = clamp(1.0f - 2.0f * flash_distance, -1.0f, 1.0f);
	float3 color = clamp((1.0f - flash_mask) * gradient_minimum.rgb + flash_mask * gradient_maximum.rgb, -1.0f, 1.0f);
	// combiner 2
	float empty = clamp(t0.b + 0.5f - gradient_minimum.w, -1.0f, 1.0f);
	color = clamp(color + flash_fraction * (meter_settings.x > 0.5f ? -flash.rgb : flash.rgb), -1.0f, 1.0f);
	// combiner 3 (mux): past the meter value is the background
	float alpha = empty >= 0.5f ? background.w : 1.0f;
	color = empty >= 0.5f ? background.rgb : color;

	PS_OUTPUT output;
	output.color = float4(saturate(color * t0.a) * meter_settings.y, saturate(alpha));
	output.depth = float4(0.0f, 0.0f, 0.0f, 0.0f);
	return output;
}
)";

// shader_transparent_generic: the stages of each shader become straight line code (h1_generic_shader_get)
static const char k_h1_generic_pixel_shader_header[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D map0 : register(s0);
sampler2D map1 : register(s1);
sampler2D map2 : register(s2);
sampler2D map3 : register(s3);
samplerCUBE cube0 : register(s4);

float4 map_transform[8] : register(c0);
float4 stage_constants[14] : register(c8);	// constant color 0 and 1 per stage
float4 settings : register(c71);			// stage count, first map is a cube map, fade mode, fade intensity
float4 vertex_light : register(c72);		// vertex color 0 (diffuse light)
float4 blend_settings : register(c73);		// framebuffer blend function

float2 map_texcoord(float2 texcoord, int index)
{
	float4 scale_offset = map_transform[index * 2];
	float4 rotation = map_transform[index * 2 + 1];
	float2 uv = texcoord * scale_offset.xy + scale_offset.zw - rotation.zw;
	return float2(uv.x * rotation.x - uv.y * rotation.y, uv.x * rotation.y + uv.y * rotation.x) + rotation.zw;
}

PS_OUTPUT main(PS_INPUT input)
{
	float4 m0 = settings.y > 0.5f ? texCUBE(cube0, reflect(-normalize(input.view), normalize(input.normal))) : tex2D(map0, map_texcoord(input.texcoord, 0));
	float4 m1 = tex2D(map1, map_texcoord(input.texcoord, 1));
	float4 m2 = tex2D(map2, map_texcoord(input.texcoord, 2));
	float4 m3 = tex2D(map3, map_texcoord(input.texcoord, 3));
	// vertex color 1 is the fade when perpendicular in color, the fade when parallel in alpha, both faded by fog
	float facing = saturate(abs(dot(normalize(input.view), normalize(input.normal))));
	float transmittance = fog_transmittance(input.world) * settings.w;
	float4 v0 = float4(vertex_light.rgb, transmittance);
	float4 v1 = float4(facing, facing, facing, 1.0f - facing) * transmittance;
	float4 r0 = 0.0f;
	float4 r1 = 0.0f;
	float4 k0, k1;
	float3 cab, ccd, csum;
	float aab, acd, asum;
)";

static const char k_h1_generic_pixel_shader_footer[] = R"(
	// fade: vertex alpha 0 without a fade mode, else vertex color 1 (alpha when perpendicular, blue when parallel)
	float f = saturate(settings.z < 0.5f ? v0.a : (settings.z < 1.5f ? v1.a : v1.b));
	float blend = blend_settings.x;
	if (blend < 0.5f)							// alpha blend
		r0.a = r0.a * f;
	else if (blend < 1.5f || abs(blend - 5.0f) < 0.5f)	// multiply, component min: fade to white
		r0.rgb = r0.rgb * f + (1.0f - f);
	else if (blend < 2.5f)						// double multiply: fade to gray
		r0.rgb = r0.rgb * f + 0.5f * (1.0f - f);
	else if (blend < 6.5f)						// add, subtract, component max
		r0.rgb = r0.rgb * f;
	else										// alpha multiply add
	{
		r0.rgb = r0.rgb * f;
		r0.a = r0.a * f;
	}

	PS_OUTPUT output;
	output.color = saturate(r0);
	output.depth = pack_depth(input.depth);
	return output;
}
)";

// shader_transparent_chicago(_extended): map chain with color and alpha functions
static const char k_h1_chicago_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D map0 : register(s0);
sampler2D map1 : register(s1);
sampler2D map2 : register(s2);
sampler2D map3 : register(s3);
samplerCUBE cube0 : register(s4);

float4 map_transform[8] : register(c0);
float4 functions[4] : register(c8);		// per map: color function, alpha function, alpha replicate
float4 settings : register(c12);		// map count, first map is a cube map
float4 fade_settings : register(c13);	// framebuffer blend function, fade mode, fade intensity

float2 map_texcoord(float2 texcoord, int index)
{
	float4 scale_offset = map_transform[index * 2];
	float4 rotation = map_transform[index * 2 + 1];
	float2 uv = texcoord * scale_offset.xy + scale_offset.zw - rotation.zw;
	return float2(uv.x * rotation.x - uv.y * rotation.y, uv.x * rotation.y + uv.y * rotation.x) + rotation.zw;
}

float4 combine(float4 current, float4 next, float function)
{
	// the xbox combiner table: current, next map, multiply, double multiply, add, add signed current,
	// add signed next, subtract current, subtract next, blend current alpha, blend current alpha inverse,
	// blend next alpha, blend next alpha inverse
	int f = (int)function;
	if (f == 0) return current;
	if (f == 1) return next;
	if (f == 2) return current * next;
	if (f == 3) return 2.0f * current * next;
	if (f == 4) return current + next;
	if (f == 5) return next + 2.0f * current - 1.0f;
	if (f == 6) return current + 2.0f * next - 1.0f;
	if (f == 7) return next - current;
	if (f == 8) return current - next;
	if (f == 9) return lerp(current, next, current.a);
	if (f == 10) return lerp(next, current, current.a);
	if (f == 11) return lerp(current, next, next.a);
	return lerp(next, current, next.a);
}

PS_OUTPUT main(PS_INPUT input)
{
	float4 maps[4];
	if (settings.y > 0.5f)
		maps[0] = texCUBE(cube0, reflect(-normalize(input.view), normalize(input.normal)));
	else
		maps[0] = tex2D(map0, map_texcoord(input.texcoord, 0));
	maps[1] = tex2D(map1, map_texcoord(input.texcoord, 1));
	maps[2] = tex2D(map2, map_texcoord(input.texcoord, 2));
	maps[3] = tex2D(map3, map_texcoord(input.texcoord, 3));

	float4 result = maps[0];
	int map_count = (int)settings.x;
	[unroll]
	for (int j = 1; j < 4; j++)
	{
		if (j < map_count)
		{
			// alpha replicate of the previous map feeds this map's alpha into the color combine
			float4 next_color = functions[j - 1].z > 0.5f ? maps[j].aaaa : maps[j];
			float3 color = saturate(combine(result, next_color, functions[j - 1].x).rgb);
			float alpha = saturate(combine(result.aaaa, maps[j].aaaa, functions[j - 1].y).a);
			result = float4(color, alpha);
		}
	}

	// the stage halo 1 appends for the framebuffer blend function, faded by fog and the fade mode
	float facing = saturate(abs(dot(normalize(input.view), normalize(input.normal))));
	float transmittance = fog_transmittance(input.world) * fade_settings.z;
	float f = saturate(fade_settings.y < 0.5f ? transmittance : (fade_settings.y < 1.5f ? (1.0f - facing) * transmittance : facing * transmittance));
	float blend = fade_settings.x;
	if (blend < 0.5f)
		result.a = result.a * f;
	else if (blend < 1.5f || abs(blend - 5.0f) < 0.5f)
		result.rgb = result.rgb * f + (1.0f - f);
	else if (blend < 2.5f)
		result.rgb = result.rgb * f + 0.5f * (1.0f - f);
	else if (blend < 6.5f)
		result.rgb = result.rgb * f;
	else
	{
		result.rgb = result.rgb * f;
		result.a = result.a * f;
	}

	PS_OUTPUT output;
	output.color = saturate(result);
	output.depth = pack_depth(input.depth);
	return output;
}
)";


// shader_transparent_water: the background is multiplied by the base map, then the ripple bumped
// reflection is added (rasterizer_xbox_water.c). The ripple bump map Halo builds each frame is
// summed here from the four ripple layers.
static const char k_h1_water_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D base_map : register(s0);
sampler2D ripple_map : register(s1);
samplerCUBE reflection_map : register(s4);

float4 ripple_transforms[4] : register(c0);	// uv scale, uv offset per ripple
float4 ripple_weights : register(c4);		// first pair blend, second pair blend, pair blend
float4 perpendicular_tint : register(c5);	// rgb tint, a brightness
float4 parallel_tint : register(c6);
float4 settings : register(c7);				// x: pass (0 background, 1 reflection), y: base map alpha modulates reflection

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;
	output.depth = pack_depth(input.depth);
	float4 base = tex2D(base_map, input.texcoord);
	if (settings.x < 0.5f)
	{
		output.color = float4(base.rgb, 1.0f);
		return output;
	}

	float3 n0 = tex2D(ripple_map, input.texcoord * ripple_transforms[0].xy + ripple_transforms[0].zw).rgb * 2.0f - 1.0f;
	float3 n1 = tex2D(ripple_map, input.texcoord * ripple_transforms[1].xy + ripple_transforms[1].zw).rgb * 2.0f - 1.0f;
	float3 n2 = tex2D(ripple_map, input.texcoord * ripple_transforms[2].xy + ripple_transforms[2].zw).rgb * 2.0f - 1.0f;
	float3 n3 = tex2D(ripple_map, input.texcoord * ripple_transforms[3].xy + ripple_transforms[3].zw).rgb * 2.0f - 1.0f;
	float3 bump = lerp(lerp(n3, n2, ripple_weights.y), lerp(n1, n0, ripple_weights.x), ripple_weights.z);

	// tangent frame from the texture coordinate derivatives
	float3 position = -input.view;
	float3 dp1 = ddx(position), dp2 = ddy(position);
	float2 duv1 = ddx(input.texcoord), duv2 = ddy(input.texcoord);
	float3 normal = normalize(input.normal);
	float3 dp2perp = cross(dp2, normal), dp1perp = cross(normal, dp1);
	float3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 binormal = dp2perp * duv1.y + dp1perp * duv2.y;
	float frame_scale = rsqrt(max(dot(tangent, tangent), dot(binormal, binormal)) + 1e-12f);
	float3 bumped = normalize(tangent * frame_scale * bump.x + binormal * frame_scale * bump.y + normal * max(bump.z, 0.05f));

	float3 view = normalize(input.view);
	float3 reflection = texCUBE(reflection_map, reflect(-view, bumped)).rgb;
	float facing = saturate(dot(view, normal));
	float4 tint = lerp(parallel_tint, perpendicular_tint, facing);
	// final combiner: (1 - tint) * reflection^8 + tint * reflection
	float3 reflection2 = reflection * reflection;
	float3 reflection4 = reflection2 * reflection2;
	float3 color = lerp(reflection4 * reflection4, reflection, tint.rgb);
	float opacity = settings.y > 0.5f ? tint.a * base.a : 1.0f;
	output.color = float4(color * opacity, 1.0f);
	return output;
}
)";


// shader_transparent_glass (rasterizer_xbox_transparent_geometry.c): the background is multiplied by the
// tint, a cube map reflection is added and the diffuse map is alpha blended over it with the lightmap
static const char k_h1_glass_pixel_shader[] = H1_PIXEL_SHADER_COMMON R"(
sampler2D map0 : register(s0);			// tint map, bump map or diffuse map
sampler2D map1 : register(s1);			// diffuse detail map
sampler2D lightmap : register(s2);
samplerCUBE reflection_map : register(s4);

float4 tint_color : register(c0);
float4 perpendicular_color : register(c1);	// rgb color, a brightness
float4 parallel_color : register(c2);
float4 map_scales : register(c3);			// map 0 scale, map 1 scale
float4 settings : register(c4);				// x: pass (0 tint, 1 reflection, 2 diffuse), y: bumped reflection, z: has lightmap

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;
	output.depth = pack_depth(input.depth);
	float4 m0 = tex2D(map0, input.texcoord * map_scales.x);

	if (settings.x < 0.5f)
	{
		output.color = float4(tint_color.rgb * m0.rgb, 1.0f);
		return output;
	}
	if (settings.x > 1.5f)
	{
		float4 detail = tex2D(map1, input.texcoord * map_scales.y);
		float3 light = settings.z > 0.5f ? tex2D(lightmap, input.lightmap_texcoord).rgb : float3(1.0f, 1.0f, 1.0f);
		output.color = float4(saturate(2.0f * m0.rgb * detail.rgb) * light, m0.a * detail.a);
		return output;
	}

	float3 normal = normalize(input.normal);
	if (settings.y > 0.5f)
	{
		// tangent frame from the texture coordinate derivatives
		float3 position = -input.view;
		float3 dp1 = ddx(position), dp2 = ddy(position);
		float2 duv1 = ddx(input.texcoord), duv2 = ddy(input.texcoord);
		float3 dp2perp = cross(dp2, normal), dp1perp = cross(normal, dp1);
		float3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
		float3 binormal = dp2perp * duv1.y + dp1perp * duv2.y;
		float frame_scale = rsqrt(max(dot(tangent, tangent), dot(binormal, binormal)) + 1e-12f);
		float3 bump = m0.rgb * 2.0f - 1.0f;
		normal = normalize(tangent * frame_scale * bump.x + binormal * frame_scale * bump.y + normal * max(bump.z, 0.05f));
	}

	float3 view = normalize(input.view);
	float3 reflection = texCUBE(reflection_map, reflect(-view, normal)).rgb;
	float facing = saturate(dot(view, normal));
	float4 color = lerp(parallel_color, perpendicular_color, facing * facing);
	float3 reflection2 = reflection * reflection;
	float3 reflection4 = reflection2 * reflection2;
	output.color = float4(lerp(reflection4 * reflection4, reflection, color.rgb) * color.a, 1.0f);
	return output;
}
)";

/* globals */

int32 g_h1_render_debug_mode = 0;
bool g_h1_render_debug_camera = false;
// rasterizer.c rasterizer_global_defaults: the first person weapon's clip distances
static const real32 k_h1_first_person_near_clip_distance = 0.01171875f;
static const real32 k_h1_first_person_far_clip_distance = 1024.f;
static bool g_h1_render_first_person_projection = false;
real_point3d g_h1_render_debug_camera_position = {};
real32 g_h1_render_debug_camera_yaw = 0.f;
real32 g_h1_render_debug_camera_pitch = 0.f;

static IDirect3DVertexDeclaration9* g_h1_vertex_declaration = NULL;
static IDirect3DVertexShader9* g_h1_vertex_shader = NULL;
static IDirect3DPixelShader9* g_h1_environment_shader = NULL;
static IDirect3DPixelShader9* g_h1_model_shader = NULL;
static std::unordered_map<datum, IDirect3DPixelShader9*> g_h1_generic_shaders;
static IDirect3DPixelShader9* g_h1_chicago_shader = NULL;
static IDirect3DPixelShader9* g_h1_water_shader = NULL;
static IDirect3DPixelShader9* g_h1_meter_shader = NULL;
static IDirect3DPixelShader9* g_h1_glass_shader = NULL;
static IDirect3DPixelShader9* g_h1_environment_fog_shader = NULL;
static IDirect3DPixelShader9* g_h1_particle_shader = NULL;
static IDirect3DTexture9* g_h1_default_textures[4] = {};
static bool g_h1_fog_context_fogged = false;
static bool g_h1_fog_context_has_centroid = false;
static real_point3d g_h1_fog_context_centroid = {};
static const real32* g_h1_object_function_values = NULL;
static int16 g_h1_shader_permutation_index = 0;
static const real_rgb_color* g_h1_object_change_colors = NULL;

/* prototypes */

static IDirect3DPixelShader9* h1_compile_pixel_shader(const char* source, const char* name);
static IDirect3DTexture9* h1_solid_texture(uint32 color);
static real32 h1_periodic_function(int16 function, real32 x);
static void h1_bind_framebuffer_blend(int16 function);
static IDirect3DPixelShader9* h1_generic_shader_get(datum shader_index, const h1_sotr* shader);

/* public code */

bool h1_render_shaders_initialize(void)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();

	const D3DVERTEXELEMENT9 elements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};
	device->CreateVertexDeclaration(elements, &g_h1_vertex_declaration);

	LPD3DXBUFFER code = NULL;
	LPD3DXBUFFER errors = NULL;
	if (SUCCEEDED(D3DXCompileShader(k_h1_vertex_shader, (UINT)strlen(k_h1_vertex_shader), NULL, NULL, "main", "vs_3_0", 0, &code, &errors, NULL)))
	{
		device->CreateVertexShader((const DWORD*)code->GetBufferPointer(), &g_h1_vertex_shader);
		code->Release();
	}
	else if (errors)
	{
		h1_log("shaders: vertex shader: %s", (const char*)errors->GetBufferPointer());
	}
	if (errors) errors->Release();

	g_h1_environment_shader = h1_compile_pixel_shader(k_h1_environment_pixel_shader, "environment");
	g_h1_model_shader = h1_compile_pixel_shader(k_h1_model_pixel_shader, "model");
	g_h1_chicago_shader = h1_compile_pixel_shader(k_h1_chicago_pixel_shader, "transparent chicago");
	g_h1_water_shader = h1_compile_pixel_shader(k_h1_water_pixel_shader, "transparent water");
	g_h1_meter_shader = h1_compile_pixel_shader(k_h1_meter_pixel_shader, "transparent meter");
	g_h1_glass_shader = h1_compile_pixel_shader(k_h1_glass_pixel_shader, "transparent glass");
	g_h1_environment_fog_shader = h1_compile_pixel_shader(k_h1_environment_fog_pixel_shader, "environment fog");
	g_h1_particle_shader = h1_compile_pixel_shader(k_h1_particle_pixel_shader, "particle");

	g_h1_default_textures[0] = h1_solid_texture(0xFFFFFFFF);
	g_h1_default_textures[1] = h1_solid_texture(0xFF808080);
	g_h1_default_textures[2] = h1_solid_texture(0xFF000000);
	g_h1_default_textures[3] = h1_solid_texture(0xFF8080FF);

	return g_h1_vertex_declaration && g_h1_vertex_shader && g_h1_environment_shader && g_h1_model_shader && g_h1_chicago_shader && g_h1_water_shader && g_h1_glass_shader;
}

void h1_render_shaders_dispose(void)
{
	IUnknown* resources[] =
	{
		g_h1_vertex_declaration, g_h1_vertex_shader, g_h1_environment_shader, g_h1_model_shader, g_h1_chicago_shader, g_h1_water_shader, g_h1_glass_shader, g_h1_meter_shader,
		g_h1_default_textures[0], g_h1_default_textures[1], g_h1_default_textures[2], g_h1_default_textures[3],
	};
	for (int32 i = 0; i < NUMBEROF(resources); i++)
	{
		if (resources[i])
		{
			resources[i]->Release();
		}
	}
	g_h1_vertex_declaration = NULL;
	g_h1_vertex_shader = NULL;
	g_h1_environment_shader = NULL;
	g_h1_model_shader = NULL;
	for (auto& entry : g_h1_generic_shaders)
	{
		if (entry.second)
		{
			entry.second->Release();
		}
	}
	g_h1_generic_shaders.clear();
	g_h1_chicago_shader = NULL;
	g_h1_water_shader = NULL;
	g_h1_glass_shader = NULL;
	g_h1_meter_shader = NULL;
	csmemset(g_h1_default_textures, 0, sizeof(g_h1_default_textures));
	return;
}

IDirect3DTexture9* h1_render_default_texture(int32 index)
{
	return g_h1_default_textures[index];
}

IDirect3DVertexDeclaration9* h1_render_vertex_declaration(void)
{
	return g_h1_vertex_declaration;
}

IDirect3DVertexShader9* h1_render_vertex_shader(void)
{
	return g_h1_vertex_shader;
}

void h1_render_set_first_person_projection(bool enabled)
{
	g_h1_render_first_person_projection = enabled;
	return;
}

void h1_render_set_camera_constants(const real_matrix4x3* object_to_world, bool sky)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const s_frame* frame = global_window_parameters_get();
	const real_matrix4x3* view = &frame->projection.world_to_view;
	real32 projection[4][4];
	csmemcpy(projection, frame->projection.projection_matrix.matrix, sizeof(projection));
	if (g_h1_render_first_person_projection)
	{
		// render_camera_hack_frustum_z with direct3d's zero to one depth of a view looking down -z
		const real32 z_near = k_h1_first_person_near_clip_distance;
		const real32 z_far = k_h1_first_person_far_clip_distance;
		projection[0][2] = 0.f;
		projection[1][2] = 0.f;
		projection[2][2] = z_far / (z_near - z_far);
		projection[3][2] = z_near * z_far / (z_near - z_far);
	}

	// halo 4x3 matrices transform row vectors: rows are forward, left, up (scaled) and position
	real32 world_to_view[4][4] =
	{
		{ view->n[0][0] * view->scale, view->n[0][1] * view->scale, view->n[0][2] * view->scale, 0.f },
		{ view->n[1][0] * view->scale, view->n[1][1] * view->scale, view->n[1][2] * view->scale, 0.f },
		{ view->n[2][0] * view->scale, view->n[2][1] * view->scale, view->n[2][2] * view->scale, 0.f },
		{ view->n[3][0], view->n[3][1], view->n[3][2], 1.f },
	};
	real_point3d camera_point = frame->camera.point;
	if (g_h1_render_debug_camera)
	{
		// development: fixed camera, halo 2 view space is x right, y up, z backward
		const real32 cy = cosf(g_h1_render_debug_camera_yaw), sy = sinf(g_h1_render_debug_camera_yaw);
		const real32 cp = cosf(g_h1_render_debug_camera_pitch), sp = sinf(g_h1_render_debug_camera_pitch);
		const real32 forward[3] = { cy * cp, sy * cp, sp };
		const real32 left[3] = { -sy, cy, 0.f };
		const real32 up[3] = { -cy * sp, -sy * sp, cp };
		const real32 eye[3] = { g_h1_render_debug_camera_position.x, g_h1_render_debug_camera_position.y, g_h1_render_debug_camera_position.z };
		for (int32 i = 0; i < 3; i++)
		{
			world_to_view[i][0] = -left[i];
			world_to_view[i][1] = up[i];
			world_to_view[i][2] = -forward[i];
			world_to_view[i][3] = 0.f;
		}
		world_to_view[3][0] = (eye[0] * left[0] + eye[1] * left[1] + eye[2] * left[2]);
		world_to_view[3][1] = -(eye[0] * up[0] + eye[1] * up[1] + eye[2] * up[2]);
		world_to_view[3][2] = (eye[0] * forward[0] + eye[1] * forward[1] + eye[2] * forward[2]);
		world_to_view[3][3] = 1.f;
		camera_point = g_h1_render_debug_camera_position;
	}
	if (sky)
	{
		// the sky is centered on the camera
		world_to_view[3][0] = 0.f;
		world_to_view[3][1] = 0.f;
		world_to_view[3][2] = 0.f;
	}

	real32 object_to_world_4x4[4][4] =
	{
		{ 1.f, 0.f, 0.f, 0.f },
		{ 0.f, 1.f, 0.f, 0.f },
		{ 0.f, 0.f, 1.f, 0.f },
		{ 0.f, 0.f, 0.f, 1.f },
	};
	if (object_to_world)
	{
		for (int32 r = 0; r < 3; r++)
		{
			for (int32 c = 0; c < 3; c++)
			{
				object_to_world_4x4[r][c] = object_to_world->n[r][c] * object_to_world->scale;
			}
			object_to_world_4x4[3][r] = object_to_world->n[3][r];
		}
	}

	real32 object_to_view[4][4];
	for (int32 r = 0; r < 4; r++)
	{
		for (int32 c = 0; c < 4; c++)
		{
			object_to_view[r][c] =
				object_to_world_4x4[r][0] * world_to_view[0][c] +
				object_to_world_4x4[r][1] * world_to_view[1][c] +
				object_to_world_4x4[r][2] * world_to_view[2][c] +
				object_to_world_4x4[r][3] * world_to_view[3][c];
		}
	}

	real32 transposed[4][4];
	for (int32 r = 0; r < 4; r++)
	{
		for (int32 c = 0; c < 4; c++)
		{
			transposed[c][r] =
				object_to_view[r][0] * projection[0][c] +
				object_to_view[r][1] * projection[1][c] +
				object_to_view[r][2] * projection[2][c] +
				object_to_view[r][3] * projection[3][c];
		}
	}
	device->SetVertexShaderConstantF(0, &transposed[0][0], 4);

	// halo 2 view space looks down -z
	const real32 view_forward[4] = { -object_to_view[0][2], -object_to_view[1][2], -object_to_view[2][2], -object_to_view[3][2] };
	device->SetVertexShaderConstantF(4, view_forward, 1);

	const real32 depth_range = frame->camera.z_far - frame->camera.z_near;
	const real32 depth_scale[4] = { depth_range > 0.f ? 1.f / depth_range : 0.f, sky ? 1.f : 0.f, 0.f, 0.f };
	device->SetVertexShaderConstantF(5, depth_scale, 1);

	// object rotation (columns of the 3x3 as rows for transforming normals) and camera in object space
	real32 rotation[3][4];
	for (int32 r = 0; r < 3; r++)
	{
		for (int32 c = 0; c < 3; c++)
		{
			rotation[r][c] = object_to_world_4x4[c][r];
		}
		rotation[r][3] = object_to_world_4x4[3][r];
	}
	device->SetVertexShaderConstantF(6, &rotation[0][0], 3);

	real_point3d camera = camera_point;
	real32 camera_object[4] = { camera.x, camera.y, camera.z, 1.f };
	if (sky)
	{
		camera_object[0] = camera_object[1] = camera_object[2] = 0.f;
	}
	else if (object_to_world)
	{
		const real32 dx = camera.x - object_to_world->n[3][0];
		const real32 dy = camera.y - object_to_world->n[3][1];
		const real32 dz = camera.z - object_to_world->n[3][2];
		const real32 inverse_scale = object_to_world->scale != 0.f ? 1.f / object_to_world->scale : 1.f;
		camera_object[0] = (dx * object_to_world->n[0][0] + dy * object_to_world->n[0][1] + dz * object_to_world->n[0][2]) * inverse_scale;
		camera_object[1] = (dx * object_to_world->n[1][0] + dy * object_to_world->n[1][1] + dz * object_to_world->n[1][2]) * inverse_scale;
		camera_object[2] = (dx * object_to_world->n[2][0] + dy * object_to_world->n[2][1] + dz * object_to_world->n[2][2]) * inverse_scale;
	}
	device->SetVertexShaderConstantF(9, camera_object, 1);
	return;
}

e_h1_render_pass h1_render_shader_pass(uint32 shader_group)
{
	switch (shader_group)
	{
	case 'senv':
	case 'soso':
		return _h1_render_pass_opaque;
	default:
		return _h1_render_pass_transparent;
	}
}

static IDirect3DBaseTexture9* h1_texture_or_default(const h1_tag_reference& reference, int32 default_index)
{
	IDirect3DBaseTexture9* texture = h1_bitmap_texture_get(reference);
	return texture ? texture : g_h1_default_textures[default_index];
}

static void h1_set_sampler_addressing(DWORD stage, bool clamp_u, bool clamp_v, bool unfiltered)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetSamplerState(stage, D3DSAMP_ADDRESSU, clamp_u ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
	device->SetSamplerState(stage, D3DSAMP_ADDRESSV, clamp_v ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
	device->SetSamplerState(stage, D3DSAMP_MAGFILTER, unfiltered ? D3DTEXF_POINT : D3DTEXF_LINEAR);
	device->SetSamplerState(stage, D3DSAMP_MINFILTER, unfiltered ? D3DTEXF_POINT : D3DTEXF_ANISOTROPIC);
	return;
}

// map transform: (u scale, v scale, u offset, v offset), (cos, sin, rotation center u, v)
static void h1_map_transform(real32* out, real32 u_scale, real32 v_scale, real32 u_offset, real32 v_offset, real32 rotation_degrees,
	int16 u_function, real32 u_period, real32 u_phase, real32 u_animation_scale,
	int16 v_function, real32 v_period, real32 v_phase, real32 v_animation_scale,
	int16 rotation_function, real32 rotation_period, real32 rotation_phase, real32 rotation_scale, real_point2d rotation_center,
	real32 game_time)
{
	real32 u = u_offset;
	real32 v = v_offset;
	real32 rotation = rotation_degrees;
	if (u_period != 0.f) u += h1_periodic_function(u_function, game_time / u_period + u_phase) * u_animation_scale;
	if (v_period != 0.f) v += h1_periodic_function(v_function, game_time / v_period + v_phase) * v_animation_scale;
	if (rotation_period != 0.f) rotation += h1_periodic_function(rotation_function, game_time / rotation_period + rotation_phase) * rotation_scale;

	out[0] = u_scale != 0.f ? u_scale : 1.f;
	out[1] = v_scale != 0.f ? v_scale : 1.f;
	out[2] = u;
	out[3] = v;
	const real32 radians = DEGREES_TO_RADIANS(rotation);
	out[4] = cosf(radians);
	out[5] = sinf(radians);
	out[6] = rotation_center.x;
	out[7] = rotation_center.y;
	return;
}

int32 h1_render_shader_subpass_count(uint32 shader_group)
{
	switch (shader_group)
	{
	case 'swat': return 2;
	case 'sgla': return 3;
	default: return 1;
	}
}

void h1_render_shader_fog_context_set(bool fogged, const real_point3d* centroid)
{
	g_h1_fog_context_fogged = fogged;
	g_h1_fog_context_has_centroid = centroid != NULL;
	g_h1_fog_context_centroid = centroid ? *centroid : real_point3d{};
	return;
}

bool h1_render_particle_shader_bind(int16 framebuffer_blend_function, bool nonlinear_tint, uint16 primary_map_flags, IDirect3DBaseTexture9* texture,
	IDirect3DBaseTexture9* secondary_texture, uint16 secondary_map_flags, e_h1_particle_shader_mode mode)
{
	if (!g_h1_particle_shader)
	{
		return false;
	}
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetPixelShader(g_h1_particle_shader);
	device->SetTexture(0, texture ? texture : g_h1_default_textures[0]);
	// map flags: point sampled, u clamped, v clamped
	h1_set_sampler_addressing(0, TEST_BIT(primary_map_flags, 1), TEST_BIT(primary_map_flags, 2), TEST_BIT(primary_map_flags, 0));
	if (secondary_texture)
	{
		device->SetTexture(1, secondary_texture);
		h1_set_sampler_addressing(1, TEST_BIT(secondary_map_flags, 1), TEST_BIT(secondary_map_flags, 2), TEST_BIT(secondary_map_flags, 0));
	}
	const real32 settings[4] = { (real32)framebuffer_blend_function, nonlinear_tint ? 1.f : 0.f, secondary_texture ? 1.f : 0.f, (real32)mode };
	device->SetPixelShaderConstantF(0, settings, 1);
	const real32 secondary_transform[2][4] = { { 1.f, 1.f, 0.f, 0.f }, { 1.f, 0.f, 0.f, 0.f } };
	device->SetPixelShaderConstantF(1, &secondary_transform[0][0], 2);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	h1_bind_framebuffer_blend(framebuffer_blend_function);
	if (mode == _h1_particle_shader_mode_lens_flare)
	{
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	}
	device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	h1_fog_set_shader_constants(h1_fog_active() ? _h1_fog_shader_mode_transparent : _h1_fog_shader_mode_none, NULL);
	return true;
}

bool h1_render_environment_fog_bind(void)
{
	if (!g_h1_environment_fog_shader)
	{
		return false;
	}
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetPixelShader(g_h1_environment_fog_shader);
	for (DWORD stage = 0; stage < 2; stage++)
	{
		IDirect3DBaseTexture9* texture = h1_fog_density_texture(stage == 1);
		device->SetTexture(stage, texture ? texture : g_h1_default_textures[2]);
		device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	}
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	h1_fog_set_shader_constants(_h1_fog_shader_mode_none, NULL);
	h1_fog_set_environment_fog_constants();
	return true;
}

void h1_render_shader_permutation_set(int16 permutation_index)
{
	g_h1_shader_permutation_index = permutation_index;
	return;
}

void h1_render_shader_object_animation_set(const real32* function_values, const real_rgb_color* change_colors)
{
	g_h1_object_function_values = function_values;
	g_h1_object_change_colors = change_colors;
	return;
}

bool h1_render_shader_bind(uint32 shader_group, datum shader_index, const s_h1_render_lighting* lighting, IDirect3DBaseTexture9* lightmap, real32 game_time, int32 subpass)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	void* definition = g_h1_cache_file->tag_get(shader_group, shader_index);
	if (!definition)
	{
		return false;
	}

	// halo 1 fogs opaque structure with the environment fog pass, opaque models in their shader, transparents through their fade
	e_h1_fog_shader_mode fog_mode = _h1_fog_shader_mode_none;
	if (g_h1_fog_context_fogged && h1_fog_active())
	{
		if (h1_render_shader_pass(shader_group) == _h1_render_pass_transparent)
		{
			fog_mode = _h1_fog_shader_mode_transparent;
		}
		else if (g_h1_fog_context_has_centroid)
		{
			fog_mode = _h1_fog_shader_mode_model;
		}
	}
	h1_fog_set_shader_constants(fog_mode, g_h1_fog_context_has_centroid ? &g_h1_fog_context_centroid : NULL);
	h1_effects_set_light_constants();

	switch (shader_group)
	{
	case 'senv':
	{
		const h1_senv* shader = (const h1_senv*)definition;
		// missing detail maps are neutral for their combine function
		const int32 detail_default = shader->detail_map_function == 1 ? 0 : 1;
		const int32 micro_default = shader->micro_detail_map_function == 1 ? 0 : 1;

		device->SetPixelShader(g_h1_environment_shader);
		device->SetTexture(0, h1_texture_or_default(shader->base_map, 0));
		device->SetTexture(1, h1_texture_or_default(shader->primary_detail_map, detail_default));
		device->SetTexture(2, h1_texture_or_default(shader->secondary_detail_map, detail_default));
		device->SetTexture(3, h1_texture_or_default(shader->micro_detail_map, micro_default));
		device->SetTexture(4, lightmap ? lightmap : g_h1_default_textures[0]);
		for (DWORD stage = 0; stage < 4; stage++)
		{
			h1_set_sampler_addressing(stage, false, false, false);
		}
		h1_set_sampler_addressing(4, true, true, false);

		const real32 detail_scales[4] =
		{
			shader->primary_detail_map_scale != 0.f ? shader->primary_detail_map_scale : 1.f,
			shader->secondary_detail_map_scale != 0.f ? shader->secondary_detail_map_scale : 1.f,
			shader->micro_detail_map_scale != 0.f ? shader->micro_detail_map_scale : 1.f,
			0.f
		};
		const real32 modes[4] =
		{
			(real32)shader->type,
			(real32)shader->detail_map_function,
			(real32)shader->micro_detail_map_function,
			TEST_BIT(shader->flags_3, 0) ? 1.f : 0.f	// alpha tested
		};
		real32 ambient[4] = { 1.f, 1.f, 1.f, lightmap ? 1.f : 0.f };
		if (!lightmap && lighting)
		{
			ambient[0] = lighting->ambient.red + lighting->light0_color.red;
			ambient[1] = lighting->ambient.green + lighting->light0_color.green;
			ambient[2] = lighting->ambient.blue + lighting->light0_color.blue;
		}
		device->SetPixelShaderConstantF(0, detail_scales, 1);
		device->SetPixelShaderConstantF(1, modes, 1);
		device->SetPixelShaderConstantF(2, ambient, 1);
		const real32 debug_mode[4] = { (real32)g_h1_render_debug_mode, 0.f, 0.f, 0.f };
		device->SetPixelShaderConstantF(3, debug_mode, 1);

		// shaders.c shader_environment_texture_animation_evaluate
		const real32 u_offset = shader->u_animation_period != 0.f ? h1_periodic_function(shader->u_animation_function, game_time / shader->u_animation_period) * shader->u_animation_scale : 0.f;
		const real32 v_offset = shader->v_animation_period != 0.f ? h1_periodic_function(shader->v_animation_function, game_time / shader->v_animation_period) * shader->v_animation_scale : 0.f;

		// the self-illumination colors (on and off colors blended by their animation functions)
		IDirect3DBaseTexture9* self_illumination_map = h1_bitmap_texture_get(shader->map);
		auto animation = [game_time](int16 function, real32 period, real32 phase) -> real32
		{
			return period != 0.f ? h1_periodic_function(function, (game_time + phase) / period) : 0.f;
		};
		const real32 primary_value = animation(shader->primary_animation_function, shader->primary_animation_period, shader->primary_animation_phase);
		const real32 secondary_value = animation(shader->secondary_animation_function, shader->secondary_animation_period, shader->secondary_animation_phase);
		const real32 plasma_value = animation(shader->plasma_animation_function, shader->plasma_animation_period, shader->plasma_animation_phase);
		const real32 self_illumination_constants[5][4] =
		{
			{ u_offset, v_offset, shader->map_scale != 0.f ? shader->map_scale : 1.f, self_illumination_map ? 1.f : 0.f },
			{
				shader->primary_off_color.red * (1.f - primary_value) + shader->primary_on_color.red * primary_value,
				shader->primary_off_color.green * (1.f - primary_value) + shader->primary_on_color.green * primary_value,
				shader->primary_off_color.blue * (1.f - primary_value) + shader->primary_on_color.blue * primary_value,
				0.f
			},
			{
				shader->secondary_off_color.red * (1.f - secondary_value) + shader->secondary_on_color.red * secondary_value,
				shader->secondary_off_color.green * (1.f - secondary_value) + shader->secondary_on_color.green * secondary_value,
				shader->secondary_off_color.blue * (1.f - secondary_value) + shader->secondary_on_color.blue * secondary_value,
				0.f
			},
			{ shader->plasma_on_color.red, shader->plasma_on_color.green, shader->plasma_on_color.blue, plasma_value },
			{ shader->plasma_off_color.red, shader->plasma_off_color.green, shader->plasma_off_color.blue, 0.f },
		};
		device->SetPixelShaderConstantF(4, &self_illumination_constants[0][0], 5);
		device->SetTexture(5, self_illumination_map ? self_illumination_map : g_h1_default_textures[2]);
		// self-illumination flags: unfiltered
		h1_set_sampler_addressing(5, false, false, TEST_BIT(shader->flags_5, 0));
		return true;
	}
	case 'soso':
	{
		const h1_soso* shader = (const h1_soso*)definition;
		device->SetPixelShader(g_h1_model_shader);
		device->SetTexture(0, h1_texture_or_default(shader->base_map, 0));
		// missing maps are the default 2d bitmaps of their usage: multipurpose white, detail gray, reflection black
		IDirect3DBaseTexture9* multipurpose = h1_bitmap_texture_get(shader->multipurpose_map);
		device->SetTexture(1, multipurpose ? multipurpose : g_h1_default_textures[0]);
		device->SetTexture(2, h1_texture_or_default(shader->detail_map, 1));
		IDirect3DBaseTexture9* reflection = h1_bitmap_texture_get(shader->reflection_cube_map);
		device->SetTexture(4, reflection);
		device->SetSamplerState(4, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(4, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		device->SetSamplerState(4, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
		device->SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(4, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(4, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		for (DWORD stage = 0; stage < 3; stage++)
		{
			h1_set_sampler_addressing(stage, false, false, false);
		}

		const real32 detail_scale = shader->detail_map_scale != 0.f ? shader->detail_map_scale : 1.f;
		const real32 map_scale[4] =
		{
			shader->map_u_scale != 0.f ? shader->map_u_scale : 1.f,
			shader->map_v_scale != 0.f ? shader->map_v_scale : 1.f,
			detail_scale,
			detail_scale * (shader->detail_map_v_scale != 0.f ? shader->detail_map_v_scale : 1.f),
		};
		// flags: detail after reflection, two sided, not alpha tested, alpha blended decal, true atmospheric fog, disable two sided culling
		const bool two_sided = TEST_BIT(shader->flags_3, 1);
		const real32 modes[4] =
		{
			(real32)shader->detail_function,
			TEST_BIT(shader->flags_3, 2) ? 0.f : 1.f,
			(real32)shader->detail_mask,
			TEST_BIT(shader->flags_3, 0) ? 1.f : 0.f,	// detail after reflection
		};
		s_h1_render_lighting default_lighting = { { 0.4f, 0.4f, 0.4f }, { -0.577f, -0.577f, -0.577f }, { 0.8f, 0.8f, 0.8f }, { 0.f, 0.f, 1.f }, { 0.2f, 0.2f, 0.25f }, { 0.5f, 1.f, 1.f, 1.f } };
		const s_h1_render_lighting* light = lighting ? lighting : &default_lighting;
		const real32 constants[5][4] =
		{
			{ light->ambient.red, light->ambient.green, light->ambient.blue, 1.f },
			{ light->light0_direction.i, light->light0_direction.j, light->light0_direction.k, 0.f },
			{ light->light0_color.red, light->light0_color.green, light->light0_color.blue, 1.f },
			{ light->light1_direction.i, light->light1_direction.j, light->light1_direction.k, 0.f },
			{ light->light1_color.red, light->light1_color.green, light->light1_color.blue, 1.f },
		};
		device->SetPixelShaderConstantF(0, map_scale, 1);
		device->SetPixelShaderConstantF(1, modes, 1);
		device->SetPixelShaderConstantF(2, &constants[0][0], 5);

		// self-illumination: the animation color (no random phase), scaled by its color source
		const real32 animation = shader->animation_period != 0.f ? h1_periodic_function(shader->animation_function, game_time / shader->animation_period) : 0.f;
		real_rgb_color self_illumination =
		{
			shader->animation_color_lower_bound.red + (shader->animation_color_upper_bound.red - shader->animation_color_lower_bound.red) * animation,
			shader->animation_color_lower_bound.green + (shader->animation_color_upper_bound.green - shader->animation_color_lower_bound.green) * animation,
			shader->animation_color_lower_bound.blue + (shader->animation_color_upper_bound.blue - shader->animation_color_lower_bound.blue) * animation,
		};
		if (shader->color_source > 0 && shader->color_source <= 4 && g_h1_object_change_colors)
		{
			const real_rgb_color* external = &g_h1_object_change_colors[shader->color_source - 1];
			self_illumination.red *= external->red;
			self_illumination.green *= external->green;
			self_illumination.blue *= external->blue;
		}
		const real_rgb_color change = shader->change_color_source > 0 && shader->change_color_source <= 4 && g_h1_object_change_colors ?
			g_h1_object_change_colors[shader->change_color_source - 1] : real_rgb_color{ 1.f, 1.f, 1.f };

		// the reflection fades out past its cutoff distance from the camera
		real32 reflection_fraction = 1.f;
		if (shader->reflection_cutoff_distance != 0.f && g_h1_fog_context_has_centroid)
		{
			const s_frame* frame = global_window_parameters_get();
			const real_vector3d to_model = { g_h1_fog_context_centroid.x - frame->camera.point.x, g_h1_fog_context_centroid.y - frame->camera.point.y, g_h1_fog_context_centroid.z - frame->camera.point.z };
			const real32 distance = to_model.i * frame->camera.forward.i + to_model.j * frame->camera.forward.j + to_model.k * frame->camera.forward.k;
			const real32 range = shader->reflection_falloff_distance - shader->reflection_cutoff_distance;
			reflection_fraction = range != 0.f ? PIN((distance - shader->reflection_cutoff_distance) / range, 0.f, 1.f) : 1.f;
		}
		const real_point3d camera_point = global_window_parameters_get()->camera.point;
		const real_argb_color tint = light->reflection_tint;
		const real32 model_constants[6][4] =
		{
			{ self_illumination.red, self_illumination.green, self_illumination.blue, 0.f },
			{ change.red, change.green, change.blue, 0.f },
			// rasterizer_xbox_models.c: the tints and brightnesses times the lighting's reflection tint
			{ shader->perpendicular_tint_color.red * tint.red, shader->perpendicular_tint_color.green * tint.green,
				shader->perpendicular_tint_color.blue * tint.blue, shader->perpendicular_brightness * reflection_fraction * tint.alpha },
			{ shader->parallel_tint_color.red * tint.red, shader->parallel_tint_color.green * tint.green,
				shader->parallel_tint_color.blue * tint.blue, shader->parallel_brightness * reflection_fraction * tint.alpha },
			{ reflection ? 1.f : 0.f, 0.f, 0.f, shader->translucency },
			{ camera_point.x, camera_point.y, camera_point.z, 1.f },
		};
		device->SetPixelShaderConstantF(7, &model_constants[0][0], 6);
		device->SetRenderState(D3DRS_CULLMODE, two_sided ? D3DCULL_NONE : D3DCULL_CCW);
		return true;
	}
	case 'sotr':
	{
		const h1_sotr* shader = (const h1_sotr*)definition;
		IDirect3DPixelShader9* pixel_shader = h1_generic_shader_get(shader_index, shader);
		if (!pixel_shader)
		{
			return false;
		}
		device->SetPixelShader(pixel_shader);

		real32 transforms[8][4] = {};
		for (int32 i = 0; i < k_h1_maximum_shader_maps; i++)
		{
			const h1_sotr_maps* map = g_h1_cache_file->block_get(shader->maps, i);
			if (!map)
			{
				device->SetTexture(i, g_h1_default_textures[0]);
				transforms[i * 2][0] = transforms[i * 2][1] = 1.f;
				transforms[i * 2 + 1][0] = 1.f;
				continue;
			}

			const bool cube = i == 0 && shader->first_map_type != 0;
			IDirect3DBaseTexture9* texture = h1_bitmap_texture_get(map->map);
			device->SetTexture(cube ? 4 : i, texture ? texture : (cube ? NULL : g_h1_default_textures[0]));
			if (cube)
			{
				device->SetTexture(0, g_h1_default_textures[0]);
			}
			// map flags: unfiltered, u clamped, v clamped
			h1_set_sampler_addressing(cube ? 4 : i, TEST_BIT(map->flags, 1), TEST_BIT(map->flags, 2), TEST_BIT(map->flags, 0));
			h1_map_transform(&transforms[i * 2][0], map->map_u_scale, map->map_v_scale, map->map_u_offset, map->map_v_offset, map->map_rotation,
				map->u_animation_function, map->u_animation_period, map->u_animation_phase, map->u_animation_scale,
				map->v_animation_function, map->v_animation_period, map->v_animation_phase, map->v_animation_scale,
				map->rotation_animation_function, map->rotation_animation_period, map->rotation_animation_phase, map->rotation_animation_scale, map->rotation_animation_center,
				game_time);
		}
		device->SetPixelShaderConstantF(0, &transforms[0][0], 8);

		const int32 stage_count = MIN(shader->stages.count, (int32)k_h1_maximum_generic_stages);
		real32 stage_constants[k_h1_maximum_generic_stages * 2][4] = {};
		for (int32 i = 0; i < stage_count; i++)
		{
			const h1_sotr_stages* stage = g_h1_cache_file->block_get(shader->stages, i);

			// constant color 0 animates between its bounds, by the object's a out when the stage says so
			real32 t = 0.f;
			if (g_h1_object_function_values && TEST_BIT(stage->flags, 2))
			{
				t = g_h1_object_function_values[0];
			}
			else if (stage->color0_animation_period != 0.f)
			{
				t = h1_periodic_function(stage->color0_animation_function, game_time / stage->color0_animation_period);
			}
			const real_argb_color* lower = &stage->color0_animation_lower_bound;
			const real_argb_color* upper = &stage->color0_animation_upper_bound;
			stage_constants[i * 2][0] = lower->red + (upper->red - lower->red) * t;
			stage_constants[i * 2][1] = lower->green + (upper->green - lower->green) * t;
			stage_constants[i * 2][2] = lower->blue + (upper->blue - lower->blue) * t;
			stage_constants[i * 2][3] = lower->alpha + (upper->alpha - lower->alpha) * t;
			// tinted by one of the object's change colors
			if (g_h1_object_change_colors && stage->color0_source > 0 && stage->color0_source <= 4)
			{
				const real_rgb_color* color = &g_h1_object_change_colors[stage->color0_source - 1];
				stage_constants[i * 2][0] *= color->red;
				stage_constants[i * 2][1] *= color->green;
				stage_constants[i * 2][2] *= color->blue;
			}
			stage_constants[i * 2 + 1][0] = stage->color1.red;
			stage_constants[i * 2 + 1][1] = stage->color1.green;
			stage_constants[i * 2 + 1][2] = stage->color1.blue;
			stage_constants[i * 2 + 1][3] = stage->color1.alpha;
		}
		device->SetPixelShaderConstantF(8, &stage_constants[0][0], k_h1_maximum_generic_stages * 2);

		real32 fade_intensity = 1.f;
		if (g_h1_object_function_values && shader->framebuffer_fade_source > 0 && shader->framebuffer_fade_source <= 4)
		{
			fade_intensity = g_h1_object_function_values[shader->framebuffer_fade_source - 1];
		}
		const real32 settings[4] = { (real32)stage_count, shader->first_map_type != 0 ? 1.f : 0.f, (real32)shader->framebuffer_fade_mode, fade_intensity };
		device->SetPixelShaderConstantF(71, settings, 1);
		const real32 blend_settings[4] = { (real32)shader->framebuffer_blend_function, 0.f, 0.f, 0.f };
		device->SetPixelShaderConstantF(73, blend_settings, 1);
		const real32 vertex_light[4] = { 1.f, 1.f, 1.f, 1.f };
		device->SetPixelShaderConstantF(72, vertex_light, 1);

		// flags: alpha tested, decal, two sided, first map is in screenspace, draw before water, ignore effect, scale first map with distance, numeric
		device->SetRenderState(D3DRS_CULLMODE, TEST_BIT(shader->flags_3, 2) ? D3DCULL_NONE : D3DCULL_CCW);
		h1_bind_framebuffer_blend(shader->framebuffer_blend_function);
		return true;
	}
	case 'sgla':
	{
		const h1_sgla* glass = (const h1_sgla*)definition;
		const real_rgb_color* tint = &glass->background_tint_color;
		real32 constants[5][4] = {};
		switch (subpass)
		{
		case 0:
			// tint: the background is multiplied by the tint color and map
			if (glass->background_tint_map.index == NONE && tint->red == 0.f && tint->green == 0.f && tint->blue == 0.f)
			{
				return false;
			}
			device->SetTexture(0, h1_texture_or_default(glass->background_tint_map, 0));
			constants[3][0] = glass->background_tint_map_scale != 0.f ? glass->background_tint_map_scale : 1.f;
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		case 1:
		{
			// reflection, dynamic mirrors fall back to the cube map
			IDirect3DBaseTexture9* reflection = h1_bitmap_texture_get(glass->reflection_map);
			if (!reflection || (glass->perpendicular_brightness <= 0.f && glass->parallel_brightness <= 0.f))
			{
				return false;
			}
			const bool bumped = glass->reflection_type == 0 && glass->bump_map.index != NONE && !TEST_BIT(glass->flags, 3);
			device->SetTexture(0, h1_texture_or_default(glass->bump_map, 3));
			device->SetTexture(4, reflection);
			h1_set_sampler_addressing(4, true, true, false);
			constants[3][0] = glass->bump_map_scale != 0.f ? glass->bump_map_scale : 1.f;
			constants[4][1] = bumped ? 1.f : 0.f;
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		}
		default:
			// diffuse
			if (glass->diffuse_map.index == NONE && glass->diffuse_detail_map.index == NONE)
			{
				return false;
			}
			device->SetTexture(0, h1_texture_or_default(glass->diffuse_map, 1));
			device->SetTexture(1, h1_texture_or_default(glass->diffuse_detail_map, 1));
			device->SetTexture(2, lightmap ? lightmap : g_h1_default_textures[0]);
			constants[3][0] = glass->diffuse_map_scale != 0.f ? glass->diffuse_map_scale : 1.f;
			constants[3][1] = glass->diffuse_detail_map_scale != 0.f ? glass->diffuse_detail_map_scale : 1.f;
			constants[4][2] = lightmap ? 1.f : 0.f;
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		}
		device->SetPixelShader(g_h1_glass_shader);
		for (DWORD stage = 0; stage < 3; stage++)
		{
			h1_set_sampler_addressing(stage, false, false, false);
		}
		h1_set_sampler_addressing(2, true, true, false);

		constants[0][0] = tint->red; constants[0][1] = tint->green; constants[0][2] = tint->blue; constants[0][3] = 1.f;
		constants[1][0] = glass->perpendicular_tint_color.red; constants[1][1] = glass->perpendicular_tint_color.green;
		constants[1][2] = glass->perpendicular_tint_color.blue; constants[1][3] = glass->perpendicular_brightness;
		constants[2][0] = glass->parallel_tint_color.red; constants[2][1] = glass->parallel_tint_color.green;
		constants[2][2] = glass->parallel_tint_color.blue; constants[2][3] = glass->parallel_brightness;
		constants[4][0] = (real32)subpass;
		device->SetPixelShaderConstantF(0, &constants[0][0], 5);

		// flags: alpha tested, decal, two sided, bump map is specular mask
		device->SetRenderState(D3DRS_CULLMODE, TEST_BIT(glass->flags, 2) ? D3DCULL_NONE : D3DCULL_CCW);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		return true;
	}
	case 'smet':
	{
		const h1_smet* shader = (const h1_smet*)definition;
		if (!g_h1_meter_shader)
		{
			return false;
		}
		// the meter's inputs are the object's function values (1 to 4: a to d), 1 without them
		auto source_value = [](int16 source) -> real32
		{
			return source >= 1 && source <= 4 && g_h1_object_function_values ? g_h1_object_function_values[source - 1] : 1.f;
		};
		const real32 meter_brightness = source_value(shader->meter_brightness_source);
		const real32 flash_brightness = source_value(shader->flash_brightness_source);
		const real32 meter_value = source_value(shader->value_source);
		const real32 gradient_value = source_value(shader->gradient_source);
		const real32 flash_extension = source_value(shader->flash_extension_source);
		const real32 flash_alpha = 1.f / MAX(gradient_value * 8.f, 1.f);
		// flags: decal, two sided, flash color is negative, tint mode 2, unfiltered
		const bool tint_mode_2 = TEST_BIT(shader->flags_3, 3);
		const real32 constants[5][4] =
		{
			{ flash_brightness * shader->flash_color.red, flash_brightness * shader->flash_color.green, flash_brightness * shader->flash_color.blue, flash_extension },
			{ shader->gradient_minimum_color.red, shader->gradient_minimum_color.green, shader->gradient_minimum_color.blue, meter_value },
			{ shader->gradient_maximum_color.red, shader->gradient_maximum_color.green, shader->gradient_maximum_color.blue, flash_alpha },
			{ shader->background_color.red, shader->background_color.green, shader->background_color.blue, tint_mode_2 ? shader->background_transparency : 0.f },
			{ TEST_BIT(shader->flags_3, 2) ? 1.f : 0.f, tint_mode_2 ? 1.f : meter_brightness, 0.f, 0.f },
		};
		device->SetPixelShader(g_h1_meter_shader);
		device->SetPixelShaderConstantF(0, &constants[0][0], 5);
		device->SetTexture(0, h1_texture_or_default(shader->map, 0));
		h1_set_sampler_addressing(0, false, false, TEST_BIT(shader->flags_3, 4));
		device->SetRenderState(D3DRS_CULLMODE, TEST_BIT(shader->flags_3, 1) ? D3DCULL_NONE : D3DCULL_CCW);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		// the frame buffer is tinted: by the tint color, or (tint mode 2) by the meter's alpha with the tint on the meter
		const real32 tint_scale = tint_mode_2 ? meter_brightness : 1.f;
		const D3DCOLOR tint = D3DCOLOR_COLORVALUE(PIN(shader->tint_color_2.red * tint_scale, 0.f, 1.f), PIN(shader->tint_color_2.green * tint_scale, 0.f, 1.f),
			PIN(shader->tint_color_2.blue * tint_scale, 0.f, 1.f), 1.f);
		device->SetRenderState(D3DRS_BLENDFACTOR, tint);
		device->SetRenderState(D3DRS_SRCBLEND, tint_mode_2 ? D3DBLEND_BLENDFACTOR : D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, tint_mode_2 ? D3DBLEND_SRCALPHA : D3DBLEND_BLENDFACTOR);
		return true;
	}
	case 'swat':
	{
		const h1_swat* water = (const h1_swat*)definition;
		// flags: base map alpha modulates reflection, base map color modulates background
		if (subpass == 0 && !TEST_BIT(water->flags, 1))
		{
			return false;
		}
		device->SetPixelShader(g_h1_water_shader);

		device->SetTexture(0, h1_texture_or_default(water->base_map, 0));
		h1_set_sampler_addressing(0, true, true, false);
		const h1_swat_ripples* first_ripple = g_h1_cache_file->block_get(water->ripples, 0);
		IDirect3DBaseTexture9* ripple_texture = h1_bitmap_texture_get(water->ripple_maps, first_ripple ? first_ripple->map_index : 0);
		device->SetTexture(1, ripple_texture ? ripple_texture : g_h1_default_textures[3]);
		h1_set_sampler_addressing(1, false, false, false);
		device->SetTexture(4, h1_bitmap_texture_get(water->reflection_map));
		h1_set_sampler_addressing(4, true, true, false);

		// each ripple layer scrolls inside the ripple map, which itself scrolls over the surface
		const real32 ripple_scale = water->ripple_scale != 0.f ? water->ripple_scale : 1.f;
		const real32 global_u = cosf(water->ripple_animation_angle) * water->ripple_animation_velocity * game_time;
		const real32 global_v = sinf(water->ripple_animation_angle) * water->ripple_animation_velocity * game_time;
		real32 contributions[4] = {};
		real32 transforms[4][4] = {};
		for (int32 i = 0; i < 4; i++)
		{
			const h1_swat_ripples* ripple = g_h1_cache_file->block_get(water->ripples, i);
			const real32 repeats = ripple && ripple->map_repeats > 0 ? (real32)ripple->map_repeats : 1.f;
			contributions[i] = ripple ? ripple->contribution_factor : 0.f;
			const real32 u = ripple ? game_time * ripple->animation_velocity * cosf(ripple->animation_angle) + ripple->map_offset.i : 0.f;
			const real32 v = ripple ? game_time * ripple->animation_velocity * sinf(ripple->animation_angle) + ripple->map_offset.j : 0.f;
			transforms[i][0] = ripple_scale * repeats;
			transforms[i][1] = ripple_scale * repeats;
			transforms[i][2] = global_u * repeats + u;
			transforms[i][3] = global_v * repeats + v;
		}
		if (contributions[0] == 0.f && contributions[1] == 0.f) contributions[1] = 1.f;
		if (contributions[2] == 0.f && contributions[3] == 0.f) contributions[3] = 1.f;
		const real32 first_pair = contributions[0] + contributions[1];
		const real32 second_pair = contributions[2] + contributions[3];
		const real32 weights[4] = { contributions[0] / first_pair, contributions[2] / second_pair, first_pair / (first_pair + second_pair), 0.f };
		const real32 perpendicular[4] = { water->view_perpendicular_tint_color.red, water->view_perpendicular_tint_color.green, water->view_perpendicular_tint_color.blue, water->view_perpendicular_brightness };
		const real32 parallel[4] = { water->view_parallel_tint_color.red, water->view_parallel_tint_color.green, water->view_parallel_tint_color.blue, water->view_parallel_brightness };
		const real32 settings[4] = { (real32)subpass, TEST_BIT(water->flags, 0) ? 1.f : 0.f, 0.f, 0.f };
		device->SetPixelShaderConstantF(0, &transforms[0][0], 4);
		device->SetPixelShaderConstantF(4, weights, 1);
		device->SetPixelShaderConstantF(5, perpendicular, 1);
		device->SetPixelShaderConstantF(6, parallel, 1);
		device->SetPixelShaderConstantF(7, settings, 1);

		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		device->SetRenderState(D3DRS_SRCBLEND, subpass == 0 ? D3DBLEND_ZERO : D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, subpass == 0 ? D3DBLEND_SRCCOLOR : D3DBLEND_ONE);
		return true;
	}
	case 'schi':
	case 'scex':
	{
		// shader_transparent_chicago_extended keeps its 4 stage maps at the same place as chicago keeps its maps
		const h1_schi* shader = (const h1_schi*)definition;
		device->SetPixelShader(g_h1_chicago_shader);

		const int32 map_count = MIN(shader->maps.count, (int32)k_h1_maximum_shader_maps);
		real32 transforms[8][4] = {};
		real32 functions[4][4] = {};

		// rasterizer_xbox_transparent_geometry.c: a numeric shader's first map shows one digit of the object's function (the
		// part's permutation is the place), out of its bitmaps (8 of them counts with function d, the compass)
		int32 first_bitmap_index = 0;
		if (TEST_BIT(shader->flags_3, 7) && g_h1_object_function_values && map_count > 0 && !TEST_BIT(shader->extra_flags, 1))
		{
			const h1_schi_maps* map = g_h1_cache_file->block_get(shader->maps, 0);
			const h1_bitm* bitmap_group = map->map.index != NONE ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', map->map.index) : NULL;
			const int32 frame_count = bitmap_group ? bitmap_group->bitmaps.count : 0;
			if (frame_count > 0)
			{
				const int32 function_index = frame_count == 8 ? 3 : 0;
				const int32 counter_limit = (uint8)shader->numeric_counter_limit;
				int32 counter_value = PIN((int32)floorf(counter_limit * g_h1_object_function_values[function_index] + 0.5f), 0, counter_limit);
				for (int32 digit_index = 0; digit_index < g_h1_shader_permutation_index; digit_index++)
				{
					counter_value /= frame_count;
				}
				first_bitmap_index = counter_value % frame_count;
			}
		}
		for (int32 i = 0; i < k_h1_maximum_shader_maps; i++)
		{
			const h1_schi_maps* map = i < map_count ? g_h1_cache_file->block_get(shader->maps, i) : NULL;
			if (!map)
			{
				device->SetTexture(i, g_h1_default_textures[0]);
				transforms[i * 2][0] = transforms[i * 2][1] = 1.f;
				transforms[i * 2 + 1][0] = 1.f;
				continue;
			}

			const bool cube = i == 0 && shader->first_map_type != 0;
			IDirect3DBaseTexture9* texture = h1_bitmap_texture_get(map->map, i == 0 ? first_bitmap_index : 0);
			device->SetTexture(cube ? 4 : i, texture ? texture : (cube ? NULL : g_h1_default_textures[0]));
			if (cube)
			{
				device->SetTexture(0, g_h1_default_textures[0]);
			}
			// map flags: unfiltered, alpha replicate, u clamped, v clamped
			h1_set_sampler_addressing(cube ? 4 : i, TEST_BIT(map->flags, 2), TEST_BIT(map->flags, 3), TEST_BIT(map->flags, 0));
			h1_map_transform(&transforms[i * 2][0], map->map_u_scale, map->map_v_scale, map->map_u_offset, map->map_v_offset, map->map_rotation,
				map->u_animation_function, map->u_animation_period, map->u_animation_phase, map->u_animation_scale,
				map->v_animation_function, map->v_animation_period, map->v_animation_phase, map->v_animation_scale,
				map->rotation_animation_function, map->rotation_animation_period, map->rotation_animation_phase, map->rotation_animation_scale, map->rotation_animation_center,
				game_time);
			functions[i][0] = map->color_function;
			functions[i][1] = map->alpha_function;
			functions[i][2] = TEST_BIT(map->flags, 1) ? 1.f : 0.f;
		}
		device->SetPixelShaderConstantF(0, &transforms[0][0], 8);
		device->SetPixelShaderConstantF(8, &functions[0][0], 4);
		const real32 settings[4] = { (real32)MAX(map_count, 1), shader->first_map_type != 0 ? 1.f : 0.f, 0.f, 0.f };
		device->SetPixelShaderConstantF(12, settings, 1);
		real32 fade_intensity = 1.f;
		if (g_h1_object_function_values && shader->framebuffer_fade_source > 0 && shader->framebuffer_fade_source <= 4)
		{
			fade_intensity = g_h1_object_function_values[shader->framebuffer_fade_source - 1];
		}
		const real32 fade_settings[4] = { (real32)shader->framebuffer_blend_function, (real32)shader->framebuffer_fade_mode, fade_intensity, 0.f };
		device->SetPixelShaderConstantF(13, fade_settings, 1);

		// flags: alpha tested, decal, two sided, first map is in screenspace, draw before water, ignore effect, scale first map with distance, numeric
		device->SetRenderState(D3DRS_CULLMODE, TEST_BIT(shader->flags_3, 2) ? D3DCULL_NONE : D3DCULL_CCW);
		h1_bind_framebuffer_blend(shader->framebuffer_blend_function);
		return true;
	}
	default:
		return false;
	}
}

void h1_render_shader_unbind(void)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0xF);
	return;
}

/* private code */

static void h1_bind_framebuffer_blend(int16 function)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	// transparent geometry never writes halo 2's depth target
	device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
	device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

	switch (function)
	{
	case _h1_framebuffer_blend_alpha_blend:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	case _h1_framebuffer_blend_multiply:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		break;
	case _h1_framebuffer_blend_double_multiply:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
		break;
	case _h1_framebuffer_blend_add:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	case _h1_framebuffer_blend_subtract:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
		break;
	case _h1_framebuffer_blend_component_min:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MIN);
		break;
	case _h1_framebuffer_blend_component_max:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
		break;
	case _h1_framebuffer_blend_alpha_multiply_add:
	default:
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		break;
	}
	return;
}

real32 h1_periodic_function_evaluate(int16 function, real32 x)
{
	return h1_periodic_function(function, x);
}

// Halo 1 periodic functions over x (in periods)
static real32 h1_periodic_function(int16 function, real32 x)
{
	const real32 t = x - floorf(x);
	switch (function)
	{
	case 0: return 1.f;											// one
	case 1: return 0.f;											// zero
	case 2: case 3: return 0.5f - 0.5f * cosf(t * 2.f * _pi);	// cosine
	case 4: case 5: return t < 0.5f ? 2.f * t : 2.f - 2.f * t;	// diagonal wave
	case 6: case 7: return t;									// slide
	case 8: case 9: case 10: case 11:							// noise, jitter, wander, spark
	{
		const real32 s = sinf(x * 12.9898f) * 43758.5453f;
		return s - floorf(s);
	}
	default:
		return 0.f;
	}
}

static IDirect3DPixelShader9* h1_compile_pixel_shader(const char* source, const char* name)
{
	LPD3DXBUFFER code = NULL;
	LPD3DXBUFFER errors = NULL;
	IDirect3DPixelShader9* shader = NULL;
	if (SUCCEEDED(D3DXCompileShader(source, (UINT)strlen(source), NULL, NULL, "main", "ps_3_0", 0, &code, &errors, NULL)))
	{
		rasterizer_dx9_device_get_interface()->CreatePixelShader((const DWORD*)code->GetBufferPointer(), &shader);
		code->Release();
	}
	else if (errors)
	{
		h1_log("shaders: %s pixel shader: %s", name, (const char*)errors->GetBufferPointer());
	}
	if (errors) errors->Release();
	return shader;
}

static IDirect3DTexture9* h1_solid_texture(uint32 color)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const bool use_d3d9_ex = rasterizer_globals_get()->use_d3d9_ex;
	IDirect3DTexture9* texture = NULL;
	IDirect3DTexture9* staging = NULL;
	if (FAILED(device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, use_d3d9_ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &texture, NULL)))
	{
		return NULL;
	}
	IDirect3DTexture9* target = texture;
	if (use_d3d9_ex && SUCCEEDED(device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &staging, NULL)))
	{
		target = staging;
	}
	D3DLOCKED_RECT locked;
	if (SUCCEEDED(target->LockRect(0, &locked, NULL, 0)))
	{
		*(uint32*)locked.pBits = color;
		target->UnlockRect(0);
	}
	if (staging)
	{
		device->UpdateTexture(staging, texture);
		staging->Release();
	}
	return texture;
}

// expression for a combiner input (before its mapping)
static std::string h1_generic_input(int16 source, bool alpha)
{
	static const char* const k_registers[] = { "m0", "m1", "m2", "m3", "v0", "v1", "r0", "r1", "k0", "k1" };
	if (source == 0) return alpha ? "0.0f" : "float3(0.0f, 0.0f, 0.0f)";
	if (source == 1) return alpha ? "1.0f" : "float3(1.0f, 1.0f, 1.0f)";
	if (source == 2) return alpha ? "0.5f" : "float3(0.5f, 0.5f, 0.5f)";
	if (source == 3) return alpha ? "-1.0f" : "float3(-1.0f, -1.0f, -1.0f)";
	if (source == 4) return alpha ? "-0.5f" : "float3(-0.5f, -0.5f, -0.5f)";

	// 5-14 first half, 15-24 second half: color inputs use rgb then alpha, alpha inputs use alpha then blue
	const bool second_half = source >= 15;
	const int16 index = second_half ? source - 15 : source - 5;
	if (!VALID_INDEX(index, NUMBEROF(k_registers)))
	{
		return alpha ? "0.0f" : "float3(0.0f, 0.0f, 0.0f)";
	}

	const std::string reg = k_registers[index];
	if (alpha)
	{
		return reg + (second_half ? ".b" : ".a");
	}
	return reg + (second_half ? ".aaa" : ".rgb");
}

static std::string h1_generic_input_mapping(const std::string& x, int16 mapping)
{
	switch (mapping)
	{
	case 0: return "saturate(" + x + ")";
	case 1: return "(1.0f - saturate(" + x + "))";
	case 2: return "(2.0f * saturate(" + x + ") - 1.0f)";
	case 3: return "(1.0f - 2.0f * saturate(" + x + "))";
	case 4: return "(saturate(" + x + ") - 0.5f)";
	case 5: return "(0.5f - saturate(" + x + "))";
	case 6: return "(" + x + ")";
	default: return "(-(" + x + "))";
	}
}

static std::string h1_generic_output_mapping(const std::string& x, int16 mapping)
{
	std::string result;
	switch (mapping)
	{
	case 1: result = "(" + x + ") * 0.5f"; break;
	case 2: result = "(" + x + ") * 2.0f"; break;
	case 3: result = "(" + x + ") * 4.0f"; break;
	case 4: result = "((" + x + ") - 0.5f)"; break;
	case 5: result = "(((" + x + ") - 0.5f) * 2.0f)"; break;
	default: result = "(" + x + ")"; break;
	}
	return "clamp(" + result + ", -1.0f, 1.0f)";
}

static const char* h1_generic_output_register(int16 output)
{
	static const char* const k_outputs[] = { NULL, "r0", "r1", "v0", "v1", "m0", "m1", "m2", "m3" };
	return VALID_INDEX(output, NUMBEROF(k_outputs)) ? k_outputs[output] : NULL;
}

static IDirect3DPixelShader9* h1_generic_shader_get(datum shader_index, const h1_sotr* shader)
{
	auto found = g_h1_generic_shaders.find(shader_index);
	if (found != g_h1_generic_shaders.end())
	{
		return found->second;
	}

	std::string source = k_h1_generic_pixel_shader_header;
	char line[256];
	if (shader->stages.count <= 0)
	{
		// without stages the xbox combiner passes the first map through
		source += "\tr0 = m0;\n";
	}
	const int32 stage_count = MIN(shader->stages.count, (int32)k_h1_maximum_generic_stages);
	for (int32 i = 0; i < stage_count; i++)
	{
		const h1_sotr_stages* stage = g_h1_cache_file->block_get(shader->stages, i);
		sprintf_s(line, "\t// stage %d\n\tk0 = stage_constants[%d];\n\tk1 = stage_constants[%d];\n", i, i * 2, i * 2 + 1);
		source += line;

		const std::string ca = h1_generic_input_mapping(h1_generic_input(stage->input_a, false), stage->input_a_mapping);
		const std::string cb = h1_generic_input_mapping(h1_generic_input(stage->input_b, false), stage->input_b_mapping);
		const std::string cc = h1_generic_input_mapping(h1_generic_input(stage->input_c, false), stage->input_c_mapping);
		const std::string cd = h1_generic_input_mapping(h1_generic_input(stage->input_d, false), stage->input_d_mapping);
		const std::string aa = h1_generic_input_mapping(h1_generic_input(stage->input_a_2, true), stage->input_a_mapping_2);
		const std::string ab = h1_generic_input_mapping(h1_generic_input(stage->input_b_2, true), stage->input_b_mapping_2);
		const std::string ac = h1_generic_input_mapping(h1_generic_input(stage->input_c_2, true), stage->input_c_mapping_2);
		const std::string ad = h1_generic_input_mapping(h1_generic_input(stage->input_d_2, true), stage->input_d_mapping_2);

		// output functions: multiply or dot product
		source += "\tcab = " + (stage->output_ab_function ? "dot(" + ca + ", " + cb + ").xxx" : ca + " * " + cb) + ";\n";
		source += "\tccd = " + (stage->output_cd_function ? "dot(" + cc + ", " + cd + ").xxx" : cc + " * " + cd) + ";\n";
		// flags: color mux, alpha mux (select by spare 0 alpha)
		source += TEST_BIT(stage->flags, 0) ? "\tcsum = r0.a >= 0.5f ? ccd : cab;\n" : "\tcsum = cab + ccd;\n";
		source += "\taab = " + aa + " * " + ab + ";\n";
		source += "\tacd = " + ac + " * " + ad + ";\n";
		source += TEST_BIT(stage->flags, 1) ? "\tasum = r0.a >= 0.5f ? acd : aab;\n" : "\tasum = aab + acd;\n";

		source += "\tcab = " + h1_generic_output_mapping("cab", stage->output_mapping) + ";\n";
		source += "\tccd = " + h1_generic_output_mapping("ccd", stage->output_mapping) + ";\n";
		source += "\tcsum = " + h1_generic_output_mapping("csum", stage->output_mapping) + ";\n";
		source += "\taab = " + h1_generic_output_mapping("aab", stage->output_mapping_2) + ";\n";
		source += "\tacd = " + h1_generic_output_mapping("acd", stage->output_mapping_2) + ";\n";
		source += "\tasum = " + h1_generic_output_mapping("asum", stage->output_mapping_2) + ";\n";

		// every output is computed from the stage inputs before any register is written
		const char* color_ab = h1_generic_output_register(stage->output_ab);
		const char* color_cd = h1_generic_output_register(stage->output_cd);
		const char* color_sum = h1_generic_output_register(stage->output_ab_cd_mux_sum);
		const char* alpha_ab = h1_generic_output_register(stage->output_ab_2);
		const char* alpha_cd = h1_generic_output_register(stage->output_cd_2);
		const char* alpha_sum = h1_generic_output_register(stage->output_ab_cd_mux_sum_2);
		if (color_ab) { sprintf_s(line, "\t%s.rgb = cab;\n", color_ab); source += line; }
		if (color_cd) { sprintf_s(line, "\t%s.rgb = ccd;\n", color_cd); source += line; }
		if (color_sum) { sprintf_s(line, "\t%s.rgb = csum;\n", color_sum); source += line; }
		if (alpha_ab) { sprintf_s(line, "\t%s.a = aab;\n", alpha_ab); source += line; }
		if (alpha_cd) { sprintf_s(line, "\t%s.a = acd;\n", alpha_cd); source += line; }
		if (alpha_sum) { sprintf_s(line, "\t%s.a = asum;\n", alpha_sum); source += line; }
	}
	source += k_h1_generic_pixel_shader_footer;

	IDirect3DPixelShader9* pixel_shader = h1_compile_pixel_shader(source.c_str(), g_h1_cache_file->tag_name_get(shader_index));
	g_h1_generic_shaders[shader_index] = pixel_shader;
	return pixel_shader;
}
