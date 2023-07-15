/*
 * Open Surge Engine
 * background.c - level background/foreground
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

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "background.h"
#include "actor.h"
#include "../core/sprite.h"
#include "../core/video.h"
#include "../core/asset.h"
#include "../core/logfile.h"
#include "../core/timer.h"
#include "../core/nanoparser.h"
#include "../util/numeric.h"
#include "../util/rect.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../third_party/fast_draw.h"

/* forward declarations */
typedef struct bglayer_t bglayer_t;
typedef struct bgbehavior_t bgbehavior_t;
typedef struct bgbehavior_default_t bgbehavior_default_t;
typedef struct bgbehavior_circular_t bgbehavior_circular_t;
typedef struct bgbehavior_linear_t bgbehavior_linear_t;

/* bgtheme struct: represents a .bg file */
struct bgtheme_t {
    bglayer_t **layer; /* array of bglayer_t* */
    int layer_count; /* length of layer[] */
    int background_count; /* number of background layers */
    int foreground_count; /* number of foreground layers */
    char* filepath; /* filepath of the background */
};

/* bglayer struct: a background (or foreground) layer */
struct bglayer_t {
    spriteinfo_t *data; /* this is not stored in the main hash */
    const animation_t* animation; /* animation 0 of the sprite of the layer */

    v2d_t initial_position; /* initial position */
    v2d_t scroll_speed; /* scroll speed */
    bool repeat_x, repeat_y; /* repeat layer? */
    float zindex; /* 0.0 (far) <= zindex <= 1.0 (near) */

    bgbehavior_t *behavior; /* behavior */
    int group_index; /* for deferred drawing */
};
static bglayer_t *bglayer_new(); /* constructor */
static bglayer_t *bglayer_delete(bglayer_t *layer); /* destructor */





/* behaviors of layers */

/* abstract behavior */
struct bgbehavior_t {
    v2d_t offset; /* given in pixels */
    void (*update)(bgbehavior_t*); /* update function */
    bgbehavior_t* (*delete)(bgbehavior_t*); /* destructor */
};
static void bgbehavior_update(bgbehavior_t *behavior); /* update behavior */
static bgbehavior_t *bgbehavior_delete(bgbehavior_t *behavior); /* class destructor */

/* default behavior */
struct bgbehavior_default_t {
    bgbehavior_t base; /* inherits from bgbehavior_t */
};
static bgbehavior_t *bgbehavior_default_new(); /* constructor */
static bgbehavior_t* bgbehavior_default_delete(bgbehavior_t *behavior); /* destructor */
static void bgbehavior_default_update(bgbehavior_t *behavior); /* private method */

/* circular strategy (elliptical trajectory) */
struct bgbehavior_circular_t {
    bgbehavior_t base; /* base class */
    float elapsed_time; /* in seconds */
    v2d_t amplitude; /* in pixels */
    v2d_t angular_speed; /* in radians per second */
    v2d_t initial_phase; /* in radians */
};
static bgbehavior_t *bgbehavior_circular_new(float amplitude_x, float amplitude_y, float angularspeed_x, float angularspeed_y, float initialphase_x, float initialphase_y); /* constructor */
static bgbehavior_t* bgbehavior_circular_delete(bgbehavior_t *behavior); /* destructor */
static void bgbehavior_circular_update(bgbehavior_t *behavior); /* private method */

/* linear strategy */
struct bgbehavior_linear_t {
    bgbehavior_t base; /* base class */
    v2d_t speed; /* in pixels per second */
};
static bgbehavior_t *bgbehavior_linear_new(float speed_x, float speed_y); /* constructor */
static bgbehavior_t* bgbehavior_linear_delete(bgbehavior_t *behavior); /* destructor */
static void bgbehavior_linear_update(bgbehavior_t *behavior); /* private method */




/* internal utilities */

/* preprocessing */
#define IS_FOREGROUND_LAYER(layer)    ((layer)->zindex > 0.5f)
static void sort_layers(bgtheme_t *bgtheme);
static int sort_cmp(const void *a, const void *b);
static void split_layers(bgtheme_t *bgtheme);
static void group_layers(bgtheme_t *bgtheme);

/* rendering */
#define WANT_FAST_DRAW 1
typedef void (*renderstrategy_t)(const image_t*,v2d_t,void*);
static void render_layers(bglayer_t* const *layers, int layer_count, v2d_t camera_position, void* data, renderstrategy_t render_image);
static void render_without_cache(const image_t* image, v2d_t position, void* data);
static void render_with_cache(const image_t* image, v2d_t position, void* data);

