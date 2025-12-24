/*
 * Open Surge Engine
 * audio.c - audio module
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include "audio.h"
#include "engine.h"
#include "asset.h"
#include "resourcemanager.h"
#include "logfile.h"
#include "timer.h"
#include "video.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/numeric.h"

#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

/* a pool of samples */
typedef struct poolsample_t poolsample_t;
struct poolsample_t {
    uint32_t unique_id;
    ALLEGRO_SAMPLE_INSTANCE* sample_instance;
    const ALLEGRO_MIXER* parent;
};

/* sound structure */
struct sound_t {
    ALLEGRO_SAMPLE* sample;
    char* filepath; /* relative path */
};

/* music structure */
struct music_t {
    ALLEGRO_AUDIO_STREAM* stream;
    bool is_paused;
    char* filepath; /* relative path */
};

/* private stuff */
#define MAX_SIMULTANEOUS_SAMPLES        16 /* how many samples can be played at the same time */
#define SAMPLE_POOL_SIZE                (2 * MAX_SIMULTANEOUS_SAMPLES) /* MAX_SIMULTANEOUS_SAMPLES per mixer * 2 mixers */
#define NULL_SAMPLE_HANDLE              ((samplehandle_t)0)
#define UNDEFINED_ID                    ((uint32_t)0xFFFFFFFFu)
#define DEFAULT_VOLUME                  1.0f
#define DEFAULT_MIXER_PERCENTAGE        0.5f
#define DEFAULT_MUFFLER_PROFILE         MUFFLER_MEDIUM

static const char* MUFFLER_PROFILE_NAME[] = {
    [MUFFLER_OFF] = "off",
    [MUFFLER_LOW] = "low",
    [MUFFLER_MEDIUM] = "medium",
    [MUFFLER_HIGH] = "high"
};

static ALLEGRO_VOICE* voice = NULL;
static ALLEGRO_MIXER* master_mixer = NULL;
static ALLEGRO_MIXER* music_mixer = NULL;
static ALLEGRO_MIXER* sound_mixer = NULL;
static ALLEGRO_MIXER* primary_sound_mixer = NULL;
static ALLEGRO_MIXER* secondary_sound_mixer = NULL;
static poolsample_t sample_pool[SAMPLE_POOL_SIZE];

static music_t *current_music = NULL; /* music being played at the moment (NULL if none) */
static float master_volume = DEFAULT_VOLUME; /* a value in [0,1] affecting all musics and sounds */
static float mixer_percentage = DEFAULT_MIXER_PERCENTAGE; /* a value in [0,1] that controls music & sfx volume */
static bool is_globally_muted = false; /* global mute / unmute */
static mufflerprofile_t current_muffler_profile = DEFAULT_MUFFLER_PROFILE;
static int current_muffler_flags = MUFFLE_NOTHING;

static int preload_sample(const char* vpath, void* data);
static void set_master_gain(float gain);
static void handle_haltresume_event(const ALLEGRO_EVENT* event, void* context);
static samplehandle_t acquire_sample_from_pool();
static ALLEGRO_SAMPLE_INSTANCE* get_sample_instance(samplehandle_t handle);
static void init_muffler();
static void update_muffler(mufflerprofile_t profile, int flags);
static void muffle_mixer(ALLEGRO_MIXER* mixer, mufflerprofile_t profile);
static const float* muffler_sigma(mufflerprofile_t profile);
static void muffler_postprocess(void* buf, unsigned int num_samples, void* data);




/*
 *
 * music management
 *
 */


/*
 * music_load()
 * Loads a music from a file
 */
music_t *music_load(const char *path)
{
    music_t *m;

    if(*path == '\0') /* empty path */
        return NULL;

    if(NULL == (m = resourcemanager_find_music(path))) {
        const char* fullpath = asset_path(path);
        logfile_message("Loading music \"%s\"...", fullpath);

        /* build the music object */
        m = mallocx(sizeof *m);
        m->is_paused = false;
        m->filepath = str_dup(path);
        if(NULL == (m->stream = al_load_audio_stream(fullpath, 4, 1024)))
            fatal_error("Can't load music \"%s\"", path);
        
        /* configure the audio stream */
        al_attach_audio_stream_to_mixer(m->stream, music_mixer);
        al_set_audio_stream_playmode(m->stream, ALLEGRO_PLAYMODE_LOOP);
        al_set_audio_stream_playing(m->stream, false);

        /* adding it to the resource manager */
        resourcemanager_add_music(path, m);
        resourcemanager_ref_music(path);
    }
    else
        resourcemanager_ref_music(path);

    return m;
}


