/*
 * Open Surge Engine
 * engine.c - game engine facade
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include "engine.h"
#include "global.h"
#include "scene.h"
#include "storyboard.h"
#include "asset.h"
#include "resourcemanager.h"
#include "logfile.h"
#include "timer.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "font.h"
#include "sprite.h"
#include "lang.h"
#include "screenshot.h"
#include "fadefx.h"
#include "prefs.h"
#include "commandline.h"
#include "modutils.h"
#include "nanoparser.h"
#include "config.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/legacy/enemy.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"
#include "../entities/legacy/nanocalc/nanocalcext.h"
#include "../entities/actor.h"
#include "../entities/player.h"
#include "../entities/character.h"
#include "../entities/renderqueue.h"
#include "../entities/mobilegamepad.h"
#include "../scripting/scripting.h"
#include "../scripting/loaderthread.h"
#include "../scenes/quest.h"
#include "../scenes/level.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_native_dialog.h>

/* minimum Allegro version */
#if ALLEGRO_VERSION_INT < ALLEGRO_MIN_VERSION_INT
#error "This build requires a newer version of Allegro"
#endif

/* minimum SurgeScript version */
#if !SURGESCRIPT_VERSION_IS_AT_LEAST(SURGESCRIPT_MIN_SUP, SURGESCRIPT_MIN_SUB, SURGESCRIPT_MIN_WIP, 0)
#error "This build requires a newer version of SurgeScript"
#endif



/* Allegro 5 events & event listeners */
typedef struct event_listener_t event_listener_t;
struct event_listener_t {
    ALLEGRO_EVENT_TYPE event_type;
    void* data;
    void (*callback)(const ALLEGRO_EVENT*,void*);
};

typedef struct event_listener_list_t event_listener_list_t;
struct event_listener_list_t {
    event_listener_t event_listener;
    event_listener_list_t* next;
};

#define EVENT_LISTENER_TABLE_SIZE 64
static event_listener_list_t* event_listener_table[EVENT_LISTENER_TABLE_SIZE];

static void init_event_listener_table();
static void release_event_listener_table();
static void add_to_event_listener_table(event_listener_t event_listener);
static bool remove_from_event_listener_table(event_listener_t event_listener);
static void call_event_listeners(const ALLEGRO_EVENT* event);

static ALLEGRO_EVENT_QUEUE* a5_event_queue = NULL;
static void a5_handle_timer_event(const ALLEGRO_EVENT* event, void* data);
static void a5_handle_haltresume_event(const ALLEGRO_EVENT* event, void* data);
static void a5_handle_hotkey(const ALLEGRO_EVENT* event, void* data);

static void a5_handle_remaining_display_events();



/* private stuff ;) */
static void clean_garbage();
static void render_overlay();
static void init_basic_stuff(const commandline_t* cmd);
static void init_managers(const commandline_t* cmd);
static void load_managers_preferences(const commandline_t* cmd);
static void init_accessories(const commandline_t* cmd);
static void push_initial_scene(const commandline_t* cmd);
static void release_accessories();
static void release_managers();
static void release_basic_stuff();
static void init_nanocalc();
static void release_nanocalc();
static void parser_error(const char *msg, void* context);
static void parser_warning(const char *msg, void* context);
static void calc_error(const char *msg);
static void perform_extra_validation(const commandline_t* cmd);
static const char* INTRO_QUEST = "quests/intro.qst";
static const char* SSAPP_LEVEL = "levels/surgescript.lev";
static const double TARGET_FPS = 60.0; /* frames per second */
static const double GC_INTERVAL = 10.0; /* in seconds (garbage collector) */
static ALLEGRO_TIMER* a5_timer = NULL;
static bool wants_to_quit = false;
static bool wants_to_restart = false;
static bool is_initialized = false;
static commandline_t stored_cmd;

/* Global Prefs */
prefs_t* prefs = NULL; /* public */

/* MODs */
static int compatibility_version_code = 0;
static uint32_t game_id = GAME_ID_UNAVAILABLE;





/* public functions */

/*
 * engine_init()
 * Initializes the subsystems of the engine
 */
void engine_init(const commandline_t* cmd)
{
    /* initialize subsystems */
    init_basic_stuff(cmd);
    init_managers(cmd);
    init_accessories(cmd);

    is_initialized = true;

    /* initialize game data */
    player_set_lives(PLAYER_INITIAL_LIVES);
    player_set_score(0);
    push_initial_scene(cmd);

    /* initialize in immersive mode */
    video_set_immersive(true);

    /* perform extra validation */
    perform_extra_validation(cmd);
}



