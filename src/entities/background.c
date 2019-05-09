/*
 * Open Surge Engine
 * background.c - level background/foreground
 * Copyright (C) 2010, 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <math.h>
#include "background.h"
#include "actor.h"
#include "../core/sprite.h"
#include "../core/video.h"
#include "../core/assetfs.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/logfile.h"
#include "../core/timer.h"
#include "../core/nanoparser/nanoparser.h"

/* forward declarations */
typedef struct background_t background_t;
typedef struct bgstrategy_t bgstrategy_t;
typedef struct bgstrategy_default_t bgstrategy_default_t;
typedef struct bgstrategy_circular_t bgstrategy_circular_t;
typedef struct bgstrategy_linear_t bgstrategy_linear_t;

/* === bgtheme struct (an ordered set of backgrounds) === */
struct bgtheme_t {
    background_t **data; /* array of background_t* */
    int length; /* length of the data vector */
    char* filepath; /* filepath of the background */
};

/* === <<abstract>> bgstrategy_t === */
struct bgstrategy_t {
    background_t *background; /* the background instance we're linked to */
    void (*update)(bgstrategy_t*); /* update function */
};
static bgstrategy_t *bgstrategy_delete(bgstrategy_t *strategy); /* class destructor */


/* === background struct === */
struct background_t {
    actor_t *actor; /* actor */
    spriteinfo_t *data; /* this is not stored in the main hash */
    int repeat_x, repeat_y; /* repeat background? */
    float zindex; /* 0.0 (far) <= zindex <= 1.0 (near) */
    bgstrategy_t *strategy; /* Strategy design pattern */
};
static background_t *background_new(); /* constructor */
static background_t *background_delete(background_t *bg); /* destructor */


/* === concrete strategies (background behaviors) === */

/* default background strategy */
struct bgstrategy_default_t {
    bgstrategy_t base; /* inherits from bgstrategy_t */
};
static bgstrategy_t *bgstrategy_default_new(background_t *background); /* class constructor */
static void bgstrategy_default_update(bgstrategy_t *strategy); /* private class method*/

/* circular background strategy (elliptical trajectory) */
struct bgstrategy_circular_t {
    bgstrategy_t base; /* base class */
    float timer;
    float amplitude_x, amplitude_y;
    float angularspeed_x, angularspeed_y;
    float initialphase_x, initialphase_y;
};
static bgstrategy_t *bgstrategy_circular_new(background_t *background, float amplitude_x, float amplitude_y, float angularspeed_x, float angularspeed_y, float initialphase_x, float initialphase_y); /* class constructor */
static void bgstrategy_circular_update(bgstrategy_t *strategy); /* private class method */

/* linear background strategy */
struct bgstrategy_linear_t {
    bgstrategy_t base; /* base class */
    float speed_x, speed_y;
};
static bgstrategy_t *bgstrategy_linear_new(background_t *background, float speed_x, float speed_y); /* class constructor */
static void bgstrategy_linear_update(bgstrategy_t *strategy); /* private class method */


/* === internal methods === */
static void sort_backgrounds(bgtheme_t *bgtheme);
static int sort_cmp(const void *a, const void *b);
static void render(const bgtheme_t *theme, v2d_t camera_position, int foreground);
static int traverse(const parsetree_statement_t *stmt, void *bgtheme);
static int traverse_background_attributes(const parsetree_statement_t *stmt, void *background);
static void validate_background(const background_t *bg);


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
    fullpath = assetfs_fullpath(filepath);

    bgtheme = mallocx(sizeof *bgtheme);
    bgtheme->filepath = str_dup(filepath);
    bgtheme->data = NULL;
    bgtheme->length = 0;

    tree = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program_ex(tree, (void*)bgtheme, traverse);
    tree = nanoparser_deconstruct_tree(tree);

    sort_backgrounds(bgtheme);
    return bgtheme;
}

/*
 * background_unload()
 * Unloads a background theme
 */
