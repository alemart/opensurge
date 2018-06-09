/*
 * Open Surge Engine
 * soundfactory.h - sound factory
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

#ifndef _SOUNDFACTORY_H
#define _SOUNDFACTORY_H

#include "audio.h"

/* initializes the sound factory */
void soundfactory_init();

/* releases the sound factory */
void soundfactory_release();

/* given a sound name, returns the corresponding sound effect */
sound_t *soundfactory_get(const char *sound_name);

#endif
