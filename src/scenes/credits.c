/*
 * Open Surge Engine
 * credits.c - credits scene
 * Copyright (C) 2009-2012  Alexandre Martins <alemartf@gmail.com>
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
#include "credits.h"
#include "options.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/font.h"
#include "../core/assetfs.h"
#include "../core/logfile.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/sfx.h"


/* private data */
#define CREDITS_FILE               "config/credits.dat"
#define CREDITS_BGFILE             "themes/scenes/credits.bg"
static int quit;
static font_t *title, *text, *back;
static input_t *input;
static bgtheme_t *bgtheme;
static music_t *music;
static scene_t *next_scene;
static const int scroll_speed = 24;
static char* read_credits_file();




/* public functions */

/*
 * credits_init()
 * Initializes the scene
 */
void credits_init(void *foo)
{
    char *credits_text = read_credits_file();

    /* initializing stuff... */
    quit = FALSE;
    next_scene = NULL;
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);

    title = font_create("menu.title");
    font_set_text(title, "%s", lang_get("CREDITS_TITLE"));
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 5));
    font_set_align(title, FONTALIGN_CENTER);

    back = font_create("menu.text");
    font_set_text(back, "%s", lang_get("CREDITS_BACK"));
    font_set_position(back, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(back).y - 5));

    text = font_create("menu.text");
    font_set_text(text, "\n%s", credits_text);
    font_set_width(text, VIDEO_SCREEN_W - 20);
    font_set_position(text, v2d_new(10, VIDEO_SCREEN_H));

    bgtheme = background_load(CREDITS_BGFILE);

    fadefx_in(color_rgb(0,0,0), 1.0);

    /* done! */
    free(credits_text);
}


/*
 * credits_release()
 * Releases the scene
 */
void credits_release()
{
    bgtheme = background_unload(bgtheme);

    font_destroy(title);
    font_destroy(text);
    font_destroy(back);

    input_destroy(input);
    music_unref(music);
}


/*
 * credits_update()
 * Updates the scene
 */
void credits_update()
{
    float dt = timer_get_delta();
    v2d_t textpos;

    /* background movement */
    background_update(bgtheme);

    /* text movement */
    textpos = font_get_position(text);
    textpos.y -= scroll_speed * dt;
    if(textpos.y < -font_get_textsize(text).y)
        textpos.y = VIDEO_SCREEN_H;
    font_set_position(text, textpos);

    /* quit */
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play(SFX_BACK);
            next_scene = NULL;
            quit = TRUE;
        }
    }

    /* music */
    if(!music_is_playing())
        music_play(music, true);

    /* fade-out */
    if(quit) {
        if(fadefx_is_over()) {
            scenestack_pop();
            if(next_scene != NULL)
                scenestack_push(next_scene, NULL);
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0);
    }
}



/*
 * credits_render()
 * Renders the scene
 */
void credits_render()
{
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    font_render(text, cam);
    background_render_fg(bgtheme, cam);
    font_render(title, cam);
    font_render(back, cam);
}







/* private stuff */

/* reads the contents of CREDITS_FILE */
char* read_credits_file()
{
    const char* fullpath;
    char *buf;
    FILE* fp;
    long size;

    fullpath = assetfs_fullpath(CREDITS_FILE);
    if(NULL == (fp = fopen(fullpath, "r"))) {
        fatal_error("Can't open \"%s\" for reading.", CREDITS_FILE);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buf = mallocx((1 + size) * sizeof(*buf));
    buf[size] = '\0';
    if(fread(buf, 1, size, fp) != size)
        logfile_message("Warning: invalid return value of fread() when reading \"%s\"", CREDITS_FILE);

    fclose(fp);
    return buf;
}
