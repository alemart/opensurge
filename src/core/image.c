/*
 * Open Surge Engine
 * image.c - image implementation
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

#include <string.h>
#include <stdint.h>
#include "image.h"
#include "video.h"
#include "stringutil.h"
#include "logfile.h"
#include "asset.h"
#include "util.h"
#include "resourcemanager.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

/* convert imageflags_t to ALLEGRO_FLIP flags */
#define FLIPPY(flags) ((((flags) & IF_HFLIP) != 0) * ALLEGRO_FLIP_HORIZONTAL + (((flags) & IF_VFLIP) != 0) * ALLEGRO_FLIP_VERTICAL)

/* check if an expression is a power of two */
#define IS_POWER_OF_TWO(n) (((n) & ((n) - 1)) == 0)

/* image type */
struct image_t {
    ALLEGRO_BITMAP* data; /* this must be the first field */
    int w, h;
    char* path; /* relative path */
};

/* misc */
static image_t* target = NULL; /* drawing target */
static const int MAX_IMAGE_SIZE = 4096; /* maximum image size for broad compatibility with video cards */

/*
 * image_load()
 * Loads a image from a file.
 * Supported types: PNG, JPG, BMP, PCX, TGA
 */
image_t* image_load(const char* path)
{
    image_t* img;

    if(NULL == (img = resourcemanager_find_image(path))) {
        const char* fullpath = asset_path(path);
        logfile_message("Loading image \"%s\"...", fullpath);

        /* build the image object */
        img = mallocx(sizeof *img);

        /* loading the image */
        if(NULL == (img->data = al_load_bitmap(fullpath))) {
            fatal_error("Failed to load image \"%s\"", fullpath);
            free(img);
            return NULL;
        }

        /* checking the size */
        img->w = al_get_bitmap_width(img->data);
        img->h = al_get_bitmap_height(img->data);
        if(img->w > MAX_IMAGE_SIZE || img->h > MAX_IMAGE_SIZE) {
            /* ensure broad compatibility with video cards */
            al_destroy_bitmap(img->data);
            free(img);
            fatal_error("Failed to load \"%s\": images can't be larger than %dx%d", path, MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);
            return NULL;
        }

        /* convert mask to alpha */
        al_convert_mask_to_alpha(img->data, al_map_rgb(255, 0, 255));

        /* adding the image to the resource manager */
        img->path = str_dup(path);
        resourcemanager_add_image(img->path, img);
        resourcemanager_ref_image(img->path);
    }
    else
        resourcemanager_ref_image(path);

    return img;
}



/*
 * image_save()
 * Saves a image to a file
 */
void image_save(const image_t* img, const char *path)
{
    const char* fullpath = asset_path(path);

    if(al_save_bitmap(fullpath, img->data))
        logfile_message("Saved image to \"%s\"", fullpath);
    else
        logfile_message("Failed to save image to \"%s\"", fullpath);
}



/*
 * image_create()
 * Creates a new image of a given size
 */
image_t* image_create(int width, int height)
{
    ALLEGRO_BITMAP* bmp;
    ALLEGRO_STATE state;
    image_t* img;

    if(width > MAX_IMAGE_SIZE || height > MAX_IMAGE_SIZE)
        logfile_message("WARNING: image_create(%d,%d) - larger than %dx%d", width, height, MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);

    if(width <= 0 || height <= 0) {
        fatal_error("Can't create image of size %dx%d", width, height);
        return NULL;
    }

    if(NULL == (bmp = al_create_bitmap(width, height))) {
        logfile_message("ERROR: image_create(%d,%d) failed", width, height);
        return NULL;
    }

    al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
    al_set_target_bitmap(bmp);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_restore_state(&state);

    img = mallocx(sizeof *img);
    img->data = bmp;
    img->w = width;
    img->h = height;
    img->path = NULL;
    
    return img;
}


/*
 * image_destroy()
 * Destroys an image. This is called automatically
 * while unloading the resource manager.
 */
void image_destroy(image_t* img)
{
    if(img->data != NULL)
        al_destroy_bitmap(img->data);

    if(img->path != NULL)
        free(img->path);

    if(target == img)
        target = NULL;

    free(img);
}

/*
 * image_create_shared()
 * Creates a sub-image, ie., an image sharing memory with an
 * existing image, but possibly with a different size. Please
 * remember to free the sub-image before freeing the parent
 * image to avoid memory leaks and potential crashes acessing
 * memory which has been freed.
 */
