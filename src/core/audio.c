/*
 * Open Surge Engine
 * audio.c - audio module
 * Copyright (C) 2008-2012, 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "assetfs.h"
#include "stringutil.h"
#include "resourcemanager.h"
#include "logfile.h"
#include "timer.h"
#include "util.h"

#if defined(A5BUILD)

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
    char* filepath; /* relative path */
};

/* private stuff */
static const int PREFERRED_NUMBER_OF_SAMPLES = 16; /* how many samples can be played at the same time */
static music_t *current_music = NULL; /* music being played at the moment (NULL if none) */

#elif !defined(__USE_OPENAL__)

#include <allegro.h>
#include <logg.h>

/* private definitions */
#define IS_WAV(path)                (strchr((path), '.') && str_icmp(strrchr((path), '.'), ".wav") == 0)
#define IS_OGG(path)                (strchr((path), '.') && str_icmp(strrchr((path), '.'), ".ogg") == 0)
#define IS_VALID_FORMAT(path)       (IS_OGG(path) || IS_WAV(path))
#define SOUND_INVALID_VOICE         -1
#define PREFERRED_NUMBER_OF_VOICES  16
#define MUSIC_DURATION(m)           ((float)(m->stream->len) / (float)(m->stream->freq))

/* music structure */
struct music_t {
    LOGG_Stream *stream;
    int is_paused;
    float elapsed_time;
    char *filepath; /* relative */
};

/* sound structure */
struct sound_t {
    SAMPLE *data;
    int voice_id;
    float vol;
    char *filepath; /* relative */
};

/* private stuff */
static music_t *current_music; /* music being played at the moment (NULL if none) */

#else

#include <allegro.h>
#include <AL/alure.h> /* PulseAudio isn't Allegro-friendly */

/* private definitions */
#define IS_WAV(path)                (strchr((path), '.') && str_icmp(strrchr((path), '.'), ".wav") == 0)
#define IS_OGG(path)                (strchr((path), '.') && str_icmp(strrchr((path), '.'), ".ogg") == 0)
#define IS_VALID_FORMAT(path)       (IS_OGG(path) || IS_WAV(path))
#define SOUND_INVALID_VOICE         -1
#define PREFERRED_NUMBER_OF_VOICES  16
#define NUM_BUFS 3

/* music structure */
struct music_t {
    alureStream *stream;
    volatile int is_paused;
    volatile int is_playing;
    char *filepath; /* relative */
};

/* sound structure */
struct sound_t {
    ALuint buf; /* sound buffer */
    volatile ALuint *src; /* points to an element of src[] */
    volatile int is_playing;
    volatile float vol, pan, freq;
    char *filepath; /* relative */
};

/* private stuff */
static music_t *current_music; /* music being played at the moment (NULL if none) */

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

#endif


/*
 * music_load()
 * Loads a music from a file
 */
