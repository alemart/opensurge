/*
 * Open Surge Engine
 * logfile.h - logfile module
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

#ifndef _LOGFILE_H
#define _LOGFILE_H

/* logfile flags */
#define LOGFILE_TXT         0x1 /* requires the asset manager to be initialized */
#define LOGFILE_CONSOLE     0x2 /* will print logs to stdout */

void logfile_init(int flags); /* initializes the logfile module */
void logfile_message(const char *fmt, ...); /* prints a message to the logfile (printf style) */
void logfile_release(int flags); /* releases the logfile module */

#endif