image_t* image_create_shared(const image_t* parent, int x, int y, int width, int height)
{
    image_t* img;
    int pw, ph;

    if(width <= 0 || height <= 0) {
        fatal_error("Can't create shared image of size %d x %d", width, height);
        return NULL;
    }

    pw = parent->w;
    ph = parent->h;
    x = clip(x, 0, pw-1);
    y = clip(y, 0, ph-1);
    width = clip(width, 0, pw-x);
    height = clip(height, 0, ph-y);

    img = mallocx(sizeof *img);
    img->w = width;
    img->h = height;
    if(NULL == (img->data = al_create_sub_bitmap(parent->data, x, y, width, height)))
        fatal_error("Failed to create shared image of \"%s\": %d, %d, %d, %d", parent->path ? parent->path : "", x, y, width, height);

    img->path = NULL;
    if(parent->path != NULL) {
        img->path = str_dup(parent->path);
        resourcemanager_ref_image(img->path); /* reference it, otherwise the parent may be destroyed */
    }

    return img;
}


/*
 * image_unload()
 * Will try to release the resource from
 * the memory. You will call this if you
 * don't need the resource anymore.
 *
 * Used for reference counting. Normally you
 * don't need to bother with this, unless
 * you care about reducing memory usage.
 *
 * Returns the no. of references to the image
 */
int image_unload(image_t* img)
{
    if(img->path != NULL)
       return resourcemanager_unref_image(img->path);
    else
        return -1; /* error; the image was not loaded from a file */
}

/*
 * image_clone()
 * Clones an existing image; make sure you
 * call image_destroy() after usage
 */
image_t* image_clone(const image_t* src)
{
    image_t* img = mallocx(sizeof *img);

    img->w = src->w;
    img->h = src->h;
    img->path = NULL;
    if(NULL == (img->data = al_clone_bitmap(src->data)))
        fatal_error("Failed to clone image \"%s\" sized %dx%d", src->path ? src->path : "", src->w, src->h);

    return img;
}

/*
 * image_snapshot()
 * Take a snapshot of the game. Remember to
 * destroy the created image after usage.
 */
image_t* image_snapshot()
{
    ALLEGRO_STATE state;
    image_t* img = mallocx(sizeof *img);
    ALLEGRO_BITMAP* screen = al_get_backbuffer(al_get_current_display());
    al_store_state(&state, ALLEGRO_STATE_BITMAP);

    /*al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);*/ /* doesn't work properly */
    img->w = al_get_bitmap_width(screen);
    img->h = al_get_bitmap_height(screen);
    img->path = NULL;
    if(NULL == (img->data = al_create_bitmap(img->w, img->h)))
        fatal_error("Failed to take snapshot");
    al_set_target_bitmap(img->data);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(screen, 0.0f, 0.0f, 0);

    al_restore_state(&state);
    return img;
}

/*
 * image_enable_linear_filtering()
 * Enable linear filtering
 */
void image_enable_linear_filtering(image_t* img)
{
    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

    ALLEGRO_BITMAP* root = img->data;
    while(al_get_parent_bitmap(root) != NULL)
        root = al_get_parent_bitmap(root);

    int width = al_get_bitmap_width(root), height = al_get_bitmap_height(root);
    bool is_pot = IS_POWER_OF_TWO(width) && IS_POWER_OF_TWO(height);
    int mipmap = is_pot ? ALLEGRO_MIPMAP : 0;

    int flags = al_get_bitmap_flags(img->data);
    al_set_new_bitmap_flags(flags | (mipmap | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR));
    al_convert_bitmap(img->data);

    /*

    According to the Allegro docs, "if this bitmap is a sub-bitmap, then it,
    its parent and all the sibling sub-bitmaps are also converted". Does
    this mean that the parent (root) bitmap must be a POT texture? The size
    of the sub-bitmaps shouldn't matter.

    ALLEGRO_MIPMAP
    "This can only be used for bitmaps whose width and height is a power of two"

    https://liballeg.org/a5docs/trunk/graphics.html#al_convert_bitmap
    https://liballeg.org/a5docs/trunk/graphics.html#al_set_new_bitmap_flags

    */

    al_restore_state(&state);
}

