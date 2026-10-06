#include "stdafx.h"
#include "hud_messaging.h"

#include "interface/hud.h"
#include "halo1/h1_map_loader.h"

/* globals */

// Pointer to the crosshair and text scale for the below hook
float* p_text_scale_factor;

// halo 1 maps: hud_messaging.c places the messages at the hud globals' messaging point, the top left of the title safe frame
// (rasterizer_xbox.c, 48 by 36 of a 640 by 480 screen) and 60 down, in a 480 high window as wide as the screen's shape
static void __cdecl h1_hud_messaging_point(int32* top, int16* left)
{
	if (!h1_maps_active())
	{
		return;
	}
	const rectangle2d* viewport_bounds = Memory::GetAddress<rectangle2d*>(0x4E66F8);
	const rectangle2d* window_bounds = Memory::GetAddress<rectangle2d*>(0x4E6700);
	const real32 height = (real32)(window_bounds->bottom - window_bounds->top);
	const real32 width = (real32)(window_bounds->right - window_bounds->left);
	if (height <= 0.f || width <= 0.f)
	{
		return;
	}
	const real32 pixel_scale = height / 480.f;
	const real32 frame_x0 = (real32)(int32)(48.f * (width / pixel_scale) / 640.f);
	*left = (int16)(window_bounds->left - viewport_bounds->left + (int32)(frame_x0 * pixel_scale));
	*top = window_bounds->top - viewport_bounds->top + (int32)((36.f + 60.f) * pixel_scale);
	return;
}

__declspec(naked) void ui_hud_left_messaging_top_scale()
{
	__asm
	{
		// sp: 1938h
		// mov     dl, [esp + 27h]
		// add     esp, 12

		fild dword ptr [esp + 18h]
		push eax
		mov eax, [p_text_scale_factor]
		fmul dword ptr [eax]
		pop eax
		fistp dword ptr [esp + 18h]

		// halo 1 maps: halo 1's messaging point (the top at esp + 18h, the left word at esp + 2Ch)
		pushad
		lea eax, [esp + 20h + 2Ch]
		push eax
		lea eax, [esp + 4 + 20h + 18h]
		push eax
		call h1_hud_messaging_point
		add esp, 8
		popad

		// original code
		mov     ecx, ebx
		imul    ecx, 4E0h
		retn
	}
}


/* public code */

void hud_messaging_apply_hooks(void)
{
	// remove checks preventing pick-up messaging from displaying
	// in splitscreen mode
	NopFill(Memory::GetAddress(0x2217BE), 10);
	NopFill(Memory::GetAddress(0x221879), 10);
	NopFill(Memory::GetAddress(0x220DF7), 10);
	NopFill(Memory::GetAddress(0x220DAC), 10);

	// nop call + cmp
	NopFill(Memory::GetAddress(0x5D928), 8);
	// force jmp
	WriteValue(Memory::GetAddress(0x5D930), (uint8)0xEB);

	p_text_scale_factor = get_secondary_hud_scale();
	Codecave(Memory::GetAddress(0x22D29E), ui_hud_left_messaging_top_scale, 3);
	return;
}

void __cdecl hud_messaging_update(int32 user_index)
{
	INVOKE(0x22D1BD, 0x0, hud_messaging_update, user_index);
	return;
}

void __cdecl hud_messaging_clear(void)
{
	INVOKE(0x22CE83, 0x206863, hud_messaging_clear);
	return;
}

void __cdecl hud_messaging_post(int32 user_index, string_id string_id)
{
	INVOKE(0x22DEA4, 0x206BB7, hud_messaging_post, user_index, string_id);
	return;
}