bgtheme_t* background_unload(bgtheme_t *bgtheme)
{
    logfile_message("Will unload background \"%s\"...", bgtheme->filepath);

    if(bgtheme->data != NULL) {
        for(int i = 0; i < bgtheme->length; i++)
            bgtheme->data[i] = background_delete(bgtheme->data[i]);

        free(bgtheme->data);
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
    for(int i = 0; i < bgtheme->length; i++) {
        background_t *bg = bgtheme->data[i];
        bg->strategy->update(bg->strategy);
    }
}

/*
 * background_render_bg()
 * Renders the background
 */
void background_render_bg(const bgtheme_t *bgtheme, v2d_t camera_position)
{
    render(bgtheme, camera_position, FALSE);
}

/*
 * background_render_fg()
 * Renders the foreground
 */
void background_render_fg(const bgtheme_t *bgtheme, v2d_t camera_position)
{
    render(bgtheme, camera_position, TRUE);
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

background_t *background_new()
{
    background_t *bg = mallocx(sizeof *bg);

    bg->actor = actor_create();
    bg->data = NULL;
    bg->strategy = NULL;
    bg->repeat_x = FALSE;
    bg->repeat_y = FALSE;
    bg->zindex = 0.0f;

    return bg;
}

background_t *background_delete(background_t *bg)
{
    bg->strategy = bgstrategy_delete(bg->strategy);
    spriteinfo_destroy(bg->data);
    actor_destroy(bg->actor);
    free(bg);

    return NULL;
}

bgstrategy_t *bgstrategy_delete(bgstrategy_t *strategy)
{
    free(strategy);
    return NULL;
}

bgstrategy_t *bgstrategy_default_new(background_t *background)
{
    bgstrategy_default_t *me = mallocx(sizeof *me);
    bgstrategy_t *base = (bgstrategy_t*)me;

    base->background = background;
    base->update = bgstrategy_default_update;

    return base;
}

void bgstrategy_default_update(bgstrategy_t *strategy)
{
    ; /* empty */
}


bgstrategy_t *bgstrategy_circular_new(background_t *background, float amplitude_x, float amplitude_y, float angularspeed_x, float angularspeed_y, float initialphase_x, float initialphase_y)
{
    bgstrategy_circular_t *me = mallocx(sizeof *me);
    bgstrategy_t *base = (bgstrategy_t*)me;

    base->background = background;
    base->update = bgstrategy_circular_update;
    me->timer = 0.0f;
    me->amplitude_x = amplitude_x;
    me->amplitude_y = amplitude_y;
    me->angularspeed_x = (2.0f * PI) * angularspeed_x;
    me->angularspeed_y = (2.0f * PI) * angularspeed_y;
    me->initialphase_x = (initialphase_x * PI) / 180.0f;
    me->initialphase_y = (initialphase_y * PI) / 180.0f;

    return base;
}


void bgstrategy_circular_update(bgstrategy_t *strategy)
{
    bgstrategy_circular_t *me = (bgstrategy_circular_t*)strategy;
    background_t *bg = strategy->background;
    float dt = timer_get_delta();
    float t, sx, cy;

    t = (me->timer += dt);
    sx = sin(me->angularspeed_x * t + me->initialphase_x);
    cy = cos(me->angularspeed_y * t + me->initialphase_y);

    /* elliptical trajectory */
    bg->actor->position.x += (-me->angularspeed_x * me->amplitude_x * sx) * dt;
    bg->actor->position.y += (me->angularspeed_y * me->amplitude_y * cy) * dt;
}


bgstrategy_t *bgstrategy_linear_new(background_t *background, float speed_x, float speed_y)
{
    bgstrategy_linear_t *me = mallocx(sizeof *me);
    bgstrategy_t *base = (bgstrategy_t*)me;

    base->background = background;
    base->update = bgstrategy_linear_update;
    me->speed_x = speed_x;
    me->speed_y = speed_y;

    return base;
}


void bgstrategy_linear_update(bgstrategy_t *strategy)
{
    bgstrategy_linear_t *me = (bgstrategy_linear_t*)strategy;
    background_t *bg = strategy->background;
    float dt = timer_get_delta();

    bg->actor->position.x += me->speed_x * dt;
    bg->actor->position.y += me->speed_y * dt;
}


void render(const bgtheme_t *bgtheme, v2d_t camera_position, int foreground)
{
    int i;
    v2d_t halfscreen = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);
    v2d_t topleft = v2d_subtract(camera_position, halfscreen);
    background_t *bg;

    for(i=0; i<bgtheme->length; i++) {
        bg = bgtheme->data[i];
        if((!foreground && bg->zindex <= 0.5f) || (foreground && bg->zindex > 0.5f)) {
            bg->actor->position.x += topleft.x * bg->actor->speed.x;
            bg->actor->position.y += topleft.y * bg->actor->speed.y;

            actor_render_repeat_xy(bg->actor, halfscreen, bg->repeat_x, bg->repeat_y);

            bg->actor->position.y -= topleft.y * bg->actor->speed.y;
            bg->actor->position.x -= topleft.x * bg->actor->speed.x;
        }
    }
}

void sort_backgrounds(bgtheme_t *bgtheme)
{
    /* merge_sort is a stable sorting algorithm. stdlib's qsort may not be. */
    merge_sort(bgtheme->data, bgtheme->length, sizeof *(bgtheme->data), sort_cmp);
}

int sort_cmp(const void *a, const void *b)
{
    const background_t *i = *((const background_t**)a);
    const background_t *j = *((const background_t**)b);

    if(nearly_equal(i->zindex, j->zindex))
        return 0;
    else if(i->zindex < j->zindex)
        return -1;
    else
        return 1;
}

int traverse(const parsetree_statement_t *stmt, void *bgtheme)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    background_t *bg;
    bgtheme_t *theme = (bgtheme_t*)bgtheme;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "background") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_program(p1, "Can't read background. Missing background attributes");

        bg = background_new();
        theme->data = reallocx(theme->data, (++(theme->length)) * sizeof(*(theme->data)));
        theme->data[theme->length-1] = bg;
        nanoparser_traverse_program_ex(nanoparser_get_program(p1), (void*)bg, traverse_background_attributes);
        validate_background(bg);
        actor_change_animation(bg->actor, bg->data->animation_data[0]);
    }
    else
        fatal_error("Can't read background. Unknown identifier: '%s'", identifier);

    return 0;
}

