/*
 * Open Surge Engine
 * settings.c - settings screen
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

#if !defined(__ANDROID__)
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#else
#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_android.h>
#endif

#include <stdbool.h>
#include "settings.h"
#include "../core/global.h"
#include "../core/asset.h"
#include "../core/animation.h"
#include "../core/audio.h"
#include "../core/video.h"
#include "../core/engine.h"
#include "../core/commandline.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/fadefx.h"
#include "../core/input.h"
#include "../core/logfile.h"
#include "../core/font.h"
#include "../core/prefs.h"
#include "../core/lang.h"
#include "../core/web.h"
#include "../util/v2d.h"
#include "../util/point2d.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/numeric.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/mobilegamepad.h"
#include "../entities/sfx.h"

typedef struct settings_entry_t settings_entry_t;
typedef struct settings_entryvt_t settings_entryvt_t;
typedef enum settings_entrytype_t settings_entrytype_t;

enum settings_entrytype_t
{
    TYPE_TITLE,
    TYPE_SUBTITLE,
    TYPE_SETTING
};

struct settings_entryvt_t
{
    void (*on_change)(settings_entry_t*); /* change setting */
    void (*on_enter)(settings_entry_t*); /* enter (e.g., submenus) */
    void (*on_highlight)(settings_entry_t*); /* highlight option */
    void (*on_dehighlight)(settings_entry_t*);

    void (*on_init)(settings_entry_t*);
    void (*on_release)(settings_entry_t*);
    void (*on_update)(settings_entry_t*);

    bool (*is_visible)(settings_entry_t*);
};

struct settings_entry_t
{
    settings_entrytype_t type;
    const settings_entryvt_t* vt;
    void* data;

    font_t* key;
    font_t* value;

    const char* key_name;
    const char** possible_values;
    int number_of_possible_values;
    int index_of_current_value;

    point2d_t position;
};

/* constants */
const char* OPTIONS_MUSICFILE = "musics/options.ogg"; /* public */
static const char* BGFILE = "themes/scenes/options.bg";
static const char* CURSOR_ICON_SPRITE_NAME = "UI Pointer";
static const char* FLAG_ICON_SPRITE_NAME = "Flag Icon";
static const char* FONT_NAME[] = {
    [TYPE_TITLE] = "MenuTitle",
    [TYPE_SUBTITLE] = "MenuBold",
    [TYPE_SETTING] = "MenuText"
};
static fontalign_t FONT_ALIGN[] = {
    [TYPE_TITLE] = FONTALIGN_CENTER,
    [TYPE_SUBTITLE] = FONTALIGN_LEFT,
    [TYPE_SETTING] = FONTALIGN_LEFT
};
static float FONT_RELATIVE_XPOS[] = {
    [TYPE_TITLE] = 0.5f,
    [TYPE_SUBTITLE] = 0.05f,
    [TYPE_SETTING] = 0.05f
};
static const char* FONT_COLOR_HIGHLIGHT = "$COLOR_HIGHLIGHT";
static const char* FONT_COLOR_DEFAULT = "ffffff";
static const float FADE_TIME = 0.5f;
static const char* FADE_COLOR = "000000";

/* file chooser */
#define EVENT_FILECHOOSER_ASYNC ALLEGRO_GET_EVENT_TYPE('F', 'I', 'L', 'E')

typedef struct {
    bool want_chooser_of_folders;
    ALLEGRO_EVENT_SOURCE event_source;
    ALLEGRO_FILECHOOSER* file_chooser[2];
    ALLEGRO_THREAD* thread;
} filechooser_data_t;

static void* filechooser_run(ALLEGRO_THREAD* thread, void* arg);
static void filechooser_handle_event(const ALLEGRO_EVENT* event, void* arg);

#if defined(__ANDROID__)
static char* application_id(char* buffer, size_t buffer_size);
#endif

/* helpers */
#if defined(__ANDROID__)
#define IS_MOBILE_PLATFORM 1
#else
#define IS_MOBILE_PLATFORM 0
#endif
#define in_mobile_mode() mobilegamepad_is_available()

#ifndef WANT_PLAYMOD
#define WANT_PLAYMOD 0
#endif

#define MULTIPLICATION_SIGN     "\xc3\x97" /* utf-8 encoding for code point 0xD7 */
#define TIMES(k)                #k MULTIPLICATION_SIGN

/* languages */
#define MAX_LANGUAGES 63
static struct {
    char* path_list[1+MAX_LANGUAGES]; /* NULL-terminated arrays */
    char* name_list[1+MAX_LANGUAGES];
    char* id_list[1+MAX_LANGUAGES];
} languages;

static void load_lang_list();
static void unload_lang_list();
static int dirfill(const char* filename, void* param);
static const char* filepath_of_lang(const char* lang_id);

/* private */
static v2d_t camera;
static input_t* input = NULL;
static bgtheme_t* background = NULL;
static music_t* music = NULL;
static actor_t* flag_icon = NULL;
static actor_t* cursor_icon = NULL;
static bool was_immersive = false;
static bool fade_in = false, fade_out = false;
static scene_t* next_scene = NULL;
static void* next_scene_arg = NULL;

/* vtables */
#define vt_title (settings_entryvt_t) { nop, nop, nop, nop, nop, nop, nop, visible }
static void nop(settings_entry_t* e) { (void)e; } /* no operation */
static bool visible(settings_entry_t* e) { (void)e; return true; } /* make visible */
static bool invisible(settings_entry_t* e) { (void)e; return false; } /* make invisible */

#define vt_back (settings_entryvt_t){ nop, go_back, nop, nop, nop, nop, nop, visible }
static void go_back(settings_entry_t* e);

/* Graphics */
#define vt_graphics (settings_entryvt_t) { nop, nop, nop, nop, nop, nop, nop, visible }

#define vt_quality (settings_entryvt_t){ change_quality, nop, nop, nop, init_quality, nop, nop, visible }
static void init_quality(settings_entry_t* e);
static void change_quality(settings_entry_t* e);

#define vt_resolution (settings_entryvt_t){ change_resolution, nop, nop, nop, init_resolution, nop, nop, !IS_MOBILE_PLATFORM ? visible : invisible }
static void init_resolution(settings_entry_t* e);
static void change_resolution(settings_entry_t* e);

#define vt_fullscreen (settings_entryvt_t){ change_fullscreen, nop, nop, nop, init_fullscreen, nop, update_fullscreen, !IS_MOBILE_PLATFORM ? visible : invisible }
static void init_fullscreen(settings_entry_t* e);
static void change_fullscreen(settings_entry_t* e);
static void update_fullscreen(settings_entry_t* e);

#define vt_showfps (settings_entryvt_t){ change_showfps, nop, nop, nop, init_showfps, nop, update_showfps, visible }
static void init_showfps(settings_entry_t* e);
static void change_showfps(settings_entry_t* e);
static void update_showfps(settings_entry_t* e);

