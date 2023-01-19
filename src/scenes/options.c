/*
 * Open Surge Engine
 * options.c - options screen
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

#include <math.h>
#include <stdbool.h>
#include "options.h"
#include "util/grouptree.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/logfile.h"
#include "../core/font.h"
#include "../core/prefs.h"
#include "../core/web.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/sfx.h"
#include "stageselect.h"


/* public data */
const char *OPTIONS_MUSICFILE = "musics/options.ogg";

/* private data */
static const char *OPTIONS_BGFILE = "themes/scenes/options.bg";
static bool quit, fadein;
static font_t *title;
static actor_t *icon;
static input_t *input;
static scene_t *jump_to;
static float scene_time;
static bgtheme_t *bgtheme;
static bool stageselect_enable_debug;
static music_t* music;
static const int OFFSET_X = 60;

/* private methods */
static void save_preferences();
static void open_donate_page();

/* group tree */
static int option; /* current option: 0 <= option <= option_count - 1 */
static int option_count;
static group_t *root;
static group_t *create_grouptree();

/* deprecated? */
#define ENABLE_RESOLUTION 0




/* public functions */

/*
 * options_init()
 * Initializes the scene
 */
void options_init(void *foo)
{
    option = 0;
    option_count = 0;
    quit = false;
    scene_time = 0;
    input = input_create_user(NULL);
    jump_to = NULL;
    fadein = true;
    music = music_load(OPTIONS_MUSICFILE);

    stageselect_enable_debug = false;

    title = font_create("MenuTitle");
    font_set_text(title, "%s", "$OPTIONS_TITLE");
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 10));
    font_set_align(title, FONTALIGN_CENTER);

    bgtheme = background_load(OPTIONS_BGFILE);

    icon = actor_create();
    actor_change_animation(icon, sprite_get_animation("UI Pointer", 0));
    icon->position = v2d_new(-50,-50);

    root = create_grouptree();
    grouptree_init_all(root);
}


/*
 * options_release()
 * Releases the scene
 */
void options_release()
{
    grouptree_release_all(root);
    grouptree_destroy_all(root);

    bgtheme = background_unload(bgtheme);

    actor_destroy(icon);
    font_destroy(title);
    input_destroy(input);
    music_unref(music);
}


/*
 * options_update()
 * Updates the scene
 */
void options_update()
{
    float dt = timer_get_delta();
    scene_time += dt;

    /* title */
    font_set_text(title, "%s", "$OPTIONS_TITLE");

    /* fade in */
    if(fadein) {
        fadefx_in(color_rgb(0,0,0), 1.0);
        fadein = false;
    }

    /* background movement */
    background_update(bgtheme);

    /* menu option */
    if(!quit && jump_to == NULL && !fadefx_is_fading()) {
        /* select next option */
        if(input_button_pressed(input, IB_DOWN)) {
            option = (option + 1) % option_count;
            sound_play(SFX_CHOOSE);
        }

        /* select previous option */
        if(input_button_pressed(input, IB_UP)) {
            option = (option + (option_count - 1)) % option_count;
            sound_play(SFX_CHOOSE);
        }

        /* go back... */
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play(SFX_BACK);
            quit = true;
        }
    }

    /* updating the group tree */
    grouptree_update_all(root);

    /* music */
    if(quit) {
        if(!fadefx_is_fading())
            music_stop();
    }
    else if(!music_is_playing() && scene_time >= 0.2)
        music_play(music, true);

    /* quit */
    if(quit) {
        if(fadefx_is_over()) {
            save_preferences();
            scenestack_pop();
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0);
    }

    /* pushing a scene into the stack */
    if(jump_to != NULL) {
        if(fadefx_is_over()) {
            save_preferences();

            if(jump_to == storyboard_get_scene(SCENE_STAGESELECT)) {
                scenestack_push(jump_to, &stageselect_enable_debug);
                stageselect_enable_debug = false;
            }
            else {
                bool from_options_screen = true;
                scenestack_push(jump_to, &from_options_screen);
            }

            jump_to = NULL;
            fadein = true;
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0);
    }
}



/*
 * options_render()
 * Renders the scene
 */
void options_render()
{
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    font_render(title, cam);
    grouptree_render_all(root, cam);
    actor_render(icon, cam);
}






/* private methods */

