/*
 * Open Surge Engine
 * audio.c - audio module
 * Copyright (C) 2008-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <allegro.h>
#include <stdlib.h>
#include "audio.h"
#include "osspec.h"
#include "stringutil.h"
#include "resourcemanager.h"
#include "logfile.h"
#include "timer.h"
#include "util.h"

#ifndef __USE_OPENAL__
#include <logg.h>
#else
#include <AL/alure.h> /* PulseAudio isn't Allegro-friendly */
#endif

/* private definitions */
#define IS_WAV(path)                (str_icmp((path)+strlen(path)-4, ".wav") == 0)
#define IS_OGG(path)                (str_icmp((path)+strlen(path)-4, ".ogg") == 0)
#define IS_VALID_FORMAT(path)       (IS_OGG(path) || IS_WAV(path))
#define SOUND_INVALID_VOICE         -1
#define PREFERRED_NUMBER_OF_VOICES  16

/* private stuff */
#ifndef __USE_OPENAL__
struct music_t {
    LOGG_Stream *stream;
    int loops_left;
    int is_paused;
    float elapsed_time;
};

struct sound_t {
    SAMPLE *data;
    int voice_id;
};

#define MUSIC_DURATION(m)           ((float)(m->stream->len) / (float)(m->stream->freq))
#else
struct music_t {
    alureStream *stream;
    volatile int is_paused;
    volatile int is_playing;
};

struct sound_t {
    ALuint buf; /* sound buffer */
    volatile ALuint *src; /* points to an element of src[] */
    volatile int loops_left;
    volatile int is_playing;
    volatile float vol, pan, freq;
};

static int quiet;

static ALuint src[PREFERRED_NUMBER_OF_VOICES]; /* audio sources for sound_t's */
static int src_count; /* number of valid elements of src[] */
static int src_ptr; /* next src[] index to be used */
static ALuint srcmus; /* audio source for the music */

static ALuint *sbuf; /* array of ALuint: will hold all created buffers */
static int sbuf_capacity; /* capacity of sbuf[] */
static int sbuf_count; /* length of sbuf[] */

static void eos_callback(void *userdata, ALuint source);
static void eom_callback(void *userdata, ALuint source);

#define NUM_BUFS 3
#endif


/* private stuff*/
static music_t *current_music; /* music being played at the moment (NULL if none) */


/*
 * music_load()
 * Loads a music from a file
 */
#ifndef __USE_OPENAL__
music_t *music_load(const char *path)
{
    char abs_path[1024];
    music_t *m;

    if(NULL == (m = resourcemanager_find_music(path))) {
        resource_filepath(abs_path, path, sizeof(abs_path), RESFP_READ);
        logfile_message("music_load('%s')", abs_path);

        /* build the music object */
        m = mallocx(sizeof *m);
        m->loops_left = 0;
        m->is_paused = FALSE;
        m->elapsed_time = 0.0f;

        /* load the ogg stream */
        m->stream = logg_get_stream(abs_path, 255, 128, 0);
        if(m->stream == NULL) {
            logfile_message("music_load() error: can't get ogg stream");
            free(m);
            return NULL;
        }

        /* adding it to the resource manager */
        resourcemanager_add_music(path, m);
        resourcemanager_ref_music(path);

        /* done! */
        logfile_message("music_load() ok");
    }
    else
        resourcemanager_ref_music(path);

    return m;
}
#else
music_t *music_load(const char *path)
{
    char abs_path[1024];
    music_t *m;

    if(NULL == (m = resourcemanager_find_music(path))) {
        resource_filepath(abs_path, path, sizeof(abs_path), RESFP_READ);
        logfile_message("music_load('%s')", abs_path);

        /* build the music object */
        m = mallocx(sizeof *m);
        m->is_playing = FALSE;
        m->is_paused = FALSE;

        /* load the stream */
        if(!(IS_VALID_FORMAT(path) && (m->stream = alureCreateStreamFromFile(abs_path, 250000, 0, NULL)))) {

            if(!IS_VALID_FORMAT(path)) {
                logfile_message("music_load() invalid file format");
                alureDestroyStream(m->stream, 0, NULL);
            }
            else
                logfile_message("music_load() error: %s", alureGetErrorString());

            free(m);
            return NULL;
        }

        /* adding it to the resource manager */
        resourcemanager_add_music(path, m);
        resourcemanager_ref_music(path);

        /* done! */
        logfile_message("music_load() ok");
    }
    else
        resourcemanager_ref_music(path);

    return m;
}
#endif



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
#ifndef __USE_OPENAL__
int music_unref(const char *path)
{
    return resourcemanager_unref_music(path);
}
#else
int music_unref(const char *path)
{
    return resourcemanager_unref_music(path);
}
#endif



