/*
 * Open Surge Engine
 * confirmbox.c - confirm box
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

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "confirmbox.h"
#include "../entities/actor.h"
#include "../entities/sfx.h"
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
#include "../scripting/scripting.h"


/* private data */
#define NO_OPTION    -1
#define OPTION_1      0
#define OPTION_2      1
#define MAX_OPTIONS   2

static image_t *box, *background;
static v2d_t boxpos;
static font_t *textfnt;
static font_t *optionfnt[MAX_OPTIONS][2];
static actor_t *arrow;
static input_t *input;
static char text[1024], option[MAX_OPTIONS][128];
static int option_count;
static int current_option = NO_OPTION;
static bool fxfade_in, fxfade_out;

static void setup_message(const confirmboxdata_t* confirmbox);


/* public functions */

/*
 * confirmbox_init()
 * Receives a pointer to a confirmboxdata_t
 */
void confirmbox_init(void *confirmbox)
{
    /* setup message & options */
    setup_message((confirmboxdata_t*)confirmbox);

    /* setup gfx */
    background = image_clone(video_get_backbuffer());
    box = sprite_get_image(sprite_get_animation("Confirm Box", 0), 0);
    boxpos = v2d_new( (VIDEO_SCREEN_W - image_width(box))/2 , VIDEO_SCREEN_H );
    arrow = actor_create();
    actor_change_animation(arrow, sprite_get_animation("UI Pointer", 0));

    /* initialize variables related to the fade effect */
    fxfade_in = true;
    fxfade_out = false;

    /* setup fonts */
    textfnt = font_create("dialogbox");
    font_set_text(textfnt, "%s", text);
    for(int i = 0; i < option_count; i++) {
        optionfnt[i][0] = font_create("dialogbox");
        optionfnt[i][1] = font_create("dialogbox");
        font_set_text(optionfnt[i][0], "%s", option[i]);
        font_set_text(optionfnt[i][1], "<color=$COLOR_HIGHLIGHT>%s</color>", option[i]);
    }

    /* setup input device */
    input = input_create_user(NULL);

    /* pause the SurgeScript VM */
    scripting_pause_vm();
}


/*
 * confirmbox_release()
 * Releases the scene
 */
void confirmbox_release()
{
    /* unpause the SurgeScript VM */
    scripting_resume_vm();

    /* release input device */
    input_destroy(input);

    /* release fonts */
    for(int i = 0; i < option_count; i++) {
        font_destroy(optionfnt[i][0]);
        font_destroy(optionfnt[i][1]);
    }
    font_destroy(textfnt);

    /* release gfx */
    actor_destroy(arrow);
    image_destroy(background);
}


/*
 * confirmbox_update()
 * Updates the scene
 */
void confirmbox_update()
{
    float dt = timer_get_delta(), speed = 5 * VIDEO_SCREEN_H;
    v2d_t size;
    int i;

    /* fade-in */
    if(fxfade_in) {
        if( boxpos.y <= (VIDEO_SCREEN_H - image_height(box))/2 )
            fxfade_in = false;
        else
            boxpos.y -= speed*dt;
    }

    /* fade-out */
    if(fxfade_out) {
        if( boxpos.y >= VIDEO_SCREEN_H ) {
            fxfade_out = false;
            scenestack_pop();
            return;
        }
        else
            boxpos.y += speed*dt;
    }

    /* positioning stuff */
    font_set_width(textfnt, image_width(box) - 16);
    font_set_position(textfnt, v2d_new(boxpos.x + 8 , boxpos.y + 8));
    for(i=0; i<option_count; i++) {
        size = font_get_textsize(optionfnt[i][0]);
        font_set_position(optionfnt[i][0], v2d_new(
            boxpos.x + (2 * i + 1) * image_width(box) / (2 * option_count) - size.x / 2,
            boxpos.y + image_height(box) - size.y - 8
        ));
        font_set_position(optionfnt[i][1], font_get_position(optionfnt[i][0]));
    }
    arrow->position = v2d_subtract(
        v2d_add(font_get_position(optionfnt[current_option][0]), arrow->hot_spot),
        v2d_new(image_width(actor_image(arrow)) * 1.4f, -image_height(actor_image(arrow)) * 0.5f)
    );

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
            fxfade_out = true;
        }
    }
}



/*
 * confirmbox_render()
 * Renders the scene
 */
void confirmbox_render()
{
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);
    int i, k;

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
 * setup_message()
 * Sets up the message to be displayed
 * Note: confirmbox->option2 may be NULL
 */
void setup_message(const confirmboxdata_t* confirmbox)
{
    /* copy text fields */
    str_cpy(text, confirmbox->message, sizeof(text));
    str_cpy(option[0], confirmbox->option1, sizeof(option[0]));
    str_cpy(option[1], confirmbox->option2 != NULL ? confirmbox->option2 : "", sizeof(option[1]));

    /* number of options */
    option_count = (confirmbox->option2 != NULL) ? 2 : 1;

    /* default option */
    current_option = OPTION_1;
    if(option_count > 1 && confirmbox->default_option == 2)
        current_option = OPTION_2;
}