/* saves the user preferences */
void save_preferences()
{
    extern prefs_t* prefs;

#if ENABLE_RESOLUTION
    prefs_set_int(prefs, ".resolution", video_get_resolution());
#endif
    prefs_set_bool(prefs, ".fullscreen", video_is_fullscreen());
    prefs_set_bool(prefs, ".showfps", video_is_fps_visible());
}

/* opens a donate page */
void open_donate_page()
{
    const char* donate_url = "http://opensurge2d.org/contribute";
    char url[128];
    
    snprintf(url, sizeof(url),
        "%s?v=%s&lang=%s",
        donate_url, GAME_VERSION_STRING, lang_get("LANG_ID")
    );
    launch_url(url);
}




/* --------------------------------------- */
/* group tree programming: derived classes */
/* --------------------------------------- */


/* <<abstract>> Fixed label */
static void group_fixedlabel_init(group_t *g, char *lang_key)
{
    group_label_init(g);
    g->data = mallocx(256 * sizeof(char));
    str_cpy((char*)(g->data), lang_key, 256);
    font_set_text(g->font, "%s", lang_get(lang_key));
}

static void group_fixedlabel_release(group_t *g)
{
    free((char*)(g->data));
    group_label_release(g);
}

static void group_fixedlabel_update(group_t *g)
{
    group_label_update(g);
    font_set_text(g->font, "%s", lang_get((char*)(g->data)));
}

static void group_fixedlabel_render(group_t *g, v2d_t camera_position)
{
    group_label_render(g, camera_position);
}


/* <<abstract>> Highlightable label */
typedef struct group_highlightable_data_t {
    int option_index;
    char lang_key[256];
} group_highlightable_data_t;

static void group_highlightable_init(group_t *g, char *lang_key, int option_index)
{
    group_highlightable_data_t *data;

    group_label_init(g);
    font_set_text(g->font, "%s", lang_get(lang_key));

    g->data = mallocx(sizeof(group_highlightable_data_t));
    data = (group_highlightable_data_t*)(g->data);
    data->option_index = option_index;
    str_cpy(data->lang_key, lang_key, sizeof(data->lang_key));
}

static void group_highlightable_release(group_t *g)
{
    free((group_highlightable_data_t*)(g->data));
    group_label_release(g);
}

static int group_highlightable_is_highlighted(group_t *g) /* "inheritance" in C */
{
    group_highlightable_data_t *data = (group_highlightable_data_t*)(g->data);
    return (option == data->option_index);
}

static void group_highlightable_update(group_t *g)
{
    group_highlightable_data_t *data = (group_highlightable_data_t*)(g->data);

    group_label_update(g);
    font_set_text(g->font, "%s", lang_get(data->lang_key));
    if(group_highlightable_is_highlighted(g)) {
        font_set_text(g->font, "<color=$COLOR_HIGHLIGHT>%s</color>", lang_get(data->lang_key));
        icon->position = v2d_add(font_get_position(g->font), v2d_new(-20 + 3 * cosf(TWO_PI * scene_time), 0));
    }
}

static void group_highlightable_render(group_t *g, v2d_t camera_position)
{
    group_label_render(g, camera_position);
}


/* -------------------------- */


/* Root node */
static void group_root_init(group_t *g)
{
    group_label_init(g);
    font_set_text(g->font, "");
    font_set_position(g->font, v2d_new(OFFSET_X, 25));
}

static void group_root_release(group_t *g)
{
    group_label_release(g);
}

static void group_root_update(group_t *g)
{
    group_label_update(g);
}

static void group_root_render(group_t *g, v2d_t camera_position)
{
    group_label_render(g, camera_position);
}

static group_t *group_root_create()
{
    return group_create(group_root_init, group_root_release, group_root_update, group_root_render);
}


/* "Graphics" label */
static void group_graphics_init(group_t *g)
{
    group_fixedlabel_init(g, "OPTIONS_GRAPHICS");
}

static void group_graphics_release(group_t *g)
{
    group_fixedlabel_release(g);
}

static void group_graphics_update(group_t *g)
{
    group_fixedlabel_update(g);
}

static void group_graphics_render(group_t *g, v2d_t camera_position)
{
    group_fixedlabel_render(g, camera_position);
}

