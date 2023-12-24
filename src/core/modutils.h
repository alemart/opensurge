/*
 * Open Surge Engine
 * modutils.h - utilities for MODs & compatibility mode
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

#ifndef _MODUTILS_H
#define _MODUTILS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define GAME_ID_UNAVAILABLE         0xFFFFFFFFu

uint32_t find_game_id(const char* game_title, const char* game_version, const char* game_dirname, const char* required_engine_version);
char* guess_engine_version_of_mod(char* buffer, size_t buffer_size);
const char** select_files_for_compatibility_pack(const char* engine_version, uint32_t game_id);
bool generate_surge_cfg(const char* game_title, void** out_file_data, size_t* out_file_size);

#endif