/* Audio */
#define vt_audio (settings_entryvt_t){ nop, nop, nop, nop, nop, nop, nop, visible }

#define vt_volume (settings_entryvt_t){ change_volume, nop, nop, nop, init_volume, nop, nop, visible }
static void init_volume(settings_entry_t* e);
static void change_volume(settings_entry_t* e);

#define vt_mixer (settings_entryvt_t){ change_mixer, nop, nop, nop, init_mixer, nop, nop, visible }
static void init_mixer(settings_entry_t* e);
static void change_mixer(settings_entry_t* e);

#define vt_muffler (settings_entryvt_t){ change_muffler, nop, highlight_muffler, dehighlight_muffler, init_muffler, nop, nop, visible }
static void init_muffler(settings_entry_t* e);
static void change_muffler(settings_entry_t* e);
static void highlight_muffler(settings_entry_t* e);
static void dehighlight_muffler(settings_entry_t* e);

#define vt_mute (settings_entryvt_t){ change_mute, nop, nop, nop, init_mute, nop, update_mute, visible }
static void init_mute(settings_entry_t* e);
static void change_mute(settings_entry_t* e);
static void update_mute(settings_entry_t* e);

/* Controls */
#define vt_controls (settings_entryvt_t){ nop, nop, nop, nop, nop, nop, nop, display_gamepadopacity }

#define vt_gamepadopacity (settings_entryvt_t){ change_gamepadopacity, nop, nop, nop, init_gamepadopacity, nop, nop, display_gamepadopacity }
static void init_gamepadopacity(settings_entry_t* e);
static void change_gamepadopacity(settings_entry_t* e);
static bool display_gamepadopacity(settings_entry_t* e);

/* Game */
#define vt_game (settings_entryvt_t){ nop, nop, nop, nop, nop, nop, nop, visible }

#define vt_language (settings_entryvt_t){ change_language, nop, nop, nop, init_language, nop, nop, visible }
static void init_language(settings_entry_t* e);
static void change_language(settings_entry_t* e);

#define vt_developermode (settings_entryvt_t){ nop, enter_developermode, nop, nop, nop, nop, nop, display_developermode }
static bool enable_developermode = false;
static void enter_developermode(settings_entry_t* e);
static bool display_developermode(settings_entry_t* e);

#define vt_stageselect (settings_entryvt_t){ nop, enter_stageselect, highlight_stageselect, nop, init_stageselect, release_stageselect, update_stageselect, visible }
static void enter_stageselect(settings_entry_t* e);
static void highlight_stageselect(settings_entry_t* e);
static void update_stageselect(settings_entry_t* e);
static void init_stageselect(settings_entry_t* e);
static void release_stageselect(settings_entry_t* e);

#define vt_credits (settings_entryvt_t){ nop, enter_credits, nop, nop, nop, nop, nop, visible }
static void enter_credits(settings_entry_t* e);

/* MODs */
#define vt_mods (settings_entryvt_t){ nop, nop, nop, nop, nop, nop, nop, display_mods }
static bool want_compatibility_mode = true;
static bool want_zipped_mods = false;
static bool display_mods(settings_entry_t* e);
static bool display_mods_from_base_game(settings_entry_t* e);
static bool display_mods_from_mod(settings_entry_t* e);

#define vt_playgame (settings_entryvt_t){ nop, enter_playgame, nop, nop, init_playgame, release_playgame, nop, display_mods_from_base_game }
static void enter_playgame(settings_entry_t* e);
static void init_playgame(settings_entry_t* e);
static void release_playgame(settings_entry_t* e);

#define vt_modstorage (settings_entryvt_t){ change_modstorage, nop, nop, nop, nop, nop, nop, display_mods_from_base_game }
static void change_modstorage(settings_entry_t* e);

#define vt_compatibilitymode (settings_entryvt_t) { change_compatibilitymode, nop, nop, nop, nop, nop, nop, display_mods_from_base_game }
static void change_compatibilitymode(settings_entry_t* e);

#define vt_backtobasegame (settings_entryvt_t){ nop, enter_backtobasegame, nop, nop, nop, nop, nop, display_mods_from_mod }
static void enter_backtobasegame(settings_entry_t* e);

/* Engine */
#define vt_engine (settings_entryvt_t){ nop, nop, nop, nop, nop, nop, nop, visible }

#define vt_about (settings_entryvt_t){ nop, show_info, nop, nop, nop, nop, nop, visible }
static void show_info(settings_entry_t* e);

#define vt_share (settings_entryvt_t){ nop, share, nop, nop, nop, nop, nop, visible }
static void share(settings_entry_t* e);

#define vt_download (settings_entryvt_t){ nop, download, nop, nop, nop, nop, nop, visible }
static void download(settings_entry_t* e);

#define vt_submitfeedback (settings_entryvt_t){ nop, submitfeedback, nop, nop, nop, nop, nop, display_submitfeedback }
static void submitfeedback(settings_entry_t* e);
static bool display_submitfeedback(settings_entry_t* e);

#define vt_reportissue (settings_entryvt_t){ nop, reportissue, nop, nop, nop, nop, nop, display_reportissue }
static void reportissue(settings_entry_t* e);
static bool display_reportissue(settings_entry_t* e);




