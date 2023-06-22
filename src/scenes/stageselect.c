/*
 * Open Surge Engine
 * stageselect.c - stage selection screen
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
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include "stageselect.h"
#include "util/levparser.h"
#include "options.h"
#include "level.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/v2d.h"
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
#include "../util/numeric.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/player.h"
#include "../entities/mobilegamepad.h"
#include "../entities/sfx.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"



/* stage data */
typedef struct {
    char* filepath; /* relative path */
    char name[256]; /* stage name */
    int act; /* zone number */
    bool is_quest; /* is this entry a quest file (.qst)? */
} stagedata_t;

static stagedata_t* stagedata_load(const char *filename, bool is_quest);
static void stagedata_unload(stagedata_t *s);
static bool interpret_level_line(const char *filepath, int fileline, const char *identifier, int param_count, const char** param, void* data);



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
static const stagedata_t* selected_stage; /* the level or quest we're going to load (NULL if none) */



/* private functions */
static void load_stage_list();
static void unload_stage_list();
static int dirfill(const char *vpath, void *param);
static int sort_cmp(const void *a, const void *b);
static int debug_sort_cmp(const void *a, const void *b);
static int load_selection();
static void save_selection(int option);






/* public functions */

/*
 * stageselect_init()
 * Initializes the scene
 */