/*
 * music_destroy()
 * Destroys a music. This is called automatically
 * while unloading the resource manager.
 */
#ifndef __USE_OPENAL__
void music_destroy(music_t *music)
{
    if(music != NULL) {
        if(music == current_music)
            music_stop();

        logg_destroy_stream(music->stream);
        free(music);
    }
}
#else
void music_destroy(music_t *music)
{
    if(music != NULL) {
        if(music == current_music)
            music_stop();

        alureDestroyStream(music->stream, 0, NULL);
        free(music);
    }
}
#endif


/*
 * music_play()
 * Plays the given music and loops [loop] times.
 * Set loop equal to INFINITY to make it loop forever.
 */
#ifndef __USE_OPENAL__
void music_play(music_t *music, int loop)
{
    music_stop();

    if(music != NULL) {
        music->loops_left = loop;
        music->is_paused = FALSE;
        music->elapsed_time = 0.0f;
        music->stream->loop = (loop >= INFINITY); /* "gambiarra", because LOGG lacks features */
    }

    current_music = music;
    music_set_volume(1.0f);
}
#else
void music_play(music_t *music, int loop)
{
    music_stop();

    if(music != NULL) {
        if(alurePlaySourceStream(srcmus, music->stream, NUM_BUFS, loop, eom_callback, (void*)music) == AL_FALSE) {
            current_music = NULL;
            return;
        }

        music->is_playing = TRUE;
        music->is_paused = FALSE;
    }

    current_music = music;
    music_set_volume(1.0f);
}

void eom_callback(void *userdata, ALuint source)
{
    music_t *music = (music_t*)userdata;

    if(music != NULL) {
        alureRewindStream(music->stream);
        music->is_playing = FALSE;
        music->is_paused = FALSE;
    }

    (void)source;
}
#endif


/*
 * music_stop()
 * Stops the current music (if any)
 */
#ifndef __USE_OPENAL__
void music_stop()
{
    if(current_music != NULL) {
        char *filename = str_dup(current_music->stream->filename);
        logg_destroy_stream(current_music->stream);
        current_music->stream = logg_get_stream(filename, 255, 128, 0);
        free(filename);
    }

    current_music = NULL;
}
#else
void music_stop()
{
    if(current_music != NULL)
        alureStopSource(srcmus, AL_TRUE); /* will call eom_callback */

    current_music = NULL;
}
#endif


/*
 * music_pause()
 * Pauses the current music
 */
#ifndef __USE_OPENAL__
void music_pause()
{
    if(current_music != NULL && !(current_music->is_paused)) {
        current_music->is_paused = TRUE;
        voice_stop(current_music->stream->audio_stream->voice);
    }
}
#else
void music_pause()
{
    if(current_music != NULL && current_music->is_playing && !(current_music->is_paused)) {
        current_music->is_paused = TRUE;
        alurePauseSource(srcmus);
    }
}
#endif



/*
 * music_resume()
 * Resumes the current music
 */
#ifndef __USE_OPENAL__
void music_resume()
{
    if(current_music != NULL && current_music->is_paused) {
        current_music->is_paused = FALSE;
        voice_start(current_music->stream->audio_stream->voice);
    }
}
#else
void music_resume()
{
    if(current_music != NULL && current_music->is_playing && current_music->is_paused) {
        current_music->is_paused = FALSE;
        alureResumeSource(srcmus);
    }
}
#endif


/*
 * music_set_volume()
 * Changes the volume of the current music.
 * 0.0f (quiet) <= volume <= 1.0f (loud)
 * default = 1.0f
 */
#ifndef __USE_OPENAL__
void music_set_volume(float volume)
{
    if(current_music != NULL) {
        volume = clip(volume, 0.0f, 1.0f);
        current_music->stream->volume = (int)(255.0f * volume);
        voice_set_volume(current_music->stream->audio_stream->voice, current_music->stream->volume);
    }
}
#else
void music_set_volume(float volume)
{
    if(current_music != NULL) {
        volume = clip(volume, 0.0f, 1.0f);
        alSourcef(srcmus, AL_GAIN, volume);
    }
}
#endif