/* declaration of the options */
static const struct
{
    settings_entrytype_t type;
    const char* key;
    const char** possible_values; /* NULL-terminated array */
    int index_of_default_value;
    settings_entryvt_t vt;
    int padding_top;
} entry_decl[] = {

    /* Title */
    { TYPE_TITLE, "$OPTIONS_COLORED_TITLE", (const char*[]){ NULL }, 0, vt_title, 8 },

    /* Graphics */
    { TYPE_SUBTITLE, "$OPTIONS_GRAPHICS", (const char*[]){ NULL }, 0, vt_graphics, 8 },
    { TYPE_SETTING, "$OPTIONS_QUALITY", (const char*[]){ "$OPTIONS_QUALITY_LOW", "$OPTIONS_QUALITY_MEDIUM", "$OPTIONS_QUALITY_HIGH", NULL }, 1, vt_quality, 8 },
    { TYPE_SETTING, "$OPTIONS_RESOLUTION", (const char*[]){ TIMES(1), TIMES(2), TIMES(3), TIMES(4), NULL }, 1, vt_resolution, 0 },
    { TYPE_SETTING, "$OPTIONS_FULLSCREEN", (const char*[]){ "$OPTIONS_NO", "$OPTIONS_YES", NULL }, 0, vt_fullscreen, 0 },
    { TYPE_SETTING, "$OPTIONS_FPS", (const char*[]){ "$OPTIONS_NO", "$OPTIONS_YES", NULL }, 0, vt_showfps, 0 },

    /* Audio */
    { TYPE_SUBTITLE, "$OPTIONS_AUDIO", (const char*[]){ NULL }, 0, vt_audio, 8 },
    { TYPE_SETTING, "$OPTIONS_VOLUME", (const char*[]){ "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", NULL }, 10, vt_volume, 8 },
    { TYPE_SETTING, "$OPTIONS_MIXER", (const char*[]){ "$OPTIONS_MIXER_MIN", "5%", "10%", "15%", "20%", "25%", "30%", "35%", "40%", "45%", "$OPTIONS_MIXER_MID", "55%", "60%", "65%", "70%", "75%", "80%", "85%", "90%", "95%", "$OPTIONS_MIXER_MAX", NULL }, 10, vt_mixer, 0 },
    { TYPE_SETTING, "$OPTIONS_MUFFLER", (const char*[]){ "$OPTIONS_MUFFLER_OFF", "$OPTIONS_MUFFLER_LOW", "$OPTIONS_MUFFLER_MEDIUM", "$OPTIONS_MUFFLER_HIGH", NULL }, 2, vt_muffler, 0 },
    { TYPE_SETTING, "$OPTIONS_MUTE", (const char*[]){ "$OPTIONS_NO", "$OPTIONS_YES", NULL }, 0, vt_mute, 0 },

    /* Controls */
    { TYPE_SUBTITLE, "$OPTIONS_CONTROLS", (const char*[]){ NULL }, 0, vt_controls, 8 },
    { TYPE_SETTING, "$OPTIONS_GAMEPADOPACITY", (const char*[]){ "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", NULL }, 10, vt_gamepadopacity, 8 },

    /* Game */
    { TYPE_SUBTITLE, "$OPTIONS_GAME", (const char*[]){ NULL }, 0, vt_game, 8 },
    { TYPE_SETTING, "$OPTIONS_LANGUAGE", (const char**)languages.name_list, 0, vt_language, 8 },
    { TYPE_SETTING, "$OPTIONS_DEVELOPERMODE", (const char*[]){ NULL }, 0, vt_developermode, 0 },
    { TYPE_SETTING, "$OPTIONS_STAGESELECT", (const char*[]){ NULL }, 0, vt_stageselect, 0 },
    { TYPE_SETTING, "$OPTIONS_CREDITS", (const char*[]){ NULL }, 0, vt_credits, 0 },

    /* MODs */
    { TYPE_SUBTITLE, "$OPTIONS_MODS", (const char*[]) { NULL }, 0, vt_mods, 8 },
    { TYPE_SETTING, "$OPTIONS_PLAYMOD", (const char*[]) { NULL }, 0, vt_playgame, 8 },
    { TYPE_SETTING, "$OPTIONS_MODSTORAGE", (const char*[]) { "$OPTIONS_MODSTORAGE_ARCHIVE", "$OPTIONS_MODSTORAGE_FOLDER", NULL }, 0, vt_modstorage, 0 },
    { TYPE_SETTING, "$OPTIONS_COMPATIBILITYMODE", (const char*[]) { "$OPTIONS_OFF", "$OPTIONS_ON", NULL }, 1, vt_compatibilitymode, 0 },
    { TYPE_SETTING, "$OPTIONS_BACKTOBASEGAME", (const char*[]) { NULL }, 0, vt_backtobasegame, 8 },

    /* Engine */
    { TYPE_SUBTITLE, "$OPTIONS_ENGINE", (const char*[]){ NULL }, 0, vt_engine, 8 },
    { TYPE_SETTING, "$OPTIONS_ABOUT", (const char*[]){ NULL }, 0, vt_about, 8 },
    { TYPE_SETTING, "$OPTIONS_SHARE", (const char*[]){ NULL }, 0, vt_share, 0 },
    { TYPE_SETTING, "$OPTIONS_REPORTISSUE", (const char*[]){ NULL }, 0, vt_reportissue, 0 },
#if IS_MOBILE_PLATFORM
    { TYPE_SETTING, "$OPTIONS_RATEAPP", (const char*[]){ NULL }, 0, vt_submitfeedback, 0 },
    { TYPE_SETTING, "$OPTIONS_DOWNLOAD_DESKTOP", (const char*[]){ NULL }, 0, vt_download, 0 },
#else
    { TYPE_SETTING, "$OPTIONS_SUBMITFEEDBACK", (const char*[]){ NULL }, 0, vt_submitfeedback, 0 },
    { TYPE_SETTING, "$OPTIONS_DOWNLOAD_MOBILE", (const char*[]){ NULL }, 0, vt_download, 0 },
#endif

    /* Back (last entry) */
    { TYPE_SETTING, "$OPTIONS_BACK", (const char*[]){ NULL }, 0, vt_back, 16 }

};

/* entries */
enum { number_of_entries = sizeof(entry_decl) / sizeof(entry_decl[0]) };
static settings_entry_t entry[number_of_entries];
static void init_entries();
static void update_entries();
static void render_entries(v2d_t camera_position);
static void release_entries();
static void rebuild_entries();
static inline int array_count(const char** arr);

/* settings (highlightable entries, i.e., visible entries whose type is TYPE_SETTING) */
static settings_entry_t* setting[number_of_entries] = { NULL };
static int number_of_settings = 0;
static int index_of_highlighted_setting = 0;
#define index_of_back (number_of_settings - 1) /* assumes that BACK is the last entry */

/* misc */
static void update_flag_icon(const settings_entry_t* e);
static void update_music();
static void handle_controls();
static bool handle_fading();
static void update_camera();
static void save_preferences();
static const char* create_url(const char* path);
static bool is_base_game();





/*
 * settings_init()
 * Initialize scene
 */
void settings_init(void* data)
{
    camera = v2d_multiply(video_get_screen_size(), 0.5f);
    fade_in = true;
    fade_out = false;
    next_scene = NULL;
    next_scene_arg = NULL;

    /* options */
    enable_developermode = false;
    want_compatibility_mode = true;
    want_zipped_mods = true;

    /* initialize objects */
    background = background_load(BGFILE);
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);
    flag_icon = actor_create();
    cursor_icon = actor_create();
    actor_change_animation(cursor_icon, sprite_get_animation(CURSOR_ICON_SPRITE_NAME, 0));

    /* setup entries */
    index_of_highlighted_setting = 0;
    load_lang_list();
    init_entries();

    /* immersive mode */
    was_immersive = video_is_immersive();
    video_set_immersive(false);

    /* reconfigure joysticks */
    input_reconfigure_joysticks();
}

/*
 * settings_release()
 * Release scene
 */
void settings_release()
{
    /* immersive mode */
    video_set_immersive(was_immersive);

    /* release entries */
    release_entries();
    unload_lang_list();

    /* release objects */
    actor_destroy(cursor_icon);
    actor_destroy(flag_icon);
    music_unref(music);
    input_destroy(input);
    background_unload(background);
}

/*
 * settings_update()
 * Update scene
 */
