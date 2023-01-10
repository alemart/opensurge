/*
 * Open Surge Engine
 * pause.c - pause menu
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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
#include <math.h>
#include "pause.h"
#include "quest.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/input.h"
#include "../core/sprite.h"
#include "../core/audio.h"
#include "../core/timer.h"
#include "../core/font.h"
#include "../core/asset.h"
#include "../entities/actor.h"
#include "../scenes/level.h"
#include "../scripting/scripting.h"



/* states of the pause menu */
typedef enum pause_state_t pause_state_t;
enum pause_state_t
{
    STATE_APPEARING,    /* the player has just paused the game */
    STATE_SUSTAINING,   /* waiting for player input */
    STATE_DISAPPEARING  /* closing the pause menu */
};

#define INITIAL_STATE STATE_APPEARING
static pause_state_t state = INITIAL_STATE;

static bool update_appearing();
static bool update_sustaining();
static bool update_disappearing();
static bool (*update[])() = {
    [STATE_APPEARING] = update_appearing,
    [STATE_SUSTAINING] = update_sustaining,
    [STATE_DISAPPEARING] = update_disappearing
};






/* options of the pause menu */
typedef enum pause_option_t pause_option_t;
enum pause_option_t
{
    OPTION_CONTINUE,    /* continue game */
    OPTION_RESTART,     /* restart level */
    OPTION_EXIT,        /* exit game */

    OPTION_COUNT        /* number of options */
};

static const pause_option_t NEXT_OPTION[] = {
    [OPTION_CONTINUE] = OPTION_RESTART,
    [OPTION_RESTART] = OPTION_EXIT,
    [OPTION_EXIT] = OPTION_EXIT
};

static const pause_option_t PREVIOUS_OPTION[] = {
    [OPTION_CONTINUE] = OPTION_CONTINUE,
    [OPTION_RESTART] = OPTION_CONTINUE,
    [OPTION_EXIT] = OPTION_RESTART
};

enum { UNHIGHLIGHTED, HIGHLIGHTED };

#define INITIAL_OPTION OPTION_CONTINUE
static pause_option_t option = INITIAL_OPTION;

static bool confirm_continue();
static bool confirm_restart();
static bool confirm_exit();
static bool (*confirm[])() = {
    [OPTION_CONTINUE] = confirm_continue,
    [OPTION_RESTART] = confirm_restart,
    [OPTION_EXIT] = confirm_exit
};



/* possible orientations of the (options of the) pause menu */
typedef enum pause_orientation_t pause_orientation_t;
enum pause_orientation_t
{
    ORIENTATION_VERTICAL,           /* from top to bottom */
    ORIENTATION_HORIZONTAL,         /* from left to right */
    ORIENTATION_VERTICAL_INVERSE,   /* from bottom to top */
    ORIENTATION_HORIZONTAL_INVERSE  /* from right to left */
};

static inputbutton_t NEXT_BUTTON[] = {

    /* which button is "next" changes according to the orientation of the pause menu */
    [ORIENTATION_VERTICAL] = IB_DOWN,
    [ORIENTATION_HORIZONTAL] = IB_RIGHT,
    [ORIENTATION_VERTICAL_INVERSE] = IB_UP,
    [ORIENTATION_HORIZONTAL_INVERSE] = IB_LEFT

};

static inputbutton_t PREVIOUS_BUTTON[] = {

    /* which button is "previous" changes according to the orientation of the pause menu */
    [ORIENTATION_VERTICAL] = IB_UP,
    [ORIENTATION_HORIZONTAL] = IB_LEFT,
    [ORIENTATION_VERTICAL_INVERSE] = IB_DOWN,
    [ORIENTATION_HORIZONTAL_INVERSE] = IB_RIGHT

};

#define DEFAULT_ORIENTATION ORIENTATION_VERTICAL
static pause_orientation_t orientation = DEFAULT_ORIENTATION;
static pause_orientation_t guess_orientation(v2d_t direction);



/* confirm & cancel buttons */
static const inputbutton_t ACTION_BUTTON = IB_FIRE1;
static const inputbutton_t START_BUTTON = IB_FIRE3;
static const inputbutton_t BACK_BUTTON = IB_FIRE4;






