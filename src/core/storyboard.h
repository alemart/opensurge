/*
 * Open Surge Engine
 * storyboard.h - storyboard (stores the scenes of the game)
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

#ifndef _STORYBOARD_H
#define _STORYBOARD_H

/* available scenes */
typedef enum scenetype_t {
    SCENE_INTRO,
    SCENE_LEVEL,
    SCENE_PAUSE,
    SCENE_GAMEOVER,
    SCENE_QUEST,
    SCENE_CONFIRMBOX,
    SCENE_LANGSELECT,
    SCENE_CREDITS,
    SCENE_OPTIONS,
    SCENE_STAGESELECT,
    SCENE_EDITORHELP,
    SCENE_EDITORPAL,
    SCENE_MOBILEMENU,
    SCENE_MOBILEPOPUP
} scenetype_t;

/* Storyboard */
void storyboard_init();
void storyboard_release();
struct scene_t* storyboard_get_scene(scenetype_t type);

#endif
