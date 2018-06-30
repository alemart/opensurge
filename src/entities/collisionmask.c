/*
 * Open Surge Engine
 * collisionmask.h - collision mask
 * Copyright (C) 2012, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
    char* mask;
    int width;
    int height;
};


/* public methods */
collisionmask_t *collisionmask_create(const struct image_t *image, int x, int y, int width, int height)
{
    collisionmask_t *cm = mallocx(sizeof *cm);
    uint32 maskcolor = video_get_maskcolor();

    cm->width = max(1, width);
    cm->height = max(1, height);
    cm->mask = mallocx(cm->width * cm->height);
    for(int j = 0; j < cm->height; j++) {
        for(int i = 0; i < cm->width; i++)
            cm->mask[j * cm->width + i] = (image_getpixel(image, x + i, y + j) != maskcolor);
    }

    return cm;
}

collisionmask_t *collisionmask_destroy(collisionmask_t *cm)
{
    free(cm->mask);
    free(cm);
    return NULL;
}

int collisionmask_width(const collisionmask_t* cm)
{
    return cm->width;
}

int collisionmask_height(const collisionmask_t* cm)
{
    return cm->height;
}
