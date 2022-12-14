/*
 * Open Surge Engine
 * mobile_gamepad.h - virtual gamepad for mobile devices
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

#include <allegro5/allegro.h>
#include <stdbool.h>
#include "mobile_gamepad.h"
#include "logfile.h"
#include "video.h"
#include "timer.h"
#include "v2d.h"
#include "../entities/actor.h"



/* settings */
#define WANT_MOUSE_INPUT         0 /*1*/ /* Will the mobile gamepad will be activated by mouse input? For testing only */
#define ENABLE_MOUSE_INPUT       ((WANT_MOUSE_INPUT) && !defined(__ANDROID__))
#define ENABLE_MOBILE_GAMEPAD    (defined(__ANDROID__) || (ENABLE_MOUSE_INPUT))



/* mobile controls */
enum { DPAD, DPAD_STICK, ACTION_BUTTON };

/* states of a button */
enum { UNPRESSED, PRESSED };

/* multi-touch */
#define MAX_TOUCHES (sizeof(((ALLEGRO_TOUCH_INPUT_STATE*)0)->touches) / sizeof(((ALLEGRO_TOUCH_INPUT_STATE*)0)->touches[0]))
typedef struct touch_t touch_t;
struct touch_t {

    /* whether or not this touch entry is "down". An entry that
       is not "down" is free to be overwritten at any time */
    bool down;

    /* position of the touch in window coordinates, given in pixels */
    v2d_t position;

    /* id: useful for gesture recognition TODO? */
    /*int id;*/

};




/* utilities */
#define IDLE_STATE (mobilegamepad_state_t){ \
    .dpad = MOBILEGAMEPAD_DPAD_CENTER, \
    .buttons = MOBILEGAMEPAD_BUTTON_NONE \
}

#define NO_TOUCH (touch_t){ \
    .down = false, \
    .position = (v2d_t){ .x = 0.0f, .y = 0.0f } \
}

#define IS_POWER_OF_TWO(n) (((n) & ((n) - 1)) == 0)
#define VALIDATE_MASK(x)   typedef char _assert_ ## x [ IS_POWER_OF_TWO(1+(x)) * 2 - 1 ]

static const float DEG2RAD = 0.01745329251994329576f;



/* graphical utilities */

/* sprites are designed based on this resolution */
static const v2d_t REFERENCE_RESOLUTION = {
    .x = 426 * 4,
    .y = 240 * 4
};

/* position of the controls in relative window coordinates, i.e., [0,1] x [0,1] */
static const v2d_t RELATIVE_POSITION[] = {

    [DPAD] = {
        .x = 0.12f,
        .y = 0.8f
    },

    [DPAD_STICK] = { /* same as DPAD */
        .x = 0.12f,
        .y = 0.8f
    },

    [ACTION_BUTTON] = {
        .x = 0.88f,
        .y = 0.8f
    }

};

static const char* SPRITE_NAME[] = {
    [DPAD]          = "Mobile Gamepad - Directional Stick",
    [DPAD_STICK]    = "Mobile Gamepad - Directional Stick",
    [ACTION_BUTTON] = "Mobile Gamepad - Action Button"
};

static const int DPAD_ANIMATION_NUMBER[16] = {
    [MOBILEGAMEPAD_DPAD_CENTER]                          = 0,
    [MOBILEGAMEPAD_DPAD_RIGHT]                           = 1,
    [MOBILEGAMEPAD_DPAD_UP | MOBILEGAMEPAD_DPAD_RIGHT]   = 2,
    [MOBILEGAMEPAD_DPAD_UP]                              = 3,
    [MOBILEGAMEPAD_DPAD_UP | MOBILEGAMEPAD_DPAD_LEFT]    = 4,
    [MOBILEGAMEPAD_DPAD_LEFT]                            = 5,
    [MOBILEGAMEPAD_DPAD_DOWN | MOBILEGAMEPAD_DPAD_LEFT]  = 6,
    [MOBILEGAMEPAD_DPAD_DOWN]                            = 7,
    [MOBILEGAMEPAD_DPAD_DOWN | MOBILEGAMEPAD_DPAD_RIGHT] = 8
};

