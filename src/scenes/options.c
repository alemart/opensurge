/*
 * Open Surge Engine
 * options.c - options screen
 * Copyright (C) 2010-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "../core/soundfactory.h"
#include "../core/font.h"
#include "../core/prefs.h"
#include "../core/modmanager.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "stageselect.h"


/* private data */
#define OPTIONS_BGFILE                  "themes/menus/options.bg"
static int quit, fadein;
static font_t *title;
static actor_t *icon;
static input_t *input;
static scene_t *jump_to;
static float scene_time;
static bgtheme_t *bgtheme;
static int stageselect_enable_debug;
static music_t* music;
static const int OFFSET_X = 60;

/* private methods */
static void save_preferences();

/* group tree */
static int option; /* current option: 0 <= option <= option_count - 1 */
static int option_count;
static group_t *root;
static group_t *create_grouptree();





/* public functions */

/*
 * options_init()
 * Initializes the scene
 */
void options_init(void *foo)
{
    option = 0;
    option_count = 0;
    quit = FALSE;
    scene_time = 0;
    input = input_create_user(NULL);
    jump_to = NULL;
    fadein = TRUE;
    music = music_load(OPTIONS_MUSICFILE);

    stageselect_enable_debug = FALSE;

    title = font_create("menu.title");
    font_set_text(title, "%s", "$OPTIONS_TITLE");
    font_set_position(title, v2d_new((VIDEO_SCREEN_W - font_get_textsize(title).x)/2, 10));

    bgtheme = background_load(OPTIONS_BGFILE);

    icon = actor_create();
    actor_change_animation(icon, sprite_get_animation("SD_GUIARROW", 0));
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
        fadein = FALSE;
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
            quit = TRUE;
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
        music_play(music, TRUE);

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
                stageselect_enable_debug = FALSE;
            }
            else
                scenestack_push(jump_to, NULL);

            jump_to = NULL;
            fadein = TRUE;
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
    prefs_t* prefs = modmanager_prefs();
    prefs_set_int(prefs, ".resolution", video_get_resolution());
    prefs_set_bool(prefs, ".fullscreen", video_is_fullscreen());
    prefs_set_bool(prefs, ".smoothgfx", video_is_smooth());
    prefs_set_bool(prefs, ".showfps", video_is_fps_visible());
    prefs_set_bool(prefs, ".gamepad", !input_is_joystick_ignored());
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
        icon->position = v2d_add(font_get_position(g->font), v2d_new(-20+3*cos(2*PI*scene_time),0));
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
                video_changemode(video_get_resolution(), video_is_smooth(), !video_is_fullscreen());
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                if(video_is_fullscreen()) {
                    sound_play(SFX_CONFIRM);
                    video_changemode(video_get_resolution(), video_is_smooth(), FALSE);
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(!video_is_fullscreen()) {
                    sound_play(SFX_CONFIRM);
                    video_changemode(video_get_resolution(), video_is_smooth(), TRUE);
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
    f = font_create("menu.text");
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


/* "Smooth Graphics" label */
static void group_smooth_init(group_t *g)
{
    group_highlightable_init(g, "OPTIONS_SMOOTHGFX", option_count++);
}

static void group_smooth_release(group_t *g)
{
    group_highlightable_release(g);
}

static int group_smooth_is_highlighted(group_t *g)
{
    return group_highlightable_is_highlighted(g);
}

static void group_smooth_update(group_t *g)
{
    int resolution = (video_get_resolution() == VIDEORESOLUTION_1X) ? VIDEORESOLUTION_2X : video_get_resolution();

    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_smooth_is_highlighted(g) && video_get_color_depth() == 32) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                video_changemode(resolution, !video_is_smooth(), video_is_fullscreen());
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                if(video_is_smooth()) {
                    sound_play(SFX_CONFIRM);
                    video_changemode(resolution, FALSE, video_is_fullscreen());
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(!video_is_smooth()) {
                    sound_play(SFX_CONFIRM);
                    video_changemode(resolution, TRUE, video_is_fullscreen());
                }
            }
        }
    }
}

static void group_smooth_render(group_t *g, v2d_t camera_position)
{
    font_t *f;
    char v[2][80];

    /* base class */
    group_highlightable_render(g, camera_position);

    /* derived class */
    f = font_create("menu.text");
    font_set_position(f, v2d_new(OFFSET_X + 175, font_get_position(g->font).y));

    str_cpy(v[0], lang_get("OPTIONS_YES"), sizeof(v[0]));
    str_cpy(v[1], lang_get("OPTIONS_NO"), sizeof(v[1]));

    if(video_get_color_depth() == 32) {
        if(video_is_smooth())
            font_set_text(f, "<color=$COLOR_HIGHLIGHT>%s</color>  %s", v[0], v[1]);
        else
            font_set_text(f, "%s  <color=$COLOR_HIGHLIGHT>%s</color>", v[0], v[1]);
    }
    else
        font_set_text(f, "<color=ff8888>%s</color>", lang_get("OPTIONS_SMOOTHGFX_ERROR"));

    font_render(f, camera_position);
    font_destroy(f);
}