/*
 * music_unref()
 * Will try to release the resource from
 * the memory. You will call this if, and
 * only if, you are sure you don't need the
 * resource anymore (i.e., you're not holding
 * any pointers to it)
 *
 * Used for reference counting. Normally you
 * don't need to bother with this, unless
 * you care about reducing memory usage.
 * Note that 'music_ref()' must not exist.
 * Returns the no. of references to the music
 */
int music_unref(music_t *music)
{
    if(music != NULL)
        return resourcemanager_unref_music(music->filepath);
    return 0;
}


/*
 * music_destroy()
 * Destroys a music. This is called automatically
 * while unloading the resource manager.
 */
void music_destroy(music_t *music)
{
    if(music != NULL) {
        if(music == current_music) {
            music_stop();
            current_music = NULL;
        }

        al_destroy_audio_stream(music->stream);
        free(music->filepath);
        free(music);
    }
}


/*
 * music_play()
 * Plays a music.
 * Set loop to TRUE to make it loop continuously.
 */
void music_play(music_t *music, bool loop)
{
    music_stop();

    if(music != NULL) {
        ALLEGRO_PLAYMODE mode = loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
        al_set_audio_stream_playmode(music->stream, mode);
        al_set_audio_stream_playing(music->stream, true);
        music->is_paused = false;
    }

    current_music = music;
    music_set_volume(1.0f);
}

/*
 * music_stop()
 * Stops the current music (if any)
 */
void music_stop()
{
    if(current_music != NULL) {
        al_set_audio_stream_playing(current_music->stream, false);
        al_rewind_audio_stream(current_music->stream);
        current_music->is_paused = false; /* it's stopped, not paused */
    }

    current_music = NULL;
}

/*
 * music_pause()
 * Pauses the current music
 */
void music_pause()
{
    if(current_music != NULL && !(current_music->is_paused)) {
        al_set_audio_stream_playing(current_music->stream, false);
        current_music->is_paused = true;
    }
}

/*
 * music_resume()
 * Resumes the current music
 */
void music_resume()
{
    if(current_music != NULL && current_music->is_paused) {
        al_set_audio_stream_playing(current_music->stream, true);
        current_music->is_paused = false;
    }
}

/*
 * music_set_volume()
 * Changes the volume of the current music.
 * zero means silence; 1.0 means default volume
 */
void music_set_volume(float volume)
{
    if(current_music != NULL) {
        float gain = max(volume, 0.0f);
        al_set_audio_stream_gain(current_music->stream, gain);
    }
}

/*
 * music_get_volume()
 * Returns the volume of the current music.
 */
float music_get_volume()
{
    if(current_music != NULL)
        return al_get_audio_stream_gain(current_music->stream);
    else
        return 0.0f;
}

/*
 * music_is_playing()
 * Checks if a music is playing
 */
bool music_is_playing()
{
    return (current_music != NULL) && al_get_audio_stream_playing(current_music->stream);
}

/*
 * music_duration()
 * Music duration, in seconds
 */
float music_duration()
{
    if(current_music != NULL) {
        /* this may be zero if the length is unknown */
        return al_get_audio_stream_length_secs(current_music->stream);
    }

    return 0.0f;
}

/*
 * music_current()
 * The currently playing music.
 * May be NULL
 */
music_t *music_current()
{
    return current_music;
}

/*
 * music_path()
 * Returns the filepath of the specified music
 */
const char *music_path(const music_t *music)
{
    return (music != NULL) ? music->filepath : "";
}

/*
 * music_is_paused()
 * Checks if the currently playing music is paused
 */
bool music_is_paused()
{
    return (current_music != NULL) && (current_music->is_paused);
}




/*
 *
 * sound management
 *
 */


/*
 * sound_load()
 * Load a sound effect from a file
 */