/*
 * image_disable_linear_filtering()
 * Disable linear filtering
 */
void image_disable_linear_filtering(image_t* img)
{
    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

    int flags = al_get_bitmap_flags(img->data);
    al_set_new_bitmap_flags(flags & ~(ALLEGRO_MIPMAP | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR));
    al_convert_bitmap(img->data);

    al_restore_state(&state);
}

/*
 * image_lock()
 * Locks the image, enabling fast in-memory pixel access
 */
void image_lock(image_t* img)
{
    al_lock_bitmap(img->data, al_get_bitmap_format(img->data), ALLEGRO_LOCK_READWRITE);
}

/*
 * image_unlock()
 * Unlocks the image
 */
void image_unlock(image_t* img)
{
    al_unlock_bitmap(img->data);
}


/*
 * image_width()
 * The width of the image
 */
int image_width(const image_t* img)
{
    return img->w;
}


/*
 * image_height()
 * The height of the image
 */
int image_height(const image_t* img)
{
    return img->h;
}


/*
 * image_getpixel()
 * Returns the pixel at the given position on the image
 */
color_t image_getpixel(const image_t* img, int x, int y)
{
    return (color_t){ al_get_pixel(img->data, x, y) };
}


/*
 * image_putpixel()
 * Puts a pixel on the target image. Make sure to lock it first
 */
void image_putpixel(int x, int y, color_t color)
{
    al_put_pixel(x, y, color._color);
}


/*
 * image_line()
 * Draws a line from (x1,y1) to (x2,y2) using the specified color
 */
void image_line(int x1, int y1, int x2, int y2, color_t color)
{
    al_draw_line(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, color._color, 0.0f);
}


/*
 * image_ellipse()
 * Draws an ellipse with the specified centre, radius and color
 */
void image_ellipse(int cx, int cy, int radius_x, int radius_y, color_t color)
{
    al_draw_ellipse(cx + 0.5f, cy + 0.5f, radius_x, radius_y, color._color, 0.0f);
}


/*
 * image_ellipsefill()
 * Draws a filled ellipse with the specified centre, radius and color
 */
void image_ellipsefill(int cx, int cy, int radius_x, int radius_y, color_t color)
{
    al_draw_filled_ellipse(cx + 0.5f, cy + 0.5f, radius_x, radius_y, color._color);
}


/*
 * image_rect()
 * Draws a rectangle
 */
void image_rect(int x1, int y1, int x2, int y2, color_t color)
{
    al_draw_rectangle(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, color._color, 0.0f);
}


/*
 * image_rectfill()
 * Draws a filled rectangle
 */
void image_rectfill(int x1, int y1, int x2, int y2, color_t color)
{
    al_draw_filled_rectangle(x1, y1, x2 + 1.0f, y2 + 1.0f, color._color);
}


/*
 * image_clear()
 * Clears an given image with some color
 */
void image_clear(color_t color)
{
    al_clear_to_color(color._color);
}


/*
 * image_blit()
 * Blits a surface onto another
 */
void image_blit(const image_t* src, int src_x, int src_y, int dest_x, int dest_y, int width, int height)
{
    al_draw_bitmap_region(src->data, src_x, src_y, width, height, dest_x, dest_y, 0);
}


/*
 * image_draw()
 * Draws an image onto the destination surface
 * at the specified position
 *
 * flags: refer to the IF_* defines (Image Flags)
 */
void image_draw(const image_t* src, int x, int y, imageflags_t flags)
{
    al_draw_bitmap(src->data, x, y, FLIPPY(flags));
}



/*
 * image_draw_scaled()
 * Draws a scaled image onto the destination surface
 * at the specified position
 *
 * scale: (1.0, 1.0) is the original size
 *        (2.0, 2.0) stands for a double-sized image
 *        (0.5, 0.5) stands for a smaller image
 */
void image_draw_scaled(const image_t* src, int x, int y, v2d_t scale, imageflags_t flags)
{ 
    al_draw_scaled_bitmap(
        src->data,
        0.0f, 0.0f, src->w, src->h,
        x, y, scale.x * src->w, scale.y * src->h,
        FLIPPY(flags)
    );
}

/*
 * image_draw_scaled_trans()
 * Draw scaled with alpha
 */
