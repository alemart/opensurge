/*
 * Open Surge Engine
 * install.h - installs/builds/lists Open Surge games
 * Copyright (C) 2018-2019  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _INSTALL_H
#define _INSTALL_H

#include <stdbool.h>

/* Open Surge games (MODs) are packed as zip files */

/* Usually, these routines are used in console mode */
bool install_game(const char* zip_fullpath, char* out_gameid, size_t out_gameid_size, bool interactive_mode); /* returns true on success */
int foreach_installed_game(int (*callback)(const char*,void*), void* data); /* all installed games */
bool is_game_installed(const char* gameid); /* is a game installed? */
bool build_game(const char* gameid); /* builds a game package; returns true on success */
bool uninstall_game(const char* gameid, bool interactive_mode); /* returns true on success */

#endif
