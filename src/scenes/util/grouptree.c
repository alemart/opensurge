/*
 * Open Surge Engine
 * grouptree.c - group trees
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

#include "grouptree.h"
#include "../../util/util.h"
#include "../../core/video.h"


/* tree manipulation */





/*
 * grouptree_destroy_all()
 * destroys the whole tree.
 * ps: release_all() must be called first
 */
void grouptree_destroy_all(group_t *root)
{
    int i;

    if(root) {
        for(i=0; i<root->child_count; i++)
            grouptree_destroy_all(root->child[i]);
        free(root);
    }
}


/*
 * grouptree_init_all()
 * initializes root's and root's children internal data (without creating them)
 */
void grouptree_init_all(group_t *root)
{
    int i;

    if(root) {
        root->init(root);
        for(i=0; i<root->child_count; i++)
            grouptree_init_all(root->child[i]);
    }
}

/*
 * grouptree_release_all()
 * releases root's and root's children internal data (without destroying them)
 */
void grouptree_release_all(group_t *root)
{
    int i;

    if(root) {
        for(i=0; i<root->child_count; i++)
            grouptree_release_all(root->child[i]);
        root->release(root);
    }
}


/*
 * grouptree_update_all()
 * updates root and its children
 */
void grouptree_update_all(group_t *root)
{
    int i;

    if(root) {
        for(i=0; i<root->child_count; i++)
            grouptree_update_all(root->child[i]);
        root->update(root);
    }
}

/*
 * grouptree_render_all()
 * renders root and its children
 */
void grouptree_render_all(group_t *root, v2d_t camera_position)
{
    int i;

    if(root) {
        for(i=0; i<root->child_count; i++)
            grouptree_render_all(root->child[i], camera_position);
        root->render(root, camera_position);
    }
}



/*
 * grouptree_nodecount()
 * returns:
 * 1 (root) +
 * root->child_count +
 * root->child[i]->child_count +
 * root->child[i]->child[j]->child_count +
 * ...
 */
int grouptree_nodecount(group_t *root)
{
    int i, sum = 0;

    if(root) {
        for(i=0; i<root->child_count; i++)
            sum += grouptree_nodecount(root->child[i]);
        return 1 + sum;
    }
    else
        return 0;
}









/* <<abstract>> base class */

/*
 * group_create()
 * creates a group node, but doesn't initialize it
 */
group_t *group_create(void (*init)(group_t*), void (*release)(group_t*), void (*update)(group_t*), void (*render)(group_t*,v2d_t))
{
    group_t *g = mallocx(sizeof *g);

    /* methods */
    g->init = init;
    g->release = release;
    g->update = update;
    g->render = render;

    /* internal data */
    g->font = NULL;
    g->data = NULL;
    g->parent = NULL;
    g->child_count = 0;

    /* done! */
    return g;
}

/*
 * group_addchild()
 * adds a child to g
 */
void group_addchild(group_t *g, group_t *child)
{
    if(g->child_count < GROUPTREE_MAXCHILDREN) {
        g->child[ g->child_count++ ] = child;
        child->parent = g;
    }
}













/* labels - they are just labels that do nothing */

/*
 * group_label_create()
 * creates a label: shortcut to group_create(...METHODS...)
 */
group_t* group_label_create()
{
    return group_create(group_label_init, group_label_release, group_label_update, group_label_render);
}

/*
 * group_label_init()
 * initializes g's internal data (without touching g's children)
 */
void group_label_init(group_t *g)
{
    v2d_t pos;

    g->font = font_create("MenuText");
    font_set_text(g->font, "LABEL"); /* if you want a different text, please derive this class */

    /* calculating my position... */
    if(g->parent != NULL) {
        int my_id, i, nodecount=0;
        v2d_t spacing = (g->parent->font == NULL) ? v2d_new(0,0) : v2d_new(8,12);

        for(my_id=0; my_id < g->parent->child_count; my_id++) {
            if(g->parent->child[my_id] == g)
                break;
        }

        for(i=0; i<my_id; i++)
            nodecount += grouptree_nodecount(g->parent->child[i])-1;

        pos = font_get_position(g->parent->font);
        pos.x += spacing.x * 3;
        pos.y += (1+nodecount+my_id) * spacing.y * 1.5f;
        font_set_position(g->font, pos);
    }
}

/*
 * group_label_release()
 * releases g's internal data (without touching g's children)
 */
void group_label_release(group_t *g)
{
    font_destroy(g->font);
}

/*
 * group_label_update()
 * updates g (without touching its children)
 */
void group_label_update(group_t *g)
{
}

/*
 * group_label_render()
 * renders g (without touching its children)
 */
void group_label_render(group_t *g, v2d_t camera_position)
{
    font_render(g->font, camera_position);
}

