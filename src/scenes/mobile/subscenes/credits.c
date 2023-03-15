/*
 * Open Surge Engine
 * info.c - credits subscene for mobile devices
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

#include <stdio.h>
#include "credits.h"
#include "../util/touch.h"
#include "../../credits.h"
#include "../../../core/timer.h"
#include "../../../core/video.h"
#include "../../../core/image.h"
#include "../../../core/input.h"
#include "../../../core/color.h"
#include "../../../core/font.h"
#include "../../../core/util.h"
#include "../../../core/stringutil.h"
#include "../../../core/asset.h"
#include "../../../core/global.h"

typedef struct mobile_subscene_credits_t mobile_subscene_credits_t;
struct mobile_subscene_credits_t {
    mobile_subscene_t super;
    image_t* backbuffer;
    font_t* font;
    input_t* mouse;
    int text_height;
    v2d_t touch_previous;
    v2d_t smooth_scroll;
};

static void init(mobile_subscene_t*);
static void release(mobile_subscene_t*);
static void update(mobile_subscene_t*,v2d_t);
static void render(mobile_subscene_t*,v2d_t);
static const mobile_subscene_t super = { .init = init, .release = release, .update = update, .render = render };

#define BACKGROUND_COLOR "303030" /* RGB hex code */
static const v2d_t FONT_POSITION = { .x = 4, .y = 128 };
static const char FONT_NAME[] = "MenuText";
static const float SCROLL_SPEED = 30.0f;

#define MOD_CREDITS_FILE "credits.txt"
static const char* mod_credits();

static float SMOOTH_SCROLL_COEFFICIENT = 0.97f;
static void on_touch_start(v2d_t touch_start, void* subscene_ptr);
static void on_touch_move(v2d_t touch_start, v2d_t touch_current, void* subscene_ptr);



/*
 * mobile_subscene_credits()
 * Returns a new instance of the credits subscene
 */
mobile_subscene_t* mobile_subscene_credits()
{
    mobile_subscene_credits_t* subscene = mallocx(sizeof *subscene);

    subscene->super = super;
    subscene->font = NULL;
    subscene->backbuffer = NULL;
    subscene->mouse = NULL;
    
    return (mobile_subscene_t*)subscene;
}






/*
 * init()
 * Initializes the subscene
 */
void init(mobile_subscene_t* subscene_ptr)
{
    mobile_subscene_credits_t* subscene = (mobile_subscene_credits_t*)subscene_ptr;

    /* generate the credits text */
    const char* base_text;
    const char** assets_argv;
    int assets_argc;

    credits_text(&base_text, &assets_argc, &assets_argv);

    /* create a mouse input */
    input_t* mouse = input_create_mouse();
    subscene->mouse = mouse;

    /* create a backbuffer */
    image_t* backbuffer = image_create_backbuffer(VIDEO_SCREEN_W, VIDEO_SCREEN_H);
    subscene->backbuffer = backbuffer;

    /* create a font */
    font_t* font = font_create(FONT_NAME);

    font_set_position(font, FONT_POSITION);
    font_set_align(font, FONTALIGN_LEFT);
    font_set_width(font, VIDEO_SCREEN_W - 2 * FONT_POSITION.x);

    subscene->font = font;

    /* set the credits text */
    font_set_textargumentsv(font, assets_argc, assets_argv);
    font_set_text(font, "%s\n\n%s\n%s", "$CREDITS_TITLE", mod_credits(), base_text);

    /* misc */
    subscene->text_height = font_get_textsize(font).y;
    subscene->touch_previous = v2d_new(0.0f, 0.0f);
    subscene->smooth_scroll = v2d_new(0.0f, 0.0f);
}

/*
 * release()
 * Releases the subscene
 */
void release(mobile_subscene_t* subscene_ptr)
{
    mobile_subscene_credits_t* subscene = (mobile_subscene_credits_t*)subscene_ptr;

    font_destroy(subscene->font);
    image_destroy(subscene->backbuffer);
    input_destroy(subscene->mouse);

    free(subscene);
}

/*
 * update()
 * Updates the subscene
 */
