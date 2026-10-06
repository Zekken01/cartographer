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
// halo 2's motion sensor (its sweep and blips): halo 1's draws instead, when halo 2 would draw its own (the game variant's
// motion sensor setting)
bool h1_hud_hides_halo2_motion_sensor(void);

// cinematics.c: the scenario's cutscene titles the scripts show (cinematic_set_title), drawn over the interface
void h1_cinematic_titles_reset(void);
void h1_cinematic_set_title_delayed(int16 title_index, real32 delay);
void h1_cinematic_titles_render(void);

// hud_messaging.c: the scripts' help text (hud_set_help_text, shown while show_hud_help_text allows it, flashing with
// enable_hud_help_flash) and objectives (hud_set_objective_text, shown for the hud globals' objective time), drawn with the
// cinematic titles
void h1_hud_text_reset(void);
void h1_hud_set_help_text(int16 message_index);
bool h1_hud_show_help_text(bool show);
void h1_hud_enable_help_flash(bool flash);
void h1_hud_set_objective_text(int16 message_index);
// hud.c's player action state message (hud_set_state_message of the hud globals' hud messages, its first custom icon the text),
// shown while it's set each tick in the messaging lines (help text replaces it, as in halo 1)
void h1_hud_set_state_message(int16 message_index, const wchar_t* custom_text);

// hud_nav_points.c: the scripts' nav points at cutscene flags and objects, drawn with the hud's waypoint arrows
void h1_hud_nav_points_reset(void);
void h1_hud_activate_nav_point_flag(int16 nav_index, int16 flag_index, real32 vertical_offset);
void h1_hud_activate_nav_point_object(int16 nav_index, datum object_index, real32 vertical_offset);
void h1_hud_deactivate_nav_point_flag(int16 flag_index);
void h1_hud_deactivate_nav_point_object(datum object_index);
