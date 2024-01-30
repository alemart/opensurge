/*
 * Open Surge Engine
 * quest.h - quest module
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#include <stdbool.h>

/*

A quest is an immutable list specified in a .qst
file stored in the quests/ folder. An entry of such
a list may be:

a) a level, i.e., a .lev file stored in levels/
b) another quest, i.e., a .qst file
c) a built-in scene of the engine

The quest scene is used to dispatch the player
to the appropriate scenes (see ../scenes/quest.h).

*/

typedef struct quest_t quest_t;

/* instantiation */
quest_t* quest_load(const char* filepath); /* relative filepath */
quest_t* quest_unload(quest_t* quest);

/* quest properties */
const char* quest_name(const quest_t* quest);
const char* quest_file(const quest_t* quest);

/* entries of the quest */
int quest_entry_count(const quest_t* quest);
const char* quest_entry_path(const quest_t* quest, int index); /* index = 0, 1, 2... */
bool quest_entry_is_level(const quest_t* quest, int index);
bool quest_entry_is_quest(const quest_t* quest, int index);
bool quest_entry_is_builtin_scene(const quest_t* quest, int index);

#endif