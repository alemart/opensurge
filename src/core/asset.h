/*
 * Open Surge Engine
 * asset.h - asset manager (virtual filesystem)
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _ASSET_H
#define _ASSET_H

#include <stdlib.h>
#include <stdbool.h>

void asset_init(const char* argv0, const char* optional_gamedir);
void asset_release();

bool asset_exists(const char* virtual_path);
const char* asset_path(const char* virtual_path);
void asset_foreach_file(const char* virtual_path_of_directory, const char* extension_filter, int (*callback)(const char* virtual_path, void* user_data), void* user_data, bool recursive);

char* asset_user_datadir(char* dest, size_t dest_size);
char* asset_shared_datadir(char* dest, size_t dest_size);
bool asset_purge_user_data();

#endif