static const int DPAD_STICK_ANIMATION_NUMBER = 9;

static const int BUTTON_ANIMATION_NUMBER[] = {
    [UNPRESSED] = 0,
    [PRESSED]   = 1
};

static const int DPAD_STICK_ANGLE[16] = { /* clockwise (y-axis grows downwards) */
    [MOBILEGAMEPAD_DPAD_CENTER]                          = 0,
    [MOBILEGAMEPAD_DPAD_RIGHT]                           = 0,
    [MOBILEGAMEPAD_DPAD_UP | MOBILEGAMEPAD_DPAD_RIGHT]   = -45,
    [MOBILEGAMEPAD_DPAD_UP]                              = -90,
    [MOBILEGAMEPAD_DPAD_UP | MOBILEGAMEPAD_DPAD_LEFT]    = -135,
    [MOBILEGAMEPAD_DPAD_LEFT]                            = -180,
    [MOBILEGAMEPAD_DPAD_DOWN | MOBILEGAMEPAD_DPAD_LEFT]  = -225,
    [MOBILEGAMEPAD_DPAD_DOWN]                            = -270,
    [MOBILEGAMEPAD_DPAD_DOWN | MOBILEGAMEPAD_DPAD_RIGHT] = -315
};

static const float DPAD_STICK_MOVEMENT_LENGTH = 0.2f; /* a value relative to the radius of the dpad */
static const float DPAD_STICK_MOVEMENT_TIME = 0.05f; /* in seconds */

#define DPAD_ANIMATION_NUMBER_MASK (sizeof(DPAD_ANIMATION_NUMBER) / sizeof(DPAD_ANIMATION_NUMBER[0]) - 1)
VALIDATE_MASK(DPAD_ANIMATION_NUMBER_MASK);

#define DPAD_STICK_ANGLE_MASK (sizeof(DPAD_STICK_ANGLE) / sizeof(DPAD_STICK_ANGLE[0]) - 1)
VALIDATE_MASK(DPAD_STICK_ANGLE_MASK);

static const float FADE_TIME = 0.5f; /* used when showing/hiding the controls; given in seconds */



/* D-Pad sensitivity */

static const v2d_t DPAD_AXIS_THRESHOLD = {
    .x = 0.5f,    /* cos(60 degrees) ~ 120 degrees horizontally */
    .y = 0.707f   /* sin(45 degrees) ~ 90 degrees vertically */
};

static const float DPAD_DEADZONE_THRESHOLD = 0.15f; /* a percentage of the radius of the dpad */



/* private stuff */

/* current state of the mobile gamepad */
static mobilegamepad_state_t current_state = IDLE_STATE;

/* is the mobile gamepad enabled? */
static bool is_enabled = false;

/* is the mobile gamepad visible? */
static bool is_visible = true;

/* alpha value used for fading in and fading out the mobile gamepad */
static float alpha = 1.0f;

/* radii of the controls, in pixels */
static int radius[] = {
    [DPAD] = 0,
    [DPAD_STICK] = 0, /* unused */
    [ACTION_BUTTON] = 0
};

/* actors */
static actor_t* actor[] = {
    [DPAD] = NULL,
    [DPAD_STICK] = NULL,
    [ACTION_BUTTON] = NULL
};

/* number of controls */
static const int NUM_CONTROLS = sizeof(actor) / sizeof(actor[0]);

/* misc */
static void trigger(int control, v2d_t offset);
static void animate_actors();
static void update_actors();
static void render_actors();
static void handle_fade_effect();
static void enable_linear_filtering();





/* ----- public API ----- */


/*
 * mobilegamepad_init()
 * Initializes the mobile gamepad
 */
