/*
 * Open Surge Engine
 * credits.c - credits scene
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
#include "settings.h"
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
#include "../util/csv.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/mobilegamepad.h"
#include "../entities/sfx.h"



/* private data */
#define CREDITS_MOD_FILE "credits.txt"
static const char* CREDITS_BGFILE = "themes/scenes/credits.bg";
static const float SCROLL_SPEED = 30.0f;
extern const char CREDITS_TEXT[];

static bool quit;
static font_t *title, *text, *back;
static input_t *input;
static bgtheme_t *bgtheme;
static music_t *music;
static scene_t *next_scene;
static int text_height;
static bool was_immersive;

#define ASSETS_CATEGORIES 6
#define ASSETS_TEXT_MAXLEN 65536
extern const char CREDITS_ASSETS_CSV[];
static char assets_text_buffer[ASSETS_CATEGORIES * ASSETS_TEXT_MAXLEN] = "";
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
    /* generate the credits text */
    const char* base_text;
    const char** assets_argv;
    int assets_argc;

    credits_text(&base_text, &assets_argc, &assets_argv);

    /* load components */
    quit = false;
    next_scene = NULL;
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);
    bgtheme = background_load(CREDITS_BGFILE);

    /* load title */
    title = font_create("MenuTitle");
    font_set_text(title, "%s", lang_get("CREDITS_COLORED_TITLE"));
    font_set_position(title, v2d_new(VIDEO_SCREEN_W / 2, 5));
    font_set_align(title, FONTALIGN_CENTER);

    /* load footer */
    back = font_create("MenuText");
    font_set_text(back, "%s", lang_get("CREDITS_BACK"));
    font_set_position(back, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(back).y - 5));

    /* load the font that will display the credits */
    text = font_create("MenuText");
    font_set_textargumentsv(text, assets_argc, assets_argv);
    font_set_text(text, "%s\n%s", credits_mod_text(), base_text);
    font_set_width(text, VIDEO_SCREEN_W - 20);
    font_set_position(text, v2d_new(10, VIDEO_SCREEN_H));
    text_height = font_get_textsize(text).y;

    /* fade-in */
    fadefx_in(color_rgb(0,0,0), 1.0);

    /* immersive mode */
    was_immersive = video_is_immersive();
    video_set_immersive(false);
}


/*
 * credits_release()
 * Releases the scene
 */
void credits_release()
{
    video_set_immersive(was_immersive);

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

    /* display the mobile gamepad */
    mobilegamepad_fadein();

    /* scroll the text faster */
    if(input_button_down(input, IB_DOWN))
        scroll_speed_multiplier = -5.0f;
    else if(input_button_down(input, IB_UP) || input_button_down(input, IB_FIRE1))
        scroll_speed_multiplier = 5.0f;

    /* text movement */
    textpos = font_get_position(text);
    textpos.y -= (scroll_speed_multiplier * SCROLL_SPEED) * dt;
    if(textpos.y < -text_height || textpos.y > VIDEO_SCREEN_H)
        textpos.y = VIDEO_SCREEN_H;
    font_set_position(text, textpos);

    /* quit */
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_FIRE4) || input_button_pressed(input, IB_FIRE2)) {
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

/*
 * credits_text()
 * Generates the credits text and stores it in statically allocated buffers
 */
void credits_text(const char** base_text, int* assets_argc, const char*** assets_argv)
{
    static const char* assets_arguments[ASSETS_CATEGORIES]; /* must allocate statically */

    /* parse the text from the assets CSV file */
    for(int i = 0; i < ASSETS_CATEGORIES; i++) {
        char* text_buffer = assets_text_buffer + i * ASSETS_TEXT_MAXLEN;

        assets_aggregator_t helper = { assets_filter[i], "", "", 0 };
        csv_parse(CREDITS_ASSETS_CSV, ";", aggregate_assets, &helper);
        str_cpy(text_buffer, helper.text_buffer, ASSETS_TEXT_MAXLEN);

        assets_arguments[i] = text_buffer;
    }

    /* return the credits text */
    *base_text = CREDITS_TEXT;
    *assets_argc = ASSETS_CATEGORIES;
    *assets_argv = assets_arguments;
}

/*
 * credits_mod_text()
 * Credits text of a mod; returns a statically allocated buffer
 */
const char* credits_mod_text()
{
    static char buffer[65536] = "";
    const char* game_name = opensurge_game_name();
    bool verbatim = false;

    /* not a mod? */
    buffer[0] = '\0';
    if(0 == strcmp(game_name, "Surge the Rabbit"))
        return buffer;

    /* add a default text */
    snprintf(buffer, sizeof(buffer),
        "<color=$COLOR_HIGHLIGHT>%.48s</color>\n"
        "Created by %.48s authors\n\n"
        "(missing %s file)\n\n",
        game_name,
        game_name,
        CREDITS_MOD_FILE
    );

    /* read MOD credits file, if it exists */
    const char* fullpath = asset_path(CREDITS_MOD_FILE);
    ALLEGRO_FILE* fp = al_fopen(fullpath, "r");

    if(fp != NULL) {
        const int64_t max_file_size = sizeof(buffer) - 3;
        int64_t file_size = al_fsize(fp);

        if(file_size > max_file_size) /* we don't expect large files */
            file_size = max_file_size;

        if(file_size > 0) {
            size_t size = (size_t)file_size;
            size_t n = al_fread(fp, buffer, size);

            buffer[n] = '\n';
            buffer[n+1] = '\n';
            buffer[n+2] = '\0';

            verbatim = true;
        }

        al_fclose(fp);
    }

    /* a limitation of the current approach is that the MOD credits file may
       unduly trigger text arguments $1 ... $9 . Not sure this is the best way :/ */
    if(verbatim) {
        for(char* p = buffer; *p != '\0'; p++) {

            /* we can't write "$9.99", but we can write "$ 9.99" */
            if(*p == '$' && *(p+1) >= '0' && *(p+1) <= '9')
                *p = ' '; /* not correct */

            /* text <within tags> won't show up either, so... I should add a
               <verbatim> mode to the font module, so that text shows up unprocessed */
            else if(*p == '<')
                *p = '[';
            else if(*p == '>')
                *p = ']';

        }
    }

    /* add a horizontal line */
    const char hr[] = "_____________________[ Engine / base ]_____________________";
    size_t length = strlen(buffer);
    if(sizeof(buffer) - 1 > length)
        str_cpy(buffer + length, hr, sizeof(buffer) - 1 - length);

    /* done! */
    return buffer;
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