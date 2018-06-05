/*
 * Open Surge Engine
 * preferences.h - user preferences (saved in a file)
 * Copyright (C) 2010, 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _PREFERENCES_H
#define _PREFERENCES_H

/* initializes this module */
int preferences_init(); /* returns TRUE if a previous preferences file exists, FALSE otherwise */
int preferences_file_exists();

/* accessors */
int preferences_get_videoresolution(); /* returns a VIDEORESOLUTION_* value (see video.h) */
int preferences_get_fullscreen(); /* fullscreen mode? */
int preferences_get_smooth(); /* display smooth graphics? */
int preferences_get_showfps(); /* show fps? */
int preferences_get_usegamepad(); /* use gamepad? */
const char* preferences_get_languagepath(); /* returns the language filepath */

/* mutators */
/* each time you call a mutator, the preferences are saved automatically */
void preferences_set_videoresolution(int resolution);
void preferences_set_fullscreen(int fullscreen);
void preferences_set_smooth(int smooth);
void preferences_set_showfps(int fullscreen);
void preferences_set_languagepath(const char *filepath);
void preferences_set_usegamepad(int usegamepad);

#endif