sound_t* sound_load(const char* path)
{
    sound_t* sound;

    if(NULL == (sound = resourcemanager_find_sample(path))) {
        const char* fullpath = asset_path(path);
        logfile_message("Loading sound \"%s\"...", fullpath);

        /* build the sound object */
        sound = mallocx(sizeof *sound);
        sound->filepath = str_dup(path);

        /* load the sample */
        if(NULL == (sound->sample = al_load_sample(fullpath)))
            fatal_error("Can't load sound \"%s\"", path);

        /* adding it to the resource manager */
        resourcemanager_add_sample(path, sound);
        resourcemanager_ref_sample(path);
    }
    else
        resourcemanager_ref_sample(path);

    return sound;
}

/*
 * sound_unref()
 * Will try to release the resource from
 * the memory. You will call this if, and
 * only if, you are sure you don't need the
 * resource anymore (i.e., you're not holding
 * any pointers to it)
 *
 * Used for reference counting. Normally you
 * don't need to bother with this, unless
 * you care about reducing memory usage.
 * Note that 'sound_ref()' must not exist.
 * Returns the no. of references to the sample
 */
int sound_unref(sound_t* sound)
{
    return resourcemanager_unref_sample(sound->filepath);
}

/*
 * sound_destroy()
 * Release a sound effect. This is called
 * automatically when releasing the main hash
 */
void sound_destroy(sound_t* sound)
{
    if(sound == NULL)
        return;

    al_destroy_sample(sound->sample);
    free(sound->filepath);
    free(sound);
}

/*
 * sound_play()
 * Play a sound effect
 */
samplehandle_t sound_play(const sound_t* sound)
{
    return sound_play_ex(sound, 1.0f, 0.0f, 1.0f);
}

/*
 * sound_play_ex()
 * Play a sound effect with extra options
 *
 * 0.0 <= volume (defaults to 1.0)
 * (left speaker) -1.0 <= pan <= 1.0 (right speaker)
 * 1.0 = default speed
 *
 * Returns a handle for dynamic, fine-grained control of the playing sound effect
 */
samplehandle_t sound_play_ex(const sound_t* sound, float volume, float pan, float speed)
{
    ALLEGRO_SAMPLE_INSTANCE* spl;
    samplehandle_t handle;

    /* validate */
    if(sound == NULL)
        return NULL_SAMPLE_HANDLE;

    /* prepare a sample instance and a sample handle */
    if(NULL_SAMPLE_HANDLE == (handle = acquire_sample_from_pool()))
        return NULL_SAMPLE_HANDLE;

    if(NULL == (spl = get_sample_instance(handle)))
        return NULL_SAMPLE_HANDLE;

    /* adjust the parameters */
    volume = max(volume, 0.0f); /* values > 1 may clip the audio */
    pan = clip(pan, -1.0f, 1.0f);
    speed = max(speed, 1.0f / 64.0f); /* a min speed of 1/64 comes from the internals of Allegro */

    /* set the parameters */
    al_set_sample_instance_gain(spl, volume);
    al_set_sample_instance_pan(spl, pan);
    al_set_sample_instance_speed(spl, speed);
    al_set_sample_instance_playing(spl, ALLEGRO_PLAYMODE_ONCE);

    /* play the sample */
    al_set_sample(spl, sound->sample);
    al_play_sample_instance(spl);

    /* return the handle */
    return handle;
}

/*
 * sound_stop()
 * Stop a sound effect
 */
void sound_stop(samplehandle_t handle)
{
    ALLEGRO_SAMPLE_INSTANCE* spl;

    if(NULL == (spl = get_sample_instance(handle)))
        return;

    al_stop_sample_instance(spl);
}

/*
 * sound_is_playing()
 * Check if a sound effect is playing
 */
bool sound_is_playing(samplehandle_t handle)
{
    ALLEGRO_SAMPLE_INSTANCE* spl;

    if(NULL == (spl = get_sample_instance(handle)))
        return false;

    return al_get_sample_instance_playing(spl);
}

/*
 * sound_get_volume()
 * Get the volume of a sound effect
 * 0.0f means silence; 1.0f, the default volume
 */
