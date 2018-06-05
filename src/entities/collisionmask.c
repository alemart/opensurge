/*
 * Open Surge Engine
 * collisionmask.h - collision mask
 * Copyright (C) 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "collisionmask.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/stringutil.h"
#include "../core/util.h"
#include "../core/sprite.h"
#include "../core/nanoparser/nanoparser.h"

/* private stuff ;) */
struct collisionmask_t {
    image_t *mask; /* it's a sub-image from some sheet */
};

typedef struct cmdetails_t {
    const char *source_file;
    int x, y, w, h;
} cmdetails_t;

static int traverse(const parsetree_statement_t *stmt, void *cmdetails);


/* public methods */
collisionmask_t *collisionmask_create(const struct image_t *image, int x, int y, int width, int height)
{
    collisionmask_t *cm = mallocx(sizeof *cm);
    cm->mask = image_create_shared(image, x, y, width, height);
    return cm;
}

collisionmask_t *collisionmask_create_from_parsetree(const struct parsetree_program_t *block)
{
    cmdetails_t s = { NULL, 0, 0, 0, 0 };

    nanoparser_traverse_program_ex(block, (void*)(&s), traverse);
    if(s.source_file == NULL)
        fatal_error("collision_mask: a source_file must be specified");

    return collisionmask_create(image_load(s.source_file), s.x, s.y, s.w, s.h);
}

collisionmask_t *collisionmask_create_from_sprite(const struct spriteinfo_t *sprite)
{
    return collisionmask_create(image_load(sprite->source_file), sprite->rect_x, sprite->rect_y, sprite->frame_w, sprite->frame_h);
}

collisionmask_t *collisionmask_destroy(collisionmask_t *cm)
{
    image_destroy(cm->mask);
    free(cm);
    return NULL;
}

int collisionmask_check(collisionmask_t *cm, int x, int y)
{
    return image_getpixel(cm->mask, x, y) != video_get_maskcolor();
}

const image_t *collisionmask_image(const collisionmask_t *cm)
{
    return cm->mask;
}









/* aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa */



/* this will read a collision_mask { ... } block */
int traverse(const parsetree_statement_t *stmt, void *cmdetails)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4;
    cmdetails_t *s = (cmdetails_t*)cmdetails;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "source_file") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "collision_mask: must provide path to source_file");
        s->source_file = nanoparser_get_string(p1);
    }
    else if(str_icmp(identifier, "source_rect") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);
        p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");
        nanoparser_expect_string(p2, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");
        nanoparser_expect_string(p3, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");
        nanoparser_expect_string(p4, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");

        s->x = max(0, atoi(nanoparser_get_string(p1)));
        s->y = max(0, atoi(nanoparser_get_string(p2)));
        s->w = max(1, atoi(nanoparser_get_string(p3)));
        s->h = max(1, atoi(nanoparser_get_string(p4)));
    }

    return 0;
}

