/*
 * Open Surge Engine
 * resourcemanager.c - resource manager: a dictionary of resources
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "resourcemanager.h"
#include "hashtable.h"
#include "image.h"
#include "audio.h"
#include "logfile.h"

/* code generation */
HASHTABLE_GENERATE_CODE(image_t)
HASHTABLE_GENERATE_CODE(sound_t)
HASHTABLE_GENERATE_CODE(music_t)

/* private data */
static hashtable_image_t* images;
static hashtable_sound_t* samples;
static hashtable_music_t* musics;
static int is_valid = FALSE; /* validity flag */


/* public methods */

/* ------ resource manager -------- */
void resourcemanager_init()
{
    images = hashtable_image_t_create(image_destroy);
    samples = hashtable_sound_t_create(sound_destroy);
    musics = hashtable_music_t_create(music_destroy);
    is_valid = TRUE;
}

void resourcemanager_release()
{
    is_valid = FALSE;
    images = hashtable_image_t_destroy(images);
    samples = hashtable_sound_t_destroy(samples);
    musics = hashtable_music_t_destroy(musics);
}

void resourcemanager_release_unused_resources()
{
    if(is_valid) {
        hashtable_image_t_release_unreferenced_entries(images);
        hashtable_sound_t_release_unreferenced_entries(samples);
        hashtable_music_t_release_unreferenced_entries(musics);
    }
}




/* -------- images ------- */
void resourcemanager_add_image(const char *key, image_t *data)
{
    hashtable_image_t_add(images, key, data);
}

image_t* resourcemanager_find_image(const char *key)
{
    return hashtable_image_t_find(images, key);
}

int resourcemanager_ref_image(const char *key)
{
    return hashtable_image_t_ref(images, key);
}

int resourcemanager_unref_image(const char *key)
{
    return is_valid ? hashtable_image_t_unref(images, key) : 0;
}

/* returns TRUE on success (i.e., the image has been successfully purged) */
int resourcemanager_purge_image(const char *key)
{
    if(is_valid && resourcemanager_find_image(key) != NULL) {
        int refs = hashtable_image_t_refcount(images, key);

        /* sanity check */
        if(refs > 0) {
            /* won't purge if there are active references */
            return FALSE;
        }

        /* purge the image */
        logfile_message("resourcemanager_purge_image('%s')...", key);
        hashtable_image_t_remove(images, key);
    }

    /* done */
    return TRUE;
}

/* -------- musics --------- */
void resourcemanager_add_music(const char *key, music_t *data)
{
    hashtable_music_t_add(musics, key, data);
}

music_t* resourcemanager_find_music(const char *key)
{
    return hashtable_music_t_find(musics, key);
}

int resourcemanager_ref_music(const char *key)
{
    return hashtable_music_t_ref(musics, key);
}

int resourcemanager_unref_music(const char *key)
{
    return is_valid ? hashtable_music_t_unref(musics, key) : 0;
}

/* ------- samples ------- */
void resourcemanager_add_sample(const char *key, sound_t *data)
{
    hashtable_sound_t_add(samples, key, data);
}

sound_t* resourcemanager_find_sample(const char *key)
{
    return hashtable_sound_t_find(samples, key);
}

int resourcemanager_ref_sample(const char *key)
{
    return hashtable_sound_t_ref(samples, key);
}

int resourcemanager_unref_sample(const char *key)
{
    return is_valid ? hashtable_sound_t_unref(samples, key) : 0;
}
