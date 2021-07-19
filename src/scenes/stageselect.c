/*
 * Open Surge Engine
 * stageselect.c - stage selection screen
 * Copyright (C) 2010, 2012, 2019-2021  Alexandre Martins <alemartf@gmail.com>
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
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include "stageselect.h"
#include "options.h"
#include "level.h"
#include "../core/util.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/v2d.h"
#include "../core/assetfs.h"
#include "../core/stringutil.h"
#include "../core/logfile.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/nanoparser/nanoparser.h"
#include "../core/font.h"
#include "../core/modmanager.h"
#include "../core/prefs.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/player.h"
#include "../entities/sfx.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"



/* stage data */
typedef struct {
    char* filepath; /* relative path */
    char name[128]; /* stage name */
    int act; /* act number */
    int requires[3]; /* required version */
} stagedata_t;

static stagedata_t* stagedata_load(const char *filename);
static void stagedata_unload(stagedata_t *s);
static int traverse(const parsetree_statement_t *stmt, void *stagedata);



/* private data */
#define STAGE_BGFILE             "themes/scenes/levelselect.bg"
#define STAGE_MAXPERPAGE         (VIDEO_SCREEN_H / 30)
#define STAGE_MAX                2048 /* can't have more than STAGE_MAX levels installed */
#define STAGE_PREFSENTRY         ".lastselectedlevel"
static font_t *title; /* title */
static font_t *msg; /* message */
static font_t *page; /* page number */
static actor_t *icon; /* cursor icon */
static input_t *input; /* input device */
static float scene_time; /* scene time, in seconds */
static bgtheme_t *bgtheme; /* current background */
static enum { STAGESTATE_NORMAL, STAGESTATE_QUIT, STAGESTATE_PLAY, STAGESTATE_FADEIN } state; /* finite state machine */
static stagedata_t *stage_data[STAGE_MAX]; /* vector of stagedata_t* */
static int stage_count; /* length of stage_data[] */
static int option; /* current option: 0 .. stage_count - 1 */
static font_t **stage_label; /* vector */
static bool enable_debug; /* debug mode? must start out as false. */
static bool can_play_music; /* can play music? */
static music_t* music = NULL; /* background music */
static char* level_to_be_loaded;



/* private functions */
static void load_stage_list();
static void unload_stage_list();
static int dirfill(const char *vpath, void *param);
static int sort_cmp(const void *a, const void *b);
static int load_selection();
static void save_selection(int option);






/* public functions */

/*
 * stageselect_init()
 * Initializes the scene
 */
void stageselect_init(void *should_enable_debug)
{
    enable_debug = (*((int*)should_enable_debug) != 0);
    load_stage_list();

    scene_time = 0;
    option = enable_debug ? load_selection() : 0;
    state = STAGESTATE_NORMAL;
    input = input_create_user(NULL);
    level_to_be_loaded = NULL;
    music = music_load(OPTIONS_MUSICFILE);
    can_play_music = (!enable_debug || timer_get_ticks() >= 10000);

    title = font_create("MenuTitle");
    font_set_text(title, "%s", !enable_debug ? "$STAGESELECT_TITLE" : "$STAGESELECT_DEBUG");
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 10));
    font_set_align(title, FONTALIGN_CENTER);

    msg = font_create("MenuText");
    font_set_text(msg, "%s", "$STAGESELECT_MSG");
    font_set_position(msg, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(msg).y * 1.5f));

    page = font_create("MenuText");
    font_set_textarguments(page, 2, "0", "0");
    font_set_text(page, "%s", "$STAGESELECT_PAGE");
    font_set_position(page, v2d_new(VIDEO_SCREEN_W - font_get_textsize(page).x - 10, font_get_position(msg).y));

    icon = actor_create();
    actor_change_animation(icon, sprite_get_animation("UI Pointer", 0));

    bgtheme = background_load(STAGE_BGFILE);
    fadefx_in(color_rgb(0,0,0), 1.0);
}


/*
 * stageselect_release()
 * Releases the scene
 */
void stageselect_release()
{
    if(level_to_be_loaded != NULL) {
        free(level_to_be_loaded);
        level_to_be_loaded = NULL;
    }

    bgtheme = background_unload(bgtheme);
    unload_stage_list();

    actor_destroy(icon);
    font_destroy(title);
    font_destroy(msg);
    font_destroy(page);
    input_destroy(input);
    music_unref(music);
}


/*
 * stageselect_update()
 * Updates the scene
 */