void image_draw_scaled_trans(const image_t* src, int x, int y, v2d_t scale, float alpha, imageflags_t flags)
{
    float a = clip01(alpha);
    ALLEGRO_COLOR tint = al_map_rgba_f(a, a, a, a);

    al_draw_tinted_scaled_bitmap(
        src->data, tint,
        0.0f, 0.0f, src->w, src->h,
        x, y, scale.x * src->w, scale.y * src->h,
        FLIPPY(flags)
    );
}

/*
 * image_draw_rotated()
 * Draws a rotated image onto the destination bitmap at the specified position 
 * radians: angle given in radians (counter-clockwise)
 * cx, cy: pivot positions
 */
void image_draw_rotated(const image_t* src, int x, int y, int cx, int cy, float radians, imageflags_t flags)
{
    al_draw_rotated_bitmap(src->data, cx, cy, x, y, -radians, FLIPPY(flags));
}

/*
 * image_draw_rotated_trans()
 * Draws a rotated image with alpha
 */
void image_draw_rotated_trans(const image_t* src, int x, int y, int cx, int cy, float radians, float alpha, imageflags_t flags)
{
    float a = clip01(alpha);
    ALLEGRO_COLOR tint = al_map_rgba_f(a, a, a, a);

    al_draw_tinted_rotated_bitmap(src->data, tint, cx, cy, x, y, -radians, FLIPPY(flags));
}

/*
 * image_draw_scaled_rotated()
 * Draws a scaled and rotated image
 */
void image_draw_scaled_rotated(const image_t* src, int x, int y, int cx, int cy, v2d_t scale, float radians, imageflags_t flags)
{
    al_draw_scaled_rotated_bitmap(src->data, cx, cy, x, y, scale.x, scale.y, -radians, FLIPPY(flags));
}

/*
 * image_draw_scaled_rotated_trans()
 * Draws a scaled and rotated image with alpha
 */
void image_draw_scaled_rotated_trans(const image_t* src, int x, int y, int cx, int cy, v2d_t scale, float radians, float alpha, imageflags_t flags)
{
    float a = clip01(alpha);
    ALLEGRO_COLOR tint = al_map_rgba_f(a, a, a, a);

    al_draw_tinted_scaled_rotated_bitmap(src->data, tint, cx, cy, x, y, scale.x, scale.y, -radians, FLIPPY(flags));
}
 
/*
 * image_draw_trans()
 * Draws a translucent image
 * 0.0 (invisible) <= alpha <= 1.0 (opaque)
 */
void image_draw_trans(const image_t* src, int x, int y, float alpha, imageflags_t flags)
{
    float a = clip01(alpha);
    ALLEGRO_COLOR tint = al_map_rgba_f(a, a, a, a);

    al_draw_tinted_bitmap(src->data, tint, x, y, FLIPPY(flags));
}

/*
 * image_draw_lit()
 * Draws an image with a specific color
 */
