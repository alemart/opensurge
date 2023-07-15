/*
 * Open Surge Engine
 * background.h - level background/foreground
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

#ifndef _BACKGROUND_H
#define _BACKGROUND_H

#include "../util/v2d.h"

typedef struct bgtheme_t bgtheme_t;

bgtheme_t* background_load(const char *filepath); /* loads a bg/fg theme from a .bg file */
bgtheme_t* background_unload(bgtheme_t *bgtheme); /* unloads the bg/fg theme */

void background_update(bgtheme_t *bgtheme); /* updates the given theme */
void background_render_bg(bgtheme_t *bgtheme, v2d_t camera_position); /* renders the background */
void background_render_fg(bgtheme_t *bgtheme, v2d_t camera_position); /* renders the foreground */

const char* background_filepath(const bgtheme_t *bgtheme); /* get the filepath of the background */
int background_number_of_bg_layers(const bgtheme_t* bgtheme); /* number of background layers */
int background_number_of_fg_layers(const bgtheme_t* bgtheme); /* number of foreground layers */

#endif