void stageselect_update()
{
    int pagenum, maxpages;
    float dt = timer_get_delta();
    char pagestr[2][33];
    scene_time += dt;

    /* background movement */
    background_update(bgtheme);

    /* menu option */
    icon->position = font_get_position(stage_label[option]);
    icon->position.x += -20 + 3*cos(2*PI * scene_time);

    /* page number */
    pagenum = option/STAGE_MAXPERPAGE + 1;
    maxpages = stage_count/STAGE_MAXPERPAGE + ((stage_count%STAGE_MAXPERPAGE == 0) ? 0 : 1);
    str_from_int(pagenum, pagestr[0], sizeof(pagestr[0]));
    str_from_int(maxpages, pagestr[1], sizeof(pagestr[1]));
    font_set_textarguments(page, 2, pagestr[0], pagestr[1]);
    font_set_text(page, "%s", "$STAGESELECT_PAGE");
    font_set_position(page, v2d_new(VIDEO_SCREEN_W - font_get_textsize(page).x - 10, font_get_position(page).y));

    /* music */
    if(state == STAGESTATE_PLAY) {
        if(!fadefx_is_fading())
            music_stop();
    }
    else if(!music_is_playing()) {
        if(can_play_music)
            music_play(music, true);
    }

    /* finite state machine */
    switch(state) {
        /* normal mode (menu) */
        case STAGESTATE_NORMAL: {
            if(!fadefx_is_fading()) {
                if(input_button_pressed(input, IB_DOWN)) {
                    option = (option + 1) % stage_count;
                    sound_play(SFX_CHOOSE);
                }
                if(input_button_pressed(input, IB_UP)) {
                    option = (option + (stage_count - 1)) % stage_count;
                    sound_play(SFX_CHOOSE);
                }
                if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                    logfile_message("Loading level \"%s\" (\"%s\")...", stage_data[option]->name, stage_data[option]->filepath);

                    if(level_to_be_loaded != NULL)
                        free(level_to_be_loaded);
                    level_to_be_loaded = str_dup(stage_data[option]->filepath);

                    if(enable_debug)
                        save_selection(option);

                    sound_play(SFX_CONFIRM);
                    state = STAGESTATE_PLAY;
                }
                if(input_button_pressed(input, IB_FIRE4)) {
                    sound_play(SFX_BACK);
                    state = STAGESTATE_QUIT;
                }
            }
            break;
        }

        /* fade-out effect (quit this screen) */
        case STAGESTATE_QUIT: {
            if(fadefx_is_over()) {
                scenestack_pop();
                return;
            }
            fadefx_out(color_rgb(0,0,0), 1.0);
            break;
        }

        /* fade-out effect (play a level) */
        case STAGESTATE_PLAY: {
            if(fadefx_is_over()) {
                /* scripting: reset global variables & arrays */
                symboltable_clear(symboltable_get_global_table());
                nanocalc_addons_resetarrays();

                /* reset lives & score */
                player_set_lives(PLAYER_INITIAL_LIVES);
                player_set_score(0);

                /* push the level scene */
                scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)level_to_be_loaded);
                state = STAGESTATE_FADEIN;
                return;
            }
            fadefx_out(color_rgb(0,0,0), 1.0);
            break;
        }

        /* fade-in effect (after playing a level) */
        case STAGESTATE_FADEIN: {
            fadefx_in(color_rgb(0,0,0), 1.0);
            state = STAGESTATE_NORMAL;
            break;
        }
    }
}



/*
 * stageselect_render()
 * Renders the scene
 */
void stageselect_render()
{
    int i;
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    font_render(title, cam);
    font_render(msg, cam);
    font_render(page, cam);

    for(i=0; i<stage_count; i++) {
        if(i/STAGE_MAXPERPAGE == option/STAGE_MAXPERPAGE) {
            if(stage_data[i]->act > 0 && !enable_debug)
                font_set_text(stage_label[i], (option==i) ? "<color=$COLOR_HIGHLIGHT>%s - %s %d</color>" : "%s - %s %d", stage_data[i]->name, lang_get("STAGESELECT_ACT"), stage_data[i]->act);
            else
                font_set_text(stage_label[i], (option==i) ? "<color=$COLOR_HIGHLIGHT>%s</color>" : "%s", stage_data[i]->name);
            font_render(stage_label[i], cam);
        }
    }

    actor_render(icon, cam);
}



/* private methods */


/* loads the stage list from the level/ folder */
void load_stage_list()
{
    int i;

    video_display_loading_screen();
    logfile_message("load_stage_list()");

    /* loading data */
    stage_count = 0;
    assetfs_foreach_file("levels", ".lev", dirfill, NULL, enable_debug);
    qsort(stage_data, stage_count, sizeof(stagedata_t*), sort_cmp);

    /* fatal error */
    if(stage_count == 0)
        fatal_error("FATAL ERROR: no level files were found! Please reinstall the game.");
    else
        logfile_message("%d levels found.", stage_count);

    /* other stuff */
    stage_label = mallocx(stage_count * sizeof(font_t**));
    for(i=0; i<stage_count; i++) {
        stage_label[i] = font_create("MenuText");
        font_set_position(stage_label[i], v2d_new(25, 50 + 20 * (i % STAGE_MAXPERPAGE)));
    }
}