void settings_update()
{
    mobilegamepad_fadein(); /* display the mobile gamepad */
    background_update(background);
    update_music();

    if(!handle_fading())
        return;

    handle_controls();
    update_entries();
    update_camera();
}

/*
 * settings_render()
 * Render scene
 */
void settings_render()
{
    background_render_bg(background, camera);

    render_entries(camera);
    actor_render(flag_icon, camera);
    actor_render(cursor_icon, camera);

    background_render_fg(background, camera);
}




/*
 *
 * private
 *
 */

void init_entries()
{
    int screen_width = video_get_screen_size().x;
    int ypos = 0;

    for(int i = 0; i < number_of_entries; i++) {
        entry[i].type = entry_decl[i].type;
        entry[i].vt = &entry_decl[i].vt;
        entry[i].data = NULL;

        entry[i].key_name = entry_decl[i].key;
        entry[i].possible_values = entry_decl[i].possible_values;
        entry[i].number_of_possible_values = array_count(entry_decl[i].possible_values);
        entry[i].index_of_current_value = entry_decl[i].index_of_default_value;

        entry[i].key = font_create(FONT_NAME[entry[i].type]);
        font_set_text(entry[i].key, entry[i].key_name);
        font_set_align(entry[i].key, FONT_ALIGN[entry[i].type]);

        entry[i].value = font_create(FONT_NAME[TYPE_SETTING]);
        font_set_text(entry[i].value, "");
        font_set_align(entry[i].value, FONTALIGN_RIGHT);

        entry[i].position.x = FONT_RELATIVE_XPOS[entry[i].type] * screen_width;
        entry[i].position.y = ypos + entry_decl[i].padding_top;
        font_set_position(entry[i].key, v2d_new(entry[i].position.x, entry[i].position.y));
        font_set_position(entry[i].value, v2d_new(screen_width - entry[i].position.x, entry[i].position.y));

        bool is_visible = entry[i].vt->is_visible(&entry[i]);
        font_set_visible(entry[i].key, is_visible);
        font_set_visible(entry[i].value, is_visible);
        if(is_visible)
            ypos = entry[i].position.y + font_get_textsize(entry[i].key).y;
    }

    number_of_settings = 0;
    memset(setting, 0, sizeof(setting));
    for(int i = 0; i < number_of_entries; i++) {
        if(entry[i].type == TYPE_SETTING && entry[i].vt->is_visible(&entry[i]))
            setting[number_of_settings++] = &entry[i];
    }

    assertx(number_of_settings > 0);
    index_of_highlighted_setting = clip(index_of_highlighted_setting, 0, number_of_settings - 1);

    for(int i = 0; i < number_of_entries; i++)
        entry[i].vt->on_init(&entry[i]);
}

void release_entries()
{
    for(int i = number_of_entries - 1; i >= 0; i--)
        entry[i].vt->on_release(&entry[i]);

    for(int i = number_of_entries - 1; i >= 0; i--) {
        font_destroy(entry[i].value);
        font_destroy(entry[i].key);
    }
}

void rebuild_entries()
{
    release_entries();
    init_entries();
}

void update_entries()
{
    /* for each entry */
    for(int i = 0; i < number_of_entries; i++) {
        /* check if the entry is highlighted */
        bool is_highlighted = &entry[i] == setting[index_of_highlighted_setting];
        const char* color = is_highlighted ? FONT_COLOR_HIGHLIGHT : FONT_COLOR_DEFAULT;

        /* display the key */
        const char* key = entry[i].key_name;
        font_set_text(entry[i].key, "<color=%s>%s</color>", color, key);

        /* display the current value */
        if(entry[i].number_of_possible_values > 0) {
            int j = entry[i].index_of_current_value % entry[i].number_of_possible_values;
            const char* value = entry[i].possible_values[j];
            font_set_text(entry[i].value, "<color=%s>%s</color>", color, value);
        }
        else
            font_set_visible(entry[i].value, false);

        /* display the cursor icon */
        if(is_highlighted)
            cursor_icon->position = font_get_position(entry[i].key);
    }
}

void render_entries(v2d_t camera_position)
{
    for(int i = 0; i < number_of_entries; i++) {
        font_render(entry[i].key, camera_position);
        font_render(entry[i].value, camera_position);
    }
}

/* count the number of non-NULL elements of a NULL-terminated array */
int array_count(const char** arr)
{
    int cnt = 0;

    for(const char** it = arr; *it != NULL; it++)
        cnt++;

    return cnt;
}

void update_music()
{
    bool quit = (fade_out && next_scene == NULL);

    if(quit) {
        if(!fadefx_is_fading())
            music_stop();
    }
    else if(!music_is_playing())  {
        if(!fade_in)
            music_play(music, true);
    }
}

void handle_controls()
{
    /* skip if fading */
    if(fade_in || fade_out)
        return;

    /* change the highlighted setting by pressing up or down */
    if(index_of_highlighted_setting > 0) {
        if(input_button_pressed(input, IB_UP)) {
            int j = index_of_highlighted_setting;
            int i = --index_of_highlighted_setting;
            sound_play(SFX_CHOOSE);
            setting[j]->vt->on_dehighlight(setting[j]);
            setting[i]->vt->on_highlight(setting[i]);
        }
    }

    if(index_of_highlighted_setting < number_of_settings - 1) {
        if(input_button_pressed(input, IB_DOWN)) {
            int j = index_of_highlighted_setting;
            int i = ++index_of_highlighted_setting;
            sound_play(SFX_CHOOSE);
            setting[j]->vt->on_dehighlight(setting[j]);
            setting[i]->vt->on_highlight(setting[i]);
        }
    }

    /* change the value of the highlighted setting */
    if(input_button_pressed(input, IB_LEFT)) {
        int i = index_of_highlighted_setting;
        int n = setting[i]->number_of_possible_values;
        if(n > 1 && setting[i]->index_of_current_value > 0) {
            sound_play(SFX_CHOOSE);
            setting[i]->index_of_current_value--;
            setting[i]->vt->on_change(setting[i]);
        }
    }

    if(input_button_pressed(input, IB_RIGHT)) {
        int i = index_of_highlighted_setting;
        int n = setting[i]->number_of_possible_values;
        if(n > 1 && setting[i]->index_of_current_value < n-1) {
            sound_play(SFX_CHOOSE);
            setting[i]->index_of_current_value++;
            setting[i]->vt->on_change(setting[i]);
        }
    }

    if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
        int i = index_of_highlighted_setting;
        int n = setting[i]->number_of_possible_values;
        if(n > 1) {
            sound_play(SFX_CHOOSE);
            setting[i]->index_of_current_value++;
            setting[i]->index_of_current_value %= n;
            setting[i]->vt->on_change(setting[i]);
        }
    }

    /* enter the highlighted setting by pressing fire1 or fire3 */
    if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
        int i = index_of_highlighted_setting;
        int n = setting[i]->number_of_possible_values;
        if(n == 0) {
            sound_play(i < index_of_back ? SFX_CONFIRM : SFX_BACK);
            setting[i]->vt->on_enter(setting[i]);
        }
    }

    /* go back by pressing fire2 or fire4 */
    if(input_button_pressed(input, IB_FIRE2) || input_button_pressed(input, IB_FIRE4)) {
        int i = index_of_back;
        sound_play(SFX_BACK);
        setting[i]->vt->on_enter(setting[i]);
    }

    /* update all settings */
    for(int i = 0; i < number_of_settings; i++)
        setting[i]->vt->on_update(setting[i]);
}

