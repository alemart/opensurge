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
#include "../util/util.h"
#include "../util/stringutil.h"

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
    actor_t *actor; /* actor */
    spriteinfo_t *data; /* this is not stored in the main hash */

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
#define IS_FOREGROUND_LAYER(layer)    ((layer)->zindex > 0.5f)
static void sort_layers(bgtheme_t *bgtheme);
static int sort_cmp(const void *a, const void *b);
static void split_layers(bgtheme_t *bgtheme);
static void group_layers(bgtheme_t *bgtheme);
static void render_layers(const bgtheme_t *theme, v2d_t camera_position, bool foreground);
static int traverse(const parsetree_statement_t *stmt, void *bgtheme);
static int traverse_layer_attributes(const parsetree_statement_t *stmt, void *bglayer);
static void validate_layer(const bglayer_t *layer);




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
        bglayer_t *layer = bgtheme->layer[i];
        bgbehavior_update(layer->behavior);
    }
}

/*
 * background_render_bg()
 * Renders the background
 */
void background_render_bg(const bgtheme_t *bgtheme, v2d_t camera_position)
{
    render_layers(bgtheme, camera_position, false);
}

/*
 * background_render_fg()
 * Renders the foreground
 */
void background_render_fg(const bgtheme_t *bgtheme, v2d_t camera_position)
{
    render_layers(bgtheme, camera_position, true);
}

/*
 * background_filepath()
 * Returns the filepath of the background
 */
const char* background_filepath(const bgtheme_t *bgtheme)
{
    return bgtheme->filepath;
}





/* private methods */



/* create a new layer */
bglayer_t *bglayer_new()
{
    bglayer_t *layer = mallocx(sizeof *layer);

    layer->actor = actor_create();
    layer->data = NULL;
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
    actor_destroy(layer->actor);
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








/* render background or foreground layers */
void render_layers(const bgtheme_t *bgtheme, v2d_t camera_position, bool foreground)
{
    v2d_t halfscreen = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, halfscreen);
    int count = !foreground ? bgtheme->background_count : bgtheme->foreground_count;
    int start = !foreground ? 0 : bgtheme->background_count;
    /*int end = start + count;*/
    bool held = false;

    for(int i = 0; i < count; i++) {
        const bglayer_t* layer = bgtheme->layer[start + i];
        const bglayer_t* prev_layer = bgtheme->layer[start + ((i + (count-1)) % count)];

        /* compute the position the layer in screen space */
        v2d_t scroll = v2d_compmult(layer->scroll_speed, topleft);
        v2d_t offset = v2d_add(layer->behavior->offset, scroll);
        layer->actor->position = v2d_add(layer->initial_position, offset);
        layer->actor->position.x = floorf(0.5 + layer->actor->position.x); /* round to nearest integer */
        layer->actor->position.y = floorf(0.5 + layer->actor->position.y);

        /* enable deferred drawing */
        if(layer->group_index > prev_layer->group_index)
            image_hold_drawing(held = true);

        /* render */
        actor_render_repeat_xy(layer->actor, halfscreen, layer->repeat_x, layer->repeat_y);

        /* disable deferred drawing */
        if(held && layer->group_index == 1)
            image_hold_drawing(held = false);
    }
}

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
    #define layer_image(layer) actor_image((layer)->actor)
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

        if(image_same_root(a, b))
            layer[i]->group_index = 1 + layer[i+1]->group_index;
    }

    /* group background layers */
    for(int i = bgtheme->background_count - 2; i >= 0; i--) {
        const image_t* a = layer_image(layer[i]);
        const image_t* b = layer_image(layer[i+1]);

        if(image_same_root(a, b))
            layer[i]->group_index = 1 + layer[i+1]->group_index;
    }
}







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
        actor_change_animation(layer->actor, layer->data->animation_data[0]);
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
    }
    else
        fatal_error("Can't read background attributes. Unknown identifier: '%s'", identifier);

    return 0;
}

/* validate a layer */
void validate_layer(const bglayer_t *layer)
{
    if(layer->data == NULL)
        fatal_error("Can't read background layer: no sprite data given");
}