static group_t *group_graphics_create()
{
    return group_create(group_graphics_init, group_graphics_release, group_graphics_update, group_graphics_render);
}


/* "Fullscreen" label */
static void group_fullscreen_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_FULLSCREEN", option_count++);
}

static void group_fullscreen_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_fullscreen_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_fullscreen_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_fullscreen_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                video_set_fullscreen(!video_is_fullscreen());
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                if(video_is_fullscreen()) {
                    sound_play(SFX_CONFIRM);
                    video_set_fullscreen(false);
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(!video_is_fullscreen()) {
                    sound_play(SFX_CONFIRM);
                    video_set_fullscreen(true);
                }
            }
        }
    }
}

static void group_fullscreen_render(group_t *g, v2d_t camera_position)
{
    font_t *f;
    char v[2][80];

    /* base class */
    group_highlightable_render(g, camera_position);

    /* derived class */
    f = font_create("MenuText");
    font_set_position(f, v2d_new(OFFSET_X + 175, font_get_position(g->font).y));

    str_cpy(v[0], lang_get("OPTIONS_YES"), sizeof(v[0]));
    str_cpy(v[1], lang_get("OPTIONS_NO"), sizeof(v[1]));

    if(video_is_fullscreen())
        font_set_text(f, "<color=$COLOR_HIGHLIGHT>%s</color>  %s", v[0], v[1]);
    else
        font_set_text(f, "%s  <color=$COLOR_HIGHLIGHT>%s</color>", v[0], v[1]);

    font_render(f, camera_position);
    font_destroy(f);
}

static group_t *group_fullscreen_create()
{
    return group_create(group_fullscreen_init, group_fullscreen_release, group_fullscreen_update, group_fullscreen_render);
}


/* "Show FPS" label */
static void group_fps_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_FPS", option_count++);
}

static void group_fps_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_fps_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_fps_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_fps_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                video_set_fps_visible(!video_is_fps_visible());
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                if(video_is_fps_visible()) {
                    sound_play(SFX_CONFIRM);
                    video_set_fps_visible(false);
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(!video_is_fps_visible()) {
                    sound_play(SFX_CONFIRM);
                    video_set_fps_visible(true);
                }
            }
        }
    }
}

static void group_fps_render(group_t *g, v2d_t camera_position)
{
    font_t *f;
    char v[2][80];

    /* base class */
    group_highlightable_render(g, camera_position);

    /* derived class */
    f = font_create("MenuText");
    font_set_position(f, v2d_new(OFFSET_X + 175, font_get_position(g->font).y));

    str_cpy(v[0], lang_get("OPTIONS_YES"), sizeof(v[0]));
    str_cpy(v[1], lang_get("OPTIONS_NO"), sizeof(v[1]));

    if(video_is_fps_visible())
        font_set_text(f, "<color=$COLOR_HIGHLIGHT>%s</color>  %s", v[0], v[1]);
    else
        font_set_text(f, "%s  <color=$COLOR_HIGHLIGHT>%s</color>", v[0], v[1]);

    font_render(f, camera_position);
    font_destroy(f);
}

static group_t *group_fps_create()
{
    return group_create(group_fps_init, group_fps_release, group_fps_update, group_fps_render);
}


/* "Resolution" label */
static void group_resolution_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_RESOLUTION", option_count++);
}

