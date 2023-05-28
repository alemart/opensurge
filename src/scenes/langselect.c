/*
 * Open Surge Engine
 * langselect.c - language selection screen
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

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "langselect.h"
#include "options.h"
#include "../core/global.h"
#include "../core/scene.h"
#include "../core/asset.h"
#include "../core/logfile.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/font.h"
#include "../core/prefs.h"
#include "../core/mobile_gamepad.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/sfx.h"


/* private data */
static const char* LANG_BGFILE = "themes/scenes/langselect.bg";
static const int LANG_MAXPERCOL = 5;
static const int LANG_MAXCOLS = 3;
typedef struct {
    char name[128];
    char author[128];
    char filepath[1024];
} lngdata_t;

static bool quit;
static int lngcount;
static font_t *title, **lngfnt[2], *author_label;
static lngdata_t *lngdata;
static int option; /* current option: 0..n-1 */
static actor_t *arrow;
static input_t *input;
static float scene_time;
static float option_time;
static bgtheme_t *bgtheme;
static music_t *music;
static bool fresh_install;
static bool came_from_options;
static v2d_t sliding_camera;
static int column_width;

/* private functions */
static void save_preferences(const char *filepath);
static void load_lang_list();
static void unload_lang_list();
static int dirfill(const char *filename, void *param);
static int dircount(const char *filename, void *param);
static int sort_cmp(const void *a, const void *b);
static int change_option(int new_option);
static int option_of(const char *language_name);






/* public functions */

/*
 * langselect_init()
 * Initializes the scene
 */
void langselect_init(void *param)
{
    extern prefs_t* prefs;

    quit = false;
    option = 0;
    option_time = 9999.0f;
    scene_time = 0.0f;
    fresh_install = !prefs_has_item(prefs, ".langpath");
    came_from_options = (param != NULL) && *((bool*)param);
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);
    sliding_camera = v2d_new(0, VIDEO_SCREEN_H / 2);
    column_width = VIDEO_SCREEN_W / (LANG_MAXCOLS - 0.35f);

    author_label = font_create("MenuText");

    title = font_create("MenuTitle");
    font_set_text(title, "%s", "<color=$COLOR_TITLE>SELECT YOUR\nLANGUAGE</color>");
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 5));
    font_set_align(title, FONTALIGN_CENTER);

    bgtheme = background_load(LANG_BGFILE);

    arrow = actor_create();
    actor_change_animation(arrow, sprite_get_animation("UI Pointer", 0));

    load_lang_list();
    if(lngcount <= 1) {
        if(came_from_options)
            video_showmessage("No translations are available!");
        scenestack_pop();
        return;
    }
    option = option_of(lang_get("LANG_NAME"));

    fadefx_in(color_rgb(0,0,0), 1.0f);
}


/*
 * langselect_release()
 * Releases the scene
 */
void langselect_release()
{
    unload_lang_list();
    bgtheme = background_unload(bgtheme);

    actor_destroy(arrow);
    font_destroy(title);
    font_destroy(author_label);
    input_destroy(input);
    music_unref(music);
}


/*
 * langselect_update()
 * Updates the scene
 */
void langselect_update()
{
    float dest_x, dt = timer_get_delta();

    /* update timers */
    scene_time += dt;
    option_time += dt;

    /* background movement */
    background_update(bgtheme);

    /* display the mobile gamepad */
    mobilegamepad_fadein();

    /* menu option */
    arrow->position = font_get_position(lngfnt[0][option]);
    arrow->position.x += -20 + 3 * cosf(TWO_PI * scene_time);
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_DOWN)) {
            if(option / LANG_MAXPERCOL == (option + 1) / LANG_MAXPERCOL) {
                if(option < lngcount - 1)
                    change_option(option + 1);
            }
        }
        if(input_button_pressed(input, IB_UP)) {
            if(option / LANG_MAXPERCOL == (option - 1) / LANG_MAXPERCOL) {
                if(option > 0)
                    change_option(option - 1);
            }
        }
        if(input_button_pressed(input, IB_LEFT)) {
            if(option - LANG_MAXPERCOL >= 0)
                change_option(option - LANG_MAXPERCOL);
        }
        if(input_button_pressed(input, IB_RIGHT)) {
            if(option + LANG_MAXPERCOL < lngcount)
                change_option(option + LANG_MAXPERCOL);
        }
        if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
            const char *filepath = lngdata[option].filepath;
            logfile_message("Loading language \"%s\", \"%s\"", lngdata[option].name, filepath);
            lang_loadfile(filepath);
            save_preferences(filepath);
            sound_play(SFX_CONFIRM);
            quit = true;
        }
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play(SFX_BACK);
            quit = true;
        }
    }

    /* sliding camera */
    dest_x = font_get_position(lngfnt[0][option]).x +
             font_get_textsize(lngfnt[0][(option / LANG_MAXPERCOL) * LANG_MAXPERCOL]).x / 2;
    sliding_camera.x = lerp(sliding_camera.x, dest_x, option_time / 0.33f);

    /* author label */
    font_set_text(author_label, "<color=$COLOR_HIGHLIGHT>Translation by:</color> %s", lngdata[option].author);
    font_set_position(author_label, v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H - font_get_textsize(author_label).y - 5));
    font_set_align(author_label, FONTALIGN_CENTER);

    /* music */
    if(!music_is_playing() && !fresh_install)
        music_play(music, true);

    /* quit */
    if(quit) {
        if(fadefx_is_over()) {
            scenestack_pop();
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0f);
    }
}



