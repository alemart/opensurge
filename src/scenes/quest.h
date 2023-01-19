/*
 * Open Surge Engine
 * quest.h - quest scene
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

#ifndef _QUESTSCENE_H
#define _QUESTSCENE_H

/*
   Multiple quest scenes may be pushed onto the
   scene stack. It will work.

   This is actually a "mock" scene that just
   dispatches the player to the correct levels.
*/

/* quest scene: functions */
void quest_init(void *path_to_qst_file); /* pass an string */
void quest_update();
void quest_render();
void quest_release();

/* utilities: current quest */
void quest_abort(); /* aborts the current quest */
void quest_set_next_level(int id); /* set the next level (0..num_levels) */
int quest_next_level(); /* id of the next level (0..num_levels) */
const struct quest_t* quest_current(); /* returns the current quest, or NULL if no quest is active */

#endif
