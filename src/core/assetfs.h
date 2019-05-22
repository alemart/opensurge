/*
 * Open Surge Engine
 * assetfs.c - assetfs virtual filesystem
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _ASSETFS_H
#define _ASSETFS_H

#include <stdbool.h>

/* assetfs virtual filesystem */
void assetfs_init(const char* gameid, const char* datadir); /* datadir may be NULL (for default locations) */
void assetfs_release(); /* release the assetfs */
const char* assetfs_fullpath(const char* vpath); /* give the (absolute) fullpath of a (relative) virtual path */
bool assetfs_exists(const char* vpath); /* does the given file exist? */
int assetfs_foreach_file(const char* vpath_of_dir, const char* extension_filter, int (*callback)(const char* vpath, void* param), void* param, bool recursive); /* foreach file in vpath_of_dir */
bool assetfs_initialized(); /* checks if this subsystem has been initialized */
bool assetfs_use_strict(bool strict); /* use strict mode (defaults to true) */

/* the following are useful when writing to files */
const char* assetfs_create_config_file(const char* vpath); /* the fullpath of a user-specific config file */
const char* assetfs_create_cache_file(const char* vpath); /* user-specific non-essential (cached) data */
const char* assetfs_create_data_file(const char* vpath, bool prefer_user_space); /* create game data */
bool assetfs_is_config_file(const char* vpath);
bool assetfs_is_cache_file(const char* vpath);
bool assetfs_is_data_file(const char* vpath);

#endif
