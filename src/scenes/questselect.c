/*
 * Open Surge Engine
 * questselect.c - quest selection screen
 * Copyright (C) 2010, 2013  Alexandre Martins <alemartf@gmail.com>
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
#include "questselect.h"
#include "options.h"
#include "quest.h"
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
#include "../core/quest.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/player.h"
#include "../entities/sfx.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"



/* private data */
#define QUEST_BGFILE             "themes/scenes/questselect.bg"
#define QUEST_MAXPERPAGE         (VIDEO_SCREEN_H / 48)
static font_t *title; /* title */
static font_t *msg; /* message */
static font_t *page; /* page number */
static font_t *info; /* quest info */
static actor_t *icon; /* cursor icon */
static input_t *input; /* input device */
static float scene_time; /* scene time, in seconds */
static bgtheme_t *bgtheme; /* background */
static music_t *music; /* background music */

static enum { QUESTSTATE_NORMAL, QUESTSTATE_QUIT, QUESTSTATE_PLAY, QUESTSTATE_FADEIN } state; /* finite state machine */
char quest_to_be_loaded[1024] = "";

static quest_t **quest_data; /* vector of quest_t* */
static int quest_count; /* length of quest_data[] */
static int option; /* current option: 0 .. quest_count - 1 */
static font_t **quest_label; /* vector of font_t* */



/* private functions */
static void load_quest_list();
static void unload_quest_list();
static int dirfill(const char *vpath, void *param);
static int dircount(const char *vpath, void *param);
static int sort_cmp(const void *a, const void *b);






/* public functions */

/*
 * questselect_init()
 * Initializes the scene
 */
void questselect_init(void *foo)
{
    option = 0;
    scene_time = 0;
    state = QUESTSTATE_NORMAL;
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);

    title = font_create("MenuTitle");
    font_set_text(title, "%s", "$QUESTSELECT_TITLE");
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 10));
    font_set_align(title, FONTALIGN_CENTER);

    msg = font_create("MenuText");
    font_set_text(msg, "%s", "$QUESTSELECT_MSG");
    font_set_position(msg, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(msg).y * 1.5f));

    page = font_create("MenuText");
    font_set_textarguments(page, 2, "0", "0");
    font_set_text(page, "%s", "$QUESTSELECT_PAGE");
    font_set_position(page, v2d_new(VIDEO_SCREEN_W - font_get_textsize(page).x - 10, VIDEO_SCREEN_H - font_get_textsize(page).y * 1.5f));

    info = font_create("MenuText");
    font_set_position(info, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(info).y * 5.0f));
    font_set_textarguments(info, 3, "null", "null", "null");
    font_set_text(info, "%s", "$QUESTSELECT_INFO");

    bgtheme = background_load(QUEST_BGFILE);

    icon = actor_create();
    actor_change_animation(icon, sprite_get_animation("UI Pointer", 0));

    load_quest_list();
    fadefx_in(color_rgb(0,0,0), 1.0);
}


/*
 * questselect_release()
 * Releases the scene
 */
void questselect_release()
{
    unload_quest_list();
    actor_destroy(icon);
    bgtheme = background_unload(bgtheme);

    font_destroy(info);
    font_destroy(page);
    font_destroy(msg);
    font_destroy(title);

    input_destroy(input);
    music_unref(music);
}


/*
 * questselect_update()
 * Updates the scene
 */
