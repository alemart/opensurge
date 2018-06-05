/*
 * Open Surge Engine
 * grouptree.h - group trees
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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


/*
Example - how to build this tree:

    root
    |
    +---child 1
    |
    +---child 2
        |
        +---child 3

== 0. declaring stuff ==
    group_t *root, *child1, *child2, *child3;
    v2d_t camera_position = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

== 1. creating the nodes... ==
    child3 = group_[CHILD3CLASS]_create();
    child2 = group_[CHILD2CLASS]_create();
    group_addchild(child2, child3);
    child1 = group_[CHILD1CLASS]_create();
    root = group_[ROOTCLASS]_create();
    group_addchild(root, child1);
    group_addchild(root, child2);

== 2. initializing the tree ==
    grouptree_init_all(root);

== 3. main loop ==
    main loop {
        grouptree_update_all(root);
        grouptree_render_all(root, camera_position);
    }

== 4. releasing the tree ==
    grouptree_release_all(root);

== 5. destroying the nodes ==
    grouptree_destroy_all(root);
*/

#ifndef _GROUPTREE_H
#define _GROUPTREE_H


#include "../../core/util.h"
#include "../../core/font.h"



/* constants */
#define GROUPTREE_MAXCHILDREN          10

/* group tree */
typedef struct group_t {
    /* meta data */
    font_t *font;

    /* who needs C++ after all? ;) */
    void (*init)(struct group_t*);
    void (*release)(struct group_t*);
    void (*update)(struct group_t*);
    void (*render)(struct group_t*,v2d_t);
    void *data; /* could be anything */

    /* tree structure */
    struct group_t *parent; /* NULL iff root */
    struct group_t *child[GROUPTREE_MAXCHILDREN]; /* NULL iff leaf */
    int child_count; /* 0 iff leaf */
} group_t;

/* tree manipulation */
/* void grouptree_init_all(group_t *root); -- this doesn't exist, as the nodes of the tree are custom and must be created manually */
void grouptree_destroy_all(group_t* root); /* destroys the whole tree (release_all() should be called first) */
void grouptree_init_all(group_t *root); /* initializes root's and root's children internal data (without creating them) */
void grouptree_release_all(group_t *root); /* releases root's and root's children internal data (without destroying them) */
void grouptree_update_all(group_t *root); /* updates root and its children */
void grouptree_render_all(group_t *root, v2d_t camera_position); /* renders root and its children */


/* tree extra features */
int grouptree_nodecount(group_t *root); /* returns 1 + root->child_count + root->child[i]->child_count + root->child[i]->child[j]->child_count + ... */

/*
 * "polymorphism" in C
 * naming convention: group_CLASSNAME_METHOD
 */

/* <<abstract>> base class */
group_t *group_create(void (*init)(group_t*), void (*release)(group_t*), void (*update)(group_t*), void (*render)(group_t*,v2d_t)); /* creates a group node, but doesn't initialize it */
void group_addchild(group_t *g, group_t *child); /* adds a child */

/* labels - they are just labels that do nothing */
void group_label_init(group_t *g); /* initializes g's internal data (without touching g's children) */
void group_label_release(group_t *g); /* releases g's internal data (without touching g's children) */
void group_label_update(group_t *g); /* updates g (without touching its children) */
void group_label_render(group_t *g, v2d_t camera_position); /* renders g (without touching its children) */
group_t* group_label_create(); /* creates a label: shortcut to group_create(...METHODS...) */

#endif

