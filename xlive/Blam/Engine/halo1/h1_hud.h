#pragma once
/*
* hud_weapon.c and hud_draw.c on Halo 1 maps: the local player's Halo 1 weapon hud (the weapon hud interface's statics,
* meters, numbers, overlays and crosshairs) drawn after Halo 2's interface, laid out in Halo 1's 480 high hud window.
*/

// hud_render_weapon_interface for the local player
void h1_hud_render(void);
// interface_draw_screen's screen effect of the local player's weapon hud (the sniper rifle's scope), before the interface
void h1_hud_render_screen_effect(void);
// releases the hud's shaders
void h1_hud_dispose(void);
// halo 2's hud widgets halo 1's hud replaces on halo 1 maps (the shield, health and grenades)
bool h1_hud_hides_halo2_widget(string_id name);