float sound_get_volume(samplehandle_t handle)
{
    ALLEGRO_SAMPLE_INSTANCE* spl;

    if(NULL == (spl = get_sample_instance(handle)))
        return 0.0f; /* not playing */

    return al_get_sample_instance_gain(spl);
}

/*
 * sound_set_volume()
 * Set the volume of a sound effect
 */
void sound_set_volume(samplehandle_t handle, float volume)
{
    ALLEGRO_SAMPLE_INSTANCE* spl;

    if(NULL == (spl = get_sample_instance(handle)))
        return;

    volume = max(volume, 0.0f);
    al_set_sample_instance_gain(spl, volume);
}

/*
 * sound_stop_all()
 * Stop all sound effects
 */
void sound_stop_all()
{
    for(int i = 0; i < SAMPLE_POOL_SIZE; i++) {
        ALLEGRO_SAMPLE_INSTANCE* spl = sample_pool[i].sample_instance;

        if(!al_get_mixer_playing(sample_pool[i].parent))
            continue;

        if(!al_get_sample_instance_playing(spl))
            continue;

        al_set_sample_instance_playing(spl, false);
    }
}

/*
 * sound_swap_mixers()
 * Swap between primary and secondary sound mixers,
 * pausing all currently playing sounds
 */
void sound_swap_mixers()
{
    al_set_mixer_playing(primary_sound_mixer, !al_get_mixer_playing(primary_sound_mixer));
    al_set_mixer_playing(secondary_sound_mixer, !al_get_mixer_playing(secondary_sound_mixer));
}




/*
 *
 * audio manager
 *
 */

/*
 * audio_init()
 * Initializes the Audio Manager
 */
void audio_init()
{
    logfile_message("Initializing the audio system...");

    current_music = NULL;
    master_volume = DEFAULT_VOLUME;
    mixer_percentage = DEFAULT_MIXER_PERCENTAGE;
    is_globally_muted = false;

    if(!al_is_audio_installed()) {
        if(!al_install_audio())
            fatal_error("Can't initialize Allegro's audio addon");
    }

    if(!al_is_acodec_addon_initialized()) {
        if(!al_init_acodec_addon())
            fatal_error("Can't initialize Allegro's acodec addon");
    }

    if(NULL == (voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2)))
        fatal_error("Can't create an Allegro voice");

    ALLEGRO_MIXER** mixers[] = { &master_mixer, &music_mixer, &sound_mixer, &primary_sound_mixer, &secondary_sound_mixer, NULL };
    for(ALLEGRO_MIXER*** mixer = mixers; *mixer != NULL; ++mixer) {
        if(NULL == (**mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2)))
            fatal_error("Can't create an Allegro mixer");
    }

    if(!al_attach_mixer_to_voice(master_mixer, voice))
        fatal_error("Can't attach the master mixer");
    if(!al_attach_mixer_to_mixer(music_mixer, master_mixer))
        fatal_error("Can't attach the music mixer");
    if(!al_attach_mixer_to_mixer(sound_mixer, master_mixer))
        fatal_error("Can't attach the sound mixer");
    if(!al_attach_mixer_to_mixer(primary_sound_mixer, sound_mixer))
        fatal_error("Can't attach the primary sound mixer");
    if(!al_attach_mixer_to_mixer(secondary_sound_mixer, sound_mixer))
        fatal_error("Can't attach the secondary sound mixer");
    al_set_mixer_playing(secondary_sound_mixer, false);

    for(int i = 0; i < SAMPLE_POOL_SIZE; i++) {
        ALLEGRO_MIXER* parent = i < MAX_SIMULTANEOUS_SAMPLES ? primary_sound_mixer : secondary_sound_mixer;

        if(NULL == (sample_pool[i].sample_instance = al_create_sample_instance(NULL)))
            fatal_error("Can't create sample instance %d", i);
        if(!al_attach_sample_instance_to_mixer(sample_pool[i].sample_instance, parent))
            fatal_error("Can't attach sample instance %d", i);

        sample_pool[i].unique_id = UNDEFINED_ID;
        sample_pool[i].parent = parent;
    }

    init_muffler();

    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_HALT_DRAWING, NULL, handle_haltresume_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING, NULL, handle_haltresume_event);
}

/*
 * audio_release()
 * Releases the audio manager
 */
