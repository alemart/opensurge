/*
 * Open Surge Engine
 * engine.c - game engine facade
 * Copyright (C) 2010-2011, 2018, 2020-2022  Alexandre Martins <alemartf@gmail.com>
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

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include "engine.h"
#include "global.h"
#include "scene.h"
#include "storyboard.h"
#include "util.h"
#include "assetfs.h"
#include "resourcemanager.h"
#include "stringutil.h"
#include "logfile.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "timer.h"
#include "fadefx.h"
#include "sprite.h"
#include "lang.h"
#include "screenshot.h"
#include "modmanager.h"
#include "prefs.h"
#include "commandline.h"
#include "font.h"
#include "nanoparser/nanoparser.h"
#include "../entities/legacy/enemy.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"
#include "../entities/legacy/nanocalc/nanocalcext.h"
#include "../entities/actor.h"
#include "../entities/player.h"
#include "../entities/character.h"
#include "../scripting/scripting.h"
#include "../scenes/quest.h"
#include "../scenes/level.h"

/* minimum Allegro version */
#define AL_MIN_MAJOR       5
#define AL_MIN_MINOR       2
#define AL_MIN_REVISION    7
#if ALLEGRO_VERSION_INT < ((AL_MIN_MAJOR << 24) | (AL_MIN_MINOR << 16) | (AL_MIN_REVISION << 8))
#error "This build requires a newer version of Allegro"
#endif

/* private stuff ;) */
static void clean_garbage();
static void init_basic_stuff(const commandline_t* cmd);
static void init_managers(const commandline_t* cmd);
static void init_accessories(const commandline_t* cmd);
static void init_game_data();
static void push_initial_scene(const commandline_t* cmd);
static void release_accessories();
static void release_managers();
static void release_basic_stuff();
static void init_nanoparser();
static void release_nanoparser();
static void parser_error(const char *msg);
static void parser_warning(const char *msg);
static void calc_error(const char *msg);
static const char* a5_version_string();
static const char* INTRO_QUEST = "quests/intro.qst";
static const char* SSAPP_LEVEL = "levels/surgescript.lev";
static const double TARGET_FPS = 60.0; /* frames per second */
static const uint32_t GC_INTERVAL = 10000; /* in ms (garbage collector) */
static bool force_quit = false;

/* Allegro 5 events */
ALLEGRO_EVENT_QUEUE* a5_event_queue = NULL; /* public */
extern void a5_handle_keyboard_event(const ALLEGRO_EVENT* event);
extern void a5_handle_mouse_event(const ALLEGRO_EVENT* event);
extern void a5_handle_joystick_event(const ALLEGRO_EVENT* event);



/* public functions */

/*
 * engine_init()
 * Initializes all the subsystems of
 * the game engine
 */
void engine_init(int argc, char **argv)
{
    commandline_t cmd = commandline_parse(argc, argv);
    init_basic_stuff(&cmd);
    init_managers(&cmd);
    init_accessories(&cmd);
    init_game_data();
    push_initial_scene(&cmd);
}


/*
 * engine_mainloop()
 * Game loop
 */
void engine_mainloop()
{
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / TARGET_FPS);
    scene_t *current_scene = NULL;
    bool redraw = false;

    /* configure the timer */
    if(!timer)
        fatal_error("Can't create Allegro timer");
    al_register_event_source(a5_event_queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    /* main loop */
    while(!force_quit && !scenestack_empty()) {
        ALLEGRO_EVENT event;
        al_wait_for_event(a5_event_queue, &event);

        /* handle events */
        switch(event.type) {
            case ALLEGRO_EVENT_TIMER: {
                /* update game logic */
                ALLEGRO_EVENT next_event;

                /* updating the managers */
                timer_update();
                input_update();
                audio_update();
                clean_garbage();

                /* updating the current scene */
                current_scene = scenestack_top();
                current_scene->update();
                redraw = (current_scene == scenestack_top()); /* same scene? */

                /* prevent locking */
                while(al_peek_next_event(a5_event_queue, &next_event) && next_event.type == ALLEGRO_EVENT_TIMER && next_event.timer.source == event.timer.source)
                    al_drop_next_event(a5_event_queue);
                break;
            }

            case ALLEGRO_EVENT_KEY_DOWN:
            case ALLEGRO_EVENT_KEY_UP:
                a5_handle_keyboard_event(&event);
                break;

            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                a5_handle_mouse_event(&event);
                break;

            case ALLEGRO_EVENT_JOYSTICK_AXIS:
            case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
            case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
                a5_handle_joystick_event(&event);
                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                engine_quit();
                break;
        }

        /* render */
        if(redraw && al_is_event_queue_empty(a5_event_queue)) {
            current_scene->render();
            screenshot_update();
            fadefx_update();
            video_render();
            redraw = false;
        }
    }

    /* done */
    al_destroy_timer(timer);
}