/* .bg files */
static int traverse(const parsetree_statement_t *stmt, void *bgtheme);
static int traverse_layer_attributes(const parsetree_statement_t *stmt, void *bglayer);
static void validate_layer(const bglayer_t* layer);
static void validate_theme(const bgtheme_t* theme);




/* public methods */

/*
 * background_load()
 * Loads a background theme from a .bg file
 */
bgtheme_t* background_load(const char *filepath)
{
    bgtheme_t *bgtheme;
    parsetree_program_t *tree;
    const char *fullpath;

    logfile_message("Loading background \"%s\"...", filepath);
    fullpath = asset_path(filepath);

    /* create the struct */
    bgtheme = mallocx(sizeof *bgtheme);
    bgtheme->filepath = str_dup(filepath);
    bgtheme->layer = NULL;
    bgtheme->layer_count = 0;
    bgtheme->background_count = 0;
    bgtheme->foreground_count = 0;

    /* read the .bg file */
    tree = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program_ex(tree, (void*)bgtheme, traverse);
    tree = nanoparser_deconstruct_tree(tree);
    validate_theme(bgtheme);

    /* prepare for rendering */
    sort_layers(bgtheme);
    split_layers(bgtheme);
    group_layers(bgtheme);

    /* done! */
    return bgtheme;
}

/*
 * background_unload()
 * Unloads a background theme
 */
bgtheme_t* background_unload(bgtheme_t *bgtheme)
{
    logfile_message("Will unload background \"%s\"...", bgtheme->filepath);

    if(bgtheme->layer != NULL) {
        for(int i = 0; i < bgtheme->layer_count; i++)
            bgtheme->layer[i] = bglayer_delete(bgtheme->layer[i]);

        free(bgtheme->layer);
    }

    free(bgtheme->filepath);
    free(bgtheme);
    return NULL;
}

/*
 * background_update()
 * Updates the background
 */
void background_update(bgtheme_t *bgtheme)
{
    for(int i = 0; i < bgtheme->layer_count; i++) {
        const bglayer_t *layer = bgtheme->layer[i];
        bgbehavior_update(layer->behavior);
    }
}

/*
 * background_render_bg()
 * Renders the background
 */
void background_render_bg(const bgtheme_t *bgtheme, v2d_t camera_position)
{
    bglayer_t** layers = bgtheme->layer;
    int layer_count = bgtheme->background_count;

#if WANT_FAST_DRAW
    FAST_DRAW_CACHE* cache = fd_create_cache(layer_count, true, false);
    render_layers(layers, layer_count, camera_position, cache, render_with_cache);
    fd_flush_cache(cache); /* invokes al_draw_indexed_prim() */
    fd_destroy_cache(cache);

    /*

    there is overhead when invoking al_draw_prim()

    [1] https://www.allegro.cc/forums/thread/613609
    [2] https://www.allegro.cc/forums/thread/614949

    */
#else
    image_hold_drawing(true);
    render_layers(layers, layer_count, camera_position, NULL, render_without_cache);
    image_hold_drawing(false);
#endif
}

/*
 * background_render_fg()
 * Renders the foreground
 */
void background_render_fg(const bgtheme_t *bgtheme, v2d_t camera_position)
{
    bglayer_t** layers = bgtheme->layer + bgtheme->background_count;
    int layer_count = bgtheme->foreground_count;

    image_hold_drawing(true);
    render_layers(layers, layer_count, camera_position, NULL, render_without_cache);
    image_hold_drawing(false);
}

/*
 * background_filepath()
 * Returns the filepath of the background
 */
const char* background_filepath(const bgtheme_t *bgtheme)
{
    return bgtheme->filepath;
}

/*
 * background_number_of_bg_layers()
 * Number of background layers
 */
int background_number_of_bg_layers(const bgtheme_t* bgtheme)
{
    return bgtheme->background_count;
}

/*
 * background_number_of_fg_layers()
 * Number of foreground layers
 */
int background_number_of_fg_layers(const bgtheme_t* bgtheme)
{
    return bgtheme->foreground_count;
}



/* private methods */



/* create a new layer */
bglayer_t *bglayer_new()
{
    bglayer_t *layer = mallocx(sizeof *layer);

    layer->data = NULL;
    layer->animation = NULL;
    layer->initial_position = v2d_new(0.0f, 0.0f);
    layer->scroll_speed = v2d_new(0.0f, 0.0f);
    layer->repeat_x = false;
    layer->repeat_y = false;
    layer->zindex = 0.0f;
    layer->behavior = bgbehavior_default_new(layer);
    layer->group_index = 0;

    return layer;
}