/*
 * engine_release()
 * Releases the engine and its subsystems
 */
void engine_release()
{
    is_initialized = false;

    release_accessories();
    release_managers();
    release_basic_stuff();
}



/*
 * engine_mainloop()
 * Game loop
 */
void engine_mainloop()
{
    ALLEGRO_EVENT event;
    bool can_draw = true;
    bool is_ready_to_draw = false;

    /* setup event listeners */
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_HALT_DRAWING, &can_draw, a5_handle_haltresume_event);
    engine_add_event_listener(ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING, &can_draw, a5_handle_haltresume_event);
    engine_add_event_listener(ALLEGRO_EVENT_KEY_DOWN, NULL, a5_handle_hotkey);

    /* initialize the timer */
    if(NULL == (a5_timer = al_create_timer(1.0 / TARGET_FPS)))
        fatal_error("Can't create an Allegro timer");

    engine_add_event_source(al_get_timer_event_source(a5_timer));
    engine_add_event_listener(ALLEGRO_EVENT_TIMER, &is_ready_to_draw, a5_handle_timer_event);
    al_start_timer(a5_timer);

    /* game loop */
    while(!wants_to_quit && !wants_to_restart && !scenestack_empty()) {
        const scene_t* scene = scenestack_top();

        /* handle events & update game logic */
        al_wait_for_event(a5_event_queue, &event);
        call_event_listeners(&event);

        /* skip rendering if the scene changed */
        scene_t* current_scene = scenestack_top();
        if(current_scene != scene)
            continue;

        /* render */
        if(can_draw && is_ready_to_draw && al_is_event_queue_empty(a5_event_queue)) {
            current_scene->render();
            fadefx_update();
            video_render(render_overlay);
            screenshot_update();
            is_ready_to_draw = false;
        }
    }

    /* release the timer */
    al_stop_timer(a5_timer);
    al_destroy_timer(a5_timer);

    /* cleanup */
    a5_handle_remaining_display_events();
}

/*
 * engine_quit()
 * Quit the application
 */
void engine_quit()
{
    wants_to_quit = true;
}

/*
 * engine_restart()
 * Schedules an engine restart with the given command line arguments
 * If cmd is NULL, the previous command line arguments will be reused
 */
void engine_restart(const commandline_t* cmd)
{
    logfile_message("Will restart the engine...");

    if(cmd != NULL) {
        char* argv0 = str_dup(stored_cmd.argv0); /* we'll keep the original argv[0] */

        stored_cmd = *cmd;

        str_cpy(stored_cmd.argv0, argv0, sizeof(stored_cmd.argv0));
        free(argv0);
    }

    wants_to_restart = true;
}

/*
 * engine_must_restart()
 * Checks if there is a scheduled restart. The cmd struct will be filled
 */
bool engine_must_restart(commandline_t* cmd)
{
    if(cmd != NULL)
        *cmd = stored_cmd;

    return wants_to_restart;
}

/*
 * engine_must_quit()
 * Checks if there is a scheduled quit
 */
bool engine_must_quit()
{
    return wants_to_quit;
}

/*
 * engine_is_init()
 * Is the engine initialized?
 */
bool engine_is_init()
{
    return is_initialized;
}





/*
 * engine_add_event_listener()
 * Add an event listener, i.e., a function that handles an Allegro event
 */
void engine_add_event_listener(ALLEGRO_EVENT_TYPE event_type, void* data, void (*callback)(const ALLEGRO_EVENT*,void*))
{
    add_to_event_listener_table((event_listener_t) {
        .event_type = event_type,
        .data = data,
        .callback = callback
    });
}

/*
 * engine_remove_event_listener()
 * Remove an event listener previously added with engine_add_event_listener()
 */
bool engine_remove_event_listener(ALLEGRO_EVENT_TYPE event_type, void* data, void (*callback)(const ALLEGRO_EVENT*,void*))
{
    return remove_from_event_listener_table((event_listener_t) {
        .event_type = event_type,
        .data = data,
        .callback = callback
    });
}

/*
 * engine_add_event_source()
 * Add an event source to the Allegro event queue
 */
void engine_add_event_source(ALLEGRO_EVENT_SOURCE* event_source)
{
    /* if event_source is already registered in the event queue,
       this call does nothing according to the Allegro docs */
    al_register_event_source(a5_event_queue, event_source);
}

/*
 * engine_remove_event_source()
 * Remove an event source to the Allegro event queue
 */