/*
 * music_get_volume()
 * Returns the volume of the current music.
 * 0.0f <= volume <= 1.0f
 */
#ifndef __USE_OPENAL__
float music_get_volume()
{
    if(current_music != NULL)
        return (float)current_music->stream->volume / 255.0f;
    else
        return 0.0f;
}
#else
float music_get_volume()
{
    if(current_music != NULL) {
        float vol = 1.0f;
        alGetSourcef(srcmus, AL_GAIN, &vol);
        return vol;
    }
    else
        return 0.0f;
}
#endif



/*
 * music_is_playing()
 * Returns TRUE if a music is playing, FALSE
 * otherwise.
 */
#ifndef __USE_OPENAL__
int music_is_playing()
{
    return (current_music != NULL) && !(current_music->is_paused) && (current_music->loops_left >= 0);
}
#else
int music_is_playing()
{
    return (current_music != NULL) && !(current_music->is_paused) && (current_music->is_playing);
}
#endif


/*
 * music_duration()
 * Music duration, in seconds (it's an approximation, in reality)
 */
#ifndef __USE_OPENAL__
float music_duration()
{
    return current_music ? MUSIC_DURATION(current_music) : 0.0f;
}
#else
float music_duration()
{
    if(current_music != NULL) {
        /* the length of a sample
        ALint buf, bufSize, frequency, bitsPerSample, channels;
        alGetSourcei(srcmus, AL_BUFFER, &buf);
        alGetBufferi(buf, AL_SIZE, &bufSize);
        alGetBufferi(buf, AL_FREQUENCY, &frequency);
        alGetBufferi(buf, AL_CHANNELS, &channels);    
        alGetBufferi(buf, AL_BITS, &bitsPerSample);    
        return ((float)bufSize) / (frequency * channels * (bitsPerSample / 8)); */
        ALint buf, freq;
        alureInt64 numberOfSamples = alureGetStreamLength(current_music->stream);
        alGetSourcei(srcmus, AL_BUFFER, &buf);
        alGetBufferi(buf, AL_FREQUENCY, &freq); /* samples per second */
        return (float)(numberOfSamples / ((alureInt64)freq));
    }
    else
        return 0.0f;
}
#endif




/* sound management */


/*
 * sound_load()
 * Loads a sample from a file
 */
#ifndef __USE_OPENAL__
sound_t *sound_load(const char *path)
{
    char abs_path[1024];
    sound_t *s;

    if(NULL == (s = resourcemanager_find_sample(path))) {
        resource_filepath(abs_path, path, sizeof(abs_path), RESFP_READ);
        logfile_message("sound_load('%s')", abs_path);

        /* build the sound object */
        s = mallocx(sizeof *s);
        s->voice_id = SOUND_INVALID_VOICE;

        /* loading the sample */
        if(NULL == (s->data = IS_OGG(path) ? logg_load(abs_path) : load_sample(abs_path))) {
            logfile_message("sound_load() error: %s", allegro_error);
            free(s);
            return NULL;
        }

        /* adding it to the resource manager */
        resourcemanager_add_sample(path, s);
        resourcemanager_ref_sample(path);

        /* done! */
        logfile_message("sound_load() ok");
    }
    else
        resourcemanager_ref_sample(path);

    return s;
}
#else
sound_t *sound_load(const char *path)
{
    char abs_path[1024];
    sound_t *s;

    if(quiet)
        return NULL;

    if(NULL == (s = resourcemanager_find_sample(path))) {
        resource_filepath(abs_path, path, sizeof(abs_path), RESFP_READ);
        logfile_message("sound_load('%s')", abs_path);

        /* build the sound object */
        s = mallocx(sizeof *s);
        s->loops_left = 0;
        s->is_playing = FALSE;
        s->src = NULL;

        /* loading the sample */
        if(!(IS_VALID_FORMAT(path) && (s->buf = alureCreateBufferFromFile(abs_path)))) {
            
            if(!IS_VALID_FORMAT(path)) {
                logfile_message("sound_load() error: invalid file format");
                alDeleteBuffers(1, &(s->buf));
            }
            else
                logfile_message("sound_load() error: %s", alureGetErrorString());

            free(s);
            return NULL;
        }

        /* we'll need to release the array of buffers later */
        sbuf[sbuf_count] = s->buf;
        if(++sbuf_count >= sbuf_capacity) {
            sbuf_capacity *= 2;
            logfile_message("Expanding the array of audio buffers to hold %d elements", sbuf_capacity);
            sbuf = reallocx(sbuf, sbuf_capacity * sizeof(*sbuf));
        }

        /* adding it to the resource manager */
        resourcemanager_add_sample(path, s);
        resourcemanager_ref_sample(path);

        /* done! */
        logfile_message("sound_load() ok");
    }
    else
        resourcemanager_ref_sample(path);

    return s;
}
#endif

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
#ifndef __USE_OPENAL__
int sound_unref(const char *path)
{
    return resourcemanager_unref_sample(path);
}
#else
int sound_unref(const char *path)
{
    return resourcemanager_unref_sample(path);
}
#endif