/* destroy a layer */
bglayer_t *bglayer_delete(bglayer_t *layer)
{
    if(layer->data != NULL)
        spriteinfo_destroy(layer->data);

    layer->behavior = bgbehavior_delete(layer->behavior);

    free(layer);
    return NULL;
}







/* behaviors */

bgbehavior_t *bgbehavior_delete(bgbehavior_t *behavior)
{
    return behavior->delete(behavior);
}

void bgbehavior_update(bgbehavior_t *behavior)
{
    behavior->update(behavior);
}


/* default behavior */

bgbehavior_t *bgbehavior_default_new()
{
    bgbehavior_default_t *me = mallocx(sizeof *me);
    bgbehavior_t *base = (bgbehavior_t*)me;

    base->offset = v2d_new(0.0f, 0.0f);
    base->update = bgbehavior_default_update;
    base->delete = bgbehavior_default_delete;

    return base;
}

bgbehavior_t *bgbehavior_default_delete(bgbehavior_t *behavior)
{
    free(behavior);
    return NULL;
}

void bgbehavior_default_update(bgbehavior_t *behavior)
{
    /* do nothing */
    (void)behavior;
}



/* circular behavior */

bgbehavior_t *bgbehavior_circular_new(float amplitude_x, float amplitude_y, float angularspeed_x, float angularspeed_y, float initialphase_x, float initialphase_y)
{
    bgbehavior_circular_t *me = mallocx(sizeof *me);
    bgbehavior_t *base = (bgbehavior_t*)me;

    base->offset = v2d_new(0.0f, 0.0f);
    base->update = bgbehavior_circular_update;
    base->delete = bgbehavior_circular_delete;

    me->elapsed_time = 0.0f;
    me->amplitude = v2d_new(amplitude_x, amplitude_y);
    me->angular_speed = v2d_multiply(v2d_new(angularspeed_x, angularspeed_y), TWO_PI);
    me->initial_phase = v2d_multiply(v2d_new(initialphase_x, initialphase_y), DEG2RAD);

    return base;
}

bgbehavior_t *bgbehavior_circular_delete(bgbehavior_t *behavior)
{
    free(behavior);
    return NULL;
}

void bgbehavior_circular_update(bgbehavior_t *behavior)
{
    bgbehavior_circular_t *me = (bgbehavior_circular_t*)behavior;
    float dt = timer_get_delta();
    float t, s, c;

    t = (me->elapsed_time += dt);
    s = sinf(me->angular_speed.y * t + me->initial_phase.y);
    c = cosf(me->angular_speed.x * t + me->initial_phase.x);

    /* elliptical trajectory */
    behavior->offset.x += me->amplitude.x * (me->angular_speed.x * c) * dt;
    behavior->offset.y += me->amplitude.y * (me->angular_speed.y * s) * dt;
}



/* linear behavior */

bgbehavior_t *bgbehavior_linear_new(float speed_x, float speed_y)
{
    bgbehavior_linear_t *me = mallocx(sizeof *me);
    bgbehavior_t *base = (bgbehavior_t*)me;

    base->offset = v2d_new(0.0f, 0.0f);
    base->update = bgbehavior_linear_update;
    base->delete = bgbehavior_linear_delete;

    me->speed = v2d_new(speed_x, speed_y);

    return base;
}

bgbehavior_t *bgbehavior_linear_delete(bgbehavior_t *behavior)
{
    free(behavior);
    return NULL;
}

void bgbehavior_linear_update(bgbehavior_t *behavior)
{
    bgbehavior_linear_t *me = (bgbehavior_linear_t*)behavior;
    float dt = timer_get_delta();

    /* linear movement */
    behavior->offset.x += me->speed.x * dt;
    behavior->offset.y += me->speed.y * dt;
}







/* rendering */