void mobilegamepad_init()
{
    logfile_message("Initializing the mobile gamepad...");

    /* reset the state */
    current_state = IDLE_STATE;
    is_enabled = false;

#if !ENABLE_MOBILE_GAMEPAD

    /* disable multi-touch */
    logfile_message("The mobile gamepad isn't available in this system");
    return;

#elif !ENABLE_MOUSE_INPUT

    /* require touch input */
    if(!al_is_touch_input_installed()) {
        logfile_message("No touch input. The mobile gamepad won't be available!");
        return;
    }

#else

    /* require mouse input */
    if(!al_is_mouse_installed())
        fatal_error("No mouse input for the mobile gamepad!");

#endif

    /* initialize the radii */
    for(int i = 0; i < NUM_CONTROLS; i++)
        radius[i] = 0;

    /* create the actors */
    for(int i = 0; i < NUM_CONTROLS; i++)
        actor[i] = actor_create();

    enable_linear_filtering();

    /* make it visible */
    is_visible = true;
    alpha = 0.0f; /* make it fade in nicely when initializing */

    /* success! */
    is_enabled = true;
}

/*
 * mobilegamepad_release()
 * Releases the mobile gamepad
 */
void mobilegamepad_release()
{
    /* destroy the actors */
    for(int i = NUM_CONTROLS - 1; i >= 0; i--) {
        if(actor[i] != NULL) {
            actor_destroy(actor[i]);
            actor[i] = NULL;
        }
    }

    /* reset the state */
    current_state = IDLE_STATE;
    is_enabled = false;
}

/*
 * mobilegamepad_update()
 * Updates the mobile gamepad
 */
void mobilegamepad_update()
{
    /* do nothing if disabled */
    if(!is_enabled)
        return;

    /* reset touch */
    touch_t touch[MAX_TOUCHES];
    for(int j = 0; j < MAX_TOUCHES; j++)
        touch[j] = NO_TOUCH;

#if !ENABLE_MOUSE_INPUT

    /* read touch input */
    ALLEGRO_TOUCH_INPUT_STATE state;

    al_get_touch_input_state(&state);
    for(int i = 0, j = 0; i < MAX_TOUCHES; i++) {
        if(state.touches[i].id > 0) {
            touch[j].down = true;
            touch[j].position = v2d_new(state.touches[i].x, state.touches[i].y);
            j++;
        }
    }

#else

    /* read mouse input */
    ALLEGRO_MOUSE_STATE mouse;

    al_get_mouse_state(&mouse);
    if(mouse.buttons & 1) {
        touch[0].down = true;
        touch[0].position = v2d_new(mouse.x, mouse.y);
    }

#endif

    /* reset state */
    current_state = IDLE_STATE;

    /* detect if something is pressed */
    for(int i = 0; i < MAX_TOUCHES; i++) {
        if(touch[i].down) {
            for(int j = 0; j < NUM_CONTROLS; j++) {
                v2d_t offset = v2d_subtract(touch[i].position, actor[j]->position);
                int distance = (int)(v2d_magnitude(offset) + 0.5f);

                if(distance <= radius[j])
                    trigger(j, offset);
            }
        }
    }

    /* update actors */
    update_actors();
}

/*
 * mobilegamepad_render()
 * Renders the mobile gamepad
 */
void mobilegamepad_render()
{
    /* do nothing if disabled */
    if(!is_enabled)
        return;

    /* fading in and fading out */
    handle_fade_effect();

    /* render mobile gamepad */
    render_actors();


#if ENABLE_MOUSE_INPUT
    /* render the mouse cursor */
    ALLEGRO_MOUSE_STATE mouse;
    al_get_mouse_state(&mouse);

    float r = video_get_window_size().x * 0.01f;
    image_ellipsefill(mouse.x, mouse.y, r, r, color_rgba(255, 255, 0, 64));
#endif
}

/*
 * mobilegamepad_get_state()
 * Reads the current state of the mobile gamepad
 */
void mobilegamepad_get_state(mobilegamepad_state_t* state)
{
    *state = is_enabled && is_visible ? current_state : IDLE_STATE;
}

/*
 * mobilegamepad_fadein()
 * Makes the mobile gamepad visible
 */
void mobilegamepad_fadein()
{
    is_visible = true;
}

/*
 * mobilegamepad_fadeout()
 * Makes the mobile gamepad invisible
 */
void mobilegamepad_fadeout()
{
    is_visible = false;
}


/* ----- private ----- */

