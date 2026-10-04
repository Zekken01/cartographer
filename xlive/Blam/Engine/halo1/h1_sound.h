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

// called every main loop iteration
void h1_sound_update(void);
