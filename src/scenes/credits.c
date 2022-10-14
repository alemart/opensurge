/*
 * Open Surge Engine
 * credits.c - credits scene
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#include <stdbool.h>
#include <math.h>
#include "credits.h"
#include "options.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/font.h"
#include "../core/asset.h"
#include "../core/logfile.h"
#include "../core/stringutil.h"
#include "../core/csv.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/sfx.h"



/* private data */
static const char* CREDITS_BGFILE = "themes/scenes/credits.bg";
static const float SCROLL_SPEED = 30.0f;
extern const char CREDITS_TEXT[];

static bool quit;
static font_t *title, *text, *back;
static input_t *input;
static bgtheme_t *bgtheme;
static music_t *music;
static scene_t *next_scene;

#define ASSETS_CATEGORIES 6
#define ASSETS_TEXT_MAXLEN 65536
extern const char CREDITS_ASSETS_CSV[];
static char assets_text_buffer[ASSETS_CATEGORIES * ASSETS_TEXT_MAXLEN];
static const char* assets_filter[ASSETS_CATEGORIES] = { "music", "level", "image", "translation", "sound", "font" };
static void aggregate_assets(int field_count, const char** fields, int line_number, void* user_data);
typedef struct {
    const char* desired_type; /* used as a filter */
    char last_author[256];
    char text_buffer[ASSETS_TEXT_MAXLEN];
    int text_length;
} assets_aggregator_t;



/* public functions */

/*
 * credits_init()
 * Initializes the scene
 */
void credits_init(void *foo)
{
    const char* assets_arguments[ASSETS_CATEGORIES];

    /* parse the text from the assets CSV file */
    for(int i = 0; i < ASSETS_CATEGORIES; i++) {
        char* text_buffer = assets_text_buffer + i * ASSETS_TEXT_MAXLEN;
        assets_aggregator_t helper = { assets_filter[i], "", "", 0 };
        csv_parse(CREDITS_ASSETS_CSV, ";", aggregate_assets, &helper);
        str_cpy(text_buffer, helper.text_buffer, ASSETS_TEXT_MAXLEN);
        assets_arguments[i] = text_buffer;
    }

    /* load components */
    quit = false;
    next_scene = NULL;
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);
    bgtheme = background_load(CREDITS_BGFILE);

    /* load title */
    title = font_create("MenuTitle");
    font_set_text(title, "%s", lang_get("CREDITS_TITLE"));
    font_set_position(title, v2d_new(VIDEO_SCREEN_W / 2, 5));
    font_set_align(title, FONTALIGN_CENTER);

    /* load footer */
    back = font_create("MenuText");
    font_set_text(back, "%s", lang_get("CREDITS_BACK"));
    font_set_position(back, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(back).y - 5));

    /* load the font that will display the credits */
    text = font_create("MenuText");
    font_set_textargumentsv(text, ASSETS_CATEGORIES, assets_arguments);
    font_set_text(text, "%s", CREDITS_TEXT);
    font_set_width(text, VIDEO_SCREEN_W - 20);
    font_set_position(text, v2d_new(10, VIDEO_SCREEN_H));

    /* fade-in */
    fadefx_in(color_rgb(0,0,0), 1.0);
}


/*
 * credits_release()
 * Releases the scene
 */
void credits_release()
{
    font_destroy(title);
    font_destroy(text);
    font_destroy(back);

    background_unload(bgtheme);
    input_destroy(input);
    music_unref(music);
}


/*
 * credits_update()
 * Updates the scene
 */
void credits_update()
{
    float scroll_speed_multiplier = 1.0f;
    float dt = timer_get_delta();
    v2d_t textpos;

    /* background movement */
    background_update(bgtheme);

    /* scroll the text faster */
    if(input_button_down(input, IB_DOWN))
        scroll_speed_multiplier = -5.0f;
    else if(input_button_down(input, IB_UP) || input_button_down(input, IB_FIRE1))
        scroll_speed_multiplier = 5.0f;

    /* text movement */
    textpos = font_get_position(text);
    textpos.y -= (scroll_speed_multiplier * SCROLL_SPEED) * dt;
    if(textpos.y < -font_get_textsize(text).y || textpos.y > VIDEO_SCREEN_H)
        textpos.y = VIDEO_SCREEN_H;
    font_set_position(text, textpos);

    /* quit */
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play(SFX_BACK);
            next_scene = NULL;
            quit = true;
        }
    }

    /* music */
    if(!music_is_playing())
        music_play(music, true);

    /* fade-out */
    if(quit) {
        if(fadefx_is_over()) {
            scenestack_pop();
            if(next_scene != NULL)
                scenestack_push(next_scene, NULL);
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0);
    }
}


/*
 * credits_render()
 * Renders the scene
 */
void credits_render()
{
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    font_render(text, cam);
    background_render_fg(bgtheme, cam);
    font_render(title, cam);
    font_render(back, cam);
}


/* private stuff */

/* CSV callback */
void aggregate_assets(int field_count, const char** fields, int line_number, void* user_data)
{
    const int FIELD_TYPE = 0, FIELD_FILE = 1, FIELD_LICENSE = 2,
              FIELD_AUTHOR = 3, FIELD_WEBSITE = 4, FIELD_NOTES = 5,
              NUMBER_OF_FIELDS = 6;

    /* get the helper structure */
    assets_aggregator_t* helper = (assets_aggregator_t*)user_data;
    const int capacity = sizeof(helper->text_buffer) - 1;

    /* this macro appends a string to the text buffer */
    #define append_string(str) do { \
        const char* p = (str); \
        while(*p && helper->text_length < capacity) \
            helper->text_buffer[helper->text_length++] = *(p++); \
        helper->text_buffer[helper->text_length] = 0; \
    } while(0)

    /* this entry does not have the expected number of fields */
    if(field_count < NUMBER_OF_FIELDS) {
        logfile_message("Error when reading the credits csv file: line %d has %d fields, but %d fields are expected", line_number+1, field_count, NUMBER_OF_FIELDS);
        return;
    }

    /* this entry is not of the desired type */
    if(strcmp(fields[FIELD_TYPE], helper->desired_type) != 0)
        return;

    /* this entry has a different author than the previous one */
    if(strcmp(fields[FIELD_AUTHOR], helper->last_author) != 0) {
        append_string("\n");
        append_string(fields[FIELD_AUTHOR]);
        if(*fields[FIELD_WEBSITE]) {
            append_string(" [");
            append_string(fields[FIELD_WEBSITE]);
            append_string("]\n");
        }
        else
            append_string("\n");

        str_cpy(helper->last_author, fields[FIELD_AUTHOR], sizeof(helper->last_author));
    }

    /* write file & license to the output string */
    append_string("- ");
    append_string(fields[FIELD_FILE]);
    append_string(" (");
    append_string(fields[FIELD_LICENSE]);
    append_string(")\n");

    /* write additional notes, if present, to the output string */
    if(*fields[FIELD_NOTES]) {
        append_string("  ^ ");
        append_string(fields[FIELD_NOTES]);
        append_string("\n");
    }
}