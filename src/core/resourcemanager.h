/*
 * Open Surge Engine
 * resourcemanager.h - resource manager: a dictionary of resources
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _RESOURCEMANAGER_H
#define _RESOURCEMANAGER_H

#include <stdbool.h>

/* forward declarations */
struct image_t;
struct sound_t;
struct music_t;

/* resource manager: public methods */
void resourcemanager_init(); /* initializes the resource manager */
void resourcemanager_release(); /* releases the resource manager */
void resourcemanager_release_unused_resources(); /* memory optimization: reference counting */
bool resourcemanager_is_initialized(); /* is the resource manager initialized? */

/* data handling */
void resourcemanager_add_image(const char *key, struct image_t *data); /* adds an image to the dictionary */
struct image_t* resourcemanager_find_image(const char *key); /* finds an image in the dictionary */
int resourcemanager_ref_image(const char *key); /* increments and returns the reference counting */
int resourcemanager_unref_image(const char *key); /* decrements and returns the reference counting */
bool resourcemanager_purge_image(const char *key); /* use with care */

void resourcemanager_add_music(const char *key, struct music_t *data);
struct music_t* resourcemanager_find_music(const char *key);
int resourcemanager_ref_music(const char *key);
int resourcemanager_unref_music(const char *key);

void resourcemanager_add_sample(const char *key, struct sound_t *data);
struct sound_t* resourcemanager_find_sample(const char *key);
int resourcemanager_ref_sample(const char *key);
int resourcemanager_unref_sample(const char *key);

#endif
