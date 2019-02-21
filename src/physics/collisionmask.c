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
#include "../core/util.h"

/* private stuff ;) */
#define MEM_ALIGNMENT               sizeof(void*) /* 4 */ /* must be a power of two */
#define MASK_ALIGN(x)               (((x) + (MEM_ALIGNMENT - 1)) & ~(MEM_ALIGNMENT - 1)) /* make x a multiple of MEM_ALIGNMENT */
struct collisionmask_t {
    char* mask;
    int width;
    int height;
    int pitch;
    uint16* gmap[4];
};

/* is MEM_ALIGNMENT a power of two? */
#define IS_POWER_OF_TWO(n)          (((n) & ((n) - 1)) == 0)
typedef char _mask_assert[ !!(IS_POWER_OF_TWO(MEM_ALIGNMENT)) * 2 - 1 ];

/* utilities */
static const int MASK_MAXSIZE = UINT16_MAX; /* masks cannot be larger than this */
static uint16* create_groundmap(const collisionmask_t* mask, grounddir_t ground_direction);
static inline uint16* destroy_groundmap(uint16* gmap);

/* public methods */

/*
 * collisionmask_create()
 * Creates a new collision mask using the rectangle
 * [ x, x + width - 1 ] x [ y, y + height - 1 ]
 * of the given image
 */
collisionmask_t *collisionmask_create(const image_t *image, int x, int y, int width, int height)
{
    collisionmask_t *mask = mallocx(sizeof *mask);
    color_t pixel;
    int i, j;

    /* basic params */
    mask->width = clip(width, 1, image_width(image));
    mask->height = clip(height, 1, image_height(image));
    mask->pitch = MASK_ALIGN(mask->width);

    /* really?? */
    if(mask->width > MASK_MAXSIZE || mask->height > MASK_MAXSIZE) {
        fatal_error("Masks cannot be larger than %d pixels.", MASK_MAXSIZE);
        free(mask);
        return NULL;
    }

    /* create the collision mask */
    mask->mask = mallocx((mask->pitch * mask->height) * sizeof(*(mask->mask)));
    for(j = 0; j < mask->height; j++) {
        for(i = 0; i < mask->width; i++) {
            pixel = image_getpixel(image, x + i, y + j);
            mask->mask[j * mask->pitch + i] = !color_is_mask(pixel);
        }
    }

    /* create the ground maps */
    mask->gmap[0] = create_groundmap(mask, GD_DOWN);
    mask->gmap[1] = create_groundmap(mask, GD_LEFT);
    mask->gmap[2] = create_groundmap(mask, GD_UP);
    mask->gmap[3] = create_groundmap(mask, GD_RIGHT);

    /* done! */
    return mask;
}

/*
 * collisionmask_create_box()
 * Creates a new, solid, filled collision mask with the
 * specified dimensions
 */
collisionmask_t *collisionmask_create_box(int width, int height)
{
    collisionmask_t *mask = mallocx(sizeof *mask);
    int i, j;

    /* basic params */
    mask->width = clip(width, 1, MASK_MAXSIZE);
    mask->height = clip(height, 1, MASK_MAXSIZE);
    mask->pitch = MASK_ALIGN(mask->width);

    /* create the collision mask */
    mask->mask = mallocx((mask->pitch * mask->height) * sizeof(*(mask->mask)));
    for(j = 0; j < mask->height; j++) {
        for(i = 0; i < mask->width; i++)
            mask->mask[j * mask->pitch + i] = 1;
    }

    /* create the ground maps */
    mask->gmap[0] = create_groundmap(mask, GD_DOWN);
    mask->gmap[1] = create_groundmap(mask, GD_LEFT);
    mask->gmap[2] = create_groundmap(mask, GD_UP);
    mask->gmap[3] = create_groundmap(mask, GD_RIGHT);

    /* done! */
    return mask;
}

/*
 * collisionmask_destroy()
 * Destroys an existing collision mask
 */
collisionmask_t *collisionmask_destroy(collisionmask_t *mask)
{
    if(mask != NULL) {
        destroy_groundmap(mask->gmap[3]);
        destroy_groundmap(mask->gmap[2]);
        destroy_groundmap(mask->gmap[1]);
        destroy_groundmap(mask->gmap[0]);
        free(mask->mask);
        free(mask);
    }
    return NULL;
}

/*
 * collisionmask_width()
 * Width of the mask
 */
int collisionmask_width(const collisionmask_t* mask)
{
    return mask ? mask->width : 0;
}

/*
 * collisionmask_height()
 * Height of the mask
 */
int collisionmask_height(const collisionmask_t* mask)
{
    return mask ? mask->height : 0;
}

/*
 * collisionmask_pitch()
 * Pitch value
 */
int collisionmask_pitch(const collisionmask_t* mask)
{
    return mask ? mask->pitch : 0;
}

/*
 * collisionmask_peek()
 * Checks if a pixel is solid, with boundary checking
 */
int collisionmask_peek(const collisionmask_t* mask, int x, int y)
{
    if(mask && x >= 0 && x < mask->width && y >= 0 && y < mask->height)
        return collisionmask_at(mask, x, y, mask->pitch);
    else
        return 0;
}

/*
 * collisionmask_locate_ground()
 * Locates the ground, given pixel (x, y) in the collision mask
 */