void engine_remove_event_source(ALLEGRO_EVENT_SOURCE* event_source)
{
    /* if event_source is unregistered in the event queue,
       this call does nothing according to the Allegro docs */
    al_unregister_event_source(a5_event_queue, event_source);
}

/*
 * engine_game_id()
 * A number that uniquely identifies the current release of the
 * currently-running opensurge game. Will be GAME_ID_UNAVAILABLE
 * if unavailable.
 */
uint32_t engine_game_id()
{
    return game_id;
}

/*
 * engine_compatibility_version_code()
 * An engine version code when in compatibility mode. When not in
 * compatibility mode, this will be just the version code of this
 * release of the engine. Compare it with VERSION_CODE().
 */
int engine_compatibility_version_code()
{
    return compatibility_version_code;
}






/* private functions */

/*
 * clean_garbage()
 * Runs the garbage collector.
 */
void clean_garbage()
{
    static double last = 0.0;
    double now = timer_get_elapsed();

    /* run the GC every GC_INTERVAL seconds (approximately) */
    if(now >= last + GC_INTERVAL) {
        last = now;
        resourcemanager_release_unused_resources();
    }
    else if(now < last)
        last = now; /* time overflow... really?! */
}

/*
 * render_overlay()
 * Renders an overlay in window space (not screen space)
 */
void render_overlay()
{
    mobilegamepad_render();
}

/*
 * init_basic_stuff()
 * Initializes the basic stuff, such as Allegro.
 * Call this before everything else.
 */
void init_basic_stuff(const commandline_t* cmd)
{
    const char* gamedir = commandline_getstring(cmd->gamedir, NULL);
    bool compatibility_mode = commandline_getint(cmd->compatibility_mode, FALSE);
    const char* compatibility_version = commandline_getstring(cmd->compatibility_version, "");

    /* basic initialization */
    wants_to_quit = false;
    wants_to_restart = false;
    stored_cmd = *cmd;

    /* randomize */
    srand(time(NULL));

    /* set Allegro's trace level to debug before calling al_init() */
    if(commandline_getint(cmd->verbose, FALSE))
        al_set_config_value(al_get_system_config(), "trace", "level", "debug");

    /* initialize Allegro */
    if(!al_is_system_installed()) { /* true when the engine is restarted */
        if(!al_init())
            fatal_error("Can't initialize Allegro");
    }

    if(!al_is_native_dialog_addon_initialized()) {
        if(!al_init_native_dialog_addon())
            fatal_error("Can't initialize Allegro's native dialog addon");
    }

    if(NULL == (a5_event_queue = al_create_event_queue()))
        fatal_error("Can't create Allegro's event queue");

    /* initialize the table of event listeners */
    init_event_listener_table();

    /* set the locale */
    setlocale(LC_ALL, "en_US.UTF-8"); /* work with UTF-8 */
    setlocale(LC_NUMERIC, "C"); /* use '.' as the decimal separator on atof() */

    /* initialize nanoparser */
    nanoparser_set_error_function(parser_error, NULL);
    nanoparser_set_warning_function(parser_warning, NULL);

    /* initialize the the asset manager and the logfile module */
    if(commandline_getint(cmd->verbose, FALSE))
        logfile_init(LOGFILE_CONSOLE);

    asset_init(
        cmd->argv0, gamedir,
        compatibility_mode ? compatibility_version : NULL,
        &game_id, &compatibility_version_code
    );
    logfile_init(LOGFILE_TXT);

    /* initialize prefs and nanocalc */
    prefs = prefs_create(NULL);
    init_nanocalc();

    /* logs */
    if(compatibility_mode)
        logfile_message("Running in compatibility mode - version: %s", stringify_version_number(engine_compatibility_version_code(), NULL, 0));

    if(game_id != GAME_ID_UNAVAILABLE)
        logfile_message("Game ID: %08x", game_id);

    logfile_message("Game title: %s", config_game_title("(null)"));
    logfile_message("Game version: %s", config_game_version("(null)"));
}


/*
 * init_managers()
 * Initializes the managers
 */
void init_managers(const commandline_t* cmd)
{
    timer_init();
    video_init();
    audio_init();
    input_init();
    resourcemanager_init();
    lang_init();

    load_managers_preferences(cmd);
}

/*
 * load_managers_preferences()
 * Load user preferences (managers only)
 */