/*
 * langselect_render()
 * Renders the scene
 */
void langselect_render()
{
    int i;
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    font_render(title, cam);
    font_render(author_label, cam);

    for(i=0; i<lngcount; i++)
        font_render(lngfnt[option==i ? 1 : 0][i], sliding_camera);

    actor_render(arrow, sliding_camera);
}






/* private methods */

/* change the current option */
int change_option(int new_option)
{
    if(new_option >= 0 && new_option < lngcount) {
        option_time = 0.0f;
        sound_play(SFX_CHOOSE);
        option = new_option;
    }

    return option;
}

/* returns k such that lngdata[k].name is language_name,
   or 0 if there is no such k */
int option_of(const char *language_name)
{
    for(int k = 0; k < lngcount; k++) {
        if(str_icmp(language_name, lngdata[k].name) == 0)
            return k;
    }

    return 0;
}

/* saves the user preferences */
void save_preferences(const char *filepath)
{
    extern prefs_t* prefs;
    prefs_set_string(prefs, ".langpath", filepath);
}

/* reads the language list from the languages/ folder */
void load_lang_list()
{
    int i, c = 0;
    logfile_message("load_lang_list()");

    /* loading language data */
    lngcount = 0;
    asset_foreach_file("languages", ".lng", dircount, (void*)&lngcount, false);

    /* fatal error */
    if(lngcount == 0)
        fatal_error("FATAL ERROR: no language files were found! Please reinstall the game.");
    else
        logfile_message("%d languages found.", lngcount);

    /* grabbing language data */
    lngdata = mallocx(lngcount * sizeof(lngdata_t));
    asset_foreach_file("languages", ".lng", dirfill, (void*)&c, false);
    qsort(lngdata, lngcount, sizeof(lngdata_t), sort_cmp);

    /* other stuff */
    lngfnt[0] = mallocx(lngcount * sizeof(font_t*));
    lngfnt[1] = mallocx(lngcount * sizeof(font_t*));
    for(i=0; i<lngcount; i++) {
        int col = i / LANG_MAXPERCOL, row = i % LANG_MAXPERCOL;
        lngfnt[0][i] = font_create("MenuText");
        lngfnt[1][i] = font_create("MenuText");
        font_set_text(lngfnt[0][i], "%s", lngdata[i].name);
        font_set_text(lngfnt[1][i], "<color=$COLOR_HIGHLIGHT>%s</color>", lngdata[i].name);
        font_set_position(lngfnt[0][i], v2d_new(25 + col * column_width, 88 + row * 1.35f * font_get_textsize(lngfnt[0][i]).y));
        font_set_position(lngfnt[1][i], v2d_new(25 + col * column_width, 88 + row * 1.35f * font_get_textsize(lngfnt[1][i]).y));
    }
}

int dirfill(const char *filename, void *param)
{
    int *c = (int*)param;
    int supver, subver, wipver;

    lang_compatibility(filename, &supver, &subver, &wipver);
    if(game_version_compare(supver, subver, wipver) >= 0) {
        str_cpy(lngdata[*c].filepath, filename, sizeof(lngdata[*c].filepath));
        lang_metadata(filename, "LANG_NAME", lngdata[*c].name, sizeof( lngdata[*c].name ));
        lang_metadata(filename, "LANG_AUTHOR", lngdata[*c].author, sizeof( lngdata[*c].author ));
        (*c)++;
    }
    if(game_version_compare(supver, subver, wipver) != 0)
        logfile_message("Warning: language file \"%s\" (compatibility: %d.%d.%d) may not be fully compatible with this version of the engine (%s)", filename, supver, subver, wipver, GAME_VERSION_STRING);

    return 0;
}

int dircount(const char *filename, void *param)
{
    int *lngcount = (int*)param;
    int supver, subver, wipver;

    lang_compatibility(filename, &supver, &subver, &wipver);
    if(game_version_compare(supver, subver, wipver) >= 0)
        (*lngcount)++;

    return 0;
}

/* unloads the language list */
void unload_lang_list()
{
    int i;

    logfile_message("unload_lang_list()");

    for(i=0; i<lngcount; i++) {
        font_destroy(lngfnt[0][i]);
        font_destroy(lngfnt[1][i]);
    }

    free(lngfnt[0]);
    free(lngfnt[1]);
    free(lngdata);
    lngcount = 0;
}


/* comparator */
int sort_cmp(const void *a, const void *b)
{
    lngdata_t *q[2] = { (lngdata_t*)a, (lngdata_t*)b };
    if(str_icmp(q[0]->name, "English") == 0) return -1;
    if(str_icmp(q[1]->name, "English") == 0) return 1;
    return str_icmp(q[0]->name, q[1]->name);
}