void questselect_update()
{
    int i, first, last;
    int pagenum, maxpages;
    float dt = timer_get_delta();
    char pagestr[2][33];
    scene_time += dt;

    /* background movement */
    background_update(bgtheme);

    /* menu option */
    icon->position = font_get_position(quest_label[option]);
    icon->position.x += -20 + 3*cos(2*PI * scene_time);

    /* quest names */
    first = option / QUEST_MAXPERPAGE;
    last = min(option / QUEST_MAXPERPAGE + QUEST_MAXPERPAGE, quest_count) - 1;
    for(i = first; i <= last; i++) {
        if(option == i)
            font_set_text(quest_label[i], "<color=$COLOR_HIGHLIGHT>%s</color>", quest_data[i]->name);
        else
            font_set_text(quest_label[i], "%s", quest_data[i]->name);
    }

    /* page number */
    pagenum = option/QUEST_MAXPERPAGE + 1;
    maxpages = quest_count/QUEST_MAXPERPAGE + ((quest_count%QUEST_MAXPERPAGE == 0) ? 0 : 1);
    str_from_int(pagenum, pagestr[0], sizeof(pagestr[0]));
    str_from_int(maxpages, pagestr[1], sizeof(pagestr[1]));
    font_set_textarguments(page, 2, pagestr[0], pagestr[1]);
    font_set_text(page, "%s", "$QUESTSELECT_PAGE");
    font_set_position(page, v2d_new(VIDEO_SCREEN_W - font_get_textsize(page).x - 10, font_get_position(page).y));

    /* quest information */
    font_set_textarguments(info, 3, quest_data[option]->version, quest_data[option]->author, quest_data[option]->description);
    font_set_text(info, "%s", "$QUESTSELECT_INFO");

    /* music */
    if(!music_is_playing() && state != QUESTSTATE_PLAY)
        music_play(music, true);

    /* finite state machine */
    switch(state) {
        /* normal mode (menu) */
        case QUESTSTATE_NORMAL: {
            if(!fadefx_is_fading()) {
                if(input_button_pressed(input, IB_DOWN)) {
                    option = (option+1) % quest_count;
                    sound_play(SFX_CHOOSE);
                }
                if(input_button_pressed(input, IB_UP)) {
                    option = (((option-1) % quest_count) + quest_count) % quest_count;
                    sound_play(SFX_CHOOSE);
                }
                if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                    str_cpy(quest_to_be_loaded, quest_data[option]->file, sizeof(quest_to_be_loaded));
                    sound_play(SFX_CONFIRM);
                    state = QUESTSTATE_PLAY;
                    music_stop();
                }
                if(input_button_pressed(input, IB_FIRE4)) {
                    sound_play(SFX_BACK);
                    state = QUESTSTATE_QUIT;
                }
            }
            break;
        }

        /* fade-out effect (quit this screen) */
        case QUESTSTATE_QUIT: {
            if(fadefx_is_over()) {
                scenestack_pop();
                return;
            }
            fadefx_out(color_rgb(0,0,0), 1.0);
            break;
        }

        /* fade-out effect (play a level) */
        case QUESTSTATE_PLAY: {
            if(fadefx_is_over()) {
                /* scripting: reset global variables & arrays */
                symboltable_clear(symboltable_get_global_table());
                nanocalc_addons_resetarrays();

                /* reset lives & score */
                player_set_lives(PLAYER_INITIAL_LIVES);
                player_set_score(0);

                /* push the quest scene */
                scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)quest_to_be_loaded);
                state = QUESTSTATE_FADEIN;
                return;
            }
            fadefx_out(color_rgb(0,0,0), 1.0);
            break;
        }

        /* fade-in effect (after playing a level) */
        case QUESTSTATE_FADEIN: {
            fadefx_in(color_rgb(0,0,0), 1.0);
            state = QUESTSTATE_NORMAL;
            break;
        }
    }
}



/*
 * questselect_render()
 * Renders the scene
 */
void questselect_render()
{
    int i, first, last;
    image_t *thumbnail;
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    thumbnail = quest_data[option]->image;
    image_blit(thumbnail, 0, 0, VIDEO_SCREEN_W - image_width(thumbnail) - 10, 60, image_width(thumbnail), image_height(thumbnail)); 

    font_render(title, cam);
    font_render(msg, cam);
    font_render(page, cam);
    font_render(info, cam);

    first = option / QUEST_MAXPERPAGE;
    last = min(option / QUEST_MAXPERPAGE + QUEST_MAXPERPAGE, quest_count) - 1;
    for(i = first; i <= last; i++)
        font_render(quest_label[i], cam);

    actor_render(icon, cam);
}






/* private methods */

/* loads the quest list from the quests/ folder */
void load_quest_list()
{
    int i, c = 0;

    video_display_loading_screen();
    logfile_message("load_quest_list()");

    /* loading data */
    quest_count = 0;
    assetfs_foreach_file("quests", ".qst", dircount, NULL, true);
    quest_data = mallocx(quest_count * sizeof(quest_t*));
    assetfs_foreach_file("quests", ".qst", dirfill, (void*)(&c), true);
    qsort(quest_data, quest_count, sizeof(quest_t*), sort_cmp);

    /* fatal error */
    if(quest_count == 0)
        fatal_error("FATAL ERROR: no quest files were found! Please reinstall the game.");
    else
        logfile_message("%d quests found.", quest_count);

    /* other stuff */
    quest_label = mallocx(quest_count * sizeof(font_t**));
    for(i=0; i<quest_count; i++) {
        quest_label[i] = font_create("MenuText");
        font_set_position(quest_label[i], v2d_new(25, 60 + 20 * (i % QUEST_MAXPERPAGE)));
    }
}


/* unloads the quest list */
void unload_quest_list()
{
    int i;

    logfile_message("unload_quest_list()");

    for(i=0; i<quest_count; i++) {
        font_destroy(quest_label[i]);
        quest_data[i] = quest_unload(quest_data[i]);
    }

    free(quest_label);
    free(quest_data);
    quest_count = 0;
}


/* callback that fills quest_data[] */
int dirfill(const char *vpath, void *param)
{
    quest_t* s = quest_load(vpath);
    int *c = (int*)param;

    if(s != NULL) {
        if(!s->is_hidden && s->level_count > 0)
            quest_data[ (*c)++ ] = s;
        else
            quest_unload(s);
    }

    return 0;
}

/* callback that counts how many levels are installed */
int dircount(const char *vpath, void *param)
{
    quest_t* s = quest_load(vpath);

    if(s != NULL) {
        if(!s->is_hidden && s->level_count > 0)
            quest_count++;
        quest_unload(s);
    }

    return 0;
}


/* comparator */
int sort_cmp(const void *a, const void *b)
{
    quest_t *s[2] = { *((quest_t**)a), *((quest_t**)b) };
    int r = str_icmp(s[0]->name, s[1]->name);
    return (r == 0) ? str_icmp(s[0]->version, s[1]->version) : r;
}
