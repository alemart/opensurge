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
#include "../../core/video.h"
#include "../../core/image.h"
#include "../../core/util.h"

/* private stuff ;) */
#define MEM_ALIGNMENT               sizeof(void*) /* 4 */ /* must be a power of two */
#define MASK_ALIGN(x)               (((x) + (MEM_ALIGNMENT - 1)) & ~(MEM_ALIGNMENT - 1))
struct collisionmask_t {
    char* mask;
    int width;
    int height;
};

/* is MEM_ALIGNMENT a power of two? */
#define IS_POWER_OF_TWO(n)          (((n) & ((n) - 1)) == 0)
typedef char _mask_assert[ !!(IS_POWER_OF_TWO(MEM_ALIGNMENT)) * 2 - 1 ];

/* public methods */
collisionmask_t *collisionmask_create(const struct image_t *image, int x, int y, int width, int height)
{
    collisionmask_t *mask = mallocx(sizeof *mask);
    uint32 maskcolor = video_get_maskcolor();
    int pitch;

    mask->width = max(1, width);
    mask->height = max(1, height);
    pitch = MASK_ALIGN(mask->width);

    mask->mask = mallocx(pitch * mask->height);
    for(int j = 0; j < mask->height; j++) {
        for(int i = 0; i < mask->width; i++)
            mask->mask[j * pitch + i] = (image_getpixel(image, x + i, y + j) != maskcolor);
    }
    video_showmessage("%d, %d", mask->width, pitch);
    return mask;
}

collisionmask_t *collisionmask_destroy(collisionmask_t *mask)
{
    if(mask != NULL) {
        free(mask->mask);
        free(mask);
    }
    return NULL;
}

int collisionmask_width(const collisionmask_t* mask)
{
    return mask ? mask->width : 0;
}

int collisionmask_height(const collisionmask_t* mask)
{
    return mask ? mask->height : 0;
}

int collisionmask_pitch(const collisionmask_t* mask)
{
    return mask ? MASK_ALIGN(mask->width) : 0;
}