static void group_resolution_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_resolution_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_resolution_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_resolution_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                switch(video_get_resolution()) {
                    case VIDEORESOLUTION_1X:
                        video_set_resolution(VIDEORESOLUTION_2X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_2X:
                        video_set_resolution(VIDEORESOLUTION_3X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_3X:
                        video_set_resolution(VIDEORESOLUTION_4X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_4X:
                        video_set_resolution(VIDEORESOLUTION_1X);
                        sound_play(SFX_CONFIRM);
                        break;

                    default:
                        break;
                }
            }

            if(input_button_pressed(input, IB_RIGHT)) {
                switch(video_get_resolution()) {
                    case VIDEORESOLUTION_1X:
                        video_set_resolution(VIDEORESOLUTION_2X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_2X:
                        video_set_resolution(VIDEORESOLUTION_3X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_3X:
                        video_set_resolution(VIDEORESOLUTION_4X);
                        sound_play(SFX_CONFIRM);
                        break;

                    default:
                        break;
                }
            }

            if(input_button_pressed(input, IB_LEFT)) {
                switch(video_get_resolution()) {
                    case VIDEORESOLUTION_4X:
                        video_set_resolution(VIDEORESOLUTION_3X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_3X:
                        video_set_resolution(VIDEORESOLUTION_2X);
                        sound_play(SFX_CONFIRM);
                        break;

                    case VIDEORESOLUTION_2X:
                        video_set_resolution(VIDEORESOLUTION_1X);
                        sound_play(SFX_CONFIRM);
                        break;

                    default:
                        break;
                }
            }
        }
    }
}

static void group_resolution_render(group_t *g, v2d_t camera_position)
{
    font_t *f;
    char v[4][80];

    /* base class */
    group_highlightable_render(g, camera_position);

    /* derived class */
    f = font_create("MenuText");
    font_set_position(f, v2d_new(OFFSET_X + 175, font_get_position(g->font).y));

    str_cpy(v[0], lang_get("OPTIONS_RESOLUTION_OPT1"), sizeof(v[0]));
    str_cpy(v[1], lang_get("OPTIONS_RESOLUTION_OPT2"), sizeof(v[1]));
    str_cpy(v[2], lang_get("OPTIONS_RESOLUTION_OPT3"), sizeof(v[2]));
    str_cpy(v[3], lang_get("OPTIONS_RESOLUTION_OPT4"), sizeof(v[3]));

    switch(video_get_resolution()) {
        case VIDEORESOLUTION_1X:
            font_set_text(f, "<color=$COLOR_HIGHLIGHT>%s</color> %s %s %s", v[0], v[1], v[2], v[3]);
            break;

        case VIDEORESOLUTION_2X:
            font_set_text(f, "%s <color=$COLOR_HIGHLIGHT>%s</color> %s %s", v[0], v[1], v[2], v[3]);
            break;

        case VIDEORESOLUTION_3X:
            font_set_text(f, "%s %s <color=$COLOR_HIGHLIGHT>%s</color> %s", v[0], v[1], v[2], v[3]);
            break;

        case VIDEORESOLUTION_4X:
            font_set_text(f, "%s %s %s <color=$COLOR_HIGHLIGHT>%s</color>", v[0], v[1], v[2], v[3]);
            break;
            
        default:
            break;
    }

    font_render(f, camera_position);
    font_destroy(f);
}

static group_t *group_resolution_create()
{
    return group_create(group_resolution_init, group_resolution_release, group_resolution_update, group_resolution_render);
}

/* "Game" label */
static void group_game_init(group_t *g)
{
    group_fixedlabel_init(g, "OPTIONS_GAME");
}

static void group_game_release(group_t *g)
{
    group_fixedlabel_release(g);
}

static void group_game_update(group_t *g)
{
    group_fixedlabel_update(g);
}

static void group_game_render(group_t *g, v2d_t camera_position)
{
    group_fixedlabel_render(g, camera_position);
}

static group_t *group_game_create()
{
    return group_create(group_game_init, group_game_release, group_game_update, group_game_render);
}

/* "Change Language" label */
static void group_changelanguage_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_LANGUAGE", option_count++);
}

static void group_changelanguage_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_changelanguage_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_changelanguage_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_changelanguage_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                jump_to = storyboard_get_scene(SCENE_LANGSELECT);
            }
        }
    }
}

static void group_changelanguage_render(group_t *g, v2d_t camera_position)
{
    group_highlightable_render(g, camera_position);
}

static group_t *group_changelanguage_create()
{
    return group_create(group_changelanguage_init, group_changelanguage_release, group_changelanguage_update, group_changelanguage_render);
}

/* "Credits" label */
static void group_credits_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_CREDITS", option_count++);
}

static void group_credits_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_credits_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_credits_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_credits_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                jump_to = storyboard_get_scene(SCENE_CREDITS);
            }
        }
    }
}

static void group_credits_render(group_t *g, v2d_t camera_position)
{
    group_highlightable_render(g, camera_position);
}

static group_t *group_credits_create()
{
    return group_create(group_credits_init, group_credits_release, group_credits_update, group_credits_render);
}

/* "Donate" label */
static void group_donate_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_DONATE", option_count++);
}

static void group_donate_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_donate_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_donate_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_donate_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                open_donate_page();
                quit = true;
            }
        }
    }
}

