/*
 * Open Surge Engine
 * quest.h - quest scene
 * Copyright (C) 2010, 2012-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _QUESTSCENE_H
#define _QUESTSCENE_H

/*
   Multiple quest scenes may be pushed onto the
   scene stack. It will work.

   This is actually a "mock" scene that just
   dispatches the player to the correct levels.
*/

/* public scene functions */
void quest_init(void *path_to_qst_file); /* pass an string */
void quest_update();
void quest_render();
void quest_release();

/* current quest */
void quest_abort(); /* aborts the current quest */
void quest_setlevel(int lev); /* jumps to the given level (0..n-1) */
int quest_currentlevel(); /* id of the current level (0..n-1) */
int quest_numberoflevels(); /* returns the number of levels (n) of the current quest */
const char *quest_getname(); /* returns the name of the current quest */

#endif