void load_managers_preferences(const commandline_t* cmd)
{
    /* read from prefs */
    videoresolution_t resolution = (videoresolution_t)commandline_getint(cmd->video_resolution,
        prefs_has_item(prefs, ".resolution") ?
        prefs_get_int(prefs, ".resolution") :
        video_best_fit_resolution()
    );

    videoquality_t quality = (videoquality_t)commandline_getint(cmd->video_quality,
        prefs_has_item(prefs, ".videoquality") ?
        prefs_get_int(prefs, ".videoquality") :
        video_get_quality()
    );

    bool fullscreen = (bool)commandline_getint(cmd->fullscreen,
        prefs_has_item(prefs, ".fullscreen") ?
        prefs_get_bool(prefs, ".fullscreen") :
        video_is_fullscreen()
    );

    bool show_fps = (bool)(!commandline_getint(cmd->hide_fps, FALSE) && commandline_getint(cmd->show_fps,
        prefs_has_item(prefs, ".showfps") ?
        prefs_get_bool(prefs, ".showfps") :
        video_is_fps_visible()
    ));

    int master_volume = (
        prefs_has_item(prefs, ".master_volume") ?
        prefs_get_int(prefs, ".master_volume") :
        (int)(audio_get_master_volume() * 100.0f)
    );

    int music_mixer = (
        prefs_has_item(prefs, ".music_mixer") ?
        prefs_get_int(prefs, ".music_mixer") :
        (int)(audio_get_mixer_percentage() * 100.0f)
    );

    mufflerprofile_t muffler_profile = (
        prefs_has_item(prefs, ".muffler_profile") ?
        (mufflerprofile_t)prefs_get_int(prefs, ".muffler_profile") :
        audio_muffler_profile()
    );

    const char* lang_path = commandline_getstring(cmd->language_filepath,
        prefs_has_item(prefs, ".langpath") ?
        prefs_get_string(prefs, ".langpath") :
        ""
    );

    /* validate preferences */
    switch(resolution) {
        case VIDEORESOLUTION_1X:
        case VIDEORESOLUTION_2X:
        case VIDEORESOLUTION_3X:
        case VIDEORESOLUTION_4X:
            break;

        default:
            resolution = VIDEORESOLUTION_1X;
            break;
    }

    switch(quality) {
        case VIDEOQUALITY_LOW:
        case VIDEOQUALITY_MEDIUM:
        case VIDEOQUALITY_HIGH:
            break;

        default:
            quality = VIDEOQUALITY_DEFAULT;
            break;
    }

    switch(muffler_profile) {
        case MUFFLER_OFF:
        case MUFFLER_LOW:
        case MUFFLER_MEDIUM:
        case MUFFLER_HIGH:
            break;

        default:
            muffler_profile = MUFFLER_OFF;
            break;
    }

    master_volume = clip(master_volume, 0, 100);
    music_mixer = clip(music_mixer, 0, 100);

    /* apply preferences */
    video_set_resolution(resolution);
    video_set_quality(quality);
    video_set_fullscreen(fullscreen);
    video_set_fps_visible(show_fps);

    audio_set_master_volume(0.01f * (float)master_volume);
    audio_set_mixer_percentage(0.01f * (float)music_mixer);
    audio_muffler_set_profile(muffler_profile);

    if(*lang_path != '\0')
        lang_loadfile(lang_path);
}


/*
 * init_accessories()
 * Initializes the accessories
 */
void init_accessories(const commandline_t* cmd)
{
    /* we'll load SurgeScript in a different thread */
    ALLEGRO_THREAD* surgescript_thread = surgescriptloaderthread_create(cmd->user_argc, cmd->user_argv);

    /* load fonts and display a loading screen */
    font_init();
    video_display_loading_screen();

    /* load sprites & images */
    sprite_init(); /* load images in the same thread of the ALLEGRO_DISPLAY */

    /* wait for the SurgeScript loading thread & release it */
    surgescriptloaderthread_destroy(surgescript_thread); /* show potential scripting errors before loading the other accessories */

    /* load various accessories */
    storyboard_init();
    scenestack_init();
    screenshot_init();
    fadefx_init();
    audio_preload(); /* preload audio samples */
    charactersystem_init();
    objects_init(); /* legacy scripting */

    /* mobile gamepad */
    bool mobile_mode = (bool)commandline_getint(cmd->mobile, FALSE);
    mobilegamepad_init(mobile_mode ? MOBILEGAMEPAD_DEFAULT_FLAGS : MOBILEGAMEPAD_DISABLED);
    if(prefs_has_item(prefs, ".gamepad_opacity")) {
        int opacity = prefs_get_int(prefs, ".gamepad_opacity");
        if(opacity > 0) /* don't make it invisible */
            mobilegamepad_set_opacity(opacity);
    }

    /* launch the SurgeScript Virtual Machine */
    scripting_launch_vm();
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
        /*if(!prefs_has_item(prefs, ".langpath"))
            scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);*/
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
    mobilegamepad_release();
    sprite_release();
}