static void group_donate_render(group_t *g, v2d_t camera_position)
{
    group_highlightable_render(g, camera_position);
}

static group_t *group_donate_create()
{
    return group_create(group_donate_init, group_donate_release, group_donate_update, group_donate_render);
}

/* "Stage Select" label */
static void group_stageselect_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_STAGESELECT", option_count++);
}

static void group_stageselect_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_stageselect_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_stageselect_update(group_t *g)
{
    static int cnt = 0;

    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_stageselect_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                jump_to = storyboard_get_scene(SCENE_STAGESELECT);
                cnt = 0;
            }
            else if(input_button_pressed(input, IB_RIGHT)) { /* stage select: debug mode trick */
                if(cnt >= 0 && ++cnt == 3) {
                    sound_play(SFX_SECRET);
                    stageselect_enable_debug = true;
                    cnt = -1;
                }
            }
            else if(input_button_pressed(input, IB_UP) || input_button_pressed(input, IB_DOWN)) {
                cnt = min(cnt, 0);
            }
        }
    }
}

static void group_stageselect_render(group_t *g, v2d_t camera_position)
{
    group_highlightable_render(g, camera_position);
}

static group_t *group_stageselect_create()
{
    return group_create(group_stageselect_init, group_stageselect_release, group_stageselect_update, group_stageselect_render);
}

/* "Back" label */
static void group_back_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_BACK", option_count++);
}

static void group_back_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_back_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_back_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_back_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                quit = true;
            }
        }
    }
}

static void group_back_render(group_t *g, v2d_t camera_position)
{
    group_highlightable_render(g, camera_position);
}

static group_t *group_back_create()
{
    return group_create(group_back_init, group_back_release, group_back_update, group_back_render);
}


/* "Enable Gamepad" label */
static void group_gamepad_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_GAMEPAD", option_count++);
}

static void group_gamepad_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_gamepad_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_gamepad_update(group_t *g)
{
    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_gamepad_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                input_ignore_joystick(!input_is_joystick_ignored());
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                if(!input_is_joystick_ignored()) {
                    sound_play(SFX_CONFIRM);
                    input_ignore_joystick(true);
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(input_is_joystick_ignored()) {
                    sound_play(SFX_CONFIRM);
                    input_ignore_joystick(false);
                }
            }
        }
    }
}

static void group_gamepad_render(group_t *g, v2d_t camera_position)
{
    font_t *f;
    char v[2][80];

    /* base class */
    group_highlightable_render(g, camera_position);

    /* derived class */
    f = font_create("MenuText");
    font_set_position(f, v2d_new(OFFSET_X + 175, font_get_position(g->font).y));

    str_cpy(v[0], lang_get("OPTIONS_YES"), sizeof(v[0]));
    str_cpy(v[1], lang_get("OPTIONS_NO"), sizeof(v[1]));

    if(!input_is_joystick_ignored())
        font_set_text(f, "<color=$COLOR_HIGHLIGHT>%s</color>  %s", v[0], v[1]);
    else
        font_set_text(f, "%s  <color=$COLOR_HIGHLIGHT>%s</color>", v[0], v[1]);

    font_render(f, camera_position);
    font_destroy(f);
}

static group_t *group_gamepad_create()
{
    return group_create(group_gamepad_init, group_gamepad_release, group_gamepad_update, group_gamepad_render);
}





/* ----------------------------------------- */
/* group tree programming: creating the tree */
/* ----------------------------------------- */

/* creates the group tree */
group_t *create_grouptree()
{
    group_t *root, *graphics, *game;

    /* section: graphics */
    graphics = group_graphics_create();
#if ENABLE_RESOLUTION
    group_addchild(graphics, group_resolution_create());
#else
    (void)group_resolution_create;
#endif
    group_addchild(graphics, group_fullscreen_create());
    group_addchild(graphics, group_fps_create());

    /* section: game */
    game = group_game_create();
    group_addchild(game, group_gamepad_create());
    group_addchild(game, group_stageselect_create());
    group_addchild(game, group_changelanguage_create());
    group_addchild(game, group_credits_create());
    group_addchild(game, group_donate_create());

    /* section: root */
    root = group_root_create();
    group_addchild(root, graphics);
    group_addchild(root, game);
    group_addchild(root, group_back_create());

    /* done! */
    return root;
}

