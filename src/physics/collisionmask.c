/*
 * Open Surge Engine
 * collisionmask.h - collision mask
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

#include <stdlib.h>
#include "collisionmask.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/logfile.h"
#include "../util/util.h"



/* Collision mask structure */
struct collisionmask_t {

    /* mask data */
    uint8_t* mask; /* this must be the first entry; it's a binary image: solid pixel is 1 and non-solid pixel is 0 */
    int width;
    int height;
    int pitch;

    /* integral mask for constant-time collision detection */
    uint32_t* integral_mask;

    /* ground maps for each ground direction */
    uint16_t* gmap[4];

};

/* cloudify */
static const int CLOUD_HEIGHT = 16 + 8; /* give it some slack for steep slopes & very high speeds */
static void cloudify_mask(collisionmask_t* mask);

/* ground maps */
static uint16_t* create_groundmap(const collisionmask_t* mask, grounddir_t ground_direction);
static inline uint16_t* clone_groundmap(uint16_t* gmap, int width, int height, grounddir_t ground_direction);
static inline uint16_t* destroy_groundmap(uint16_t* gmap);

/* integral masks */
static uint32_t* create_integral_mask(const collisionmask_t* mask);
static uint32_t* clone_integral_mask(uint32_t* integral_mask, int width, int height);
static inline uint32_t* destroy_integral_mask(uint32_t* integral_mask);

/*

INTEGRAL MASKS
--------------
by alemart



Motivation

A use case that occurs often in this engine is the need to detect collisions
between a sensor and a collision mask. A sensor is a vertical or a horizontal
line (possibly a single pixel). A collision mask is basically a binary image.
I used to check each pixel of the sensor, but that is slow because the number
of memory reads is dependent on the size of the sensor. Fortunately, there is
a better way to detect collisions, and it just came to me that I can do it in
constant time.



Definitions

Let us study the problem of testing if a rectangular area of the collision mask
has any solid pixels in it. Since sensors are rectangles, we can trivially map
this problem to a collision detection routine (see below).

- I define collision mask M as a 2D image. Each entry M[x,y] is either 0 or 1,
  indicating the solidity of the pixel at (x,y). x and y are integers such that
  0 <= x < w and 0 <= y < h. w is the width of the mask. h is the height.

- Let R = [l,r] x [t,b] denote a rectangle, where l <= r and t <= b. I will say
  that the area test between R and M succeeds if at least one entry of the set
  M intersect R = { M[x,y] | l <= x <= r and t <= y <= b } is not zero. The
  area test fails if there is no such entry.

  > Note: in order to simplify the argument, I consider that the rectangle R
  > is completely within the mask M. If it's not, you can trivially adjust it.
  > If the rectangle is completely outside the mask, then the area test fails.
  > (equivalently, you may extend M[x,y] beyond its bounds to infinity with 0s)

- I define matrix S[x,y] for all 0 <= x <= w and 0 <= y <= h as follows:

           { 0 if x == 0 or y == 0
  S[x,y] = { sum from i=0 to y-1 of ( sum from j=0 to x-1 of M[j,i] ) otherwise

  I call S the integral mask of M. It's a summed area table of a binary image.
  Note that S has (w+1) columns and (h+1) rows.

There is a collision between a sensor line and a mask if the corresponding area
test succeeds.



Efficiently performing an area test

Let R = [l,r] x [t,b] denote a rectangle and M a collision mask. The area test
between R and M succeeds if and only if:

sum from y=t to b of ( sum from x=l to r of M[x,y] ) > 0

Proof sketch: M[x,y] is either 0 or 1.

After some algebraic manipulations, I was able to rewrite above expression as a
function of the integral mask S:

S[r+1,b+1] - S[l,b+1] > S[r+1,t] - S[l,t]             (an equivalent area test)

With an integral mask, we can perform the area test in constant time! There is
no need to check individual pixels. This collision detection is blazingly fast!



Pre-computing the integral mask

Given a collision mask M, its integral mask S can be efficiently pre-computed
with the following recurrence formula:

         { 0 if x == 0 or y == 0
S[x,y] = { M[x-1,y-1] + (S[x,y-1] - S[x-1,y-1]) + S[x-1,y]   if 1 <= x <= w
         {                                                  and 1 <= y <= h

You can easily see that this formula gives:

S[x+1,y+1] = sum from i=0 to y of ( sum from j=0 to x of M[j,i] )

Prove by induction.

*/