/* sprites of the pause menu */
typedef enum pause_sprite_t pause_sprite_t;
enum pause_sprite_t
{
    SPRITE_BACKGROUND,  /* sprite at the background */
    SPRITE_CONTINUE,    /* continue game */
    SPRITE_RESTART,     /* restart level */
    SPRITE_EXIT,        /* exit game */

    SPRITE_COUNT        /* number of sprites */
};

static const char* SPRITE_NAME[] = {
    [SPRITE_BACKGROUND] =   "Pause Menu",
    [SPRITE_CONTINUE] =     "Pause Menu - Option - Continue",
    [SPRITE_RESTART] =      "Pause Menu - Option - Restart",
    [SPRITE_EXIT] =         "Pause Menu - Option - Exit"
};

static const int ANIMATION_NUMBER[][3] = {
    [SPRITE_BACKGROUND] =   { [STATE_APPEARING] = 1, [STATE_SUSTAINING] = 2, [STATE_DISAPPEARING] = 3 },
    [SPRITE_CONTINUE] =     { [HIGHLIGHTED] = 1, [UNHIGHLIGHTED] = 0 },
    [SPRITE_RESTART] =      { [HIGHLIGHTED] = 1, [UNHIGHLIGHTED] = 0 },
    [SPRITE_EXIT] =         { [HIGHLIGHTED] = 1, [UNHIGHLIGHTED] = 0 }
};

#define ANIMATION(s, a) sprite_get_animation(SPRITE_NAME[s], ANIMATION_NUMBER[s][a])
static actor_t* actor[SPRITE_COUNT] = { NULL };



/* texts of the pause menu */
typedef enum pause_text_t pause_text_t;
enum pause_text_t
{
    TEXT_TITLE,         /* the "PAUSE" text */
    TEXT_CONTINUE,      /* continue game */
    TEXT_RESTART,       /* restart level */
    TEXT_EXIT,          /* exit game */

    TEXT_COUNT          /* number of text elements */
};

static const char* FONT_NAME[] = {
    [TEXT_TITLE] =      "Pause Menu - Title",
    [TEXT_CONTINUE] =   "Pause Menu - Option",
    [TEXT_RESTART] =    "Pause Menu - Option",
    [TEXT_EXIT] =       "Pause Menu - Option"
};

static const char* FONT_TEXT[] = {
    [TEXT_TITLE] =      "$PAUSE_TITLE",
    [TEXT_CONTINUE] =   "$PAUSE_CONTINUE",
    [TEXT_RESTART] =    "$PAUSE_RESTART",
    [TEXT_EXIT] =       "$PAUSE_EXIT"
};

static const char* FONT_COLOR[] = {
    [HIGHLIGHTED] =     "$PAUSE_HIGHLIGHT",
    [UNHIGHLIGHTED] =   "$PAUSE_UNHIGHLIGHT"
};

static pause_sprite_t FONT_PARENT_SPRITE[] = { /* helps with positioning */
    [TEXT_TITLE] =      SPRITE_BACKGROUND,
    [TEXT_CONTINUE] =   SPRITE_CONTINUE,
    [TEXT_RESTART] =    SPRITE_RESTART,
    [TEXT_EXIT] =       SPRITE_EXIT
};

static font_t* font[TEXT_COUNT] = { NULL };
static fontalign_t guess_font_align(const font_t* f, v2d_t target_position);



/* sound effects */
typedef enum pause_sound_t pause_sound_t;
enum pause_sound_t {
    SOUND_APPEAR,           /* played when the pause menu is appearing */
    SOUND_HIGHLIGHT,        /* an option has just been highlighted */
    SOUND_CONFIRM,          /* the player has chosen an option */
    SOUND_CANCEL,           /* the player has pushed the back button */

    SOUND_COUNT             /* number of sound effects */
};

static const char* SOUND_PATH[] = {
    [SOUND_APPEAR] =        "samples/pause_appear.wav",
    [SOUND_HIGHLIGHT] =     "samples/pause_highlight.wav",
    [SOUND_CONFIRM] =       "samples/pause_confirm.wav",
    [SOUND_CANCEL] =        "samples/pause_cancel.wav"
};