void stageselect_init(void *should_enable_debug)
{
    enable_debug = (should_enable_debug != NULL) && (*((bool*)should_enable_debug));
    load_stage_list();

    scene_time = 0;
    option = load_selection();
    state = STAGESTATE_NORMAL;
    input = input_create_user(NULL);
    selected_stage = NULL;
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
    selected_stage = NULL;
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

    /* display the mobile gamepad */
    mobilegamepad_fadein();

    /* menu option */
    icon->position = font_get_position(stage_label[option]);
    icon->position.x += -20 + 3 * cosf(TWO_PI * scene_time);

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
                /* next */
                if(input_button_pressed(input, IB_DOWN)) {
                    option = (option + 1) % stage_count;
                    sound_play(SFX_CHOOSE);
                }

                /* previous */
                if(input_button_pressed(input, IB_UP)) {
                    option = (option + (stage_count - 1)) % stage_count;
                    sound_play(SFX_CHOOSE);
                }

                /* back */
                if(input_button_pressed(input, IB_FIRE4)) {
                    sound_play(SFX_BACK);
                    state = STAGESTATE_QUIT;
                }

                /* select */
                if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                    selected_stage = stage_data[option];
                    logfile_message(
                        "Loading %s \"%s\"...",
                        selected_stage->is_quest ? "quest" : "level",
                        selected_stage->filepath
                    );
                    save_selection(option);
                    sound_play(SFX_CONFIRM);
                    state = STAGESTATE_PLAY;
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

                /* push the next scene */
                if(selected_stage->is_quest) {

                    /* selected_stage->filepath is a .qst file */
                    scenestack_push(storyboard_get_scene(SCENE_QUEST), selected_stage->filepath);

                }
                else {

                    /* selected_stage->filepath is a .lev file */

                    /* Let's open it as a quest anyway. During gameplay, the top-most quest
                       may be aborted for any reason. If this happens, we don't want this
                       fact to affect any previously loaded quests. */

                    /* open the .lev file as a quest containing a single level */
                    scenestack_push(storyboard_get_scene(SCENE_QUEST), selected_stage->filepath);

                }

                /* done! */
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
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    font_render(title, cam);
    font_render(msg, cam);
    font_render(page, cam);

    for(int i = 0; i < stage_count; i++) {
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
    video_display_loading_screen();
    logfile_message("load_stage_list()");

    /* loading data */
    stage_count = 0;
    asset_foreach_file("levels", ".lev", dirfill, "L", enable_debug);
    if(enable_debug) {
        asset_foreach_file("quests", ".qst", dirfill, "Q", true);
        qsort(stage_data, stage_count, sizeof(stagedata_t*), debug_sort_cmp);
    }
    else
        qsort(stage_data, stage_count, sizeof(stagedata_t*), sort_cmp);

    /* fatal error */
    if(stage_count == 0)
        fatal_error("FATAL ERROR: no level files were found! Please reinstall the game.");
    else
        logfile_message("%d levels found.", stage_count);

    /* other stuff */
    stage_label = mallocx(stage_count * sizeof(font_t**));
    for(int i = 0; i < stage_count; i++) {
        stage_label[i] = font_create("MenuText");
        font_set_position(stage_label[i], v2d_new(25, 50 + 20 * (i % STAGE_MAXPERPAGE)));
    }
}



/* unloads the stage list */
void unload_stage_list()
{
    logfile_message("unload_stage_list()");

    for(int i = 0; i < stage_count; i++) {
        font_destroy(stage_label[i]);
        stagedata_unload(stage_data[i]);
    }

    free(stage_label);
    stage_count = 0;
}


/* callback that fills stage_data[] */
int dirfill(const char *vpath, void *param)
{
    /* can't have more than STAGE_MAX levels installed */
    if(stage_count >= STAGE_MAX)
        return 0;

    /* read level data */
    bool is_quest = (*((const char*)param) == 'Q');
    stagedata_t* s = stagedata_load(vpath, is_quest);
    if(s != NULL)
        stage_data[ stage_count++ ] = s;

    /* done! */
    return 0;
}

/* comparator */
int sort_cmp(const void *a, const void *b)
{
    stagedata_t *s[2] = { *((stagedata_t**)a), *((stagedata_t**)b) };
    int r = str_icmp(s[0]->name, s[1]->name);
    return (r == 0) ? (s[0]->act - s[1]->act) : r;
}

/* debug mode comparator */
int debug_sort_cmp(const void *a, const void *b)
{
    stagedata_t *s[2] = { *((stagedata_t**)a), *((stagedata_t**)b) };

    if(s[0]->is_quest != s[1]->is_quest)
        return (int)(!!s[0]->is_quest) - (int)(!!s[1]->is_quest);

    const char *p = strchr(s[0]->name, '/'), *q = strchr(s[1]->name, '/');
    int r = ((!p && !q) || (p && q)) ? str_icmp(s[0]->name, s[1]->name) : (!p && q ? -1 : 1);
    return (r == 0) ? (s[0]->act - s[1]->act) : r;
}


/* stagedata_t constructor. Returns NULL if filename is not a valid entry. */
stagedata_t* stagedata_load(const char *filename, bool is_quest)
{
    stagedata_t* s = mallocx(sizeof *s);

    /* initialize the fields */
    str_cpy(s->name, "Untitled", sizeof(s->name));
    s->act = 0;
    s->filepath = str_normalize_slashes(str_dup(filename));
    s->is_quest = is_quest;

    /* fill in the fields */
    if(enable_debug) {
        /* create a name based on the filepath */
        const int PREFIX_LENGTH = 7; /* == strlen("levels/") */
        bool skip_prefix = (0 == str_incmp(s->filepath, "levels/", PREFIX_LENGTH));
        snprintf(s->name, sizeof(s->name), "%s", s->filepath + (skip_prefix ? PREFIX_LENGTH : 0));
    }
    else if(!is_quest) {
        /* read the .lev file */
        if(!levparser_parse(s->filepath, s, interpret_level_line)) {
            logfile_message("Level select: can't parse level file \"%s\"", s->filepath);
            stagedata_unload(s);
            return NULL;
        }
    }

    /* done! */
    return s;
}

/* stagedata_t destructor */
void stagedata_unload(stagedata_t *s)
{
    free(s->filepath);
    free(s);
}

/* read a line of the .lev file */
bool interpret_level_line(const char *filepath, int fileline, const char *identifier, int param_count, const char** param, void* data)
{
    stagedata_t* s = (stagedata_t*)data;

    if(param_count == 0)
        return true;

    if(strcmp(identifier, "name") == 0)
        str_cpy(s->name, param[0], sizeof(s->name));
    else if(strcmp(identifier, "act") == 0)
        s->act = atoi(param[0]);
    else if(strcmp(identifier, "brick") == 0 || strcmp(identifier, "entity") == 0) /* optimization */
        return false; /* stop the enumeration */

    return true;
}

/* load a level that was previously selected by the user */
int load_selection()
{
    extern prefs_t* prefs;
    const char* last_selection;

    /* first run? */
    if(!prefs_has_item(prefs, STAGE_PREFSENTRY))
        return 0; /* not found; pick the 1st entry */

    /* find i such that stage_data[i]->filepath == last_selection */
    last_selection = prefs_get_string(prefs, STAGE_PREFSENTRY);
    for(int i = 0; i < stage_count; i++) {
        if(strcmp(last_selection, stage_data[i]->filepath) == 0)
            return i;
    }

    /* not found */
    return 0;
}

/* save a level selected by the user */
void save_selection(int option)
{
    extern prefs_t* prefs;

    if(option >= 0 && option < stage_count)
        prefs_set_string(prefs, STAGE_PREFSENTRY, stage_data[option]->filepath);
}