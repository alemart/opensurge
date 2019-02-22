/*
 * Open Surge Engine
 * confirmbox.c - confirm box
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "confirmbox.h"
#include "../entities/actor.h"
#include "../core/global.h"
#include "../core/font.h"
#include "../core/fadefx.h"
#include "../core/video.h"
#include "../core/sprite.h"
#include "../core/input.h"
#include "../core/audio.h"
#include "../core/timer.h"
#include "../core/scene.h"
#include "../core/stringutil.h"
#include "../core/soundfactory.h"


/* private data */
#define MAX_OPTIONS 5
#define NO_OPTION   -1
static image_t *box, *background;
static v2d_t boxpos;
static font_t *textfnt;
static font_t *optionfnt[MAX_OPTIONS][2];
static actor_t *arrow;
static input_t *input;
static char text[1024], option[MAX_OPTIONS][128];
static int option_count;
static int current_option = NO_OPTION;
static int fxfade_in, fxfade_out;

static void setup(const char *ptext, const char *option1, const char *option2);


/* public functions */

/*
 * confirmbox_init()
 * Receives an array of 3 strings:
 * - text
 * - option 1
 * - option 2 (may be null)
 */
void confirmbox_init(void *text_and_options)
{
    confirmboxdata_t *p = (confirmboxdata_t*)text_and_options;
    int i;

    setup((*p)[0], (*p)[1], (*p)[2]);

    background = image_clone(video_get_backbuffer());

    box = sprite_get_image(sprite_get_animation("SD_CONFIRMBOX", 0), 0);
    boxpos = v2d_new( (VIDEO_SCREEN_W - image_width(box))/2 , VIDEO_SCREEN_H );

    input = input_create_user(NULL);
    arrow = actor_create();
    actor_change_animation(arrow, sprite_get_animation("SD_GUIARROW", 0));

    textfnt = font_create("dialogbox");
    font_set_text(textfnt, "%s", text);
    font_set_width(textfnt, image_width(box) - 20);

    for(i=0; i<option_count; i++) {
        optionfnt[i][0] = font_create("dialogbox");
        optionfnt[i][1] = font_create("dialogbox");
        font_set_text(optionfnt[i][0], "%s", option[i]);
        font_set_text(optionfnt[i][1], "<color=$COLOR_MENUSELECTEDOPTION>%s</color>", option[i]);
    }

    current_option = 0;
    fxfade_in = TRUE;
    fxfade_out = FALSE;
}


/*
 * confirmbox_release()
 * Releases the scene
 */
void confirmbox_release()
{
    int i;

    for(i=0; i<option_count; i++) {
        font_destroy(optionfnt[i][0]);
        font_destroy(optionfnt[i][1]);
    }

    actor_destroy(arrow);
    input_destroy(input);
    font_destroy(textfnt);
    image_destroy(background);
}


/*
 * confirmbox_update()
 * Updates the scene
 */
void confirmbox_update()
{
    int i;
    float dt = timer_get_delta(), speed = 5*VIDEO_SCREEN_H;

    /* fade-in */
    if(fxfade_in) {
        if( boxpos.y <= (VIDEO_SCREEN_H - image_height(box))/2 )
            fxfade_in = FALSE;
        else
            boxpos.y -= speed*dt;
    }

    /* fade-out */
    if(fxfade_out) {
        if( boxpos.y >= VIDEO_SCREEN_H ) {
            fxfade_out = FALSE;
            scenestack_pop();
            return;
        }
        else
            boxpos.y += speed*dt;
    }

    /* positioning stuff */
    arrow->position = v2d_new(boxpos.x + current_option * image_width(box)/option_count + 10, boxpos.y + image_height(box) * 0.75);
    font_set_position(textfnt, v2d_new(boxpos.x + 10 , boxpos.y + 10));
    for(i=0; i<option_count; i++) {
        font_set_position(optionfnt[i][0], v2d_new(boxpos.x + i * image_width(box)/option_count + 25, boxpos.y + image_height(box) * 0.75));
        font_set_position(optionfnt[i][1], font_get_position(optionfnt[i][0]));
    }

    /* input */
    if(!fxfade_in && !fxfade_out) {
        if(input_button_pressed(input, IB_LEFT)) {
            /* left */
            sound_play(SFX_CHOOSE);
            current_option = ( ((current_option-1)%option_count) + option_count )%option_count;
        }
        else if(input_button_pressed(input, IB_RIGHT)) {
            /* right */
            sound_play(SFX_CHOOSE);
            current_option = (current_option+1)%option_count;
        }
        else if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
            /* confirm */
            sound_play(SFX_CONFIRM);
            fxfade_out = TRUE;
        }
    }
}



/*
 * confirmbox_render()
 * Renders the scene
 */
void confirmbox_render()
{
    int i, k;
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    image_blit(background, 0, 0, 0, 0, image_width(background), image_height(background));
    image_draw(box, boxpos.x, boxpos.y, IF_NONE);
    font_render(textfnt, cam);

    for(i=0; i<option_count; i++) {
        k = (i==current_option) ? 1 : 0;
        font_render(optionfnt[i][k], cam);
    }

    actor_render(arrow, cam);
}



/*
 * confirmbox_selected_option()
 * Returns the selected option (1, 2, ..., n), or
 * 0 if nothing has been selected.
 * This must be called AFTER this scene
 * gets released
 */
int confirmbox_selected_option()
{
    if(current_option != NO_OPTION) {
        int ret = current_option + 1;
        current_option = NO_OPTION;
        return ret;
    }
    else
        return 0; /* nothing */
}




/* ------------ private -------------- */

/*
 * setup()
 * PS: option2 may be NULL
 */
void setup(const char *ptext, const char *option1, const char *option2)
{
    current_option = -1;
    str_cpy(text, ptext, sizeof(text));
    str_cpy(option[0], option1, sizeof(option[0]));

    if(option2) {
        str_cpy(option[1], option2, sizeof(option[1]));
        option_count = 2;
    }
    else
        option_count = 1;
}