bool handle_fading()
{
    /* fade in */
    if(fade_in) {
        fade_in = false;
        fadefx_in(color_hex(FADE_COLOR), FADE_TIME);
    }

    /* fade out */
    else if(fade_out) {
        if(fadefx_is_over()) {
            fade_out = false;
            fade_in = true;

            if(next_scene == NULL) {
                /* go back */
                scenestack_pop();
                return false;
            }
            else {
                /* load another scene */
                scenestack_push(next_scene, next_scene_arg);
                return false;
            }
        }

        fadefx_out(color_hex(FADE_COLOR), FADE_TIME);
    }

    /* done */
    return true;
}

/* update the camera */
void update_camera()
{
    int ypos = setting[index_of_highlighted_setting]->position.y;
    int line_ypos = video_get_screen_size().y * 0.5f;

    int target_ypos = max(ypos, line_ypos);
    camera.y = lerp(camera.y, target_ypos, 0.25f);
}

/* saves the user preferences */
void save_preferences()
{
    extern prefs_t* prefs;

    prefs_set_int(prefs, ".resolution", video_get_resolution());
    prefs_set_int(prefs, ".videoquality", video_get_quality());
    prefs_set_bool(prefs, ".fullscreen", video_is_fullscreen());
    prefs_set_bool(prefs, ".showfps", video_is_fps_visible());

    prefs_set_int(prefs, ".master_volume", (int)(audio_get_master_volume() * 100.0f));
    prefs_set_int(prefs, ".music_mixer", (int)(audio_get_mixer_percentage() * 100.0f));
    prefs_set_int(prefs, ".muffler_profile", audio_muffler_profile());

    prefs_set_string(prefs, ".langpath", filepath_of_lang(lang_getid()));

    prefs_set_int(prefs, ".gamepad_opacity", mobilegamepad_opacity());
}

/* returns a static char[] */
const char* create_url(const char* path)
{
    static char buffer[256];
    char sep = (strchr(path, '?') != NULL) ? '&' : '?';
    char encoded_lang_id[32], encoded_version_string[128];

    encode_uri_component(GAME_VERSION_STRING, encoded_version_string, sizeof(encoded_version_string));
    encode_uri_component(lang_get("LANG_ID"), encoded_lang_id, sizeof(encoded_lang_id));

    snprintf(buffer, sizeof(buffer), "%s%s%cv=%s&lang=%s&from=options", GAME_URL, path, sep, encoded_version_string, encoded_lang_id);

    return buffer;
}

bool is_base_game()
{
    const int version_number = VERSION_CODE_EX(GAME_VERSION_SUP, GAME_VERSION_SUB, GAME_VERSION_WIP, GAME_VERSION_FIX);
    char version_string[16];

    if(0 != strcmp(opensurge_game_name(), "Surge the Rabbit"))
        return false;

    stringify_version_number(version_number, version_string, sizeof(version_string));
    if(0 != strcmp(opensurge_game_version(), version_string))
        return false;

    return true;
}





/*
 *
 * languages
 *
 */

void load_lang_list()
{
    /* initialize the lists */
    for(int i = 0; i < MAX_LANGUAGES; i++) {
        languages.path_list[i] = NULL;
        languages.name_list[i] = NULL;
        languages.id_list[i] = NULL;
    }

    /* read the .lng files */
    int counter = 0;
    asset_foreach_file("languages", ".lng", dirfill, &counter, false);

    /* sort by name */
    for(int i = 0, n = counter; i < n; i++) {
        int m = i;

        for(int j = i+1; j < n; j++) {
            if(strcmp(languages.name_list[j], languages.name_list[m]) < 0)
                m = j;
        }

        if(m != i) {
            #define SWAP(x, a, b) do { \
                char* tmp = languages.x ## _list[a]; \
                languages.x ## _list[a] = languages.x ## _list[b]; \
                languages.x ## _list[b] = tmp; \
            } while(0)

            SWAP(path, i, m);
            SWAP(name, i, m);
            SWAP(id, i, m);

            #undef SWAP
        }
    }
}

void unload_lang_list()
{
    /* release the lists */
    for(int i = MAX_LANGUAGES - 1; i >= 0; i--) {
        if(languages.id_list[i] != NULL) {
            free(languages.id_list[i]);
            languages.id_list[i] = NULL;
        }
        if(languages.name_list[i] != NULL) {
            free(languages.name_list[i]);
            languages.name_list[i] = NULL;
        }
        if(languages.path_list[i] != NULL) {
            free(languages.path_list[i]);
            languages.path_list[i] = NULL;
        }
    }
}

int dirfill(const char* filename, void* param)
{
    int* counter = (int*)param;

    /* too many languages? */
    if(*counter > MAX_LANGUAGES) {
        logfile_message("Warning: too many language files! Maximum is %d", MAX_LANGUAGES);
        return -1; /* stop enumeration */
    }

    /* check if the language file is compatible with this version of the engine */
    int supver, subver, wipver;
    lang_compatibility(filename, &supver, &subver, &wipver);

    bool is_compatible = (game_version_compare(supver, subver, wipver) >= 0);
    if(game_version_compare(supver, subver, wipver) != 0)
        logfile_message("Warning: language file \"%s\" (compatibility: %d.%d.%d) may not be fully compatible with this version of the engine (%s)", filename, supver, subver, wipver, GAME_VERSION_STRING);

    /* add to the languages list */
    if(is_compatible) {
        char langname[64], langid[16];
        lang_metadata(filename, "LANG_NAME", langname, sizeof langname);
        lang_metadata(filename, "LANG_ID", langid, sizeof langid);

        languages.path_list[*counter] = str_dup(filename);
        languages.name_list[*counter] = str_dup(langname);
        languages.id_list[*counter] = str_dup(langid);
        (*counter)++;
    }

    /* done */
    return 0;
}

const char* filepath_of_lang(const char* lang_id)
{
    for(int i = 0; languages.id_list[i] != NULL; i++) {
        if(0 == strcmp(lang_id, languages.id_list[i]))
            return languages.path_list[i];
    }

    return "";
}




/*
 *
 * vtables
 *
 */




/*
 * Back
 */

