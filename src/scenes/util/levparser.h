/*
 * Open Surge Engine
 * levparser.h - parser for level files (.lev)
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

#ifndef _LEVPARSER_H
#define _LEVPARSER_H

#include <stdbool.h>

typedef bool (*levparser_callback_t)(const char *filepath, int fileline, const char *identifier, int param_count, const char **param, void* data);

bool levparser_parse(const char* path_to_lev_file, void* data, levparser_callback_t callback);

#endif