/*

Maximum mask size

Integer MASK_MAXSIZE (m) must satisfy m^2 < 2^32, so that we can safely store
the integral mask as a matrix of 32-bit unsigned integers.

hint: the right-most entry x of the y-th row of the integral image I of M
      satisfies I(y,x) <= x*y. The equality takes place if M(y,x) = 1 for all
      pixels. Therefore, the values of I are bound by m^2.

The maximum m that satisfies that formula is m = 65535. We shouldn't use that
because we're not going to deal with masks that large and, in theory, we could
see overflow if we performed calculations based on signed 32-bit integers e.g.,
65535^2 > 2^31 - 1.

Given a number of bits b, we want a suitable m such that m^2 < 2^b. Just pick
some m <= 2^(b/2) - 1. I use b = 30, which gives a maximum m of 32767. It is
much more than enough for practical use!

*/
#define MASK_MAXSIZE 4096 /* reliable max texture size */ /* masks cannot be larger than this */
STATIC_ASSERTX(MASK_MAXSIZE <= 32767, validate_collisionmask_maxsize); /* will not cause overflow */

/* Memory alignment utility (deprecated?) */
#if 0
#define MEM_ALIGNMENT               sizeof(void*) /* must be a power of two */
#define IS_POWER_OF_TWO(n)          (((n) & ((n) - 1)) == 0)
#define MASK_ALIGN(x)               (((x) + (MEM_ALIGNMENT - 1)) & ~(MEM_ALIGNMENT - 1)) /* make x a multiple of MEM_ALIGNMENT */
STATIC_ASSERTX(IS_POWER_OF_TWO(MEM_ALIGNMENT), validate_collisionmask_memalignment); /* is MEM_ALIGNMENT a power of two? */
#else
#define MASK_ALIGN(x)               (x) /* identity function */
#endif





/*
 * public functions
 */


/*
 * collisionmask_create()
 * Creates a new collision mask using the rectangle
 * [ x, x + width - 1 ] x [ y, y + height - 1 ]
 * of the given image, which should be locked
 */
collisionmask_t *collisionmask_create(const image_t *image, int x, int y, int width, int height, int flags)
{
    collisionmask_t *mask = mallocx(sizeof *mask);

    /* basic params */
    mask->width = clip(width, 1, image_width(image));
    mask->height = clip(height, 1, image_height(image));
    mask->pitch = MASK_ALIGN(mask->width);

    /* really?? */
    if(mask->width > MASK_MAXSIZE || mask->height > MASK_MAXSIZE) {
        free(mask);
        fatal_error("Masks cannot be larger than %d pixels.", MASK_MAXSIZE);
        return NULL;
    }

    /* make sure that the image is locked */
    if(!image_is_locked(image)) {
        /* a sub-image may not be locked... the program may crash if you try to lock one (driver?) */
        logfile_message("%s: image \"%s\" is not locked", __func__, image_filepath(image));
    }

    /* create the collision mask */
    size_t mask_size = (mask->pitch * mask->height) * sizeof(*(mask->mask));
    mask->mask = mallocx(mask_size);
    memset(mask->mask, 0, mask_size);

    for(int j = 0, jp = 0; j < mask->height; j++, jp += mask->pitch) {
        for(int i = 0; i < mask->width; i++) {
            if(!color_is_transparent(image_getpixel(image, x + i, y + j)))
                mask->mask[jp + i] = 1;
        }
    }

    /* "cloudify" mask */
    if(flags & CMF_CLOUDIFY)
        cloudify_mask(mask);

    /* create the integral mask */
    mask->integral_mask = create_integral_mask(mask);

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

    /* basic params */
    mask->width = clip(width, 1, MASK_MAXSIZE);
    mask->height = clip(height, 1, MASK_MAXSIZE);
    mask->pitch = MASK_ALIGN(mask->width);

    /* create the collision mask */
    size_t mask_size = (mask->pitch * mask->height) * sizeof(*(mask->mask));
    mask->mask = mallocx(mask_size);
    memset(mask->mask, 1, mask_size);

    /* create the integral mask */
    mask->integral_mask = create_integral_mask(mask);

    /* create the ground maps */
    mask->gmap[0] = create_groundmap(mask, GD_DOWN);
    mask->gmap[1] = create_groundmap(mask, GD_LEFT);
    mask->gmap[2] = create_groundmap(mask, GD_UP);
    mask->gmap[3] = create_groundmap(mask, GD_RIGHT);

    /* done! */
    return mask;
}

