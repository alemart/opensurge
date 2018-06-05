/*
 * Open Surge Engine
 * hqx.h - neat(?) wrapper for hqx
 * Copyright (C) 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#ifndef _HQXWRAP_H
#define _HQXWRAP_H

#ifdef __GNUC__
#define __D__GNUC__
#undef __GNUC__
#endif
#ifdef _WIN32
#define __D_WIN32
#undef _WIN32
#endif

/* it's ugly, I know, but I do not want to change anything in the hqx lib */
/* the macros defined in hqx/hqx.h are pretty annoying! I just want to get rid of them. */
#include "hqx/hqx.h"

#ifdef __D__GNUC__
#define __GNUC__
#endif
#ifdef __D_WIN32
#define _WIN32
#endif

#endif
