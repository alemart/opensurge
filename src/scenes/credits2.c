/*
 * Open Surge Engine
 * credits2.c - second credits screen
 * Copyright (C) 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "credits2.h"
#include "options.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/scene.h"
#include "../core/soundfactory.h"
#include "../core/font.h"
#include "../core/osspec.h"
#include "../core/logfile.h"
#include "../entities/actor.h"
#include "../entities/background.h"


/* private data */
#define CREDITS2_FILE               "config/credits2.dat"
#define CREDITS2_BGFILE             "themes/credits2.bg"
static image_t *box;
static int quit;
static font_t *title, *text, *back;
static input_t *input;
static int line_count;
static bgtheme_t *bgtheme;

static char* read_credits2_file();




/* public functions */

/*
 * credits2_init()
 * Initializes the scene
 */
void credits2_init(void *foo)
{
    const char *p;
    char *credits2_text = read_credits2_file();

    /* initializing stuff... */
    quit = FALSE;
    input = input_create_user(NULL);

    title = font_create("menu.title");
    font_set_text(title, "%s", lang_get("CREDITS2_TITLE"));
    font_set_position(title, v2d_new((VIDEO_SCREEN_W - font_get_textsize(title).x)/2, 5));

    back = font_create("menu.text");
    font_set_text(back, "%s", lang_get("CREDITS2_KEY"));
    font_set_position(back, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(back).y - 5));

    text = font_create("menu.credits");
    font_set_text(text, "%s", credits2_text);
    font_set_width(text, VIDEO_SCREEN_W - 20);
    font_set_position(text, v2d_new(10, VIDEO_SCREEN_H));
    for(line_count=1,p=font_get_text(text); *p; p++)
        line_count += (*p == '\n') ? 1 : 0;

    box = image_create(VIDEO_SCREEN_W, 30);
    image_clear(box, image_rgb(0,0,0));

    bgtheme = background_load(CREDITS2_BGFILE);

    fadefx_in(image_rgb(0,0,0), 1.0);

    /* done! */
    free(credits2_text);
}


/*
 * credits2_release()
 * Releases the scene
 */
void credits2_release()
{
    bgtheme = background_unload(bgtheme);
    image_destroy(box);

    font_destroy(title);
    font_destroy(text);
    font_destroy(back);

    input_destroy(input);
}


/*
 * credits2_update()
 * Updates the scene
 */
void credits2_update()
{
    float dt = timer_get_delta();
    v2d_t textpos;

    /* background movement */
    background_update(bgtheme);

    /* text movement */
    textpos = font_get_position(text);
    textpos.y -= (3*font_get_textsize(text).y) * dt;
    if(textpos.y < -(line_count * (font_get_textsize(text).y + font_get_charspacing(text).y)))
        textpos.y = VIDEO_SCREEN_H;
    font_set_position(text, textpos);

    /* quit */
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play( soundfactory_get("return") );
            quit = TRUE;
        }
    }

    /* music */
    if(!music_is_playing()) {
        music_t *m = music_load(OPTIONS_MUSICFILE);
        music_play(m, INFINITY);
    }

    /* fade-out */
    if(quit) {
        if(fadefx_over()) {
            scenestack_pop();
            return;
        }
        fadefx_out(image_rgb(0,0,0), 1.0);
    }
}



/*
 * credits2_render()
 * Renders the scene
 */
void credits2_render()
{
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    font_render(text, cam);
    image_blit(box, video_get_backbuffer(), 0, 0, 0, 0, image_width(box), image_height(box));
    image_blit(box, video_get_backbuffer(), 0, 0, 0, VIDEO_SCREEN_H-20, image_width(box), image_height(box));
    font_render(title, cam);
    font_render(back, cam);
}







/* private stuff */

/* reads the contents of CREDITS2_FILE */
char* read_credits2_file()
{
    char *buf, filename[1024];
    FILE* fp;
    long size;

    resource_filepath(filename, CREDITS2_FILE, sizeof filename, RESFP_READ);
    if(NULL == (fp = fopen(filename, "r"))) {
        fatal_error("Can't open '%s' for reading.", CREDITS2_FILE);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buf = mallocx(1 + size);
    buf[size] = 0;
    if(fread(buf, 1, size, fp) != size)
        logfile_message("Warning: invalid return value of fread() when reading '%s'", CREDITS2_FILE);

    fclose(fp);
    return buf;
}
