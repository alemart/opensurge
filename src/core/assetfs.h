/*
 * Open Surge Engine
 * assetfs.c - assetfs virtual file system
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

void assetfs_init(const char* gameid, const char* datadir); /* datadir may be NULL (for default locations) */
void assetfs_release(); /* release the assetfs */
const char* assetfs_fullpath(const char* vpath); /* give the (absolute) fullpath of a (relative) virtual path */
bool assetfs_exists(const char* vpath); /* does the given file exist? */
int assetfs_foreach_file(const char* vpath_of_dir, const char* extension_filter, int (*callback)(const char* fullpath, void* param), void* param, bool recursive); /* foreach file in vpath_of_dir */

#endif