/* render layers of the background or of the foreground */
void render_layers(bglayer_t* const *layers, int layer_count, v2d_t camera_position, void* data, renderstrategy_t render_image)
{
    v2d_t screen_size = video_get_screen_size();
    v2d_t half_screen_size = v2d_multiply(screen_size, 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, half_screen_size);
    float animation_time = timer_get_elapsed();
    rect_t screen_rect = rect_new(0, 0, screen_size.x, screen_size.y);

    for(int i = 0; i < layer_count; i++) {
        const bglayer_t* layer = layers[i];
        const animation_t* animation = layer->animation;
        float frame_width = animation_frame_width(animation);
        float frame_height = animation_frame_height(animation);

        /* compute the position the layer in screen space */
        v2d_t scroll = v2d_compmult(layer->scroll_speed, topleft);
        v2d_t offset = v2d_add(layer->behavior->offset, scroll);
        v2d_t position = v2d_add(layer->initial_position, offset);
        position.x = floorf(0.5 + position.x); /* round to nearest integer */
        position.y = floorf(0.5 + position.y);

        /* tiled rendering? */
        int rows = 1, cols = 1;
        if(layer->repeat_x) {
            position.x = fmodf(position.x, frame_width) - frame_width;
            cols = 3 + (int)(screen_size.x / frame_width);
        }
        if(layer->repeat_y) {
            position.y = fmodf(position.y, frame_height) - frame_height;
            rows = 3 + (int)(screen_size.y / frame_height);
        }

        /* render */
        const image_t* image = animation_image_at_time(animation, animation_time);
        for(int y = 0; y < rows; y++) {
            for(int x = 0; x < cols; x++) {
                v2d_t image_position = v2d_new(position.x + x * frame_width, position.y + y * frame_height);
                rect_t image_rect = rect_new(image_position.x, image_position.y, frame_width, frame_height);

                if(rect_overlaps(image_rect, screen_rect)) /* clipping */
                    render_image(image, image_position, data);
            }
        }
    }
}

/* render an image */
void render_without_cache(const image_t* image, v2d_t position, void* data)
{
    image_draw(image, position.x, position.y, IF_NONE);
    (void)data;
}

/* render an image with FastDraw */
void render_with_cache(const image_t* image, v2d_t position, void* data)
{
    FAST_DRAW_CACHE* cache = (FAST_DRAW_CACHE*)data;
    fd_draw_bitmap(cache, IMAGE2BITMAP(image), position.x, position.y);
}





/* preprocessing */

/* sort layers by their z-indexes */
void sort_layers(bgtheme_t *bgtheme)
{
    /* merge_sort is a stable sorting algorithm. stdlib's qsort may not be. */
    merge_sort(bgtheme->layer, bgtheme->layer_count, sizeof *(bgtheme->layer), sort_cmp);
}

/* comparison function */
int sort_cmp(const void *a, const void *b)
{
    const bglayer_t *i = *((const bglayer_t**)a);
    const bglayer_t *j = *((const bglayer_t**)b);

    if(nearly_equal(i->zindex, j->zindex))
        return 0;

    return (i->zindex > j->zindex) - (i->zindex < j->zindex);
}

/* split background & foreground layers */
void split_layers(bgtheme_t *bgtheme)
{
    /*
     * bgtheme->layer[] is partitioned into background and foreground layers:
     *
     * layer[0 .. background_count-1] are the background layers
     * layer[background_count-1 .. layer_count-1] are the foreground layers
     */

    bgtheme->foreground_count = 0;

    for(int i = bgtheme->layer_count - 1; i >= 0; i--) {
        if(IS_FOREGROUND_LAYER(bgtheme->layer[i]))
            bgtheme->foreground_count++;
        else
            break; /* the array is assumed to be sorted */
    }

    bgtheme->background_count = bgtheme->layer_count - bgtheme->foreground_count;
}

/* group layers for deferred drawing */
void group_layers(bgtheme_t *bgtheme)
{
    #define layer_image(layer) animation_image((layer)->animation, 0)
    bglayer_t** layer = bgtheme->layer;

    /*
     * We use the technique explained at renderqueue.c for deferred drawing:
     * group_index is a piecewise monotonic decrease sequence: each piece
     * identifies a group of layers. Layers are grouped if they share a parent
     * bitmap. Grouped layers can be rendered efficiently via deferred drawing.
     */

    /* initialize indices */
    for(int i = 0; i < bgtheme->layer_count; i++)
        layer[i]->group_index = 1;

    /* group foreground layers */
    for(int i = bgtheme->layer_count - 2; i >= bgtheme->background_count; i--) {
        const image_t* a = layer_image(layer[i]);
        const image_t* b = layer_image(layer[i+1]);

        if(image_texture(a) == image_texture(b))
            layer[i]->group_index = 1 + layer[i+1]->group_index;
    }

    /* group background layers */
    for(int i = bgtheme->background_count - 2; i >= 0; i--) {
        const image_t* a = layer_image(layer[i]);
        const image_t* b = layer_image(layer[i+1]);

        if(image_texture(a) == image_texture(b))
            layer[i]->group_index = 1 + layer[i+1]->group_index;
    }

    /* warn if unoptimized */
    if(bgtheme->background_count > 0 && layer[0]->group_index < bgtheme->background_count)
        logfile_message("BACKGROUND: unoptimized multi-atlas background \"%s\"", bgtheme->filepath);

    #undef layer_image
}






/* .bg files */