/*
 * collisionmask_clone()
 * Clones a collision mask
 */
collisionmask_t* collisionmask_clone(const collisionmask_t* mask)
{
    /* clone the fields */
    collisionmask_t* clone = mallocx(sizeof *clone);
    memcpy(clone, mask, sizeof(*clone));

    /* clone the mask data */
    size_t mask_size = (mask->pitch * mask->height) * sizeof(*(mask->mask));
    clone->mask = mallocx(mask_size);
    memcpy(clone->mask, mask->mask, mask_size);

    /* clone the integral mask */
    clone->integral_mask = clone_integral_mask(mask->integral_mask, mask->width, mask->height);

    /* clone the ground maps */
    clone->gmap[0] = clone_groundmap(mask->gmap[0], mask->width, mask->height, GD_DOWN);
    clone->gmap[1] = clone_groundmap(mask->gmap[1], mask->width, mask->height, GD_LEFT);
    clone->gmap[2] = clone_groundmap(mask->gmap[2], mask->width, mask->height, GD_UP);
    clone->gmap[3] = clone_groundmap(mask->gmap[3], mask->width, mask->height, GD_RIGHT);

    /* done! */
    return clone;
}

/*
 * collisionmask_destroy()
 * Destroys an existing collision mask
 */
collisionmask_t *collisionmask_destroy(collisionmask_t *mask)
{
    /* nothing to do? */
    if(!mask)
        return NULL;

    /* release the ground maps */
    destroy_groundmap(mask->gmap[3]);
    destroy_groundmap(mask->gmap[2]);
    destroy_groundmap(mask->gmap[1]);
    destroy_groundmap(mask->gmap[0]);

    /* release the integral mask */
    destroy_integral_mask(mask->integral_mask);

    /* release the mask data & struct */
    free(mask->mask);
    free(mask);

    /* done */
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
 * collisionmask_pixel_test()
 * Checks if a pixel is solid, with boundary checking
 */
bool collisionmask_pixel_test(const collisionmask_t* mask, int x, int y)
{
    if(mask && x >= 0 && x < mask->width && y >= 0 && y < mask->height)
        return collisionmask_at(mask, x, y, mask->pitch);
    else
        return false;
}

/*
 * collisionmask_area_test()
 * Quickly checks if any pixel of the rectangle [left,right] x [top,bottom] is solid.
 * We expect left <= right and top <= bottom. Coordinates are inclusive.
 */
bool collisionmask_area_test(const collisionmask_t* mask, int left, int top, int right, int bottom)
{
    /* validate input */
    if(!mask || left > right || top > bottom)
        return false;

    /* is the rectangle outside the mask? */
    int r = mask->width - 1;
    int b = mask->height - 1;

    if(right < 0 || left > r || bottom < 0 || top > b)
        return false;

    /* enforce limits */
    if(left < 0)
        left = 0;
    if(right > r)
        right = r;
    if(top < 0)
        top = 0;
    if(bottom > b)
        bottom = b;

    /* super fast area test */
    int p = MASK_ALIGN(mask->width + 1); /* pitch of the integral mask */
    const uint32_t* s = mask->integral_mask;
    return s[(bottom+1)*p + (right+1)] - s[(bottom+1)*p + left] > s[top*p + (right+1)] - s[top*p + left];

    /* there is no overflow nor unsigned integer wraparound. Both sides of the
       comparison are non-negative. Reminder: s[y][x] = 0 if x = 0 or y = 0;
       s[y][x] = sum from i=0 to y-1 of ( sum from j=0 to x-1 of M[i][j] )
       if 1 <= x <= width and 1 <= y <= height. M[i][j] is in {0,1} for all i,j.
       Therefore, s[y][x+k] - s[y][x] >= 0 for any valid k >= 0 and for all x,y. */
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

    /* out of bounds check */
    if(x < 0 || x >= mask->width) {
        /* minimum level */
        if(ground_direction == GD_DOWN)
            return mask->height - 1;
        else if(ground_direction == GD_UP)
            return 0;

        /* clip x */
        else if(x < 0)
            x = 0;
        else
            x = mask->width - 1;
    }

    if(y < 0 || y >= mask->height) {
        /* minimum level */
        if(ground_direction == GD_RIGHT)
            return mask->width - 1;
        else if(ground_direction == GD_LEFT)
            return 0;

        /* clip y */
        else if(y < 0)
            y = 0;
        else
            y = mask->height - 1;
    }

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

/*
 * collisionmask_to_image()
 * Creates a binary image with colored solid pixels and transparent passable pixels
 * You must destroy the image_t* after usage
 */
image_t* collisionmask_to_image(const collisionmask_t* mask, color_t color)
{
    image_t* target = image_drawing_target();
    image_t* img = image_create(mask->width, mask->height);
    color_t transparent = color_rgba(0, 0, 0, 0);

    /* draw the mask */
    image_set_drawing_target(img);
    image_clear(transparent);
    image_lock(img, "rw");

    for(int y = 0; y < mask->height; y++) {
        for(int x = 0; x < mask->width; x++) {
            if(collisionmask_at(mask, x, y, mask->pitch))
                image_putpixel(x, y, color);
        }
    }

    image_unlock(img);
    image_set_drawing_target(target);

    /* done */
    return img;
}




/*
 * "cloudify" masks
 */

/* make the collision mask solid only from the top */
void cloudify_mask(collisionmask_t* mask)
{
    for(int i = 0; i < mask->width; i++) {
        int l = CLOUD_HEIGHT;
        for(int j = 0, jp = 0; j < mask->height; j++, jp += mask->pitch) {
            if(mask->mask[jp + i]) {
                if(--l < 0)
                    mask->mask[jp + i] = 0;
            }
            else
                l = CLOUD_HEIGHT;
        }
    }
}




/*
 * ground maps
 */


/* Creates a new ground map */
uint16_t* create_groundmap(const collisionmask_t* mask, grounddir_t ground_direction)
{
    int w = mask->width, h = mask->height;
    int pitch = mask->pitch;
    uint16_t* gmap = NULL;
    int x, y, p;

    /* compute the position of the ground */
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
uint16_t* destroy_groundmap(uint16_t* gmap)
{
    if(gmap != NULL)
        free(gmap);
    return NULL;
}

/* Clones an existing ground map */
uint16_t* clone_groundmap(uint16_t* gmap, int width, int height, grounddir_t ground_direction)
{
    uint16_t* clone = NULL;
    size_t size = 0;
    int pitch = 0;

    switch(ground_direction)
    {
        case GD_DOWN:
        case GD_UP:
            pitch = MASK_ALIGN(width);
            size = (pitch * height) * sizeof(*clone);
            break;

        case GD_LEFT:
        case GD_RIGHT:
            pitch = MASK_ALIGN(height);
            size = (pitch *  width) * sizeof(*clone);
            break;
    }

    clone = mallocx(size);
    memcpy(clone, gmap, size);
    return clone;
}




/*
 * Integral masks
 */

/* Creates an integral mask based on a collision mask */
uint32_t* create_integral_mask(const collisionmask_t* mask)
{
    int width = mask->width;
    int height = mask->height;
    int pitch = mask->pitch;

    int p = MASK_ALIGN(width + 1); /* pitch of the integral mask */
    uint32_t* integral_mask = NULL;
    size_t integral_mask_size = (p * (height + 1)) * sizeof(*integral_mask);
    integral_mask = mallocx(integral_mask_size);

    /* initialize the first row */
    for(int x = 0; x <= width; x++)
        integral_mask[x] = 0;

    /* initialize the first column */
    for(int y = 0; y <= height; y++)
        integral_mask[y*p] = 0;

    /* compute the integral mask */
    for(int y = 1; y <= height; y++) {
        for(int x = 1; x <= width; x++) {
            /* will not overflow */
            integral_mask[y*p + x] = integral_mask[y*p + (x-1)] +
                                     (integral_mask[(y-1)*p + x] - integral_mask[(y-1)*p + (x-1)]) +
                                     (uint32_t)(collisionmask_at(mask, x-1, y-1, pitch));
        }
    }

    /* done */
    return integral_mask;
}

/* Clones an integral mask */
uint32_t* clone_integral_mask(uint32_t* integral_mask, int width, int height)
{
    int pitch = MASK_ALIGN(width + 1);
    size_t clone_size = (pitch * (height + 1)) * sizeof(*integral_mask);
    uint32_t* clone = mallocx(clone_size);

    memcpy(clone, integral_mask, clone_size);

    return clone;
}

/* Destroys an integral mask */
uint32_t* destroy_integral_mask(uint32_t* integral_mask)
{
    if(integral_mask != NULL)
        free(integral_mask);

    return NULL;
}