/* unloads the stage list */
void unload_stage_list()
{
    int i;

    logfile_message("unload_stage_list()");

    for(i=0; i<stage_count; i++) {
        font_destroy(stage_label[i]);
        stagedata_unload(stage_data[i]);
    }

    free(stage_label);
    stage_count = 0;
}


/* callback that fills stage_data[] */
int dirfill(const char *vpath, void *param)
{
    int supver, subver, wipver;
    stagedata_t *s;

    /* can't have more than STAGE_MAX levels installed */
    if(stage_count >= STAGE_MAX)
        return 0;

    s = stagedata_load(vpath);
    if(s != NULL) {
        supver = s->requires[0];
        subver = s->requires[1];
        wipver = s->requires[2];

        if(game_version_compare(supver, subver, wipver) >= 0) {
            stage_data[ stage_count++ ] = s;
            if(enable_debug) { /* debug mode: changing the names... */
                char *p = str_rstr(s->filepath, "levels/");
                if(!p) {
                    p = str_rstr(s->filepath, "levels\\");
                    if(!p)
                        snprintf(s->name, sizeof(s->name), "%s", s->filepath);
                    else
                        snprintf(s->name, sizeof(s->name), "%s", p + strlen("levels\\"));
                }
                else
                    snprintf(s->name, sizeof(s->name), "%s", p + strlen("levels/"));
            }
        }
        else {
            logfile_message("Warning: level \"%s\" isn't compatible with this version of the game (requires: %d.%d.%d).", vpath, supver, subver, wipver);
            stagedata_unload(s);
        }
    }

    return 0;
}

/* comparator */
int sort_cmp(const void *a, const void *b)
{
    stagedata_t *s[2] = { *((stagedata_t**)a), *((stagedata_t**)b) };
    const char *p = strchr(s[0]->name, '/'), *q = strchr(s[1]->name, '/');
    int r = ((!p && !q) || (p && q)) ? str_icmp(s[0]->name, s[1]->name) : (!p && q ? -1 : 1);
    return (r == 0) ? (s[0]->act - s[1]->act) : r;
}


/* stagedata_t constructor. Returns NULL if filename is not a level. */
stagedata_t* stagedata_load(const char *filename)
{
    parsetree_program_t *prog;
    stagedata_t* s = mallocx(sizeof *s);
    const char* fullpath = assetfs_fullpath(filename);
    char* p;

    s->filepath = str_dup(filename);
    while((p = strchr(s->filepath, '\\')))
        *p = '/'; /* replace '\\' by '/' */

    str_cpy(s->name, "Untitled", sizeof(s->name));
    s->act = 1;
    s->requires[0] = 0;
    s->requires[1] = 0;
    s->requires[2] = 0;

    prog = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program_ex(prog, (void*)s, traverse);
    prog = nanoparser_deconstruct_tree(prog);

    return s;
}

/* stagedata_t destructor */
void stagedata_unload(stagedata_t *s)
{
    free(s->filepath);
    free(s);
}

/* traverses a line of the level */
int traverse(const parsetree_statement_t *stmt, void *stagedata)
{
    stagedata_t *s = (stagedata_t*)stagedata;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const char *val = nanoparser_get_string(nanoparser_get_nth_parameter(param_list, 1));

    if(str_icmp(id, "name") == 0)
        str_cpy(s->name, val, sizeof(s->name));
    else if(str_icmp(id, "act") == 0)
        s->act = atoi(val);
    else if(str_icmp(id, "requires") == 0)
        sscanf(val, "%d.%d.%d", &(s->requires[0]), &(s->requires[1]), &(s->requires[2]));
    else if(str_icmp(id, "brick") == 0) /* optimization */
        return 1; /* stop the enumeration */

    return 0;
}

/* load a level that was previously selected by the user */
int load_selection()
{
    prefs_t* prefs = modmanager_prefs();
    const char* last_selection;
    int i;

    /* first run? */
    if(!prefs_has_item(prefs, STAGE_PREFSENTRY))
        return 0; /* not found; pick the 1st entry */

    /* find i such that stage_data[i]->filepath == last_selection */
    last_selection = prefs_get_string(prefs, STAGE_PREFSENTRY);
    for(i = 0; i < stage_count; i++) {
        if(strcmp(last_selection, stage_data[i]->filepath) == 0)
            return i;
    }

    /* not found */
    return 0;
}

/* save a level selected by the user */
void save_selection(int option)
{
    prefs_t* prefs = modmanager_prefs();

    if(option >= 0 && option < stage_count)
        prefs_set_string(prefs, STAGE_PREFSENTRY, stage_data[option]->filepath);
}