void go_back(settings_entry_t* e)
{
    save_preferences();
    fade_out = true;
    next_scene = NULL;
}



/*
 * Resolution
 */

void change_resolution(settings_entry_t* e)
{
    const videoresolution_t new_resolution[] = {
        [0] = VIDEORESOLUTION_1X,
        [1] = VIDEORESOLUTION_2X,
        [2] = VIDEORESOLUTION_3X,
        [3] = VIDEORESOLUTION_4X
    };

    int i = e->index_of_current_value;
    video_set_resolution(new_resolution[i]);
}

void init_resolution(settings_entry_t* e)
{
    const int index[] = {
        [VIDEORESOLUTION_1X] = 0,
        [VIDEORESOLUTION_2X] = 1,
        [VIDEORESOLUTION_3X] = 2,
        [VIDEORESOLUTION_4X] = 3
    };

    videoresolution_t r = video_get_resolution();
    e->index_of_current_value = index[r];
}



/*
 * Quality
 */

void change_quality(settings_entry_t* e)
{
    const videoquality_t new_quality[] = {
        [0] = VIDEOQUALITY_LOW,
        [1] = VIDEOQUALITY_MEDIUM,
        [2] = VIDEOQUALITY_HIGH
    };

    int i = e->index_of_current_value;
    video_set_quality(new_quality[i]);

    /* coming soon */
    if(new_quality[i] == VIDEOQUALITY_HIGH) {
        sound_play(SFX_DENY);
        alert("%s", lang_get("OPTIONS_COMINGSOON"));
    }
}

void init_quality(settings_entry_t* e)
{
    const int index[] = {
        [VIDEOQUALITY_LOW] = 0,
        [VIDEOQUALITY_MEDIUM] = 1,
        [VIDEOQUALITY_HIGH] = 2
    };

    videoquality_t q = video_get_quality();
    e->index_of_current_value = index[q];
}



/*
 * Fullscreen
 */

void change_fullscreen(settings_entry_t* e)
{
    bool new_fullscreen = (e->index_of_current_value != 0);
    video_set_fullscreen(new_fullscreen);
}

void init_fullscreen(settings_entry_t* e)
{
    int index = video_is_fullscreen() ? 1 : 0;
    e->index_of_current_value = index;
}

void update_fullscreen(settings_entry_t* e)
{
    /* note: user may press F11 at any time,
       not just when highlighting the setting.
       tiny detail? */
    int index = video_is_fullscreen() ? 1 : 0;
    e->index_of_current_value = index;
}



/*
 * Show FPS
 */

void change_showfps(settings_entry_t* e)
{
    bool new_showfps = (e->index_of_current_value != 0);
    video_set_fps_visible(new_showfps);
}

void init_showfps(settings_entry_t* e)
{
    int index = video_is_fps_visible() ? 1 : 0;
    e->index_of_current_value = index;
}

void update_showfps(settings_entry_t* e)
{
    /* the fps counter may be toggled externally */
    bool is_visible = video_is_fps_visible();
    e->index_of_current_value = is_visible ? 1 : 0;
}



/*
 * Change volume
 */

void init_volume(settings_entry_t* e)
{
    int volume = (int)(audio_get_master_volume() * 100.0f);
    int index = (volume / 10) + (volume % 10 != 0); /* ceil(volume / 10) */
    e->index_of_current_value = index;
}

void change_volume(settings_entry_t* e)
{
    int volume = e->index_of_current_value * 10;
    audio_set_master_volume((float)volume * 0.01f);

    if(volume != 0)
        audio_set_muted(false);
}



/*
 * Music mixer
 */

void init_mixer(settings_entry_t* e)
{
    int percentage = (int)(audio_get_mixer_percentage() * 100.0f);
    int index = percentage / 5;
    e->index_of_current_value = index;
}

void change_mixer(settings_entry_t* e)
{
    float percentage = e->index_of_current_value * 0.05f;
    audio_set_mixer_percentage(percentage);
}



/*
 * Underwater effect
 */

void init_muffler(settings_entry_t* e)
{
    const int index[] = {
        [MUFFLER_OFF] = 0,
        [MUFFLER_LOW] = 1,
        [MUFFLER_MEDIUM] = 2,
        [MUFFLER_HIGH] = 3
    };

    mufflerprofile_t profile = audio_muffler_profile();
    e->index_of_current_value = index[profile];
}

void change_muffler(settings_entry_t* e)
{
    const mufflerprofile_t new_profile[] = {
        [0] = MUFFLER_OFF,
        [1] = MUFFLER_LOW,
        [2] = MUFFLER_MEDIUM,
        [3] = MUFFLER_HIGH
    };

    int i = e->index_of_current_value;
    audio_muffler_set_profile(new_profile[i]);
}

void highlight_muffler(settings_entry_t* e)
{
    audio_muffler_activate(true);
}

void dehighlight_muffler(settings_entry_t* e)
{
    audio_muffler_activate(false);
}




/*
 * Unmute / mute
 */

void init_mute(settings_entry_t* e)
{
    bool is_muted = audio_is_muted();
    e->index_of_current_value = is_muted ? 1 : 0;
}

void change_mute(settings_entry_t* e)
{
    bool want_muted = (e->index_of_current_value != 0);
    audio_set_muted(want_muted);
}

void update_mute(settings_entry_t* e)
{
    /* the audio may be muted externally */
    bool is_muted = audio_is_muted();
    e->index_of_current_value = is_muted ? 1 : 0;
}



/*
 * Change opacity of the gamepad
 */

void init_gamepadopacity(settings_entry_t* e)
{
    int opacity = mobilegamepad_opacity();
    e->index_of_current_value = clip(opacity, 0, 100) / 10;
}

void change_gamepadopacity(settings_entry_t* e)
{
    int opacity = e->index_of_current_value * 10;
    mobilegamepad_set_opacity(opacity);
}

bool display_gamepadopacity(settings_entry_t* e)
{
    return mobilegamepad_is_available();
}



/*
 * Change Language
 */

void init_language(settings_entry_t* e)
{
    /* find the index of the current language */
    const char* current_lang_id = lang_getid();
    for(int i = 0; languages.id_list[i] != NULL; i++) {
        if(0 == strcmp(languages.id_list[i], current_lang_id)) {
            e->index_of_current_value = i;
            break;
        }
    }

    /* update the flag */
    update_flag_icon(e);
}

void change_language(settings_entry_t* e)
{
    int i = e->index_of_current_value;

    /* validate */
    if(i > MAX_LANGUAGES || languages.path_list[i] == NULL)
        return;

    /* load the language */
    lang_loadfile(languages.path_list[i]);

    /* update the flag */
    update_flag_icon(e);
}

