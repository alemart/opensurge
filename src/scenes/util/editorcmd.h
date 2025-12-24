/*
 * Open Surge Engine
 * editorcmd.h - level editor: commands & hotkeys
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _EDITORCMD_H
#define _EDITORCMD_H

#include <stdbool.h>
#include "../../util/v2d.h"

typedef struct editorcmd_t editorcmd_t;

editorcmd_t* editorcmd_create();
editorcmd_t* editorcmd_destroy(editorcmd_t* cmd);
bool editorcmd_is_triggered(const editorcmd_t* cmd, const char* command_name);
v2d_t editorcmd_mousepos(const editorcmd_t* cmd);

#endif