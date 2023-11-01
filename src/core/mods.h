/*
 * Open Surge Engine
 * mods.h - utilities for MODs & compatibility mode
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

#ifndef _MODS_H
#define _MODS_H

#include <stdlib.h>

bool generate_compatibility_pack(const char* engine_version, void** out_file_data, size_t* out_file_size);
char* guess_required_engine_version(char* buffer, size_t buffer_size);

bool generate_pak_file(const char** file_list, int file_count, void** out_pak_data, size_t* out_pak_size);
bool generate_pak_file_from_memory(const char** vpath, int file_count, const void** file_data, const size_t* file_size, void** out_pak_data, size_t* out_pak_size);
void release_pak_file(void* pak);

#endif