void audio_release()
{
    logfile_message("audio_release()");

    engine_remove_event_listener(ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING, NULL, handle_haltresume_event);
    engine_remove_event_listener(ALLEGRO_EVENT_DISPLAY_HALT_DRAWING, NULL, handle_haltresume_event);

    for(int i = SAMPLE_POOL_SIZE - 1; i >= 0; i--)
        al_destroy_sample_instance(sample_pool[i].sample_instance);

    al_destroy_mixer(secondary_sound_mixer);
    al_destroy_mixer(primary_sound_mixer);
    al_destroy_mixer(sound_mixer);
    al_destroy_mixer(music_mixer);
    al_destroy_mixer(master_mixer);

    al_destroy_voice(voice);

    logfile_message("audio_release() ok");
}

/*
 * audio_update()
 * Updates the audio manager
 */
void audio_update()
{
    /* when the music finishes, set current_music to NULL */
    if(current_music != NULL && !(current_music->is_paused)) {
        if(!music_is_playing()) {
            al_rewind_audio_stream(current_music->stream);
            current_music = NULL;
        }
    }
}

/*
 * audio_preload()
 * Preload samples
 */
void audio_preload()
{
    assertx(resourcemanager_is_initialized());
    logfile_message("Preloading samples...");

    /* preload the samples, so that we don't access the disk during gameplay */
    asset_foreach_file("samples/", ".wav", preload_sample, NULL, true);
    /*asset_foreach_file("samples/", ".ogg", preload_sample, NULL, true);*/
}





/*
 *
 * audio settings
 *
 */

/*
 * audio_get_master_volume()
 * Get the master volume affecting all musics and samples
 */
float audio_get_master_volume()
{
    return master_volume;
}

/*
 * audio_set_master_volume()
 * Set the master volume affecting all musics and samples
 * 0.0 <= volume <= 1.0 (default)
 */
void audio_set_master_volume(float volume)
{
    master_volume = clip(volume, 0.0f, 1.0f);
    set_master_gain(!is_globally_muted ? master_volume : 0.0f);
}

/*
 * audio_get_mixer_percentage()
 * Get the music-sfx mixer percentage
 */
float audio_get_mixer_percentage()
{
    return mixer_percentage;
}

/*
 * audio_set_mixer_percentage()
 * Set the music-sfx mixer percentage
 */
void audio_set_mixer_percentage(float percentage)
{
    float p = clip(percentage, 0.0f, 1.0f);
    float alpha = 2.0f * (p - 0.5f); /* -1 <= alpha <= 1 */

    /* 0% (only sfx, no music) ... 50% (equal music-sfx) ... 100% (only music, no sfx) */
    float music_volume = 1.0f + min(0.0f, alpha);
    float sound_volume = 1.0f - max(0.0f, alpha);

    if(!al_set_mixer_gain(music_mixer, music_volume))
        video_showmessage("Can't set the music volume to %f", music_volume);

    if(!al_set_mixer_gain(sound_mixer, sound_volume))
        video_showmessage("Can't set the sound volume to %f", sound_volume);
}

/*
 * audio_is_muted()
 * Is the audio globally muted?
 */
bool audio_is_muted()
{
    return is_globally_muted;
}

/*
 * audio_set_mute()
 * Globally mute / unmute the audio
 */
void audio_set_muted(bool muted)
{
    is_globally_muted = muted;
    set_master_gain(!is_globally_muted ? master_volume : 0.0f);
}





/*
 *
 * underwater muffler
 *
 */

/*
 * audio_muffler_profile()
 * Get the current profile of the muffler
 */
mufflerprofile_t audio_muffler_profile()
{
    return current_muffler_profile;
}

/*
 * audio_muffler_set_profile()
 * Set the profile of the muffler
 */
void audio_muffler_set_profile(mufflerprofile_t profile)
{
    /* nothing to do */
    if(current_muffler_profile == profile)
        return;

    /* log */
    logfile_message("Changing the muffler profile to %s", MUFFLER_PROFILE_NAME[profile]);

    /* update muffler */
    update_muffler(profile, current_muffler_flags);
}

/*
 * audio_muffler_is_activated()
 * Check whether or not the muffler is activated at this time
 */