static group_t *group_smooth_create()
{
    return group_create(group_smooth_init, group_smooth_release, group_smooth_update, group_smooth_render);
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
                video_show_fps(!video_is_fps_visible());
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                if(video_is_fps_visible()) {
                    sound_play(SFX_CONFIRM);
                    video_show_fps(FALSE);
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(!video_is_fps_visible()) {
                    sound_play(SFX_CONFIRM);
                    video_show_fps(TRUE);
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
    f = font_create("menu.text");
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
                        video_changemode(VIDEORESOLUTION_2X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_2X:
                        video_changemode(VIDEORESOLUTION_3X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_3X:
                        video_changemode(VIDEORESOLUTION_4X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_4X:
                        video_changemode(VIDEORESOLUTION_1X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    default:
                        break;
                }
            }
            if(input_button_pressed(input, IB_RIGHT)) {
                switch(video_get_resolution()) {
                    case VIDEORESOLUTION_1X:
                        video_changemode(VIDEORESOLUTION_2X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_2X:
                        video_changemode(VIDEORESOLUTION_3X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_3X:
                        video_changemode(VIDEORESOLUTION_4X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    default:
                        break;
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                switch(video_get_resolution()) {
                    case VIDEORESOLUTION_4X:
                        video_changemode(VIDEORESOLUTION_3X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_3X:
                        video_changemode(VIDEORESOLUTION_2X, video_is_smooth(), video_is_fullscreen());
                        sound_play(SFX_CONFIRM);
                        break;
                    case VIDEORESOLUTION_2X:
                        video_changemode(VIDEORESOLUTION_1X, video_is_smooth(), video_is_fullscreen());
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
    f = font_create("menu.text");
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
    static int cnt = 0, cnt2 = 0;
    static scenetype_t scn = SCENE_STAGESELECT;

    /* base class */
    group_highlightable_update(g);

    /* derived class */
    if(group_stageselect_is_highlighted(g)) {
        if(!fadefx_is_fading()) {
            if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
                sound_play(SFX_CONFIRM);
                jump_to = storyboard_get_scene(scn);
                cnt = cnt2 = 0;
                scn = SCENE_STAGESELECT;
            }
            else if(input_button_pressed(input, IB_RIGHT)) { /* stage select: debug mode trick */
                if(cnt >= 0 && ++cnt == 3) {
                    sound_play(SFX_SECRET);
                    scn = SCENE_STAGESELECT;
                    stageselect_enable_debug = TRUE;
                    cnt = -1;
                }
            }
            else if(input_button_pressed(input, IB_LEFT)) { /* stage select: quest select trick */
                if(cnt2 >= 0 && ++cnt2 == 3) {
                    sound_play(SFX_SECRET);
                    scn = SCENE_QUESTSELECT;
                    cnt2 = -1;
                }
            }
            else if(input_button_pressed(input, IB_UP) || input_button_pressed(input, IB_DOWN)) {
                cnt = min(cnt, 0);
                cnt2 = min(cnt2, 0);
                scn = SCENE_STAGESELECT;
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
                quit = TRUE;
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
                    input_ignore_joystick(TRUE);
                }
            }
            if(input_button_pressed(input, IB_LEFT)) {
                if(input_is_joystick_ignored()) {
                    sound_play(SFX_CONFIRM);
                    input_ignore_joystick(FALSE);
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
    f = font_create("menu.text");
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
    group_addchild(graphics, group_resolution_create());
#if !defined(A5BUILD)
    group_addchild(graphics, group_fullscreen_create());
    group_addchild(graphics, group_smooth_create());
#endif
    group_addchild(graphics, group_fps_create());

    /* section: game */
    game = group_game_create();
    group_addchild(game, group_changelanguage_create());
#if !defined(A5BUILD)
    group_addchild(game, group_gamepad_create());
#endif
    group_addchild(game, group_stageselect_create());
    group_addchild(game, group_credits_create());

    /* section: root */
    root = group_root_create();
    group_addchild(root, graphics);
    group_addchild(root, game);
    group_addchild(root, group_back_create());

    /* done! */
    return root;
}

