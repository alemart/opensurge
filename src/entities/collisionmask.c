/*
 * Open Surge Engine
 * collisionmask.h - collision mask
 * Copyright (C) 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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
    int width;
    int height;
};


/* public methods */
collisionmask_t *collisionmask_create(const struct image_t *image, int x, int y, int width, int height)
{
    collisionmask_t *cm = mallocx(sizeof *cm);
    cm->mask = image_create_shared(image, x, y, width, height);
    cm->width = width;
    cm->height = height;
    return cm;
}

collisionmask_t *collisionmask_destroy(collisionmask_t *cm)
{
    image_destroy(cm->mask);
    free(cm);
    return NULL;
}

int collisionmask_check(const collisionmask_t *cm, int x, int y)
{
    return image_getpixel(cm->mask, x, y) != video_get_maskcolor();
}

int collisionmask_width(const collisionmask_t* cm)
{
    return image_width(cm->mask);
}

int collisionmask_height(const collisionmask_t* cm)
{
    return image_height(cm->mask);
}