void update(mobile_subscene_t* subscene_ptr, v2d_t subscene_offset)
{
    mobile_subscene_credits_t* subscene = (mobile_subscene_credits_t*)subscene_ptr;
    float scroll_speed_multiplier = 1.0f;
    float dt = timer_get_delta();

    /* pause the scroll? */
    bool pause_scroll = (v2d_magnitude(subscene_offset) > 0.0f);
    if(pause_scroll) {
        subscene->smooth_scroll = v2d_new(0.0f, 0.0f);
        return;
    }

    /* faster scroll? */
    handle_touch_input_ex(subscene->mouse, subscene, on_touch_start, NULL, on_touch_move);

    font_set_position(subscene->font, v2d_add(
        font_get_position(subscene->font),
        subscene->smooth_scroll
    ));

    subscene->smooth_scroll = v2d_lerp(
        v2d_new(0.0f, 0.0f),
        subscene->smooth_scroll,
        SMOOTH_SCROLL_COEFFICIENT
    );

    /* scroll & wrap the text automatically */
    v2d_t position = font_get_position(subscene->font);

    position.y -= (scroll_speed_multiplier * SCROLL_SPEED) * dt;
    if(position.y < -subscene->text_height || position.y > VIDEO_SCREEN_H)
        position.y = VIDEO_SCREEN_H;

    font_set_position(subscene->font, position);
}

/*
 * render()
 * Renders the subscene
 */
void render(mobile_subscene_t* subscene_ptr, v2d_t subscene_offset)
{
    mobile_subscene_credits_t* subscene = (mobile_subscene_credits_t*)subscene_ptr;
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);
    int x = subscene_offset.x;
    int y = subscene_offset.y;

    /* render the font to the backbuffer of the subscene */
    image_set_drawing_target(subscene->backbuffer);
        image_clear(color_hex(BACKGROUND_COLOR));
        font_render(subscene->font, camera);
    image_set_drawing_target(video_get_backbuffer());

    /* display the backbuffer of the subscene */
    image_draw(subscene->backbuffer, x, y, IF_NONE);
}




/* private */

/* implementation of faster scroll */
void on_touch_move(v2d_t touch_start, v2d_t touch_current, void* subscene_ptr)
{
    mobile_subscene_credits_t* subscene = (mobile_subscene_credits_t*)subscene_ptr;
    v2d_t delta = v2d_new(0.0f, touch_current.y - subscene->touch_previous.y);

    subscene->smooth_scroll = delta;
    subscene->touch_previous = touch_current;
}

void on_touch_start(v2d_t touch_start, void* subscene_ptr)
{
    mobile_subscene_credits_t* subscene = (mobile_subscene_credits_t*)subscene_ptr;

    subscene->touch_previous = touch_start;
}



/* consider this to be experimental. I'm not quite sure this is the best design. */
const char* mod_credits()
{
    static char buffer[65536] = "";

    /* return the cached text */
    if(*buffer != '\0')
        return buffer;

    /* add a default text */
    const char* game_name = video_get_window_title(); /* FIXME: is the title of the window always equal to the name of the game? */
    bool add_file_notice = (0 != strcmp(game_name, "Surge the Rabbit")); /* FIXME: find a better solution */

    if(add_file_notice) {
        snprintf(buffer, sizeof(buffer),

            "%s\n"
            "Created by %s authors\n"
            "\n"
            "(add your %s to see it here)\n\n",

            game_name,
            game_name,
            MOD_CREDITS_FILE

        );
    }

    /* read MOD credits file, if it exists */
    const char* fullpath = asset_path(MOD_CREDITS_FILE);
    ALLEGRO_FILE* fp = al_fopen(fullpath, "r");

    if(fp != NULL) {
        int64_t file_size = al_fsize(fp);

        if(file_size > 0) {
            size_t size = min((size_t)file_size, sizeof(buffer) - 1); /* we don't expect large files */
            size_t n = al_fread(fp, buffer, size);

            buffer[n] = '\0';
        }

        al_fclose(fp);
    }

    /* a limitation of the current approach is that the MOD credits file may
       unduly trigger text arguments $1 ... $9 . Not sure this is the best way :/ */
    for(char* p = buffer; *p != '\0'; p++) {

        /* we can't write "$9.99", but we can write "$ 9.99" */
        if(*p == '$' && *(p+1) >= '0' && *(p+1) <= '9')
            *p = ' '; /* not correct */

        /* text <within tags> won't show up either, so... I should add a
           <verbatim> mode to the font module, so that text shows up unprocessed */

    }

    /* done! */
    return buffer;
}