static sound_t* sound[SOUND_COUNT] = { NULL };



/* private data */
static const float FADEOUT_TIME = 0.5f; /* in seconds */
static image_t* snapshot = NULL;
static input_t* input = NULL;



/* legacy pause screen: used for games built with older versions of the engine (0.6.0.x or earlier) */
static const char* LEGACY_SPRITE_NAME = "Pause";
static const char* LEGACY_SOUND_PATH = "samples/select_2.wav";
static const float LEGACY_FADEOUT_TIME = 1.0f; /* in seconds */
static bool legacy_mode = false;
static void legacy_update();
static void legacy_render();
static bool want_legacy_mode();




/*
 * pause_init()
 * Initializes the pause menu
 */
void pause_init(void *_)
{
    (void)_;

    /* take a snapshot of the game */
    snapshot = image_clone(video_get_backbuffer());

    /* create an input object */
    input = input_create_user(NULL);
    input_simulate_button_down(input, START_BUTTON); /* this assumes the start button triggered the opening of this scene */
    input_simulate_button_down(input, BACK_BUTTON);

    /* pause music & scripting */
    music_pause();
    scripting_pause_vm();

    /* should we use the legacy mode? */
    legacy_mode = want_legacy_mode();
    if(legacy_mode) {
        sound_play(sound_load(LEGACY_SOUND_PATH));
        return;
    }

    /* initial animations */
    const animation_t* anim[SPRITE_COUNT] = { NULL };
    anim[SPRITE_BACKGROUND] = ANIMATION(SPRITE_BACKGROUND, INITIAL_STATE);
    anim[SPRITE_CONTINUE] = ANIMATION(SPRITE_CONTINUE, UNHIGHLIGHTED);
    anim[SPRITE_RESTART] = ANIMATION(SPRITE_RESTART, UNHIGHLIGHTED);
    anim[SPRITE_EXIT] = ANIMATION(SPRITE_EXIT, UNHIGHLIGHTED);

    /* initialize the actors */
    for(int i = 0; i < SPRITE_COUNT; i++) {
        actor[i] = actor_create();
        actor[i]->position = v2d_new(0, 0);
        actor_change_animation(actor[i], anim[i]);
    }

    /* initialize the fonts */
    for(int i = 0; i < TEXT_COUNT; i++) {
        font[i] = font_create(FONT_NAME[i]);
        font_set_text(font[i], "%s", FONT_TEXT[i]);
        font_set_visible(font[i], false);
    }

    /* initialize the sound effects */
    for(int i = 0; i < SOUND_COUNT; i++)
        sound[i] = sound_load(SOUND_PATH[i]);

    /* initialize the state */
    state = INITIAL_STATE;
    option = INITIAL_OPTION;
    /*orientation = DEFAULT_ORIENTATION;*/

    /* compute the orientation */
    orientation = guess_orientation(v2d_subtract(
        actor_action_offset(actor[SPRITE_EXIT]),
        actor_action_offset(actor[SPRITE_CONTINUE])
    ));

    /* done! */
    sound_play(sound[SOUND_APPEAR]);
}


/*
 * pause_release()
 * Releases the pause menu
 */
void pause_release()
{
    if(!legacy_mode) {

        /* release the actors */
        for(int i = 0; i < SPRITE_COUNT; i++) {
            if(actor[i] != NULL) {
                actor_destroy(actor[i]);
                actor[i] = NULL;
            }
        }

        /* release the fonts */
        for(int i = 0; i < TEXT_COUNT; i++) {
            if(font[i] != NULL) {
                font_destroy(font[i]);
                font[i] = NULL;
            }
        }

        /* release the sound effects */
        /* done automatically by the resource manager */

    }

    /* resume music & scripting */
    scripting_resume_vm();
    music_resume();

    /* destroy the input object and the snapshot */
    input_destroy(input);
    image_destroy(snapshot);
}


/*
 * pause_update()
 * Updates the pause menu
 */