int collisionmask_locate_ground(const collisionmask_t* mask, int x, int y, grounddir_t ground_direction)
{
    int p;

    if(!mask)
        return 0;

    x = clip(x, 0, mask->width-1);
    y = clip(y, 0, mask->height-1);

    /* this is very fast */
    switch(ground_direction) {
        case GD_DOWN:
            p = MASK_ALIGN(mask->width);
            return mask->gmap[0][p * y + x];

        case GD_LEFT:
            p = MASK_ALIGN(mask->height);
            return mask->gmap[1][p * x + y];

        case GD_UP:
            p = MASK_ALIGN(mask->width);
            return mask->gmap[2][p * y + x];

        case GD_RIGHT:
            p = MASK_ALIGN(mask->height);
            return mask->gmap[3][p * x + y];
    }

    return 0;
}



/* --- private utilities --- */

/* Creates a new ground map */
uint16* create_groundmap(const collisionmask_t* mask, grounddir_t ground_direction)
{
    int w = mask->width, h = mask->height;
    int pitch = mask->pitch;
    uint16* gmap = NULL;
    int x, y, p;

    /* compute the position of the ground */
    /* here we use a dynamic programming approach: */
    switch(ground_direction)
    {
        /*
         * the ground is "down": (gravity -> "down")
         *
         *                  y                        if mask(x,y) = 1 and mask(x,y-1) = 0
         * gmap(x,y)   =    gmap(x,y-1)              if mask(x,y) = 1 and mask(x,y-1) = 1
         *                  gmap(x,y+1)              if mask(x,y) = 0 and y < h-1
         *                  y                        if mask(x,y) = 0 and y = h-1
         */
        case GD_DOWN:
            p = MASK_ALIGN(mask->width);
            gmap = mallocx((p * h) * sizeof(*gmap));
            for(x = 0; x < w; x++) {
                if(collisionmask_at(mask, x, 0, pitch))
                    gmap[x] = 0;
                for(y = 1; y < h; y++) {
                    if(collisionmask_at(mask, x, y, pitch)) {
                        if(collisionmask_at(mask, x, y-1, pitch))
                            gmap[p * y + x] = gmap[p * (y-1) + x];
                        else
                            gmap[p * y + x] = y;
                    }
                }
                if(!collisionmask_at(mask, x, h-1, pitch))
                    gmap[p * (h-1) + x] = h-1;
                for(y = h-2; y >= 0; y--) {
                    if(!collisionmask_at(mask, x, y, pitch))
                        gmap[p * y + x] = gmap[p * (y+1) + x];
                }
            }
            break;

        /* the ground is "to the left" */
        case GD_LEFT:
            p = MASK_ALIGN(mask->height);
            gmap = mallocx((p *  w) * sizeof(*gmap));
            for(y = 0; y < h; y++) {
                if(collisionmask_at(mask, w-1, y, pitch))
                    gmap[p * (w-1) + y] = w-1;
                for(x = w-2; x >= 0; x--) {
                    if(collisionmask_at(mask, x, y, pitch)) {
                        if(collisionmask_at(mask, x+1, y, pitch))
                            gmap[p * x + y] = gmap[p * (x+1) + y];
                        else
                            gmap[p * x + y] = x;
                    }
                }
                if(!collisionmask_at(mask, 0, y, pitch))
                    gmap[y] = 0;
                for(x = 1; x < w; x++) {
                    if(!collisionmask_at(mask, x, y, pitch))
                        gmap[p * x + y] = gmap[p * (x-1) + y];
                }
            }
            break;

        /* the ground is upwards */
        case GD_UP:
            p = MASK_ALIGN(mask->width);
            gmap = mallocx((p * h) * sizeof(*gmap));
            for(x = 0; x < w; x++) {
                if(collisionmask_at(mask, x, h-1, pitch))
                    gmap[p * (h-1) + x] = h-1;
                for(y = h-2; y >= 0; y--) {
                    if(collisionmask_at(mask, x, y, pitch)) {
                        if(collisionmask_at(mask, x, y+1, pitch))
                            gmap[p * y + x] = gmap[p * (y+1) + x];
                        else
                            gmap[p * y + x] = y;
                    }
                }
                if(!collisionmask_at(mask, x, 0, pitch))
                    gmap[x] = 0;
                for(y = 1; y < h; y++) {
                    if(!collisionmask_at(mask, x, y, pitch))
                        gmap[p * y + x] = gmap[p * (y-1) + x];
                }
            }
            break;

        /* the ground is "to the right" */
        case GD_RIGHT:
            p = MASK_ALIGN(mask->height);
            gmap = mallocx((p * w) * sizeof(*gmap));
            for(y = 0; y < h; y++) {
                if(collisionmask_at(mask, 0, y, pitch))
                    gmap[y] = 0;
                for(x = 1; x < w; x++) {
                    if(collisionmask_at(mask, x, y, pitch)) {
                        if(collisionmask_at(mask, x-1, y, pitch))
                            gmap[p * x + y] = gmap[p * (x-1) + y];
                        else
                            gmap[p * x + y] = x;
                    }
                }
                if(!collisionmask_at(mask, w-1, y, pitch))
                    gmap[p * (w-1) + y] = w-1;
                for(x = w-2; x >=0; x--) {
                    if(!collisionmask_at(mask, x, y, pitch))
                        gmap[p * x + y] = gmap[p * (x+1) + y];
                }
            }
            break;
    }

    /* done */
    return gmap;
}

/* Destroys an existing ground map */
uint16* destroy_groundmap(uint16* gmap)
{
    if(gmap != NULL)
        free(gmap);
    return NULL;
}