/*
 * Open Surge Engine
 * audio.c - audio module
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#include "asset.h"
#include "resourcemanager.h"
#include "logfile.h"
#include "timer.h"
#include "video.h"
#include "../util/util.h"
#include "../util/stringutil.h"

#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

/* music structure */
struct music_t {
    ALLEGRO_AUDIO_STREAM* stream;
    bool is_paused;
    char* filepath; /* relative path */
};

/* sound structure */
struct sound_t {
    ALLEGRO_SAMPLE* sample;
    ALLEGRO_SAMPLE_ID id;
    bool valid_id;
    float duration;
    float end_time;
    float volume; /* 0: silence; 1: default */
    char* filepath; /* relative path */
};

/* private stuff */
static const int PREFERRED_NUMBER_OF_SAMPLES = 16; /* how many samples can be played at the same time */

static ALLEGRO_VOICE* voice = NULL;
static ALLEGRO_MIXER* master_mixer = NULL;
static ALLEGRO_MIXER* music_mixer = NULL;
static ALLEGRO_MIXER* sound_mixer = NULL;

static music_t *current_music = NULL; /* music being played at the moment (NULL if none) */
static float master_volume = 1.0f; /* a value in [0,1] affecting all musics and sounds */
static bool globally_muted = false; /* global mute / unmute */

static int preload_sample(const char* vpath, void* data);
static void set_master_gain(float gain);

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




/* sound management */


/*
 * sound_load()
 * Loads a sample from a file
 */
sound_t *sound_load(const char *path)
{
    sound_t *s;

    if(NULL == (s = resourcemanager_find_sample(path))) {
        ALLEGRO_SAMPLE_INSTANCE* spl;
        const char* fullpath = asset_path(path);
        logfile_message("Loading sound \"%s\"...", fullpath);

        /* build the sound object */
        s = mallocx(sizeof *s);
        s->duration = 0.0f;
        s->end_time = 0.0f;
        s->valid_id = false;
        s->volume = 1.0f;
        s->filepath = str_dup(path);
        if(NULL == (s->sample = al_load_sample(fullpath)))
            fatal_error("Can't load sound \"%s\"", path);

        /* compute its duration */
        if(NULL != (spl = al_create_sample_instance(s->sample))) {
            s->duration = al_get_sample_instance_time(spl);
            al_destroy_sample_instance(spl);
        }

        /* adding it to the resource manager */
        resourcemanager_add_sample(path, s);
        resourcemanager_ref_sample(path);
    }
    else
        resourcemanager_ref_sample(path);

    return s;
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
int sound_unref(sound_t* sample)
{
    return resourcemanager_unref_sample(sample->filepath);
}

/*
 * sound_destroy()
 * Releases the given sample. This is called
 * automatically when releasing the main hash
 */
void sound_destroy(sound_t *sample)
{
    if(sample != NULL) {
        sound_stop(sample);
        al_destroy_sample(sample->sample);
        free(sample->filepath);
        free(sample);
    }
}

/*
 * sound_play()
 * Plays the given sample
 */
void sound_play(sound_t *sample)
{
    if(sample != NULL)
        sound_play_ex(sample, sample->volume, 0.0f, 1.0f);
}

/*
 * sound_play_ex()
 * Plays the given sample with extra options
 *
 * 0.0 <= volume (defaults to 1.0)
 * (left speaker) -1.0 <= pan <= 1.0 (right speaker)
 * 1.0 = default frequency
 */
void sound_play_ex(sound_t *sample, float vol, float pan, float freq)
{
    if(sample != NULL) {
        /* adjusting the parameters */
        vol = max(vol, 0.0f);
        pan = clip(pan, -1.0f, 1.0f);
        freq = max(freq, 0.0f);

        /* play the sample */
        if(al_play_sample(sample->sample, vol, pan, freq, ALLEGRO_PLAYMODE_ONCE, &sample->id)) {
            sample->end_time = timer_get_elapsed() + sample->duration; /* when does it end? */
            sample->valid_id = true;
            sample->volume = vol;
        }
        else {
            sample->end_time = 0.0f;
            sample->valid_id = false;
            sample->volume = vol;
        }
    }
}

/*
 * sound_stop()
 * Stops a sample
 */
void sound_stop(sound_t *sample)
{
    if(sample != NULL) {
        if(sample->valid_id) {
            al_stop_sample(&sample->id);
            sample->valid_id = false;
            sample->end_time = 0.0f;
        }
    }
}

/*
 * sound_is_playing()
 * Checks if a sound is playing
 */
bool sound_is_playing(sound_t *sample)
{
    if(sample != NULL)
        return timer_get_elapsed() < sample->end_time;
    else
        return false;

#if 0
    /* Unstable API (Allegro 5.2.3) */
    if(sample != NULL) {
        if(sample->valid_id) {
            ALLEGRO_SAMPLE_INSTANCE* instance = al_lock_sample_id(&sample->id);
            if(instance != NULL) {
                bool is_playing = al_get_sample_instance_playing(instance);
                al_unlock_sample_id(&sample->id);
                return is_playing;
            }
            else
                return false;
        }
    }

    return false;
#endif
}

/*
 * sound_get_volume()
 * Gets the volume of a sound.
 * 0.0f means silence; 1.0f, the default volume
 */
float sound_get_volume(sound_t *sample)
{
    if(sample != NULL)
        return sample->volume;
    else
        return 0.0f;
}

/*
 * sound_set_volume()
 * Sets the volume of a sound.
 */
void sound_set_volume(sound_t *sample, float volume)
{
    /* Unstable API (since Allegro 5.2.3) */
    if(sample != NULL) {
        sample->volume = max(0.0f, volume);
        if(sample->valid_id) {
            ALLEGRO_SAMPLE_INSTANCE* instance = al_lock_sample_id(&sample->id);
            if(instance != NULL) {
                if(al_get_sample_instance_playing(instance))
                    al_set_sample_instance_gain(instance, sample->volume);
                al_unlock_sample_id(&sample->id);
            }
        }
    }
}




/* audio manager */

/*
 * audio_init()
 * Initializes the Audio Manager
 */
void audio_init()
{
    logfile_message("Initializing the audio system...");

    current_music = NULL;
    master_volume = 1.0f;
    globally_muted = false;

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

    ALLEGRO_MIXER** mixers[] = { &master_mixer, &music_mixer, &sound_mixer, NULL };
    for(ALLEGRO_MIXER*** mixer = mixers; *mixer != NULL; ++mixer) {
        if(NULL == (**mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2)))
            fatal_error("Can't create an Allegro mixer");
    }

    if(!al_attach_mixer_to_mixer(sound_mixer, master_mixer))
        fatal_error("Can't attach the sample mixer to the master mixer");
    if(!al_attach_mixer_to_mixer(music_mixer, master_mixer))
        fatal_error("Can't attach the music mixer to the master mixer");
    if(!al_attach_mixer_to_voice(master_mixer, voice))
        fatal_error("Can't attach the master mixer to the voice");

    al_set_default_voice(voice);
    if(!al_set_default_mixer(sound_mixer))
        fatal_error("Can't set the default mixer");

    for(int samples = PREFERRED_NUMBER_OF_SAMPLES; samples > 0; samples /= 2) {
        if(al_reserve_samples(samples)) {
            logfile_message("Reserved %d samples", samples);
            break;
        }
        else
            logfile_message("Can't reserve %d samples", samples);
    }
}