void pause_update()
{
    /* legacy mode? */
    if(legacy_mode) {
        legacy_update();
        return;
    }

    /* state update */
    if(!update[state]())
        return;

    /* animate the sprites */
    #define ANIMATE_SPRITE(s, a) actor_change_animation(actor[s], ANIMATION(s, a))
    #define ANIMATE_OPTION(s, o) do { \
        if(!actor_is_transition_animation_playing(actor[s])) \
            ANIMATE_SPRITE((s), option == (o) && state == STATE_SUSTAINING ? HIGHLIGHTED : UNHIGHLIGHTED); \
    } while(0)

    ANIMATE_SPRITE(SPRITE_BACKGROUND, state);
    ANIMATE_OPTION(SPRITE_CONTINUE, OPTION_CONTINUE);
    ANIMATE_OPTION(SPRITE_RESTART, OPTION_RESTART);
    ANIMATE_OPTION(SPRITE_EXIT, OPTION_EXIT);

    /* update the fonts */
    for(int i = 0; i < TEXT_COUNT; i++) {
        pause_sprite_t parent = FONT_PARENT_SPRITE[i];
        v2d_t action_offset = actor_action_offset(actor[parent]);
        font_set_position(font[i], v2d_add(actor[parent]->position, action_offset));

        font_set_align(font[i], guess_font_align(font[i], font_get_position(font[i])));
        font_set_visible(font[i], state == STATE_SUSTAINING);
    }

    /* colorize the options */
    #define COLORIZE_TEXT(t, h)   font_set_text(font[t], "<color=%s>%s</color>", FONT_COLOR[h], FONT_TEXT[t])
    #define COLORIZE_OPTION(t, o) COLORIZE_TEXT((t), option == (o) ? HIGHLIGHTED : UNHIGHLIGHTED)

    COLORIZE_OPTION(TEXT_CONTINUE, OPTION_CONTINUE);
    COLORIZE_OPTION(TEXT_RESTART, OPTION_RESTART);
    COLORIZE_OPTION(TEXT_EXIT, OPTION_EXIT);
}


/* 
 * pause_render()
 * Renders the pause menu
 */
void pause_render()
{
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);

    /* legacy mode? */
    if(legacy_mode) {
        legacy_render();
        return;
    }

    /* render the snapshot of the game */
    image_blit(snapshot, 0, 0, 0, 0, image_width(snapshot), image_height(snapshot));

    /* render the actors */
    for(int i = 0; i < SPRITE_COUNT; i++)
        actor_render(actor[i], camera);

    /* render the fonts */
    for(int i = 0; i < TEXT_COUNT; i++)
        font_render(font[i], camera);

    /* render the overlay */
    /* TODO */
}




/* private */

/* update logic: appearing state */
bool update_appearing()
{
    /* wait for the appearing animation to finish */
    if(!actor_animation_finished(actor[SPRITE_BACKGROUND]))
        return true;

    /* change the state */
    state = STATE_SUSTAINING;

    /* success! */
    return true;
}

/* update logic: sustaining state */
bool update_sustaining()
{
    inputbutton_t next_button = NEXT_BUTTON[orientation];
    inputbutton_t previous_button = PREVIOUS_BUTTON[orientation];

    /* highlight next option */
    if(input_button_pressed(input, next_button)) {
        if(option != NEXT_OPTION[option])
            sound_play(sound[SOUND_HIGHLIGHT]);

        option = NEXT_OPTION[option];
    }

    /* highlight previous option */
    if(input_button_pressed(input, previous_button)) {
        if(option != PREVIOUS_OPTION[option])
            sound_play(sound[SOUND_HIGHLIGHT]);

        option = PREVIOUS_OPTION[option];
    }

    /* choose option */
    if(input_button_pressed(input, ACTION_BUTTON) || input_button_pressed(input, START_BUTTON)) {
        state = STATE_DISAPPEARING;
        sound_play(sound[SOUND_CONFIRM]);
    }

    /* cancel */
    if(input_button_pressed(input, BACK_BUTTON)) {
        option = OPTION_CONTINUE;
        state = STATE_DISAPPEARING;
        sound_play(sound[SOUND_CANCEL]);
    }

    /* success! */
    return true;
}

/* update logic: disappearing state */
bool update_disappearing()
{
    /* wait for the disappearing animation to finish */
    if(!actor_animation_finished(actor[SPRITE_BACKGROUND]))
        return true;

    /* perform the necessary action */
    return confirm[option]();
}