/*
 * release_managers()
 * Releases the previously loaded managers
 */
void release_managers()
{
    resourcemanager_release(); /* release bitmaps BEFORE the display! */
    video_release(); /* release the display */
    audio_release();
    input_release();
    timer_release();
}


/*
 * release_basic_stuff()
 * Releases the basic stuff, such as Allegro.
 * Call this after everything else.
 */
void release_basic_stuff()
{
    /* Release nanocalc and prefs */
    release_nanocalc();
    prefs = prefs_destroy(prefs);

    /* Release the logfile module and the asset manager */
    logfile_release(LOGFILE_TXT);
    asset_release();
    logfile_release(LOGFILE_CONSOLE);

    /* Release the table of event listeners */
    release_event_listener_table();

    /* Release Allegro */
    al_destroy_event_queue(a5_event_queue);
    a5_event_queue = NULL;

    if(!wants_to_restart)
        al_uninstall_system();
}

/*
 * init_nanocalc()
 * Initializes nanocalc
 */
void init_nanocalc()
{
    nanocalc_init(); /* initializes a basic nanocalc */
    nanocalc_set_error_function(calc_error); /* error callback */
    nanocalc_addons_init(); /* adds some mathematical functions to nanocalc */
    nanocalcext_register_bifs(); /* more bindings */
}

/*
 * release_nanocalc()
 * Releases nanocalc
 */
void release_nanocalc()
{
    nanocalc_addons_release();
    nanocalc_release();
}

/*
 * parser_error()
 * This is called by nanoparser when an error is raised
 */
void parser_error(const char *msg, void* context)
{
    (void)context;

    fatal_error("%s", msg);
}

/*
 * parser_warning()
 * This is called by nanoparser when a warning is raised
 */