void image_draw_lit(const image_t* src, int x, int y, color_t color, imageflags_t flags)
{
    /*

    "While deferred bitmap drawing is enabled, the only functions that can be
    used are the bitmap drawing functions and font drawing functions. Changing
    the state such as the blending modes will result in undefined behaviour."

    source: Allegro manual at
    https://liballeg.org/a5docs/trunk/graphics.html#al_hold_bitmap_drawing

    */

    /* temporarily disable deferred drawing if it's activated */
    bool is_held = al_is_bitmap_drawing_held();
    if(is_held)
        al_hold_bitmap_drawing(false);

    /* store the blend state */
    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_BLENDER);

    /* blending magic */
    al_set_blend_color(color._color);
    al_set_blender(

        /*

        this works with a source image with pre-multiplied alpha:

            x = dx * (1 - sa) + (sx * sa) * cc

        replace x by r, g, b, a

        x is the result of the blending
        dx is destination color
        sx is the source color
        sa is the source alpha
        (sx * sa) is the source color with pre-multiplied alpha
        cc is a constant color

        if sa = 1 (fully opaque), then we'll have x = sx * cc
        if sa = 0 (fully transparent), then we'll have x = dx

        */

        ALLEGRO_ADD, ALLEGRO_CONST_COLOR, ALLEGRO_INVERSE_ALPHA
    );
    al_draw_bitmap(src->data, x, y, FLIPPY(flags));

    al_set_blender(

        /*

        this second equation works as follows:

            y = dy + (sy * sa) * cc

        where dy = x is the result of the previous call to al_draw_bitmap()
        and sy = sx is the source pixel of the bitmap that is being drawn.
        Replace y by r, g, b, a.

        let's analyze two cases:

        1) suppose that sa = 1.0 (fully opaque pixel). This means that,
           in the previous call to al_draw_bitmap(), we had x = sx * cc
           as the result. Therefore, we'll have y = sx * cc + sx * cc =
           2 * sx * cc now, which will give us a nice colored effect with
           both additive and multiplicative blending.

        2) suppose that sa = 0.0 (fully transparent pixel). This means that,
           in the previous call to al_draw_bitmap(), we had x = dx as the
           result. Therefore, we'll have y = dx now, which means that we'll
           be preserving the background as-is.

        we could modify the cc variable of this second equation for a more
        refined control of the color. If we had two separate colors for each
        equation, cc1 and cc2, then the result would be y = sx * (cc1 + cc2).
        If cc2 = t * cc1 for some 0 < t < 1, then y' = (1 + t) * sx * cc1 <
        2 * sx * cc1. Seems like overkill, though.

        perhaps a better result would be achieved with y' = sx * cc + cc,
        because multiplicative blending doesn't always look great depending
        on the colors. How can we draw y'' = cc * sa? So far I don't see in
        Allegro an op with a constant offset, neither a way to compute 1/sx.
        Creating a temporary, single-colored bitmap could work. Is there an
        easier way? We can draw a black silhouette with blending alone.

        */

        ALLEGRO_ADD, ALLEGRO_CONST_COLOR, ALLEGRO_ONE
    );
    al_draw_bitmap(src->data, x, y, FLIPPY(flags));

    /* restore the blending state */
    al_restore_state(&state);

    /* re-enable deferred drawing if it was activated */
    if(is_held)
        al_hold_bitmap_drawing(true);
}

/*
 * image_draw_tinted()
 * Image blending: multiplication mode
 * 0.0 <= alpha <= 1.0
 */
void image_draw_tinted(const image_t* src, int x, int y, color_t color, imageflags_t flags)
{
    al_draw_tinted_bitmap(src->data, color._color, x, y, FLIPPY(flags));
}

/*
 * image_set_drawing_target()
 * Sets the drawing target
 */
void image_set_drawing_target(image_t* new_target)
{
    target = (new_target != video_get_backbuffer()) ? new_target : NULL;
    al_set_target_bitmap(image_drawing_target()->data);
}

/*
 * image_drawing_target()
 * Returns the current drawing target, i.e., the
 * image to which all drawing operations are applied
 */
image_t* image_drawing_target()
{
    return target != NULL ? target : video_get_backbuffer();
}

/*
 * image_hold_drawing()
 * Enable/disable deferred rendering for performance
 */
void image_hold_drawing(bool hold)
{
    /*

    This function works with bitmaps and truetype fonts
    See: https://liballeg.org/a5docs/trunk/graphics.html#al_hold_bitmap_drawing

    */
    static int counter = 0;

    if(hold) {

        if(0 == counter++)
            al_hold_bitmap_drawing(true);

    }
    else {

        if(0 == --counter)
            al_hold_bitmap_drawing(false);

        counter = max(0, counter);

    }
}

/*
 * image_filepath()
 * The relative path to the file of this image, if it exists.
 * An empty string ("") is returned if there is no such path.
 */
const char* image_filepath(const image_t* img)
{
    return img->path != NULL ? img->path : "";
}

/*
 * image_same_root()
 * Checks if two images have the same root (ascendant)
 */
bool image_same_root(const image_t* a, const image_t* b)
{
    ALLEGRO_BITMAP* rootA = a->data;
    ALLEGRO_BITMAP* rootB = b->data;

    /*
     * According to the Allegro docs, al_get_parent_bitmap()
     * "always returns the real bitmap, and never a sub-bitmap".
     * 
     * So, do we actually need these while loops?
     * 
     * https://liballeg.org/a5docs/trunk/graphics.html#al_get_parent_bitmap
     */

    while(al_get_parent_bitmap(rootA) != NULL)
        rootA = al_get_parent_bitmap(rootA);

    while(al_get_parent_bitmap(rootB) != NULL)
        rootB = al_get_parent_bitmap(rootB);

    return (rootA == rootB);
}