/* the player has chosen to continue playing */
bool confirm_continue()
{
    scenestack_pop();
    return false;
}

/* the player has chosen to restart the level */
bool confirm_restart()
{
    if(fadefx_is_over()) {
        level_restart();
        scenestack_pop();
        return false;
    }

    color_t black = color_rgb(0, 0, 0);
    fadefx_out(black, FADEOUT_TIME);
    return true;
}

/* the player has chosen to exit the game */
bool confirm_exit()
{
    if(fadefx_is_over()) {
        scenestack_pop();
        scenestack_pop();
        quest_abort();
        return false;
    }

    color_t black = color_rgb(0, 0, 0);
    fadefx_out(black, FADEOUT_TIME);
    return true;
}

/* guess the best alignment for a font given its target position */
fontalign_t guess_font_align(const font_t* f, v2d_t target_position)
{
    v2d_t boundary = video_get_screen_size();
    v2d_t size = font_get_textsize(f);

    if(target_position.x + size.x > boundary.x)
        return FONTALIGN_RIGHT;
    else if(target_position.x - size.x < 0.0f)
        return FONTALIGN_LEFT;
    else
        return FONTALIGN_CENTER;
}

/* guess the orientation of the pause menu */
pause_orientation_t guess_orientation(v2d_t direction)
{
    const float threshold = 0.707f; /* sin(45 deg) */
    v2d_t v = v2d_normalize(direction);

    if(v.y >= threshold)
        return ORIENTATION_VERTICAL;
    else if(v.y <= -threshold)
        return ORIENTATION_VERTICAL_INVERSE;
    else if(v.x >= 0.0f)
        return ORIENTATION_HORIZONTAL;
    else
        return ORIENTATION_HORIZONTAL_INVERSE;
}





/* legacy mode */

/* legacy pause screen: update */
void legacy_update()
{
    /* fade out to exit game */
    if(fadefx_is_over()) {
        scenestack_pop();
        scenestack_pop();
        quest_abort();
        return;
    }
    else if(fadefx_is_fading())
        return;

    /* exit game */
    if(input_button_pressed(input, IB_FIRE4)) {
        fadefx_out(color_rgb(0,0,0), LEGACY_FADEOUT_TIME);
        return;
    }

    /* unpause */
    if(input_button_pressed(input, IB_FIRE3)) {
        scenestack_pop();
        return;
    }
}

/* legacy pause screen: render */
void legacy_render()
{
    if(!sprite_animation_exists(LEGACY_SPRITE_NAME, 0))
        return;

    const image_t *icon = sprite_get_image(sprite_get_animation(LEGACY_SPRITE_NAME, 0), 0);
    v2d_t size = v2d_new(image_width(icon), image_height(icon));

    float frequency = PI_OVER_TWO;
    float elapsed_time = timer_get_elapsed();
    float scale = 1.0f + 0.5f * fabs(cosf(frequency * elapsed_time));

    v2d_t pos = v2d_new(
        (VIDEO_SCREEN_W - size.x) / 2.0f - (scale - 1.0f) * size.x / 2.0f,
        (VIDEO_SCREEN_H - size.y) / 2.0f - (scale - 1.0f) * size.y / 2.0f
    );

    image_blit(snapshot, 0, 0, 0, 0, image_width(snapshot), image_height(snapshot));
    image_draw_scaled(icon, (int)pos.x, (int)pos.y, v2d_new(scale,scale), IF_NONE);
}

/* the legacy mode will be used if any of the required assets is not found */
bool want_legacy_mode()
{
    for(int i = 0; i < SPRITE_COUNT; i++) {

        /* just take the first animation that was declared. what about the others?
           why pedantic check? we just check if the sprite exists */
        int anim_id = ANIMATION_NUMBER[i][0];
        const char* sprite_name = SPRITE_NAME[i];

        if(!sprite_animation_exists(sprite_name, anim_id))
            return true;

    }

    for(int i = 0; i < TEXT_COUNT; i++) {
        if(!font_exists(FONT_NAME[i]))
            return true;
    }

    for(int i = 0; i < SOUND_COUNT; i++) {
        if(!asset_exists(SOUND_PATH[i]))
            return true;
    }

    return false;
}