void trigger(int control, v2d_t offset)
{
    switch(control) {
        case ACTION_BUTTON:
            current_state.buttons |= MOBILEGAMEPAD_BUTTON_ACTION;
            break;

        case DPAD:
            /* ignore the deadzone: unstable angle */
            if(v2d_magnitude(offset) > radius[DPAD] * DPAD_DEADZONE_THRESHOLD) {

                /* find the direction */
                v2d_t normalized_offset = v2d_normalize(offset); /* cos(angle), sin(angle) */

                if(normalized_offset.x >= DPAD_AXIS_THRESHOLD.x)
                    current_state.dpad |= MOBILEGAMEPAD_DPAD_RIGHT;
                else if(normalized_offset.x <= -DPAD_AXIS_THRESHOLD.x)
                    current_state.dpad |= MOBILEGAMEPAD_DPAD_LEFT;

                if(normalized_offset.y >= DPAD_AXIS_THRESHOLD.y)
                    current_state.dpad |= MOBILEGAMEPAD_DPAD_DOWN;
                else if(normalized_offset.y <= -DPAD_AXIS_THRESHOLD.y)
                    current_state.dpad |= MOBILEGAMEPAD_DPAD_UP;

            }

            break;

        default:
            break;
    }
}

void animate_actors()
{
    /* compute the animation numbers */
    int anim[] = {
        [DPAD] = DPAD_ANIMATION_NUMBER[current_state.dpad & DPAD_ANIMATION_NUMBER_MASK],
        [DPAD_STICK] = DPAD_STICK_ANIMATION_NUMBER,
        [ACTION_BUTTON] = BUTTON_ANIMATION_NUMBER[(current_state.buttons & MOBILEGAMEPAD_BUTTON_ACTION) ? PRESSED : UNPRESSED]
    };

    /* change the animation of the actors */
    for(int i = 0; i < NUM_CONTROLS; i++)
        actor_change_animation(actor[i], sprite_get_animation(SPRITE_NAME[i], anim[i]));
}

void update_actors()
{
    /* compute the scale of the actors based on the size of the window */
    v2d_t window_size = video_get_window_size();
    v2d_t scale = v2d_new(window_size.x / REFERENCE_RESOLUTION.x, window_size.y / REFERENCE_RESOLUTION.y);

    /* animate the actors */
    animate_actors();

    /* update the attributes of the actors */
    for(int i = 0; i < NUM_CONTROLS; i++) {
        actor[i]->position = v2d_compmult(RELATIVE_POSITION[i], window_size);
        actor[i]->scale = scale;
        actor[i]->alpha = alpha;
    }

    /* update the radii of the controls */
    for(int i = 0; i < NUM_CONTROLS; i++)
        radius[i] = 0.5f * image_width(actor_image(actor[i])) * max(scale.x, scale.y);

    /* adjust the position of the dpad stick using polar coordinates */
    static float transition = 0.0f, angle = 0.0f;
    float ds = timer_get_delta() / DPAD_STICK_MOVEMENT_TIME;

    if(current_state.dpad != MOBILEGAMEPAD_DPAD_CENTER) {
        transition = min(1.0f, transition + ds);
        angle = DPAD_STICK_ANGLE[current_state.dpad & DPAD_STICK_ANGLE_MASK] * DEG2RAD;
    }
    else
        transition = max(0.0f, transition - ds);

    float max_length = radius[DPAD] * DPAD_STICK_MOVEMENT_LENGTH;
    float length = max_length * transition;
    v2d_t unit_vector = v2d_new(cosf(angle), sinf(angle));
    v2d_t offset = v2d_multiply(unit_vector, length);

    actor[DPAD_STICK]->position = v2d_add(actor[DPAD_STICK]->position, offset);
}

void render_actors()
{
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);

    for(int i = 0; i < NUM_CONTROLS; i++)
        actor_render(actor[i], camera);
}

void handle_fade_effect()
{
    float da = (1.0f / FADE_TIME) * timer_get_delta();

    if(is_visible)
        alpha = min(1.0f, alpha + da);
    else
        alpha = max(0.0f, alpha - da);
}

void enable_linear_filtering()
{
    animate_actors(); /* set up images */

    for(int i = 0; i < NUM_CONTROLS; i++) {
        image_t* image = actor_image(actor[i]);
        image_enable_linear_filtering(image);
    }
}