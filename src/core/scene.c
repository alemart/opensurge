/*
 * Open Surge Engine
 * scene.c - scene management
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#include <stdbool.h>
#include "scene.h"
#include "../core/util.h"
#include "../core/logfile.h"


/* private stuff */
#define SCENESTACK_CAPACITY         32 /* up to SCENESTACK_CAPACITY scenes running simultaneously */
static scene_t* scenestack[SCENESTACK_CAPACITY];
static int scenestack_size = 0;
static bool is_duplicate_scene(const scene_t* scn);




/* Scene stack */

/*
 * scenestack_init()
 * Initializes the scene stack
 */
void scenestack_init()
{
    int i;

    scenestack_size = 0;
    for(i = 0; i < SCENESTACK_CAPACITY; i++)
        scenestack[i] = NULL;
}



/*
 * scenestack_release()
 * Releases the scene stack
 */
void scenestack_release()
{
    while(!scenestack_empty())
        scenestack_pop();
}



/*
 * scenestack_push()
 * Inserts a new scene into the stack
 */
void scenestack_push(scene_t *scn, void *data)
{
    logfile_message("scenestack_push()");
    if(scenestack_size >= SCENESTACK_CAPACITY) {
        fatal_error("scenestack_push(): stack overflow");
        return;
    }
    if(is_duplicate_scene(scn)) {
        fatal_error("scenestack_push(): duplicate scenes are not supported");
        return;
    }
    scenestack[scenestack_size++] = scn;
    scn->init(data);
    logfile_message("scenestack_push(): success");
}



/*
 * scenestack_pop()
 * Deletes the top-most scene of the stack.
 * Please use "return" after calling pop()
 * inside a scene, otherwise the program
 * may crash.
 */
void scenestack_pop()
{
    logfile_message("scenestack_pop()");
    if(!scenestack_empty()) {
        scenestack[scenestack_size - 1]->release();
        scenestack[scenestack_size - 1] = NULL;
        scenestack_size--;
        logfile_message("scenestack_pop(): success");
    }
    else
        logfile_message("scenestack_pop(): empty stack");
}



/*
 * scenestack_top()
 * Returns the top-most scene of the stack.
 */
scene_t *scenestack_top()
{
    if(!scenestack_empty())
        return scenestack[scenestack_size - 1];
    else
        return NULL;
}


/*
 * scenestack_empty()
 * Is the stack empty?
 */
int scenestack_empty()
{
    return (scenestack_size == 0);
}






/* Scene struct */

/*
 * scene_create()
 * Creates a new scene
 */
scene_t *scene_create(void (*init_func)(void*), void (*update_func)(), void (*render_func)(), void (*release_func)())
{
    scene_t *s = mallocx(sizeof *s);
    s->init = init_func;
    s->update = update_func;
    s->render = render_func;
    s->release = release_func;
    return s;
}


/*
 * scene_destroy()
 * Destroys an existing scene
 */
scene_t *scene_destroy(scene_t *scn)
{
    free(scn);
    return NULL;
}



/* private */
extern void quest_init(void*);

/* Returns true if scn is already in the scene stack */
bool is_duplicate_scene(const scene_t* scn)
{
    if(scn->init != quest_init) { /* quest_* supports duplicates */
        for(int i = 0; i < scenestack_size; i++) {
            if(scenestack[i]->init == scn->init)
                return true;
        }
    }
    return false;
}