/*
 * audio_release()
 * Releases the audio manager
 */
void audio_release()
{
    logfile_message("audio_release()");

    al_destroy_mixer(music_mixer);
    al_destroy_mixer(sound_mixer);
    al_destroy_mixer(master_mixer);

    /*al_destroy_voice(voice);*/ /* crash */
    al_set_default_voice(NULL); /* use this instead */

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
    set_master_gain(!globally_muted ? master_volume : 0.0f);
}

/*
 * audio_get_music_volume()
 * Get the volume of the music mixer
 */
float audio_get_music_volume()
{
    return al_get_mixer_gain(music_mixer);
}

/*
 * audio_set_music_volume()
 * Set the volume of the music mixer
 */
void audio_set_music_volume(float volume)
{
    float gain = clip(volume, 0.0f, 1.0f);

    if(!al_set_mixer_gain(music_mixer, gain))
        video_showmessage("Can't set the music gain to %f", gain);
}

/*
 * audio_get_sound_volume()
 * Get the volume of the sound mixer
 */
float audio_get_sound_volume()
{
    return al_get_mixer_gain(sound_mixer);
}

/*
 * audio_set_sound_volume()
 * Set the volume of the sound mixer
 */
void audio_set_sound_volume(float volume)
{
    float gain = clip(volume, 0.0f, 1.0f);

    if(!al_set_mixer_gain(sound_mixer, gain))
        video_showmessage("Can't set the sound gain to %f", gain);
}

/*
 * audio_is_muted()
 * Is the audio globally muted?
 */
bool audio_is_muted()
{
    return globally_muted;
}

/*
 * audio_set_mute()
 * Globally mute / unmute the audio
 */
void audio_set_muted(bool muted)
{
    globally_muted = muted;
    set_master_gain(!globally_muted ? master_volume : 0.0f);
}



/* private */

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