void update_flag_icon(const settings_entry_t* e)
{
    const int UNKNOWN_FLAG = 0;
    point2d_t flag_offset = point2d_new(-12, 1);
    int lang_index = e->index_of_current_value;

    if(sprite_animation_exists(FLAG_ICON_SPRITE_NAME, UNKNOWN_FLAG)) {
        const animation_t* anim = sprite_get_animation(FLAG_ICON_SPRITE_NAME, UNKNOWN_FLAG);
        const char* lang_id = languages.id_list[lang_index];
        char const* const* anim_id_property = animation_user_property(anim, lang_id);

        if(anim_id_property != NULL) {
            int anim_id = atoi(*anim_id_property);
            if(sprite_animation_exists(FLAG_ICON_SPRITE_NAME, anim_id))
                anim = sprite_get_animation(FLAG_ICON_SPRITE_NAME, anim_id);
        }

        const image_t* img = animation_image(anim, 0);
        int flag_width = image_width(img);
        flag_offset.x = -flag_width / 2 - 4;

        actor_change_animation(flag_icon, anim);
        flag_icon->visible = true;
    }
    else {
        flag_icon->visible = false;
    }

    font_t* f = e->value;
    font_set_text(f, "%s", languages.name_list[e->index_of_current_value]);
    v2d_t font_size = font_get_textsize(f);

    float flag_xpos = font_get_position(f).x - font_size.x + flag_offset.x;
    float flag_ypos = e->position.y + font_size.y * 0.5f + flag_offset.y;
    flag_icon->position = v2d_new(flag_xpos, flag_ypos);
}


/*
 * Credits
 */

void enter_credits(settings_entry_t* e)
{
    static bool from_options_screen = true;

    next_scene = storyboard_get_scene(SCENE_CREDITS);
    next_scene_arg = &from_options_screen;

    fade_out = true;
    save_preferences();
}


/*
 * Stage select
 */

void enter_stageselect(settings_entry_t* e)
{
    static bool want_developer_mode = false;

    next_scene = storyboard_get_scene(SCENE_STAGESELECT);
    next_scene_arg = &want_developer_mode;

    fade_out = true;
    save_preferences();
}

void highlight_stageselect(settings_entry_t* e)
{
    int* counter = e->data;

    *counter = 0;
}

void update_stageselect(settings_entry_t* e)
{
    int* counter = e->data;

    /* Skip if not highlighted */
    if(e != setting[index_of_highlighted_setting])
        return;

    /* Developer mode trick */
    if(enable_developermode)
        return;

    if(input_button_pressed(input, IB_LEFT) || input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
        *counter = 0;
        return;
    }

    if(input_button_pressed(input, IB_RIGHT)) {
        if(++(*counter) == 3) {
            sound_play(SFX_SECRET);
            enable_developermode = true;
            rebuild_entries();
        }
    }
}

void init_stageselect(settings_entry_t* e)
{
    e->data = mallocx(sizeof(int));
    memset(e->data, 0, sizeof(int));
}

void release_stageselect(settings_entry_t* e)
{
    free(e->data);
}



/*
 * Developer mode
 */

void enter_developermode(settings_entry_t* e)
{
    static bool want_developer_mode = true;

    next_scene = storyboard_get_scene(SCENE_STAGESELECT);
    next_scene_arg = &want_developer_mode;

    fade_out = true;
    save_preferences();
}

bool display_developermode(settings_entry_t* e)
{
    return enable_developermode;
}


/*
 * Show engine information
 */

void show_info(settings_entry_t* e)
{
    next_scene = storyboard_get_scene(SCENE_INFO);
    next_scene_arg = NULL;

    fade_out = true;
    save_preferences();
}


/*
 * Visit the website of the engine
 */

void download(settings_entry_t* e)
{
#if IS_MOBILE_PLATFORM
    launch_url(create_url("/download?type=desktop"));
#else
    launch_url(create_url("/download?type=mobile"));
#endif

    (void)e;
}


/*
 * Submit feedback
 */

void submitfeedback(settings_entry_t* e)
{
#if defined(__ANDROID__)
    if(is_base_game()) {
        char app_id[64];
        application_id(app_id, sizeof(app_id));
        if(0 == strcmp(app_id, "org.opensurge2d.surgeengine") && NULL != strstr(GAME_VERSION_STRING, "googleplay")) {
            launch_url("https://play.google.com/store/apps/details?id=org.opensurge2d.surgeengine");
            return;
        }
    }
#endif

    launch_url(create_url("/feedback"));
    (void)e;
}

bool display_submitfeedback(settings_entry_t* e)
{
    (void)e;
    return is_base_game();
}


/*
 * Report an issue
 */

void reportissue(settings_entry_t* e)
{
    launch_url(create_url("/issues"));
    (void)e;
}

bool display_reportissue(settings_entry_t* e)
{
    (void)e;
    return is_base_game();
}


/*
 * Share the engine
 */

void share(settings_entry_t* e)
{
#if defined(__ANDROID__)
    const char* text = GAME_TITLE " " GAME_WEBSITE;

    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "shareText", "(Ljava/lang/String;)V");

    jstring jtext = (*env)->NewStringUTF(env, text);
    (*env)->CallVoidMethod(env, activity, method_id, jtext);
    (*env)->DeleteLocalRef(env, jtext);

    (*env)->DeleteLocalRef(env, class_id);

    (void)e;
#else
    launch_url(create_url("/share"));
    (void)e;
#endif
}


/*
 * MODs
 */

bool display_mods(settings_entry_t* e)
{
    return display_mods_from_base_game(e) || display_mods_from_mod(e);
}

bool display_mods_from_base_game(settings_entry_t* e)
{
#if WANT_PLAYMOD
    /* only Surge the Rabbit can reliably be expected to have the correct
       scripts for a proper compatibility experience. Please don't change it!

       TODO an additional integrity check would be more robust.
       maybe embed the original scripts into the executable? */
    if(!is_base_game())
        return false;

    /* no custom gamedir */
    if(asset_gamedir() != NULL)
        return false;

    return true;
#else
    return false;
#endif
}

bool display_mods_from_mod(settings_entry_t* e)
{
#if WANT_PLAYMOD
    return asset_gamedir() != NULL;
#else
    return false;
#endif
}

void enter_playgame(settings_entry_t* e)
{
    filechooser_data_t* f = (filechooser_data_t*)e->data;
    f->want_chooser_of_folders = !want_zipped_mods;

#if defined(__ANDROID__)
    /* this prevents, in SCENE_MODLOADER, two deadlocks that may happen due to
       immersive mode changes that trigger display resize events:

       a) when popping the exit scene, just before restarting the engine OR
       b) before showing an error message box

       FIXME think of a more elegant way of handling this
             freeze the immersive mode? */
    video_set_immersive(true);
#endif

    /* create and start a new thread
       only one file chooser must be active at any given time */
    if(f->thread != NULL)
        al_destroy_thread(f->thread); /* blocking */
    f->thread = al_create_thread(filechooser_run, f);
    al_start_thread(f->thread);

#if !defined(__ANDROID__)
    /* wait for the file chooser */
    al_join_thread(f->thread, NULL);
#else
    /* Android: need to handle ALLEGRO_EVENT_DISPLAY_HALT_DRAWING
       before the blocking call to the file chooser returns

       no need to wait for the file chooser with al_join_thread()
       because the Activity will be paused */
#endif
}

