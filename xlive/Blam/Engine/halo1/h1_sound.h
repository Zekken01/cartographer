#pragma once

/*
* Halo 1 ambient sound.
*
* Plays the Halo 1 map's own sound data: the background sound of the cluster the camera is in,
* its detail sounds, and the looping sounds of sound scenery and scenery attachments. The Xbox
* ADPCM samples are decoded from the cache file and mixed into a separate wave out stream, with
* the distance rolloff and fades of the Halo 1 sound manager.
*/

struct render_camera;

// decodes the sounds of the loaded halo 1 map and starts playback
void h1_sound_begin(void);

// stops playback and frees every decoded sound
void h1_sound_dispose(void);

// the camera the sounds are heard from (first window of the frame)
void h1_sound_listener_set(const render_camera* camera);

// plays a halo 1 sound once at a point (effect parts, particles)
void h1_sound_impulse(datum sound_index, const real_point3d* position, real32 scale);

// game_sound.c game_looping_sound_new for an object's attachment: a looping sound that follows the object, a handle for the
// calls below (0 when it can't play)
int32 h1_sound_looping_attached_new(datum looping_sound_index);
// where it is and whether the object's function lets it play (object_get_function_value), its scale the function's value
void h1_sound_looping_attached_update(int32 handle, const real_point3d* position, bool audible, real32 scale);
// game_looping_sound_delete: it fades out
void h1_sound_looping_attached_delete(int32 handle);

// called every main loop iteration
void h1_sound_update(void);