#if defined(A5BUILD)
music_t *music_load(const char *path)
{
    music_t *m;

    if(*path == '\0') /* empty path */
        return NULL;

    if(NULL == (m = resourcemanager_find_music(path))) {
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("Loading music \"%s\"...", fullpath);

        /* build the music object */
        m = mallocx(sizeof *m);
        m->is_paused = false;
        m->filepath = str_dup(path);
        if(NULL == (m->stream = al_load_audio_stream(fullpath, 4, 1024)))
            fatal_error("Can't load music \"%s\"", path);
        
        /* configure the audio stream */
        al_attach_audio_stream_to_mixer(m->stream, al_get_default_mixer());
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
#elif !defined(__USE_OPENAL__)
music_t *music_load(const char *path)
{
    music_t *m;

    if(*path == '\0') /* empty path */
        return NULL;

    if(NULL == (m = resourcemanager_find_music(path))) {
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("music_load('%s')", fullpath);

        /* build the music object */
        m = mallocx(sizeof *m);
        m->is_paused = FALSE;
        m->elapsed_time = 0.0f;
        m->filepath = str_dup(path);

        /* load the ogg stream */
        m->stream = logg_get_stream(fullpath, 255, 128, 0);
        if(m->stream == NULL) {
            logfile_message("music_load() error: can't get ogg stream");
            free(m->filepath);
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
    music_t *m;

    if(*path == '\0') /* empty path */
        return NULL;

    if(NULL == (m = resourcemanager_find_music(path))) {
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("music_load('%s')", fullpath);

        /* build the music object */
        m = mallocx(sizeof *m);
        m->is_playing = FALSE;
        m->is_paused = FALSE;
        m->filepath = str_dup(path);

        /* load the stream */
        if(!(IS_VALID_FORMAT(path) && (m->stream = alureCreateStreamFromFile(fullpath, 250000, 0, NULL)))) {

            if(!IS_VALID_FORMAT(path)) {
                logfile_message("music_load() invalid file format");
                alureDestroyStream(m->stream, 0, NULL);
            }
            else
                logfile_message("music_load() error: %s", alureGetErrorString());

            free(m->filepath);
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
#if defined(A5BUILD)
int music_unref(music_t *music)
{
    if(music != NULL)
        return resourcemanager_unref_music(music->filepath);
    return 0;
}
#elif !defined(__USE_OPENAL__)
int music_unref(music_t *music)
{
    if(music != NULL)
        return resourcemanager_unref_music(music->filepath);
    return 0;
}
#else
int music_unref(music_t *music)
{
    if(music != NULL)
        return resourcemanager_unref_music(music->filepath);
    return 0;
}
#endif



/*
 * music_destroy()
 * Destroys a music. This is called automatically
 * while unloading the resource manager.
 */
#if defined(A5BUILD)
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
#elif !defined(__USE_OPENAL__)
void music_destroy(music_t *music)
{
    if(music != NULL) {
        if(music == current_music)
            music_stop();

        logg_destroy_stream(music->stream);
        free(music->filepath);
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
        free(music->filepath);
        free(music);
    }
}
#endif


/*
 * music_play()
 * Plays a music.
 * Set loop to TRUE to make it loop continuously.
 */
#if defined(A5BUILD)
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
#elif !defined(__USE_OPENAL__)
void music_play(music_t *music, bool loop)
{
    music_stop();

    if(music != NULL) {
        music->is_paused = FALSE;
        music->elapsed_time = 0.0f;
        music->stream->loop = loop;
    }

    current_music = music;
    music_set_volume(1.0f);
}
#else
void music_play(music_t *music, bool loop)
{
    music_stop();

    if(music != NULL) {
        if(alurePlaySourceStream(srcmus, music->stream, NUM_BUFS, loop ? -1 : 0, eom_callback, (void*)music) == AL_FALSE) {
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
#if defined(A5BUILD)
void music_stop()
{
    if(current_music != NULL) {
        al_set_audio_stream_playing(current_music->stream, false);
        al_rewind_audio_stream(current_music->stream);
        current_music->is_paused = false; /* it's stopped, not paused */
    }

    current_music = NULL;
}
#elif !defined(__USE_OPENAL__)
void music_stop()
{
    if(current_music != NULL) {
        char *filename = str_dup(current_music->stream->filename);
        logg_destroy_stream(current_music->stream);
        current_music->stream = logg_get_stream(filename, 255, 128, 0);
        free(filename);
    }

    current_music = NULL; /* important */
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
#if defined(A5BUILD)
void music_pause()
{
    if(current_music != NULL && !(current_music->is_paused)) {
        al_set_audio_stream_playing(current_music->stream, false);
        current_music->is_paused = true;
    }
}
#elif !defined(__USE_OPENAL__)
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
#if defined(A5BUILD)
void music_resume()
{
    if(current_music != NULL && current_music->is_paused) {
        al_set_audio_stream_playing(current_music->stream, true);
        current_music->is_paused = false;
    }
}
#elif !defined(__USE_OPENAL__)
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
 * zero means silence; 1.0 means default volume
 */
#if defined(A5BUILD)
void music_set_volume(float volume)
{
    if(current_music != NULL) {
        float gain = max(volume, 0.0f);
        al_set_audio_stream_gain(current_music->stream, gain);
    }
}
#elif !defined(__USE_OPENAL__)
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
 */
#if defined(A5BUILD)
float music_get_volume()
{
    if(current_music != NULL)
        return al_get_audio_stream_gain(current_music->stream);
    else
        return 0.0f;
}
#elif !defined(__USE_OPENAL__)
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
 * Checks if a music is playing
 */
#if defined(A5BUILD)
bool music_is_playing()
{
    return (current_music != NULL) && al_get_audio_stream_playing(current_music->stream);
}
#elif !defined(__USE_OPENAL__)
bool music_is_playing()
{
    return (current_music != NULL) && !(current_music->is_paused);
}
#else
bool music_is_playing()
{
    return (current_music != NULL) && !(current_music->is_paused) && (current_music->is_playing);
}
#endif


/*
 * music_duration()
 * Music duration, in seconds
 */
#if defined(A5BUILD)
float music_duration()
{
    if(current_music != NULL) {
        /* this may be zero if the length is unknown */
        return al_get_audio_stream_length_secs(current_music->stream);
    }

    return 0.0f;
}
#elif !defined(__USE_OPENAL__)
float music_duration()
{
    /* returns an approximation */
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
#if defined(A5BUILD)
bool music_is_paused()
{
    return (current_music != NULL) && (current_music->is_paused);
}
#else
bool music_is_paused()
{
    return (current_music != NULL) && (current_music->is_paused);
}
#endif


/* sound management */


/*
 * sound_load()
 * Loads a sample from a file
 */
#if defined(A5BUILD)
sound_t *sound_load(const char *path)
{
    sound_t *s;

    if(NULL == (s = resourcemanager_find_sample(path))) {
        ALLEGRO_SAMPLE_INSTANCE* spl;
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("Loading sound \"%s\"...", fullpath);

        /* build the sound object */
        s = mallocx(sizeof *s);
        s->duration = 0.0f;
        s->end_time = 0.0f;
        s->valid_id = false;
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
#elif !defined(__USE_OPENAL__)
sound_t *sound_load(const char *path)
{
    sound_t *s;

    if(NULL == (s = resourcemanager_find_sample(path))) {
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("sound_load('%s')", fullpath);

        /* build the sound object */
        s = mallocx(sizeof *s);
        s->filepath = str_dup(path);
        s->voice_id = SOUND_INVALID_VOICE;
        s->vol = 1.0f;

        /* loading the sample */
        if(NULL == (s->data = IS_OGG(path) ? logg_load(fullpath) : load_sample(fullpath))) {
            logfile_message("sound_load() error: %s", allegro_error);
            free(s->filepath);
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
    sound_t *s;

    if(quiet)
        return NULL;

    if(NULL == (s = resourcemanager_find_sample(path))) {
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("sound_load('%s')", fullpath);

        /* build the sound object */
        s = mallocx(sizeof *s);
        s->is_playing = FALSE;
        s->src = NULL;
        s->filepath = str_dup(path);

        /* loading the sample */
        if(!(IS_VALID_FORMAT(path) && (s->buf = alureCreateBufferFromFile(fullpath)))) {
            
            if(!IS_VALID_FORMAT(path)) {
                logfile_message("sound_load() error: invalid file format");
                alDeleteBuffers(1, &(s->buf));
            }
            else
                logfile_message("sound_load() error: %s", alureGetErrorString());

            free(s->filepath);
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
#if defined(A5BUILD)
int sound_unref(sound_t* sample)
{
    return resourcemanager_unref_sample(sample->filepath);
}
#elif !defined(__USE_OPENAL__)
int sound_unref(sound_t* sample)
{
    return resourcemanager_unref_sample(sample->filepath);
}
#else
int sound_unref(sound_t* sample)
{
    return resourcemanager_unref_sample(sample->filepath);
}
#endif


/*
 * sound_destroy()
 * Releases the given sample. This is called
 * automatically when releasing the main hash
 */
#if defined(A5BUILD)
void sound_destroy(sound_t *sample)
{
    if(sample != NULL) {
        sound_stop(sample);
        al_destroy_sample(sample->sample);
        free(sample->filepath);
        free(sample);
    }
}
#elif !defined(__USE_OPENAL__)
void sound_destroy(sound_t *sample)
{
    if(sample != NULL) {
        sound_stop(sample);
        destroy_sample(sample->data);
        free(sample->filepath);
        free(sample);
    }
}
#else
void sound_destroy(sound_t *sample)
{
    if(sample != NULL) {
        sound_stop(sample);
        /* the buffers will be deleted later, in audio_release() */
        free(sample->filepath);
        free(sample);
    }
}
#endif


/*
 * sound_play()
 * Plays the given sample
 */
#if defined(A5BUILD)
void sound_play(sound_t *sample)
{
    sound_play_ex(sample, 1.0f, 0.0f, 1.0f);
}
#elif !defined(__USE_OPENAL__)
void sound_play(sound_t *sample)
{
    sound_play_ex(sample, sample->vol, 0.0, 1.0);
}
#else
void sound_play(sound_t *sample)
{
    sound_play_ex(sample, 1.0, 0.0, 1.0);
}
#endif


/*
 * sound_play_ex()
 * Plays the given sample with extra options! :)
 *
 * 0.0 <= volume (defaults to 1.0)
 * (left speaker) -1.0 <= pan <= 1.0 (right speaker)
 * 1.0 = default frequency
 */
#if defined(A5BUILD)
void sound_play_ex(sound_t *sample, float vol, float pan, float freq)
{
    if(sample != NULL) {
        /* ajusting the parameters */
        vol = max(vol, 0.0f);
        pan = clip(pan, -1.0f, 1.0f);
        freq = max(freq, 0.0f);

        /* play the sample */
        if(al_play_sample(sample->sample, 1.0f, pan, freq, ALLEGRO_PLAYMODE_ONCE, &sample->id)) {
            sample->end_time = (0.001f * timer_get_ticks()) + sample->duration; /* when does it end? */
            sample->valid_id = true;
        }
        else {
            sample->end_time = 0.0f;
            sample->valid_id = false;
        }
    }
}
#elif !defined(__USE_OPENAL__)
void sound_play_ex(sound_t *sample, float vol, float pan, float freq)
{
    int id;

    if(sample != NULL) { /* important to check for NULL */
        /* ajusting parameters */
        vol = clip(vol, 0.0f, 1.0f);
        pan = clip(pan, -1.0f, 1.0f);
        freq = max(freq, 0.0f);

        /* playing the sample */
        id = play_sample(sample->data, (int)(255.0f * vol), min(255.0f, 128.0f + (int)(128.0f * pan)), (int)(1000.0f * freq), 0);
        sample->voice_id = id < 0 ? SOUND_INVALID_VOICE : id;
    }
}
#else
void sound_play_ex(sound_t *sample, float vol, float pan, float freq)
{
    if(sample != NULL) { /* important to check for NULL */
        /* ajusting parameters */
        vol = clip(vol, 0.0f, 1.0f);
        pan = clip(pan, -1.0f, 1.0f);
        freq = max(freq, 0.0f);

        /* saving the parameters */
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

    if(sample != NULL)
        sample->is_playing = FALSE;

    (void)source;
}
#endif



/*
 * sound_stop()
 * Stops a sample
 */
#if defined(A5BUILD)
void sound_stop(sound_t *sample)
{
    if(sample != NULL) {
        if(sample->valid_id) {
            al_stop_sample(&sample->id);
            sample->valid_id = false; /* is this needed? */
        }
    }
}
#elif !defined(__USE_OPENAL__)
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
#if defined(A5BUILD)
bool sound_is_playing(sound_t *sample)
{
    if(sample != NULL)
        return (0.001f * timer_get_ticks()) < sample->end_time;
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
#elif !defined(__USE_OPENAL__)
bool sound_is_playing(sound_t *sample)
{
    if(sample && sample->voice_id != SOUND_INVALID_VOICE)
        return (voice_check(sample->voice_id) == sample->data);
    else
        return false;
}
#else
bool sound_is_playing(sound_t *sample)
{
    if(sample)
        return sample->is_playing;
    else
        return false;
}
#endif

/*
 * sound_get_volume()
 * Gets the volume of a sound.
 * 0.0f means silence; 1.0f, the default volume
 */
#if defined(A5BUILD)
float sound_get_volume(sound_t *sample)
{
    /* TODO */
    return 1.0f;

#if 0
    /* Unstable API (Allegro 5.2.3) */
    if(sample != NULL) {
        if(sample->valid_id) {
            ALLEGRO_SAMPLE_INSTANCE* instance = al_lock_sample_id(&sample->id);
            if(instance != NULL) {
                float gain = al_get_sample_instance_gain(instance);
                al_unlock_sample_id(&sample->id);
                return gain;
            }
            else
                return false;
        }
    }

    return false;
#endif
}
#elif !defined(__USE_OPENAL__)
float sound_get_volume(sound_t *sample)
{
    if(sample != NULL)
        return sample->vol;
    else
        return 1.0f;
}
#else
float sound_get_volume(sound_t *sample)
{
    /* not implemented */
    return 1.0f;
}
#endif


/*
 * sound_set_volume()
 * Sets the volume of a sound.
 */
#if defined(A5BUILD)
void sound_set_volume(sound_t *sample, float volume)
{
    /* TODO */
    /* not yet implemented */

#if 0
    /* Unstable API (Allegro 5.2.3) */
    if(sample != NULL) {
        if(sample->valid_id) {
            ALLEGRO_SAMPLE_INSTANCE* instance = al_lock_sample_id(&sample->id);
            if(instance != NULL) {
                al_set_sample_instance_gain(instance, max(0.0f, volume));
                al_unlock_sample_id(&sample->id);
            }
        }
    }
#endif
}
#elif !defined(__USE_OPENAL__)
void sound_set_volume(sound_t *sample, float volume)
{
    if(sample != NULL) {
        sample->vol = clip(volume, 0.0f, 1.0f);
        if(sample->voice_id != SOUND_INVALID_VOICE)
            voice_set_volume(sample->voice_id, (int)(sample->vol * 255));
    }
}
#else
void sound_set_volume(sound_t *sample, float volume)
{
    /* not implemented */
}
#endif






/* audio manager */

/*
 * audio_init()
 * Initializes the Audio Manager
 */
#if defined(A5BUILD)
void audio_init()
{
    int samples;

    logfile_message("Initializing the audio...");
    current_music = NULL;

    if(!al_install_audio())
        fatal_error("Can't initialize Allegro's audio addon");

    if(!al_init_acodec_addon())
        fatal_error("Can't initialize Allegro's acodec addon");

    for(samples = PREFERRED_NUMBER_OF_SAMPLES; samples > 0; samples /= 2) {
        if(al_reserve_samples(samples)) {
            logfile_message("Reserved %d samples", samples);
            break;
        }
        else
            logfile_message("Can't reserve %d samples", samples);
    }
}
#elif !defined(__USE_OPENAL__)
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
    logfile_message("Warning: unable to reserve voices - %s", allegro_error);
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
#if defined(A5BUILD)
void audio_release()
{
    logfile_message("audio_release()");
    logfile_message("audio_release() ok");
}
#elif !defined(__USE_OPENAL__)
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
#if defined(A5BUILD)
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
#elif !defined(__USE_OPENAL__)
void audio_update()
{
    /* updating the music */
    if(current_music != NULL && !(current_music->is_paused)) {
        logg_update_stream(current_music->stream);
        if(!(current_music->stream->loop)) {
            current_music->elapsed_time += timer_get_delta();
            if(current_music->elapsed_time >= MUSIC_DURATION(current_music) + 0.25f) {
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
