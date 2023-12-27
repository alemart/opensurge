/*
 * Open Surge Engine
 * config.h - game configuration - file reader
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

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdbool.h>

bool config_init();
void config_release();

const char* config_game_title(const char* default_value);
const char* config_game_version(const char* default_value);

int config_video_screen_width(int default_value);
int config_video_screen_height(int default_value);

#endif