int traverse_background_attributes(const parsetree_statement_t *stmt, void *background)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4, *p5, *p6, *p7;
    background_t *bg = (background_t*)background;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "initial_position") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "initial_position must be a pair of numbers");
        nanoparser_expect_string(p2, "initial_position must be a pair of numbers");

        bg->actor->spawn_point.x = atof(nanoparser_get_string(p1));
        bg->actor->spawn_point.y = atof(nanoparser_get_string(p2));
        bg->actor->position = bg->actor->spawn_point;
    }
    else if(str_icmp(identifier, "scroll_speed") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "scroll_speed must be a pair of numbers");
        nanoparser_expect_string(p2, "scroll_speed must be a pair of numbers");

        bg->actor->speed.x = atof(nanoparser_get_string(p1));
        bg->actor->speed.y = atof(nanoparser_get_string(p2));
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
            if(bg->strategy)
                bg->strategy = bgstrategy_delete(bg->strategy);
            bg->strategy = bgstrategy_default_new(bg);
        }
        else if(str_icmp(nanoparser_get_string(p1), "LINEAR") == 0) {
            nanoparser_expect_string(p2, "Linear background behavior expects a pair of numbers");
            nanoparser_expect_string(p3, "Linear background behavior expects a pair of numbers");

            if(bg->strategy)
                bg->strategy = bgstrategy_delete(bg->strategy);
            bg->strategy = bgstrategy_linear_new(
                bg,
                atof(nanoparser_get_string(p2)),
                atof(nanoparser_get_string(p3))
            );
        }
        else if(str_icmp(nanoparser_get_string(p1), "CIRCULAR") == 0) {
            nanoparser_expect_string(p2, "Circular background behavior expects at least four numbers");
            nanoparser_expect_string(p3, "Circular background behavior expects at least four numbers");
            nanoparser_expect_string(p4, "Circular background behavior expects at least four numbers");
            nanoparser_expect_string(p5, "Circular background behavior expects at least four numbers");

            if(bg->strategy)
                bg->strategy = bgstrategy_delete(bg->strategy);
            bg->strategy = bgstrategy_circular_new(
                bg,
                atof(nanoparser_get_string(p2)),
                atof(nanoparser_get_string(p3)),
                atof(nanoparser_get_string(p4)),
                atof(nanoparser_get_string(p5)),
                atof(nanoparser_get_string(p6)),
                atof(nanoparser_get_string(p7))
            );
        }
        else
            fatal_error("Unknown background behavior: '%s'", nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "repeat_x") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat_x expects a boolean value");
        bg->repeat_x = atob(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "repeat_y") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat_y expects a boolean value");
        bg->repeat_y = atob(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "zindex") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read background attributes: zindex expects a number between 0.0 (far) and 1.0 (near)");
        bg->zindex = clip(atof(nanoparser_get_string(p1)), 0.0f, 1.0f);
    }
    else if(str_icmp(identifier, "sprite") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Can't read background attributes: sprite block expected");
        if(bg->data != NULL)
            spriteinfo_destroy(bg->data);
        bg->data = spriteinfo_create(nanoparser_get_program(p1));
    }
    else
        fatal_error("Can't read background attributes. Unknown identifier: '%s'", identifier);

    return 0;
}

void validate_background(const background_t *bg)
{
    if(bg->data == NULL)
        fatal_error("Can't read background: no sprite data given");

    if(bg->strategy == NULL)
        fatal_error("Can't read background: no behavior given");
}

