/*
 * Open Surge Engine
 * image.c - image implementation
 * Copyright (C) 2008-2010, 2012, 2019  Alexandre Martins <alemartf@gmail.com>
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
#include "assetfs.h"
#include "util.h"
#include "resourcemanager.h"

#if defined(A5BUILD)

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

/* convert imageflags_t to ALLEGRO_FLIP flags */
#define FLIPPY(flags) ((((flags) & IF_HFLIP) ? ALLEGRO_FLIP_HORIZONTAL : 0) | (((flags) & IF_VFLIP) ? ALLEGRO_FLIP_VERTICAL : 0))

/* image type */
struct image_t {
    ALLEGRO_BITMAP* data; /* this must be the first field */
    int w, h;
    char* path; /* relative path */
};

/* misc */
static image_t* target = NULL; /* drawing target */
static const int MAX_IMAGE_SIZE = 4096; /* maximum image size for broad compatibility with video cards */

#else

#include <png.h>
#include <allegro.h>
#include <loadpng.h>
#include <jpgalleg.h>

/* image structure */
struct image_t {
    BITMAP* data; /* this must be the first field */
    int w, h;
    char* path;
};

/* useful stuff */
typedef int (*fast_getpixel_funptr)(BITMAP*,int,int);
typedef void (*fast_putpixel_funptr)(BITMAP*,int,int,int);
typedef int (*fast_makecol_funptr)(int,int,int);
typedef int (*fast_getr_funptr)(int);
typedef int (*fast_getg_funptr)(int);
typedef int (*fast_getb_funptr)(int);

/* private stuff */
static void maskcolor_bugfix(image_t* img);
static inline void draw_flipped_sprite(const image_t* src, image_t* dest, int x, int y, imageflags_t flags);
static fast_getpixel_funptr fast_getpixel_fun(); /* returns a function. this won't do any clipping, so be careful. */
static fast_putpixel_funptr fast_putpixel_fun(); /* returns a function. this won't do any clipping, so be careful. */
static fast_makecol_funptr fast_makecol_fun(); /* returns a function */
static fast_getr_funptr fast_getr_fun(); /* returns a function */
static fast_getg_funptr fast_getg_fun(); /* returns a function */
static fast_getb_funptr fast_getb_fun(); /* returns a function */

/* drawing target */
static image_t* _target = NULL;
#define get_target() (_target ? _target : video_get_backbuffer())

#endif

/*
 * image_load()
 * Loads a image from a file.
 * Supported types: PNG, JPG, BMP, PCX, TGA
 */
