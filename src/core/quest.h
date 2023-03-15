/*
 * Open Surge Engine
 * quest.h - quest module
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

#ifndef _QUEST_H
#define _QUEST_H

/*

   A quest is basically a list of level files read
   from a .qst file located in the quests/ folder.

   The quest scene is used to dispatch the player
   to the correct levels (see ../scenes/quest.h)

*/

#define QUEST_MAXLEVELS 1024

/* quest structure */
typedef struct quest_t quest_t;
struct quest_t {
    char *file; /* relative path of the quest file */
    char *name; /* quest name */

    int level_count; /* how many levels? */
    char *level_path[QUEST_MAXLEVELS]; /* relative paths of the levels */
};

quest_t *quest_load(const char *filepath); /* relative filepath */
quest_t *quest_unload(quest_t *qst);

#endif