bool audio_muffler_is_activated()
{
    return current_muffler_flags != MUFFLE_NOTHING;
}

/*
 * audio_muffler_activate()
 * Activate or deactivate the muffler
 */
void audio_muffler_activate(int flags)
{
    /* nothing to do */
    if(current_muffler_flags == flags)
        return;

    /* update muffler */
    update_muffler(current_muffler_profile, flags);
}






/*
 *
 * private
 *
 */

int preload_sample(const char* vpath, void* data)
{
    sound_load(vpath);
    return 0;
}

void set_master_gain(float gain)
{
    if(!al_set_mixer_gain(master_mixer, gain))
        video_showmessage("Can't set the master gain to %f", gain);
}

void handle_haltresume_event(const ALLEGRO_EVENT* event, void* context)
{
    /* pause & resume audio */
    switch(event->type) {
        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            logfile_message("Pausing the audio system...");
            al_set_mixer_playing(master_mixer, false);
            al_detach_voice(voice); /* stop streaming */
            break;

        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            logfile_message("Resuming the audio system...");
            if(!al_attach_mixer_to_voice(master_mixer, voice))
                logfile_message("AUDIO WARNING: can't reattach the master mixer to the voice");
            al_set_mixer_playing(master_mixer, true);
            break;
    }
}

samplehandle_t acquire_sample_from_pool()
{
    static uint32_t next_id = 1; /* zero is never used as an ID (NULL_SAMPLE_HANDLE) */

    for(int i = 0; i < SAMPLE_POOL_SIZE; i++) {
        if(!al_get_mixer_playing(sample_pool[i].parent))
            continue;

        if(al_get_sample_instance_playing(sample_pool[i].sample_instance))
            continue;

        sample_pool[i].unique_id = next_id++;
        return ((samplehandle_t)i << 32) | (samplehandle_t)(sample_pool[i].unique_id);
    }

    return NULL_SAMPLE_HANDLE;
}

ALLEGRO_SAMPLE_INSTANCE* get_sample_instance(samplehandle_t handle)
{
    uint32_t id = (uint32_t)(handle & 0xFFFFFFFFu);
    int i = (int)(handle >> 32);

    if(i < 0 || i >= SAMPLE_POOL_SIZE || sample_pool[i].unique_id != id)
        return NULL;

    return sample_pool[i].sample_instance;
}

void init_muffler()
{
    update_muffler(DEFAULT_MUFFLER_PROFILE, MUFFLE_NOTHING);
}

void update_muffler(mufflerprofile_t profile, int flags)
{
    current_muffler_profile = profile;
    current_muffler_flags = flags;

    /* only one mixer can be muffled at any given time */
    if((flags & MUFFLE_EVERYTHING) == MUFFLE_EVERYTHING) {
        muffle_mixer(master_mixer, profile);
        muffle_mixer(sound_mixer, MUFFLER_OFF);
        muffle_mixer(music_mixer, MUFFLER_OFF);
    }
    else if((flags & MUFFLE_SOUNDS) == MUFFLE_SOUNDS) {
        muffle_mixer(master_mixer, MUFFLER_OFF);
        muffle_mixer(sound_mixer, profile);
        muffle_mixer(music_mixer, MUFFLER_OFF);
    }
    else if((flags & MUFFLE_MUSICS) == MUFFLE_MUSICS) {
        muffle_mixer(master_mixer, MUFFLER_OFF);
        muffle_mixer(sound_mixer, MUFFLER_OFF);
        muffle_mixer(music_mixer, profile);
    }
    else {
        muffle_mixer(master_mixer, MUFFLER_OFF);
        muffle_mixer(sound_mixer, MUFFLER_OFF);
        muffle_mixer(music_mixer, MUFFLER_OFF);
    }
}

void muffle_mixer(ALLEGRO_MIXER* mixer, mufflerprofile_t profile)
{
    size_t num_channels = al_get_channel_count(al_get_mixer_channels(mixer));
    size_t depth_size = al_get_audio_depth_size(al_get_mixer_depth(mixer));

    if(num_channels != 2 || depth_size != sizeof(float))
        logfile_message("Can't set the mixer postprocess callback: num_channels = %u, depth_size = %u, sizeof(float) = %u", num_channels, depth_size, sizeof(float));
    else if(!al_set_mixer_postprocess_callback(mixer, profile != MUFFLER_OFF ? muffler_postprocess : NULL, (void*)muffler_sigma(profile)))
        logfile_message("Can't set the mixer postprocess callback.");
}