/*
 * sound_destroy()
 * Releases the given sample. This is called
 * automatically while releasing the main hash
 */
#ifndef __USE_OPENAL__
void sound_destroy(sound_t *sample)
{
    if(sample != NULL) {
        sound_stop(sample);
        destroy_sample(sample->data);
        free(sample);
    }
}
#else
void sound_destroy(sound_t *sample)
{
    if(sample != NULL) {
        sound_stop(sample);
        /* the buffers will be deleted later, in audio_release() */
        free(sample);
    }
}
#endif


/*
 * sound_play()
 * Plays the given sample
 */
#ifndef __USE_OPENAL__
void sound_play(sound_t *sample)
{
    sound_play_ex(sample, 1.0, 0.0, 1.0, 0);
}
#else
void sound_play(sound_t *sample)
{
    sound_play_ex(sample, 1.0, 0.0, 1.0, 0);
}
#endif


/*
 * sound_play_ex()
 * Plays the given sample with extra options! :)
 *
 * 0.0 <= volume <= 1.0
 * (left speaker) -1.0 <= pan <= 1.0 (right speaker)
 * 1.0 = default frequency
 * 0 = no loops
 */
#ifndef __USE_OPENAL__
void sound_play_ex(sound_t *sample, float vol, float pan, float freq, int loop)
{
    int id;

    if(sample) {
        /* ajusting parameters */
        vol = clip(vol, 0.0f, 1.0f);
        pan = clip(pan, -1.0f, 1.0f);
        freq = max(freq, 0.0f);
        loop = (loop < 0) ? -1 : loop;

        /* playing the sample */
        id = play_sample(sample->data, (int)(255.0f * vol), min(255.0f, 128.0f + (int)(128.0f * pan)), (int)(1000.0f * freq), loop);
        sample->voice_id = id < 0 ? SOUND_INVALID_VOICE : id;
    }
}
#else
void sound_play_ex(sound_t *sample, float vol, float pan, float freq, int loop)
{
    if(sample) {
        /* ajusting parameters */
        vol = clip(vol, 0.0f, 1.0f);
        pan = clip(pan, -1.0f, 1.0f);
        freq = max(freq, 0.0f);
        loop = (loop < 0) ? -1 : loop;

        /* saving the parameters */
        sample->loops_left = loop;
        sample->vol = vol;
        sample->pan = pan;
        sample->freq = freq;

        /* find a sound source */
        src_ptr = (src_ptr + 1) % src_count;
        sample->src = src + src_ptr;
        alureStopSource(*(sample->src), AL_FALSE);

        /* configuring... */
        alSourcei(*(sample->src), AL_BUFFER, sample->buf);
        alSourcef(*(sample->src), AL_GAIN, vol);
        alSourcef(*(sample->src), AL_PITCH, freq); /* I hope this is correct... ;) */
        alSource3f(*(sample->src), AL_POSITION, 1.0f * pan, 0.0f, 0.0f); /* ??????? */

        /* playing the sample */
        if(alurePlaySource(*(sample->src), eos_callback, (void*)sample) != AL_FALSE)
            sample->is_playing = TRUE;
        else
            sample->is_playing = FALSE;
    }
}

/* gets called when a sample finishes playing */
void eos_callback(void *userdata, ALuint source)
{
    sound_t *sample = (sound_t*)userdata;

    if(sample != NULL) {
        if(sample->loops_left > 0)
            sound_play_ex(sample, sample->vol, sample->pan, sample->freq, sample->loops_left-1);
        else
            sample->is_playing = FALSE;
    }

    (void)source;
}
#endif



/*
 * sound_stop()
 * Stops a sample
 */
#ifndef __USE_OPENAL__
void sound_stop(sound_t *sample)
{
    if(sample)
        stop_sample(sample->data);
}
#else
void sound_stop(sound_t *sample)
{
    if(sample && sample->is_playing) {
        alureStopSource(*(sample->src), AL_FALSE);
        sample->is_playing = FALSE;
    }
}
#endif


