/*
 * Open Surge Engine
 * scene.h - scene management
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _SCENE_H
#define _SCENE_H

/* Scene struct */
typedef struct scene_t {
    void (*init)(void*);
    void (*update)();
    void (*render)();
    void (*release)();
} scene_t;

scene_t *scene_create(void (*init_func)(void*), void (*update_func)(), void (*render_func)(), void (*release_func)());
scene_t *scene_destroy(scene_t *scn);



/* Scene stack */
/* this is used with the storyboard module (see storyboard.h) */
void scenestack_init();
void scenestack_release();
void scenestack_push(scene_t *scn, void *data); /* some generic data will be passed to <scene>_init() */
void scenestack_pop();
scene_t *scenestack_top();
int scenestack_empty();

#endif
