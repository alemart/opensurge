/*
 * Open Surge Engine
 * editorpal.h - level editor: item palette
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

#ifndef _EDITORPAL_H
#define _EDITORPAL_H

/* palette config */
typedef struct editorpal_config_t {
    /* palette types */
    enum {
        EDITORPAL_BRICK,
        EDITORPAL_SSOBJ
    } type;

    /* config: SurgeScript entities */
    struct {
        const char** name;
        int count;
    } ssobj;

    /* config: valid bricks */
    struct {
        const int* id;
        int count;
    } brick;
} editorpal_config_t;

/* public functions */
void editorpal_init(void* config);
void editorpal_update();
void editorpal_render();
void editorpal_release();

/* interface */
int editorpal_selected_item(); /* an index of ssobj.name[] or brick.id[]... or -1 if no item was selected */

#endif