void parser_warning(const char *msg, void* context)
{
    (void)context;

    logfile_message("%s", msg);
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
 * perform_extra_validation()
 * Extra validation
 */
void perform_extra_validation(const commandline_t* cmd)
{
    (void)cmd;

    if(config_game_title(NULL) == NULL)
        video_showmessage("Unspecified game title at surge.cfg");

    if(config_game_version(NULL) == NULL)
        video_showmessage("Unspecified game version at surge.cfg");
}

/*
 * init_event_listener_table()
 * Initializes the table of event listeners
 */
void init_event_listener_table()
{
    for(int i = 0; i < EVENT_LISTENER_TABLE_SIZE; i++) {

        /* initialize linked lists */
        event_listener_table[i] = NULL;

    }
}

/*
 * release_event_listener_table()
 * Releases the table of event listeners
 */
void release_event_listener_table()
{
    for(int i = 0; i < EVENT_LISTENER_TABLE_SIZE; i++) {

        /* release linked lists */
        for(event_listener_list_t *next, *it = event_listener_table[i]; it != NULL; it = next) {
            next = it->next;
            free(it);
        }

        event_listener_table[i] = NULL;
    }
}

/*
 * add_to_event_listener_table()
 * Add an entry to the table of event listeners
 */
void add_to_event_listener_table(event_listener_t event_listener)
{
    /* create a new node */
    event_listener_list_t* node = mallocx(sizeof *node);
    node->event_listener = event_listener;
    node->next = NULL;

    /* add the node to the last position of the list, so that the listeners
       that are registered first are executed first */
    int index = event_listener.event_type % EVENT_LISTENER_TABLE_SIZE;
    if(event_listener_table[index] != NULL) {
        event_listener_list_t* last = event_listener_table[index];
        while(last->next != NULL)
            last = last->next;
        last->next = node;
    }
    else
        event_listener_table[index] = node;

    /* note: repeated elements are not handled */
}

/*
 * remove_from_event_listener_table()
 * Remove an entry from the table of event listeners
 */
bool remove_from_event_listener_table(event_listener_t event_listener)
{
    int index = event_listener.event_type % EVENT_LISTENER_TABLE_SIZE;
    event_listener_list_t *node, *next;

    #define is_the_same_event_listener(e1, e2) \
        (((e1).event_type == (e2).event_type) && ((e1).callback == (e2).callback) && ((e1).data == (e2).data))

    node = event_listener_table[index];
    if(node == NULL) {
        /* empty list */
        return false; /* not found */
    }
    else if(is_the_same_event_listener(node->event_listener, event_listener)) {
        /* first node */
        next = node->next;
        free(node);
        event_listener_table[index] = next;
        return true;
    }
    else {
        /* not the first node */
        while(node->next != NULL) {
            if(is_the_same_event_listener(node->next->event_listener, event_listener)) {
                next = node->next->next;
                free(node->next);
                node->next = next;
                return true;
            }

            node = node->next;
        }

        return false; /* not found */
    }

    #undef is_the_same_event_listener
}

/*
 * call_event_listeners()
 * Calls the registered event listeners for a particular event
 */
void call_event_listeners(const ALLEGRO_EVENT* event)
{
    int index = event->type % EVENT_LISTENER_TABLE_SIZE;

    for(event_listener_list_t *it = event_listener_table[index]; it != NULL; it = it->next) {
        if(it->event_listener.event_type == event->type)
            it->event_listener.callback(event, it->event_listener.data);
    }
}

/*
 * a5_handle_remaining_display_events()
 * Handle the remaining display events (after the main loop) that require acknowledgement
 */
void a5_handle_remaining_display_events()
{
    ALLEGRO_EVENT event;

    while(al_get_next_event(a5_event_queue, &event)) {
        switch(event.type) {
            case ALLEGRO_EVENT_DISPLAY_RESIZE:
                al_acknowledge_resize(event.display.source);
                break;

            case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
                al_acknowledge_drawing_halt(event.display.source);
                break;

            case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
                al_acknowledge_drawing_resume(event.display.source);
                break;

            default:
                break;
        }
    }
}

/*
 * a5_handle_haltresume_event()
 * Handles a HALT_DRAWING / RESUME_DRAWING event
 */
void a5_handle_haltresume_event(const ALLEGRO_EVENT* event, void* data)
{
    bool* can_draw = (bool*)data;

    switch(event->type) {
        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            logfile_message("Received an ALLEGRO_EVENT_DISPLAY_HALT_DRAWING");
            *can_draw = false;
            al_stop_timer(a5_timer);
            timer_pause();
            al_set_default_voice(NULL);
            break;

        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            logfile_message("Received an ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING");
            al_restore_default_mixer();
            timer_resume();
            al_start_timer(a5_timer);
            *can_draw = true;
            break;
    }
}

/*
 * a5_handle_hotkey_event()
 * Handles hotkeys
 */
void a5_handle_hotkey(const ALLEGRO_EVENT* event, void* data)
{
    if(event->type != ALLEGRO_EVENT_KEY_DOWN)
        return;

    switch(event->keyboard.keycode) {
        /* F11: toggle fullscreen */
        case ALLEGRO_KEY_F11:
            video_set_fullscreen(!video_is_fullscreen());
            break;

        /* F10: toggle render queue stats report */
        case ALLEGRO_KEY_F10:
            if(!renderqueue_toggle_stats_report())
                video_showmessage("Can't toggle stats report");
            break;

        /* F9: toggle FPS counter */
        case ALLEGRO_KEY_F9:
            video_set_fps_visible(!video_is_fps_visible());
            break;

        /* F8: unmute / mute */
        case ALLEGRO_KEY_F8:
            audio_set_muted(!audio_is_muted());
            video_showmessage("%s", audio_is_muted() ? "Muted" : "Unmuted");
            break;

        /* F7: reconfigure joysticks */
        case ALLEGRO_KEY_F7:
            input_reconfigure_joysticks();
            input_print_joysticks();
            break;
    }

    (void)data;
}

/*
 * a5_handle_timer_event()
 * Update game logic
 */
void a5_handle_timer_event(const ALLEGRO_EVENT* event, void* data)
{
    bool* is_ready_to_draw = (bool*)data;

    /* update the managers */
    timer_update();
    audio_update();
    mobilegamepad_update();
    input_update();
    clean_garbage();

    /* update the current scene */
    scene_t* current_scene = scenestack_top();
    current_scene->update();
    *is_ready_to_draw = true;

    /* prevent locking */
    ALLEGRO_EVENT next_event;
    while(al_peek_next_event(a5_event_queue, &next_event) && next_event.type == ALLEGRO_EVENT_TIMER && next_event.timer.source == event->timer.source)
        al_drop_next_event(a5_event_queue);
}