/*
 * engine_release()
 * Releases the game engine and its
 * subsystems
 */
void engine_release()
{
    release_accessories();
    release_managers();
    release_basic_stuff();
}


/*
 * engine_quit()
 * Quit the application
 */
void engine_quit()
{
    force_quit = true;
}



/* private functions */

/*
 * clean_garbage()
 * Runs the garbage collector.
 */
void clean_garbage()
{
    static uint32_t last = 0;
    uint32_t now = timer_get_ticks();

    /* run the GC every GC_INTERVAL milliseconds (approximately) */
    if(now >= last + GC_INTERVAL) {
        last = now;
        resourcemanager_release_unused_resources();
    }
    else if(now < last)
        last = now; /* time overflow... really?! */
}


/*
 * init_basic_stuff()
 * Initializes the basic stuff, such as Allegro.
 * Call this before everything else.
 */
void init_basic_stuff(const commandline_t* cmd)
{
    const char* gameid = commandline_getstring(cmd->gameid, NULL);
    const char* basedir = commandline_getstring(cmd->basedir, NULL);
    const char* gamedir = commandline_getstring(cmd->gamedir, NULL);

    force_quit = false;
    srand(time(NULL));
    assetfs_init(gameid, basedir, gamedir);
    logfile_init();
    init_nanoparser();

    /* show SurgeScript version */
    logfile_message("Using SurgeScript version %s", surgescript_util_version());

    /* initialize Allegro */
    if(al_init())
        logfile_message("Using Allegro version %s", a5_version_string());
    else
        fatal_error("Can't initialize Allegro");

    if(NULL == (a5_event_queue = al_create_event_queue()))
        fatal_error("Can't create Allegro's event queue");

    if(!al_init_native_dialog_addon())
        fatal_error("Can't initialize Allegro's native dialog addon");
}


/*
 * init_managers()
 * Initializes the managers
 */
void init_managers(const commandline_t* cmd)
{
    prefs_t* prefs;

    modmanager_init();
    prefs = modmanager_prefs();

    timer_init();
    video_init(
        commandline_getint(
            cmd->video_resolution,
            prefs_has_item(prefs, ".resolution") ?
                prefs_get_int(prefs, ".resolution") : video_initial_resolution()
        ),
        commandline_getint(cmd->smooth_graphics, prefs_get_bool(prefs, ".smoothgfx")),
        commandline_getint(cmd->fullscreen, prefs_get_bool(prefs, ".fullscreen")),
        commandline_getint(cmd->color_depth, video_get_preferred_color_depth())
    );
    video_show_fps(
        commandline_getint(cmd->show_fps, prefs_get_bool(prefs, ".showfps")) &&
        !commandline_getint(cmd->hide_fps, FALSE)
    );
    audio_init();
    input_init();
    resourcemanager_init();
}


/*
 * init_accessories()
 * Initializes the accessories
 */