/* traverse a .bg file */
int traverse(const parsetree_statement_t *stmt, void *bgtheme)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    bglayer_t *layer;
    bgtheme_t *theme = (bgtheme_t*)bgtheme;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "background") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_program(p1, "Can't read background layer: missing attributes");

        layer = bglayer_new();
        theme->layer = reallocx(theme->layer, (++(theme->layer_count)) * sizeof(*(theme->layer)));
        theme->layer[theme->layer_count - 1] = layer;

        nanoparser_traverse_program_ex(nanoparser_get_program(p1), layer, traverse_layer_attributes);
        validate_layer(layer);
    }
    else
        fatal_error("Can't read background layer. Unknown identifier: '%s'", identifier);

    return 0;
}

/* traverse a layer declaration of a .bg file */
int traverse_layer_attributes(const parsetree_statement_t *stmt, void *bglayer)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4, *p5, *p6, *p7;
    bglayer_t *layer = (bglayer_t*)bglayer;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "initial_position") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "initial_position must be a pair of numbers");
        nanoparser_expect_string(p2, "initial_position must be a pair of numbers");

        layer->initial_position.x = atof(nanoparser_get_string(p1));
        layer->initial_position.y = atof(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "scroll_speed") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "scroll_speed must be a pair of numbers");
        nanoparser_expect_string(p2, "scroll_speed must be a pair of numbers");

        layer->scroll_speed.x = atof(nanoparser_get_string(p1));
        layer->scroll_speed.y = atof(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "behavior") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);
        p4 = nanoparser_get_nth_parameter(param_list, 4);
        p5 = nanoparser_get_nth_parameter(param_list, 5);
        p6 = nanoparser_get_nth_parameter(param_list, 6);
        p7 = nanoparser_get_nth_parameter(param_list, 7);

        nanoparser_expect_string(p1, "Background behavior must be a string");

        if(str_icmp(nanoparser_get_string(p1), "DEFAULT") == 0) {
            bgbehavior_delete(layer->behavior);
            layer->behavior = bgbehavior_default_new();
        }
        else if(str_icmp(nanoparser_get_string(p1), "LINEAR") == 0) {
            nanoparser_expect_string(p2, "Linear background behavior expects a pair of numbers");
            nanoparser_expect_string(p3, "Linear background behavior expects a pair of numbers");

            bgbehavior_delete(layer->behavior);
            layer->behavior = bgbehavior_linear_new(
                atof(nanoparser_get_string(p2)), /* speed in pixels per second */
                atof(nanoparser_get_string(p3))
            );
        }
        else if(str_icmp(nanoparser_get_string(p1), "CIRCULAR") == 0) {
            nanoparser_expect_string(p2, "Circular background behavior expects at least four numbers");
            nanoparser_expect_string(p3, "Circular background behavior expects at least four numbers");
            nanoparser_expect_string(p4, "Circular background behavior expects at least four numbers");
            nanoparser_expect_string(p5, "Circular background behavior expects at least four numbers");

            bgbehavior_delete(layer->behavior);
            layer->behavior = bgbehavior_circular_new(
                atof(nanoparser_get_string(p2)), /* amplitude in pixels */
                atof(nanoparser_get_string(p3)),
                atof(nanoparser_get_string(p4)), /* angular speed in cycles per second */
                atof(nanoparser_get_string(p5)),
                atof(nanoparser_get_string(p6)), /* initial phase in degrees */
                atof(nanoparser_get_string(p7))
            );
        }
        else
            fatal_error("Unknown background behavior: '%s'", nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "repeat_x") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat_x expects a boolean value");
        layer->repeat_x = atob(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "repeat_y") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat_y expects a boolean value");
        layer->repeat_y = atob(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "zindex") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read background attributes: zindex expects a number between 0.0 (far) and 1.0 (near)");
        layer->zindex = clip01(atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "sprite") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Can't read background attributes: sprite block expected");

        if(layer->data != NULL)
            spriteinfo_destroy(layer->data);
        layer->data = spriteinfo_create(nanoparser_get_program(p1));
        layer->animation = spriteinfo_get_animation(layer->data, 0);
    }
    else
        fatal_error("Can't read background attributes. Unknown identifier: '%s'", identifier);

    return 0;
}

/* validate a layer */
void validate_layer(const bglayer_t *layer)
{
    if(layer->data == NULL || layer->animation == NULL)
        fatal_error("Can't read background layer: no sprite data given");
}

/* validate a background theme */
void validate_theme(const bgtheme_t* theme)
{
    if(theme->layer == NULL || theme->layer_count == 0)
        fatal_error("Invalid background: no layers were specified in \"%s\"", theme->filepath);
}