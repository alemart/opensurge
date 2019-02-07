/*
 * Open Surge Engine
 * engine.c - game engine facade
 * Copyright (C) 2010, 2011, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <allegro.h>
#include <string.h>
#include <locale.h>
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
#include "soundfactory.h"
#include "lang.h"
#include "screenshot.h"
#include "modmanager.h"
#include "prefs.h"
#include "commandline.h"
#include "font.h"
#include "fontext.h"
#include "nanoparser/nanoparser.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"
#include "../entities/legacy/nanocalc/nanocalcext.h"
#include "../entities/actor.h"
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "../entities/character.h"
#include "../scripting/scripting.h"
#include "../scenes/quest.h"
#include "../scenes/level.h"

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
static const char* get_window_title();
static void init_nanoparser();
static void release_nanoparser();
static void parser_error(const char *msg);
static void parser_warning(const char *msg);
static void calc_error(const char *msg);
static const char* INTRO_QUEST = "quests/intro.qst";
static const char* SSAPP_LEVEL = "levels/surgescript.lev";



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
 * A classic main loop
 */
void engine_mainloop()
{
    scene_t *scn;

    while(!game_is_over() && !scenestack_empty()) {
        /* updating the managers */
        timer_update();
        input_update();
        audio_update();

        /* current scene: logic & rendering */
        scn = scenestack_top();
        scn->update();
        if(scn == scenestack_top()) /* scn may have been 'popped' out */
            scn->render();

        /* more rendering */
        screenshot_update();
        fadefx_update();
        video_render();

        /* calling the garbage collector */
        clean_garbage();
    }
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



/* private functions */

/*
 * clean_garbage()
 * Runs the garbage collector.
 */
void clean_garbage()
{
    static uint32 last = 0;
    uint32 t = timer_get_ticks();

    if(t >= last + 2000) { /* every 2 seconds */
        last = t;
        resourcemanager_release_unused_resources();
    }
}


/*
 * init_basic_stuff()
 * Initializes the basic stuff, such as Allegro.
 * Call this before everything else.
 */
void init_basic_stuff(const commandline_t* cmd)
{
    const char* gameid = commandline_getstring(cmd->gameid, NULL);
    const char* datadir = commandline_getstring(cmd->datadir, NULL);

    set_uformat(U_UTF8);
    allegro_init();
    randomize();
    assetfs_init(gameid, datadir);
    logfile_init();
    init_nanoparser();
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

    timer_init(
        commandline_getint(cmd->optimize_cpu_usage, TRUE)
    );
    video_init(
        get_window_title(),
        commandline_getint(
            cmd->video_resolution,
            prefs_has_item(prefs, ".resolution") ? prefs_get_int(prefs, ".resolution") : VIDEORESOLUTION_2X
        ),
        commandline_getint(cmd->smooth_graphics, prefs_get_bool(prefs, ".smoothgfx")),
        commandline_getint(cmd->fullscreen, prefs_get_bool(prefs, ".fullscreen")),
        commandline_getint(cmd->color_depth, video_get_desktop_color_depth())
    );
    video_show_fps(
        commandline_getint(cmd->show_fps, prefs_get_bool(prefs, ".showfps"))
    );
    audio_init();
    input_init();
    input_ignore_joystick(
        !commandline_getint(cmd->use_gamepad, prefs_get_bool(prefs, ".gamepad"))
    );
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
    video_display_loading_screen();
    sprite_init();
    font_init(commandline_getint(cmd->allow_font_smoothing, TRUE));
    fontext_register_variables();
    soundfactory_init();
    charactersystem_init();
    objects_init();
    scripting_init(cmd->user_argc, cmd->user_argv);
    storyboard_init();
    screenshot_init();
    fadefx_init();
    lang_init();
    if(custom_lang && *custom_lang)
        lang_loadfile(custom_lang);
    
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
        scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);
    }
    else if(scripting_testmode()) {
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)(SSAPP_LEVEL));
        /*scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);*/
    }
    else {
        scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)(INTRO_QUEST));
        scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);
        if(!prefs_has_item(modmanager_prefs(), ".langpath"))
            scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);
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
    lang_release();
    fadefx_release();
    screenshot_release();
    scripting_release();
    objects_release();
    charactersystem_release();
    soundfactory_release();
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
    allegro_exit();
}


/*
 * get_window_title()
 * Returns the title of the window
 */
const char* get_window_title()
{
    return GAME_TITLE " " GAME_VERSION_STRING;
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