void init_accessories(const commandline_t* cmd)
{
    prefs_t* prefs = modmanager_prefs();
    const char* custom_lang = commandline_getstring(cmd->language_filepath,
        prefs_has_item(prefs, ".langpath") ? prefs_get_string(prefs, ".langpath") : NULL
    );

    setlocale(LC_ALL, "en_US.UTF-8"); /* work with UTF-8 */
    setlocale(LC_NUMERIC, "C"); /* use '.' as the decimal separator on atof() */

    lang_init();
    if(custom_lang && *custom_lang)
        lang_loadfile(custom_lang);
    font_init(commandline_getint(cmd->allow_font_smoothing, TRUE));
    video_display_loading_screen();

    sprite_init();
    charactersystem_init();
    objects_init();
    storyboard_init();
    screenshot_init();
    fadefx_init();
    scripting_init(cmd->user_argc, cmd->user_argv);
    
    scenestack_init();
}

/*
 * init_game_data()
 * Initializes the game data
 */
void init_game_data()
{
    player_set_lives(PLAYER_INITIAL_LIVES);
    player_set_score(0);
}


/*
 * push_initial_scene()
 * Decides which scene should be pushed into the scene stack
 */
void push_initial_scene(const commandline_t* cmd)
{
    int custom_level = (commandline_getstring(cmd->custom_level_path, NULL) != NULL);
    int custom_quest = (commandline_getstring(cmd->custom_quest_path, NULL) != NULL);

    if(custom_level) {
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)(commandline_getstring(cmd->custom_level_path, "")));
    }
    else if(custom_quest) {
        scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)(commandline_getstring(cmd->custom_quest_path, "")));
        /*scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);*/
    }
    else if(scripting_testmode()) {
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)(SSAPP_LEVEL));
        /*scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);*/
    }
    else {
        scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)(INTRO_QUEST));
        if(!prefs_has_item(modmanager_prefs(), ".langpath"))
            scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);
        scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);
    }
}


/*
 * release_accessories()
 * Releases the previously loaded accessories
 */
void release_accessories()
{
    scenestack_release();
    storyboard_release();
    scripting_release();
    lang_release();
    fadefx_release();
    screenshot_release();
    objects_release();
    charactersystem_release();
    font_release();
    sprite_release();
}

/*
 * release_managers()
 * Releases the previously loaded managers
 */
void release_managers()
{
    modmanager_release();
    input_release();
    video_release();
    resourcemanager_release();
    audio_release();
    timer_release();
}


/*
 * release_basic_stuff()
 * Releases the basic stuff, such as Allegro.
 * Call this after everything else.
 */
void release_basic_stuff()
{
    release_nanoparser();
    logfile_release();
    assetfs_release();

    /* Release Allegro */
    al_destroy_event_queue(a5_event_queue);
    a5_event_queue = NULL;
}

/*
 * init_nanoparser()
 * Initializes nanoparser
 */
void init_nanoparser()
{
    /* nanoparser */
    nanoparser_set_error_function(parser_error);
    nanoparser_set_warning_function(parser_warning);

    /* nanocalc */
    nanocalc_init(); /* initializes a basic nanocalc */
    nanocalc_set_error_function(calc_error); /* error callback */
    nanocalc_addons_init(); /* adds some mathematical functions to nanocalc */
    nanocalcext_register_bifs(); /* more bindings */
}

/*
 * release_nanoparser()
 * Releases nanoparser
 */
void release_nanoparser()
{
    /* nanoparser */
    ; /* empty */

    /* nanocalc */
    nanocalc_addons_release();
    nanocalc_release();
}

/*
 * parser_error()
 * This is called by nanoparser when an error is raised
 */
void parser_error(const char *msg)
{
    fatal_error("%s", msg);
}

/*
 * parser_warning()
 * This is called by nanoparser when a warning is raised
 */
void parser_warning(const char *msg)
{
    logfile_message("WARNING: %s", msg);
}

/*
 * calc_error()
 * This is called by nanocalc when an error is raised
 */
void calc_error(const char *msg)
{
    fatal_error("%s", msg);
}

/*
 * a5_version_string()
 * Returns the Allegro version as a static char[]
 */
const char* a5_version_string()
{
    static char str[17];
    uint32_t version = al_get_allegro_version();

    snprintf(str, sizeof(str), "%u.%u.%u-%u",
        (version & 0xFF000000) >> 24,
        (version & 0xFF0000) >> 16,
        (version & 0xFF00) >> 8,
        (version & 0xFF)
    );

    return str;
}