image_t* image_load(const char* path)
{
#if defined(A5BUILD)
    image_t* img;

    if(NULL == (img = resourcemanager_find_image(path))) {
        const char* fullpath = assetfs_fullpath(path);
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
            fatal_error("Failed to load \"%s\": images can't be larger than %dx%d", path, MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);
            al_destroy_bitmap(img->data);
            free(img);
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
#else
    image_t* img;

    if(NULL == (img = resourcemanager_find_image(path))) {
        const char* fullpath = assetfs_fullpath(path);
        logfile_message("Loading image \"%s\"...", fullpath);

        /* build the image object */
        img = mallocx(sizeof *img);

        /* loading the image */
        img->data = load_bitmap(fullpath, NULL);
        if(img->data == NULL) {
            fatal_error("Failed to load image \"%s\": %s", fullpath, allegro_error);
            free(img);
            return NULL;
        }

        /* configuring the image */
        img->w = img->data->w;
        img->h = img->data->h;
        maskcolor_bugfix(img);

        /* adding it to the resource manager */
        img->path = str_dup(path);
        resourcemanager_add_image(img->path, img);
        resourcemanager_ref_image(img->path);
    }
    else
        resourcemanager_ref_image(path);

    return img;
#endif
}



/*
 * image_save()
 * Saves a image to a file
 */
void image_save(const image_t* img, const char *path)
{
#if defined(A5BUILD)
    const char* fullpath = assetfs_create_cache_file(path);

    if(al_save_bitmap(fullpath, img->data))
        logfile_message("Saved image to \"%s\"", fullpath);
    else
        logfile_message("Failed to save image to \"%s\"", fullpath);
#else
    int i, j, c, bpp = video_get_color_depth();
    const char* fullpath = assetfs_create_cache_file(path);
    const char* extension = strrchr(fullpath, '.');

    logfile_message("image_save(\"%s\")", fullpath);

    switch(bpp) {
        case 16:
        case 24:
            save_bitmap(fullpath, img->data, NULL);
            break;

        case 32:
            if(extension != NULL && str_icmp(extension, ".png") == 0) {
                /* we must do this to make loadpng save the 32-bit image properly */
                BITMAP *tmp;
                if(NULL != (tmp = create_bitmap(img->w, img->h))) {
                    for(j=0; j<tmp->h; j++) {
                        for(i=0; i<tmp->w; i++) {
                            c = getpixel(img->data, i, j);
                            putpixel(tmp, i, j, makeacol(getr(c), getg(c), getb(c), 255));
                        }
                    }
                    save_bitmap(fullpath, tmp, NULL);
                    destroy_bitmap(tmp);
                }
            }
            else
                save_bitmap(fullpath, img->data, NULL);
            break;
    }
#endif
}



/*
 * image_create()
 * Creates a new image of a given size
 */
image_t* image_create(int width, int height)
{
#if defined(A5BUILD)
    image_t* img;

    if(width <= 0 || height <= 0 || width > MAX_IMAGE_SIZE || height > MAX_IMAGE_SIZE) {
        fatal_error("Can't create image of size %d x %d", width, height);
        return NULL;
    }

    img = mallocx(sizeof *img);
    img->path = NULL;
    img->w = width;
    img->h = height;
    
    if(NULL != (img->data = al_create_bitmap(img->w, img->h))) {
        ALLEGRO_STATE state;
        al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
        al_set_target_bitmap(img->data);
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_restore_state(&state);
    }
    else
        logfile_message("ERROR - image_create(%d,%d) failed", img->w, img->h);

    return img;
#else
    image_t* img = mallocx(sizeof *img);

    img->data = create_bitmap(width, height);
    img->w = width;
    img->h = height;
    img->path = NULL;

    if(img->data != NULL)
        clear_to_color(img->data, makecol(0,0,0));
    else
        logfile_message("ERROR - image_create(%d,%d): couldn't create image", width, height);

    return img;
#endif
}


/*
 * image_destroy()
 * Destroys an image. This is called automatically
 * while unloading the resource manager.
 */
void image_destroy(image_t* img)
{
#if defined(A5BUILD)
    if(img->data != NULL)
        al_destroy_bitmap(img->data);

    if(img->path != NULL) {
        resourcemanager_unref_image(img->path);
        free(img->path);
    }

    if(target == img)
        target = NULL;

    free(img);
#else
    if(img->data != NULL)
        destroy_bitmap(img->data);

    if(img->path != NULL) {
        resourcemanager_unref_image(img->path);
        free(img->path);
    }

    if(_target == img)
        _target = NULL;

    free(img);
#endif
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
#if defined(A5BUILD)
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

    if(parent->path != NULL) {
        img->path = str_dup(parent->path);
        resourcemanager_ref_image(img->path); /* reference it, otherwise the parent may be destroyed */
    }
    else
        img->path = NULL;

    return img;
#else
    image_t* img;
    int pw, ph;

    if(width <= 0 || height <= 0)
        fatal_error("Can't create shared image of size %d x %d", width, height);

    pw = parent->w;
    ph = parent->h;
    x = clip(x, 0, pw-1);
    y = clip(y, 0, ph-1);
    width = clip(width, 0, pw-x);
    height = clip(height, 0, ph-y);

    img = mallocx(sizeof *img);
    img->w = width;
    img->h = height;
    if(NULL == (img->data = create_sub_bitmap(parent->data, x, y, width, height)))
        fatal_error("ERROR - image_create_shared(0x%p,%d,%d,%d,%d): couldn't create shared image", parent, x, y, width, height);

    if(parent->path != NULL) {
        img->path = str_dup(parent->path);
        resourcemanager_ref_image(img->path);
    }
    else
        img->path = NULL;

    return img;
#endif
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
#if defined(A5BUILD)
    image_t* img = mallocx(sizeof *img);

    img->w = src->w;
    img->h = src->h;
    img->path = NULL;
    if(NULL == (img->data = al_clone_bitmap(src->data)))
        fatal_error("Failed to clone image \"%s\" sized %dx%d", src->path ? src->path : "", src->w, src->h);

    return img;
#else
    image_t* img = mallocx(sizeof *img);
    img->w = src->w;
    img->h = src->h;
    img->path = NULL;

    if(NULL == (img->data = create_bitmap(img->w, img->h)))
        fatal_error("ERROR - image_clone(0x%p) sized %dx%d", src, img->w, img->h);
    blit(src->data, img->data, 0, 0, 0, 0, src->w, src->h);

    return img;
#endif
}

/*
 * image_clone_region()
 * Clones a region of an image; make sure you
 * call image_destroy() after usage
 */
image_t* image_clone_region(const image_t* src, int x, int y, int width, int height)
{
#if defined(A5BUILD)
    image_t* img;
    int sw, sh;

    if(width <= 0 || height <= 0) {
        fatal_error("Can't create cloned image of size %d x %d", width, height);
        return NULL;
    }

    sw = src->w;
    sh = src->h;
    x = clip(x, 0, sw-1);
    y = clip(y, 0, sh-1);
    width = clip(width, 0, sw-x);
    height = clip(height, 0, sh-y);

    img = mallocx(sizeof *img);
    img->w = width;
    img->h = height;
    img->path = NULL;
    if(NULL != (img->data = al_create_bitmap(img->w, img->h))) {
        ALLEGRO_STATE state;
        al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
        al_set_target_bitmap(img->data);
        al_clear_to_color(al_map_rgba(0, 0, 0, 0));
        al_draw_bitmap_region(src->data, x, y, width, height, 0.0f, 0.0f, 0);
        al_restore_state(&state);
    }
    else
        fatal_error("Failed to clone region of image \"%s\": %d, %d, %d, %d", src->path ? src->path : "", x, y, width, height);
    
    return img;
#else
    image_t* img;
    int sw, sh;

    if(width <= 0 || height <= 0)
        fatal_error("Can't create cloned image of size %d x %d", width, height);

    sw = src->w;
    sh = src->h;
    x = clip(x, 0, sw-1);
    y = clip(y, 0, sh-1);
    width = clip(width, 0, sw-x);
    height = clip(height, 0, sh-y);

    img = mallocx(sizeof *img);
    img->w = width;
    img->h = height;
    img->path = NULL;
    
    if(NULL == (img->data = create_bitmap(img->w, img->h)))
        fatal_error("ERROR - image_clone_region(0x%p,%d,%d,%d,%d): couldn't create cloned image", src, x, y, width, height);
    blit(src->data, img->data, x, y, 0, 0, width, height);

    return img;
#endif
}

/*
 * image_snapshot()
 * Take a snapshot of the game. Remember to
 * destroy the created image after usage.
 */
image_t* image_snapshot()
{
#if defined(A5BUILD)
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
#else
    image_t* img = mallocx(sizeof *img);
    img->w = screen->w;
    img->h = screen->h;
    img->path = NULL;

    if(NULL == (img->data = create_bitmap(img->w, img->h)))
        fatal_error("Failed to take snapshot");
    blit(screen, img->data, 0, 0, 0, 0, screen->w, screen->h);

    return img;
#endif
}

/*
 * image_lock()
 * Locks the image, enabling fast in-memory pixel access
 */
void image_lock(image_t* img)
{
#if defined(A5BUILD)
    al_lock_bitmap(img->data, al_get_bitmap_format(img->data), ALLEGRO_LOCK_READWRITE);
#else
    ;
#endif
}

/*
 * image_unlock()
 * Unlocks the image
 */
void image_unlock(image_t* img)
{
#if defined(A5BUILD)
    al_unlock_bitmap(img->data);
#else
    ;
#endif
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
#if defined(A5BUILD)
    return (color_t){ al_get_pixel(img->data, x, y) };
#else
    return (color_t){ getpixel(img->data, x, y) };
#endif
}


/*
 * image_putpixel()
 * Puts a pixel on the target image. Make sure to lock it first
 */
void image_putpixel(int x, int y, color_t color)
{
#if defined(A5BUILD)
    al_put_pixel(x, y, color._color);
#else
    putpixel(get_target()->data, x, y, color._value);
#endif
}


/*
 * image_line()
 * Draws a line from (x1,y1) to (x2,y2) using the specified color
 */
void image_line(int x1, int y1, int x2, int y2, color_t color)
{
#if defined(A5BUILD)
    al_draw_line(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, color._color, 0.0f);
#else
    line(get_target()->data, x1, y1, x2, y2, color._value);
#endif
}


/*
 * image_ellipse()
 * Draws an ellipse with the specified centre, radius and color
 */
void image_ellipse(int cx, int cy, int radius_x, int radius_y, color_t color)
{
#if defined(A5BUILD)
    al_draw_ellipse(cx + 0.5f, cy + 0.5f, radius_x, radius_y, color._color, 0.0f);
#else
    ellipse(get_target()->data, cx, cy, radius_x, radius_y, color._value);
#endif
}


/*
 * image_ellipsefill()
 * Draws a filled ellipse with the specified centre, radius and color
 */
void image_ellipsefill(int cx, int cy, int radius_x, int radius_y, color_t color)
{
#if defined(A5BUILD)
    al_draw_filled_ellipse(cx + 0.5f, cy + 0.5f, radius_x, radius_y, color._color);
#else
    ellipsefill(get_target()->data, cx, cy, radius_x, radius_y, color._value);
#endif
}


/*
 * image_rect()
 * Draws a rectangle
 */
void image_rect(int x1, int y1, int x2, int y2, color_t color)
{
#if defined(A5BUILD)
    al_draw_rectangle(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, color._color, 0.0f);
#else
    rect(get_target()->data, x1, y1, x2, y2, color._value);
#endif
}


/*
 * image_rectfill()
 * Draws a filled rectangle
 */
void image_rectfill(int x1, int y1, int x2, int y2, color_t color)
{
#if defined(A5BUILD)
    al_draw_filled_rectangle(x1, y1, x2 + 1.0f, y2 + 1.0f, color._color);
#else
    rectfill(get_target()->data, x1, y1, x2, y2, color._value);
#endif
}


/*
 * image_waterfx()
 * Pixels below y will have a water effect
 */
void image_waterfx(int y, color_t color)
{
#if defined(A5BUILD)
    image_t* target = image_drawing_target();

    /* obtain the components of the color */
    uint8_t r, g, b, a;
    color_unmap(color, &r, &g, &b, &a);

    /*

    Let's pre-multiply the alpha

    "By default Allegro uses pre-multiplied alpha for transparent blending of
    bitmaps and primitives (see al_load_bitmap_flags for a discussion of that
    feature). This means that if you want to tint a bitmap or primitive to be
    transparent you need to multiply the color components by the alpha
    components when you pass them to this function."

    Source: Allegro manual at
    https://liballeg.org/a5docs/trunk/graphics.html#al_premul_rgba

    */
    ALLEGRO_COLOR col = al_premul_rgba(r, g, b, a);

    /* adjust y */
    y = clip(y, 0, target->h);

    /* draw water effect */
    al_draw_filled_rectangle(0, y, target->w + 1.0f, target->h + 1.0f, col);
#else
    image_t* target = get_target();
    fast_getpixel_funptr fast_getpixel = fast_getpixel_fun();
    fast_putpixel_funptr fast_putpixel = fast_putpixel_fun();
    fast_makecol_funptr fast_makecol = fast_makecol_fun();
    fast_getr_funptr fast_getr = fast_getr_fun();
    fast_getg_funptr fast_getg = fast_getg_fun();
    fast_getb_funptr fast_getb = fast_getb_fun();
    int col, wr, wg, wb; /* don't use uint8  */
    int i, j;

    /* adjust y */
    y = clip(y, 0, target->h);

    /* water color */
    wr = fast_getr(color._value);
    wg = fast_getg(color._value);
    wb = fast_getb(color._value);

    /* water effect */
    if(video_get_color_depth() > 16) {
        /* fast blending algorithm (alpha = 0.5) */
        for(j=y; j<target->h; j++) {
            for(i=0; i<target->w; i++) {
                col = fast_getpixel(target->data, i, j);
                fast_putpixel(target->data, i, j, fast_makecol((fast_getr(col) + wr)/2, (fast_getg(col) + wg)/2, (fast_getb(col) + wb)/2));
            }
        }
    }
    else {
        /* fast "dithered" water, when bpp is not greater than 16 (slow computers?) */
        for(j=y; j<target->h; j++) {
            for(i=j%2; i<target->w; i+=2) {
                fast_putpixel(target->data, i, j, color._value);
            }
        }
    }
#endif
}


/*
 * image_clear()
 * Clears an given image with some color
 */
void image_clear(color_t color)
{
#if defined(A5BUILD)
    al_clear_to_color(color._color);
#else
    clear_to_color(get_target()->data, color._value);
#endif
}


/*
 * image_blit()
 * Blits a surface onto another
 */
void image_blit(const image_t* src, int src_x, int src_y, int dest_x, int dest_y, int width, int height)
{
#if defined(A5BUILD)
    al_draw_bitmap_region(src->data, src_x, src_y, width, height, dest_x, dest_y, 0);
#else
    blit(src->data, get_target()->data, src_x, src_y, dest_x, dest_y, width, height);
#endif
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
#if defined(A5BUILD)
    al_draw_bitmap(src->data, x, y, FLIPPY(flags));
#else
    draw_flipped_sprite(src, get_target(), x, y, flags);
#endif
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
#if defined(A5BUILD)
    al_draw_scaled_bitmap(src->data,
        0.0f, 0.0f, src->w, src->h,
        x, y, scale.x * src->w, scale.y * src->h,
    FLIPPY(flags));
#else
    image_t* tmp;
    int w = (int)(scale.x * src->w);
    int h = (int)(scale.y * src->h);

    tmp = image_create(w, h);
    stretch_blit(src->data, tmp->data, 0, 0, src->w, src->h, 0, 0, w, h);
    image_draw(tmp, x, y, flags);
    image_destroy(tmp);
#endif
}


/*
 * image_draw_rotated()
 * Draws a rotated image onto the destination bitmap at the specified position 
 * radians: angle given in radians (counter-clockwise)
 * cx, cy: pivot positions
 */
void image_draw_rotated(const image_t* src, int x, int y, int cx, int cy, float radians, imageflags_t flags)
{
#if defined(A5BUILD)
    al_draw_rotated_bitmap(src->data, cx, cy, x, y, -radians, FLIPPY(flags));
#else
    image_t* target = get_target();
    float conv = radians * -40.7436654315f; /* -(180/pi)*(64/90) */

    if((flags & IF_HFLIP) && !(flags & IF_VFLIP))
        pivot_sprite_v_flip(target->data, src->data, x, y, src->w - cx, src->h - cy, ftofix(conv + 128.0f));
    else if(!(flags & IF_HFLIP) && (flags & IF_VFLIP))
        pivot_sprite_v_flip(target->data, src->data, x, y, cx, src->h - cy, ftofix(conv));
    else if((flags & IF_HFLIP) && (flags & IF_VFLIP))
        pivot_sprite(target->data, src->data, x, y, src->w - cx, src->h - cy, ftofix(conv + 128.0f));
    else
        pivot_sprite(target->data, src->data, x, y, cx, cy, ftofix(conv));
#endif
}


/*
 * image_draw_scaled_rotated()
 * Draw scaled and rotated ;)
 */
void image_draw_scaled_rotated(const image_t* src, int x, int y, int cx, int cy, v2d_t scale, float radians, imageflags_t flags)
{
#if defined(A5BUILD)
    al_draw_scaled_rotated_bitmap(src->data, cx, cy, x, y, scale.x, scale.y, -radians, FLIPPY(flags));
#else
    image_t* tmp;
    int w = (int)(scale.x * src->w);
    int h = (int)(scale.y * src->h);

    tmp = image_create(w, h);
    stretch_blit(src->data, tmp->data, 0, 0, src->w, src->h, 0, 0, w, h);
    image_draw_rotated(tmp, x, y, cx, cy, radians, flags);
    image_destroy(tmp);
#endif
}

 
/*
 * image_draw_trans()
 * Draws a translucent image
 * 0.0 (invisible) <= alpha <= 1.0 (opaque)
 */
void image_draw_trans(const image_t* src, int x, int y, float alpha, imageflags_t flags)
{
#if defined(A5BUILD)
    float a = clip01(alpha);
    al_draw_tinted_bitmap(src->data, al_map_rgba_f(a, a, a, a), x, y, FLIPPY(flags));
#else
    image_t* target = get_target();

    if(video_get_color_depth() > 8) {
        int a = 255 * clip01(alpha);
        set_trans_blender(a, a, a, a);

        if(flags != IF_NONE) {
            image_t* tmp = image_create(src->w, src->h);
            clear_to_color(tmp->data, bitmap_mask_color(tmp->data));
            draw_flipped_sprite(src, tmp, 0, 0, flags);
            draw_trans_sprite(target->data, tmp->data, x, y);
            image_destroy(tmp);
        }
        else
            draw_trans_sprite(target->data, src->data, x, y);

    }
    else
        draw_flipped_sprite(src, target, x, y, flags);
#endif
}

/*
 * image_draw_lit()
 * Draws an image with a specific color
 */
void image_draw_lit(const image_t* src, int x, int y, color_t color, imageflags_t flags)
{
#if defined(A5BUILD)
    /* TODO: shader for specific contours */
    ALLEGRO_STATE state;
    al_store_state(&state, ALLEGRO_STATE_BLENDER);
    al_draw_bitmap(src->data, x, y, FLIPPY(flags));
    al_set_separate_blender(
        ALLEGRO_ADD, ALLEGRO_CONST_COLOR, ALLEGRO_INVERSE_CONST_COLOR,
        ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO
    );
    al_set_blend_color(al_map_rgba_f(0.65f, 0.65f, 0.65f, 1.0f));
    al_draw_filled_rectangle(x, y, x + src->w + 1.0f, y + src->h + 1.0f, color._color);
    al_restore_state(&state);
    /*al_draw_rectangle(x, y, x + src->w + 1.0f, y + src->h + 1.0f, color._color, 2.0f);*/
#else
    image_t* target = get_target();

    if(video_get_color_depth() > 8) {
        uint8_t r, g, b, a = 128;
        color_unmap(color, &r, &g, &b, NULL);
        set_trans_blender(r, g, b, a);

        if(flags != IF_NONE) {
            image_t* tmp = image_create(src->w, src->h);
            clear_to_color(tmp->data, bitmap_mask_color(tmp->data));
            draw_flipped_sprite(src, tmp, 0, 0, flags);
            draw_lit_sprite(target->data, tmp->data, x, y, a);
            image_destroy(tmp);
        }
        else
            draw_lit_sprite(target->data, src->data, x, y, a);
    }
    else
        draw_flipped_sprite(src, target, x, y, flags);
#endif
}

/*
 * image_draw_tinted()
 * Image blending: multiplication mode
 * 0.0 <= alpha <= 1.0
 */
void image_draw_tinted(const image_t* src, int x, int y, color_t color, imageflags_t flags)
{
#if defined(A5BUILD)
    al_draw_tinted_bitmap(src->data, color._color, x, y, FLIPPY(flags));
#else
    image_t* target = get_target();

    if(video_get_color_depth() > 8) {
        uint8_t r, g, b, a;
        color_unmap(color, &r, &g, &b, &a);
        set_multiply_blender(r, g, b, a);

        if(flags != IF_NONE) {
            image_t* tmp = image_create(src->w, src->h);
            clear_to_color(tmp->data, bitmap_mask_color(tmp->data));
            draw_flipped_sprite(src, tmp, 0, 0, flags);
            draw_lit_sprite(target->data, tmp->data, x, y, 255);
            image_destroy(tmp);
        }
        else
            draw_lit_sprite(target->data, src->data, x, y, 255);
    }
    else
        draw_flipped_sprite(src, target, x, y, flags);
#endif
}

/*
 * image_set_drawing_target()
 * Sets the drawing target
 */
void image_set_drawing_target(image_t* new_target)
{
#if defined(A5BUILD)
    target = (new_target != video_get_backbuffer()) ? new_target : NULL;
    al_set_target_bitmap(image_drawing_target()->data);
#else
    _target = (new_target != video_get_backbuffer()) ? new_target : NULL;
#endif
}

/*
 * image_drawing_target()
 * Returns the current drawing target, i.e., the
 * image to which all drawing operations are applied
 */
image_t* image_drawing_target()
{
#if defined(A5BUILD)
    return target != NULL ? target : video_get_backbuffer();
#else
    return get_target();
#endif
}



/* private methods */
#if !defined(A5BUILD)

/*
 * maskcolor_bugfix()
 * When loading certain PNGs, magenta (color key) is
 * not considered transparent. Let's fix this.
 */
void maskcolor_bugfix(image_t* img)
{
    int i, j;
    color_t pixel;
    int mask_color = bitmap_mask_color(img->data);
    fast_getpixel_funptr fast_getpixel = fast_getpixel_fun();
    fast_putpixel_funptr fast_putpixel = fast_putpixel_fun();

    for(j=0; j<img->h; j++) {
        for(i=0; i<img->w; i++) {
            pixel = (color_t){ fast_getpixel(img->data, i, j) };
            if(color_is_transparent(pixel))
                fast_putpixel(img->data, i, j, mask_color);
        }
    }
}

/*
 * draw_flipped_sprite()
 * Draws an image into another using a set of flags
 */
void draw_flipped_sprite(const image_t* src, image_t* dest, int x, int y, imageflags_t flags)
{
    if(!(flags & IF_HFLIP) && !(flags & IF_VFLIP))
        draw_sprite(dest->data, src->data, x, y);
    else if((flags & IF_HFLIP) && !(flags & IF_VFLIP))
        draw_sprite_h_flip(dest->data, src->data, x, y);
    else if(!(flags & IF_HFLIP) && (flags & IF_VFLIP))
        draw_sprite_v_flip(dest->data, src->data, x, y);
    else
        draw_sprite_vh_flip(dest->data, src->data, x, y);
}

/*
 * fast_getpixel_ptr()
 * Returns a fast getpixel function. It won't perform any
 * clipping, so take care.
 */
fast_getpixel_funptr fast_getpixel_fun()
{
    switch(video_get_color_depth()) {
        case 16: return _getpixel16;
        case 24: return _getpixel24;
        case 32: return _getpixel32;
        default: return getpixel;
    }
}

/*
 * fast_putpixel_fun()
 * Returns a fast putpixel function. It won't perform any
 * clipping, so take care.
 */
fast_putpixel_funptr fast_putpixel_fun()
{
    switch(video_get_color_depth()) {
        case 16: return _putpixel16;
        case 24: return _putpixel24;
        case 32: return _putpixel32;
        default: return putpixel;
    }
}

/* these other guys return color-depth appropriate functions
   for Allegro's equivalent to makecol, getr, getg, getb */
fast_makecol_funptr fast_makecol_fun()
{
    switch(video_get_color_depth()) {
        case 16: return makecol16;
        case 24: return makecol24;
        case 32: return makecol32;
        default: return makecol;
    }
}

fast_getr_funptr fast_getr_fun()
{
    switch(video_get_color_depth()) {
        case 16: return getr16;
        case 24: return getr24;
        case 32: return getr32;
        default: return getr;
    }
}

fast_getg_funptr fast_getg_fun()
{
    switch(video_get_color_depth()) {
        case 16: return getg16;
        case 24: return getg24;
        case 32: return getg32;
        default: return getg;
    }
}

fast_getb_funptr fast_getb_fun()
{
    switch(video_get_color_depth()) {
        case 16: return getb16;
        case 24: return getb24;
        case 32: return getb32;
        default: return getb;
    }
}

#endif