const float* muffler_sigma(mufflerprofile_t profile)
{
    /* these values were picked for a frequency of 44100 Hz */
    static const float sigma[] = {
        [MUFFLER_OFF] = 0.0f,
        [MUFFLER_LOW] = 12.5f,
        [MUFFLER_MEDIUM] = 20.0f,
        [MUFFLER_HIGH] = 25.0f /* 25: sounds good. 30: too much. */
    };

    switch(profile) {
        case MUFFLER_OFF:
        case MUFFLER_LOW:
        case MUFFLER_MEDIUM:
        case MUFFLER_HIGH:
            return &sigma[profile];

        default:
            return &sigma[DEFAULT_MUFFLER_PROFILE];
    }
}

/* this function runs in a dedicated audio thread
   Only one mixer can be muffled at any given time - notice the static variables */
void muffler_postprocess(void* buf, unsigned int num_samples, void* data)
{
    /* the input buffer is expected to be float32 stereo, where
       each sample is formatted as LR, i.e., buffer = LRLRLRLR... */
    enum {
        MAX_SAMPLES = 4096,
        MAX_SIGMA = 30,
        NUM_CHANNELS = 2,
        DEPTH_SIZE = sizeof(float)
    };

    /* read input */
    float sigma = *((const float*)data); /* no need of mutexes */

#if 0
    /* nothing to do */
    static bool is_initialized = false;
    if(sigma == 0.0f) {
        is_initialized = false;
        return;
    }
#else
    /* nothing to do */
    if(sigma == 0.0f)
        return;
#endif

    /* validate */
    if(sigma > MAX_SIGMA)
        sigma = MAX_SIGMA;

    if(num_samples > MAX_SAMPLES)
        return;

    /* calculate a Gaussian */
    static float g0[1 + 2 * (3 * MAX_SIGMA)] = { 0.0f };
    const size_t n = sizeof(g0) / sizeof(float);
    const int c = (n-1) / 2;
    static int w = -1;
    static float prev_sigma = 0.0f;

    if(fabsf(sigma - prev_sigma) > 1e-5) {
        memset(g0, 0, sizeof g0);
        w = normalized_gaussian(g0, sigma, n);
        prev_sigma = sigma;
    }

    if(w < 0) /* this shouldn't happen */
        return;

    /* store two frames of samples */
    static float samples[2 * MAX_SAMPLES * NUM_CHANNELS] = { 0.0f };
    size_t buf_size = num_samples * NUM_CHANNELS * DEPTH_SIZE;

    memcpy(samples, samples + num_samples * NUM_CHANNELS, buf_size);
    memcpy(samples + num_samples * NUM_CHANNELS, buf, buf_size);

#if 0
    /* do we really need this? no audible difference... */
    if(!is_initialized) { /* wait one more frame */
        is_initialized = true;
        return;
    }
#endif

    /* find the initial index of the output. We introduce a small delay.
       start is an even number (NUM_CHANNELS is 2) and points to a L sample */
    int window_size = 2*w + 1;
    int start = (num_samples - w) * NUM_CHANNELS; /* inclusive */

    if(window_size >= num_samples) /* this shouldn't happen */
        return;

    /* now we have window_size < num_samples */

    /*

    Let f(x) be the input signal and g(x) a Gaussian with variance sigma^2 and
    centered at zero. Compute the convolution h = f * g for each channel.

    This is a low-pass filter.

    */
    int m = num_samples * NUM_CHANNELS;
    const float* f0 = samples + start;
    const float* g = g0 + c;
    float* h = (float*)buf;

    memset(buf, 0, buf_size);

    for(int i = 0; i < m; i += NUM_CHANNELS) {
        const float* f = f0 + i;
        for(int x = -w; x <= w; x++) {
            h[i] += f[x] * g[x]; /* g[x] == g[-x] */
            h[i+1] += f[x+1] * g[x];
        }
    }
}
