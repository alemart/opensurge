/*
 * Open Surge Engine
 * gameover.c - "game over" scene
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

#include "gameover.h"
#include "quest.h"
#include "../util/numeric.h"
#include "../util/util.h"
#include "../core/font.h"
#include "../core/scene.h"
#include "../core/audio.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/timer.h"
#include "../core/lang.h"
#include "../core/mobile_gamepad.h"


/* private data */
static const float GAMEOVER_FADETIME = 2.0f;
static const float GAMEOVER_APPEARTIME = 1.0f;
static const char* GAMEOVER_MUSICFILE = "musics/gameover.ogg";
static font_t* gameover_fnt[2];
static image_t* gameover_bg;
static float gameover_timer;
static float gameover_spacing;
static float gameover_width[3];
static float gameover_height;
static music_t* music;


/*
 * gameover_init()
 * Initializes the game over screen
 */
void gameover_init(void *foo)
{
    /* get text size */
    font_t* tmp_fnt = font_create("gameover");
    font_set_text(tmp_fnt, "%s", lang_get("GAMEOVER_PART1"));
    font_set_text(tmp_fnt, "%s %s", font_get_text(tmp_fnt), lang_get("GAMEOVER_PART2"));
    gameover_width[2] = font_get_textsize(tmp_fnt).x;
    gameover_height = font_get_textsize(tmp_fnt).y;
    font_destroy(tmp_fnt);

    /* create fonts */
    gameover_fnt[0] = font_create("gameover");
    font_set_align(gameover_fnt[0], FONTALIGN_CENTER);
    font_set_text(gameover_fnt[0], "%s", lang_get("GAMEOVER_PART1"));
    gameover_width[0] = font_get_textsize(gameover_fnt[0]).x;

    gameover_fnt[1] = font_create("gameover");
    font_set_align(gameover_fnt[1], FONTALIGN_CENTER);
    font_set_text(gameover_fnt[1], "%s", lang_get("GAMEOVER_PART2"));
    gameover_width[1] = font_get_textsize(gameover_fnt[1]).x;

    /* misc */
    gameover_spacing = gameover_width[2] - gameover_width[1] - gameover_width[0];
    gameover_bg = image_clone(video_get_backbuffer());
    gameover_timer = 0;

    /* music */
    music = music_load(GAMEOVER_MUSICFILE);
    music_play(music, false);
}



/*
 * gameover_update()
 * Updates the game over screen
 */
void gameover_update()
{
    float t, dt = timer_get_delta();
    float center_x;

    /* timer */
    gameover_timer += dt;

    /* mobile gamepad */
    mobilegamepad_fadeout();

    /* fade out */
    if(gameover_timer >= GAMEOVER_APPEARTIME && !music_is_playing(music)) {
        if(fadefx_is_over()) {
            quest_abort();
            scenestack_pop();
            mobilegamepad_fadein();
            return;
        }
        fadefx_out(color_rgb(0,0,0), GAMEOVER_FADETIME);
    }

    /* position the text */
    t = gameover_timer / GAMEOVER_APPEARTIME;
    center_x = (VIDEO_SCREEN_W + (gameover_width[0] - gameover_width[1])) / 2.0f;

    font_set_position(gameover_fnt[0], v2d_new(
        lerp(-gameover_width[0], center_x - (gameover_width[0] + gameover_spacing) / 2.0f, t),
        (VIDEO_SCREEN_H - gameover_height) / 2.0f
    ));

    font_set_position(gameover_fnt[1], v2d_new(
        lerp(VIDEO_SCREEN_W + gameover_width[1], center_x + (gameover_width[1] + gameover_spacing) / 2.0f, t),
        (VIDEO_SCREEN_H - gameover_height) / 2.0f
    ));
}



/*
 * gameover_render()
 * Renders the game over screen
 */
void gameover_render()
{
    v2d_t v = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    image_blit(gameover_bg, 0, 0, 0, 0, image_width(gameover_bg), image_height(gameover_bg));
    font_render(gameover_fnt[0], v);
    font_render(gameover_fnt[1], v);
}



/*
 * gameover_release()
 * Releases the game over screen
 */
void gameover_release()
{
    music_stop();
    music_unref(music);
    image_destroy(gameover_bg);
    font_destroy(gameover_fnt[1]);
    font_destroy(gameover_fnt[0]);

    quest_abort();
}