/*
 * sound_is_playing()
 * Checks if a given sound is playing or not
 */
#ifndef __USE_OPENAL__
int sound_is_playing(sound_t *sample)
{
    if(sample && sample->voice_id != SOUND_INVALID_VOICE)
        return (voice_check(sample->voice_id) == sample->data);
    else
        return FALSE;
}
#else
int sound_is_playing(sound_t *sample)
{
    if(sample)
        return sample->is_playing;
    else
        return FALSE;
}
#endif








/* audio manager */

/*
 * audio_init()
 * Initializes the Audio Manager
 */
#ifndef __USE_OPENAL__
void audio_init()
{
    int voices;
    logfile_message("audio_init(): using Allegro for audio playback...");
    current_music = NULL;

    /* allocates a few voices */
    voices = PREFERRED_NUMBER_OF_VOICES;
    logfile_message("Reserving voices...");
    while(voices > 4) {
        reserve_voices(voices, 0);
        if(install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) == 0) {
            logfile_message("Reserved %d voices.", voices);
            logfile_message("audio_init() ok");
            return;
        }
        else
            voices /= 2;
    }
    logfile_message("Warning: unable to reserve voices.\n%s\n", allegro_error);
}
#else
void audio_init()
{
    int sources;

    logfile_message("audio_init(): using OpenAL for audio playback...");
    quiet = TRUE;

    /* initialize the OpenAL device */
    if(alureInitDevice(NULL, NULL)) {
        /* allocating some buffers */
        sbuf_capacity = 1024; /* initial capacity; the sbuf[] array grows as needed */
        sbuf_count = 0;
        logfile_message("Allocating %d audio buffers...", sbuf_capacity);
        sbuf = mallocx(sbuf_capacity * sizeof(*sbuf));

        /* allocates a few audio sources */
        sources = PREFERRED_NUMBER_OF_VOICES;
        logfile_message("Generating audio sources...");
        while(sources > 4) {
            alGenSources(sources, src);
            if(alGetError() == AL_NO_ERROR) {
                logfile_message("%d sources have been generated.", src_count = sources);
                alGenSources(1, &srcmus);
                if(alGetError() == AL_NO_ERROR) {
                    logfile_message("audio_init() ok");
                    src_ptr = 0;
                    quiet = FALSE;
                    alureStreamSizeIsMicroSec(AL_TRUE);
                    alureUpdateInterval(0.125f);
                    return;
                }
                else
                    logfile_message("Can't generate audio source for the music: %s", alureGetErrorString());
            }
            else
                sources /= 2;
        }

        logfile_message("Failed to generate audio sources: %s", alureGetErrorString());
        alureShutdownDevice();
    }
    else
        logfile_message("Failed to open OpenAL device: %s", alureGetErrorString());
}
#endif


/*
 * audio_release()
 * Releases the audio manager
 */
#ifndef __USE_OPENAL__
void audio_release()
{
    logfile_message("audio_release()");
    logfile_message("audio_release() ok");
}
#else
void audio_release()
{
    logfile_message("audio_release()");

    if(!quiet) {
        logfile_message("Deleting audio buffers...");
        alDeleteBuffers(sbuf_count, sbuf);
        free(sbuf);
        logfile_message("Deleting audio sources...");
        alDeleteSources(src_count, src);
        logfile_message("Shutting down OpenAL...");
        alureShutdownDevice();
    }

    logfile_message("audio_release() ok");
}
#endif


/*
 * audio_update()
 * Updates the audio manager
 */
#ifndef __USE_OPENAL__
void audio_update()
{
    /* updating the music */
    if(current_music != NULL && !(current_music->is_paused)) {
        logg_update_stream(current_music->stream);

        /* "gambiarra" (ugly hack), because LOGG lacks features */
        if(!(current_music->stream->loop)) {
            current_music->elapsed_time += timer_get_delta();
            if(current_music->elapsed_time >= MUSIC_DURATION(current_music) + 0.25f) {
                if(--(current_music->loops_left) >= 0)
                    music_play(current_music, current_music->loops_left);
                else
                    music_stop();
            }
        }
    }
}
#else
void audio_update()
{
    /* alureUpdate() somehow won't call the eos_callbacks... */
    /*if(!quiet)
        alureUpdate();*/
}
#endif