void init_playgame(settings_entry_t* e)
{
    filechooser_data_t* f = mallocx(sizeof *f);

    al_init_user_event_source(&f->event_source);
    engine_add_event_source(&f->event_source);
    engine_add_event_listener(EVENT_FILECHOOSER_ASYNC, f, filechooser_handle_event);

    f->file_chooser[0] = al_create_native_file_dialog(
        NULL, "Select a game",
#if defined(__ANDROID__)
        "application/zip;application/x-7z-compressed",
#else
        "*.zip;*.7z",
#endif
        ALLEGRO_FILECHOOSER_FILE_MUST_EXIST
    );

    f->file_chooser[1] = al_create_native_file_dialog(
        NULL, "Select a game", "",
        ALLEGRO_FILECHOOSER_FILE_MUST_EXIST | ALLEGRO_FILECHOOSER_FOLDER
    );

    f->thread = NULL;
    f->want_chooser_of_folders = false;

    e->data = f;
}

void release_playgame(settings_entry_t* e)
{
    filechooser_data_t* f = (filechooser_data_t*)e->data;

    if(f->thread != NULL)
        al_destroy_thread(f->thread); /* blocking */

    if(f->file_chooser[1] != NULL)
        al_destroy_native_file_dialog(f->file_chooser[1]);

    if(f->file_chooser[0] != NULL)
        al_destroy_native_file_dialog(f->file_chooser[0]);

    engine_remove_event_listener(EVENT_FILECHOOSER_ASYNC, f, filechooser_handle_event);
    engine_remove_event_source(&f->event_source);
    al_destroy_user_event_source(&f->event_source);

    free(f);
}

void change_modstorage(settings_entry_t* e)
{
#if IS_MOBILE_PLATFORM
    /* unsupported */
    if(e->index_of_current_value != 0) {
        e->index_of_current_value = 0;
        sound_play(SFX_DENY);
    }
#endif

    want_zipped_mods = (e->index_of_current_value == 0);
}

void change_compatibilitymode(settings_entry_t* e)
{
    want_compatibility_mode = (e->index_of_current_value != 0);

    /* warn the user if the compatibility mode is disabled */
    if(!want_compatibility_mode) {
        sound_play(SFX_QUESTION);

        /* Suggestion: use an in-game widget instead of a native message box */
        if(!confirm("%s", lang_get("OPTIONS_PLAYMOD_COMPATWARN"))) {
            want_compatibility_mode = true;
            e->index_of_current_value = 1;
            sound_play(SFX_BACK);
        }
        else
            sound_play(SFX_CONFIRM);
    }
}

void enter_backtobasegame(settings_entry_t* e)
{
    commandline_t cmd = commandline_parse(0, NULL);
    cmd.mobile = (IS_MOBILE_PLATFORM || in_mobile_mode()) ? TRUE : FALSE;

    save_preferences();
    engine_restart(&cmd);
}


/*
 * File Chooser
 */

void* filechooser_run(ALLEGRO_THREAD* thread, void* arg)
{
    filechooser_data_t* f = (filechooser_data_t*)arg;

    /* select the file chooser */
    ALLEGRO_FILECHOOSER* file_chooser = f->file_chooser[f->want_chooser_of_folders ? 1 : 0];

    if(file_chooser == NULL) {
        /* error */
        video_showmessage("Can't create native dialog");
        return NULL;
    }

    /* show the file chooser (this call is blocking) */
    if(!al_show_native_file_dialog(NULL, file_chooser)) {
        /* error */
        video_showmessage("Can't show native dialog");
        return NULL;
    }

    /* emit an event */
    ALLEGRO_EVENT event;
    event.type = EVENT_FILECHOOSER_ASYNC;

    if(!al_emit_user_event(&f->event_source, &event, NULL))
        video_showmessage("Can't emit event");

    /* done! */
    return NULL;
}

void filechooser_handle_event(const ALLEGRO_EVENT* event, void* arg)
{
    const filechooser_data_t* f = (const filechooser_data_t*)arg;
    const char* path_to_game = NULL;

    if(event->type != EVENT_FILECHOOSER_ASYNC)
        return;

    /* get the result of the file chooser */
    ALLEGRO_FILECHOOSER* file_chooser = f->file_chooser[f->want_chooser_of_folders ? 1 : 0];

    if(file_chooser == NULL) {
        /* error */
        logfile_message("Play a game: there was an error with the file chooser");
        video_showmessage("File Chooser ERROR!");
        sound_play(SFX_DENY);
        return;
    }
    else if(0 == al_get_native_file_dialog_count(file_chooser)) {
        /* cancelled */
        logfile_message("Play a game: the file chooser was cancelled");
        sound_play(SFX_BACK);
        return;
    }
    else {
        /* success */
        path_to_game = al_get_native_file_dialog_path(file_chooser, 0);
        logfile_message("Play a game: selected \"%s\"", path_to_game);
    }

#if defined(__ANDROID__)
    /* path_to_game is a content:// URI */
    if(path_to_game != NULL && *path_to_game != '\0')
        assertx(want_zipped_mods); /* folders are not supported */
#endif

    /* load the game */
    if(path_to_game != NULL && *path_to_game != '\0') {
        commandline_t cmd = commandline_parse(0, NULL);

        str_cpy(cmd.gamedir, path_to_game, sizeof(cmd.gamedir));
        cmd.compatibility_mode = want_compatibility_mode ? TRUE : FALSE;
        cmd.mobile = (IS_MOBILE_PLATFORM || in_mobile_mode()) ? TRUE : FALSE;

        save_preferences();
        scenestack_push(storyboard_get_scene(SCENE_MODLOADER), &cmd);
    }
}




/*
 *
 * Android-specific
 *
 */

#if defined(__ANDROID__)

/* get the application ID */
char* application_id(char* buffer, size_t buffer_size)
{
    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "getPackageName", "()Ljava/lang/String;");

    jstring jresult = (*env)->CallObjectMethod(env, activity, method_id);
    jboolean is_null_result = (*env)->IsSameObject(env, jresult, NULL);
    if(!is_null_result) {
        const char* str = (*env)->GetStringUTFChars(env, jresult, NULL);
        str_cpy(buffer, str, buffer_size);
        (*env)->ReleaseStringUTFChars(env, jresult, str);
    }
    else if(buffer_size > 0)
        buffer[0] = '\0';

    (*env)->DeleteLocalRef(env, jresult);
    (*env)->DeleteLocalRef(env, class_id);

    return buffer;
}

#endif
