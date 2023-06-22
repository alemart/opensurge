/*
 * Open Surge Engine
 * mobilegamepad.h - virtual gamepad for mobile devices
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

#include <allegro5/allegro.h>
#include <stdbool.h>
#include "mobilegamepad.h"
#include "actor.h"
#include "../scenes/level.h"
#include "../core/engine.h"
#include "../core/logfile.h"
#include "../core/video.h"
#include "../core/timer.h"
#include "../util/v2d.h"
#include "../util/numeric.h"
#include "../util/util.h"



/* mobile controls */
enum {
    DPAD,
    DPAD_STICK,
    ACTION_BUTTON,

    /* number of controls that are displayed on the screen */
    NUM_CONTROLS
};

/* states of a button */
enum { UNPRESSED, PRESSED };

/* multi-touch */
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

static const int MAX_TOUCHES = (sizeof(((ALLEGRO_TOUCH_INPUT_STATE*)0)->touches) / sizeof(((ALLEGRO_TOUCH_INPUT_STATE*)0)->touches[0]));




/* utilities */
#define LOG(...)            logfile_message("Mobile Gamepad - " __VA_ARGS__)
#define FATAL(...)          fatal_error("Mobile Gamepad - " __VA_ARGS__)
#define IS_POWER_OF_TWO(n)  (((n) & ((n) - 1)) == 0)
#define VALIDATE_MASK(x)    typedef char _assert_ ## x [ IS_POWER_OF_TWO(1+(x)) * 2 - 1 ]



/* graphical utilities */

/* sprites are designed based on this resolution */
static const v2d_t REFERENCE_RESOLUTION = {
    .x = 426 * 4,
    .y = 240 * 4
};

/* position of the controls in relative window coordinates, i.e., [0,1] x [0,1] */
static const v2d_t RELATIVE_POSITION[] = {

    [DPAD] = {
        .x = 0.135f,
        .y = 0.77f
    },

    [DPAD_STICK] = { /* same as DPAD */
        .x = 0.135f,
        .y = 0.77f
    },

    [ACTION_BUTTON] = {
        .x = 0.87f,
        .y = 0.77f
    }

};

static const char* SPRITE_NAME[] = {
    [DPAD]          = "Mobile Gamepad - Directional Stick",
    [DPAD_STICK]    = "Mobile Gamepad - Directional Stick - Ball",
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

static const int DPAD_STICK_ANIMATION_NUMBER[] = {
    [UNPRESSED] = 0,
    [PRESSED]   = 1
};

static const int BUTTON_ANIMATION_NUMBER[] = {
    [UNPRESSED] = 0,
    [PRESSED]   = 1
};

static const float DPAD_STICK_ANGLE[16] = { /* clockwise (y-axis grows downwards) */
    [MOBILEGAMEPAD_DPAD_CENTER]                          =  0.0f,
    [MOBILEGAMEPAD_DPAD_RIGHT]                           =  0.0f,
    [MOBILEGAMEPAD_DPAD_UP | MOBILEGAMEPAD_DPAD_RIGHT]   = -45.0f  * DEG2RAD,
    [MOBILEGAMEPAD_DPAD_UP]                              = -90.0f  * DEG2RAD,
    [MOBILEGAMEPAD_DPAD_UP | MOBILEGAMEPAD_DPAD_LEFT]    = -135.0f * DEG2RAD,
    [MOBILEGAMEPAD_DPAD_LEFT]                            = -180.0f * DEG2RAD,
    [MOBILEGAMEPAD_DPAD_DOWN | MOBILEGAMEPAD_DPAD_LEFT]  = -225.0f * DEG2RAD,
    [MOBILEGAMEPAD_DPAD_DOWN]                            = -270.0f * DEG2RAD,
    [MOBILEGAMEPAD_DPAD_DOWN | MOBILEGAMEPAD_DPAD_RIGHT] = -315.0f * DEG2RAD
};

#define DPAD_ANIMATION_NUMBER_MASK (sizeof(DPAD_ANIMATION_NUMBER) / sizeof(DPAD_ANIMATION_NUMBER[0]) - 1)
VALIDATE_MASK(DPAD_ANIMATION_NUMBER_MASK);

#define DPAD_STICK_ANGLE_MASK (sizeof(DPAD_STICK_ANGLE) / sizeof(DPAD_STICK_ANGLE[0]) - 1)
VALIDATE_MASK(DPAD_STICK_ANGLE_MASK);

static const float DPAD_STICK_MOVEMENT_TIME = 0.05f; /* in seconds */
static const float FADE_TIME = 0.5f; /* used when showing/hiding the controls; given in seconds */



/* D-Pad sensitivity */

static const v2d_t DPAD_AXIS_THRESHOLD = {
    .x = 0.609f,  /* cos(52.5 degrees) ~ 105 degrees horizontally */
    .y = 0.707f   /* sin(45 degrees) ~ 90 degrees vertically */
};

static const float DPAD_DEADZONE_THRESHOLD = 0.0625f; /* a percentage of the interactive radius of the dpad */



/* private stuff */

/* idle state */
static const mobilegamepad_state_t IDLE_STATE = {
    .dpad = MOBILEGAMEPAD_DPAD_CENTER,
    .buttons = MOBILEGAMEPAD_BUTTON_NONE
};

/* no touch */
static const touch_t NO_TOUCH = {
    .down = false,
    .position = (v2d_t){ .x = 0.0f, .y = 0.0f }
};

/* initialization flags */
static int flags = 0;

/* current state of the mobile gamepad */
static mobilegamepad_state_t current_state;

/* is the mobile gamepad available in this system? */
static bool is_available = false;

/* is the mobile gamepad visible? */
static bool is_visible = true;

/* alpha value used for fading in and fading out the mobile gamepad */
static float alpha = 1.0f;

/* the distance from the center of the sprites, in pixels, to which controls respond to input */
static float interactive_radius[] = {
    [DPAD] = 0.0f,
    [DPAD_STICK] = 0.0f, /* unused */
    [ACTION_BUTTON] = 0.0f
};

/* actors */
static actor_t* actor[] = {
    [DPAD] = NULL,
    [DPAD_STICK] = NULL,
    [ACTION_BUTTON] = NULL
};

/* indicates the pressing of the back button or the performing of a back gesture on a smartphone */
static bool back_pressed = false;

/* misc */
static void trigger(int control, v2d_t offset);
static void animate_actors();
static void update_actors();
static void render_actors();
static void handle_fade_effect();
static void enable_linear_filtering();
static v2d_t dpad_stick_offset(float scale);
static void a5_handle_back_event(const ALLEGRO_EVENT* event, void* data);





/* ----- public API ----- */


/*
 * mobilegamepad_init()
 * Initializes the mobile gamepad
 */
void mobilegamepad_init(int _flags)
{
    LOG("Initializing the mobile gamepad...");

    /* reset the state */
    flags = _flags;
    current_state = IDLE_STATE;
    is_available = false;
    is_visible = false;
    back_pressed = false;

    /* request touch input */
    if(flags & MOBILEGAMEPAD_WANT_TOUCH_INPUT) {
        if(!al_is_touch_input_installed()) {
            LOG("Touch input isn't available");
            flags &= ~MOBILEGAMEPAD_WANT_TOUCH_INPUT;
        }
        else
            LOG("Will accept touch input");
    }

    /* request mouse input */
    if(flags & MOBILEGAMEPAD_WANT_MOUSE_INPUT) {
        if(!al_is_mouse_installed()) {
            LOG("Mouse input isn't available");
            flags &= ~MOBILEGAMEPAD_WANT_MOUSE_INPUT;
        }
        else
            LOG("Will accept mouse input");
    }

    /* disable the mobile gamepad */
    if(0 == (flags & (MOBILEGAMEPAD_WANT_TOUCH_INPUT | MOBILEGAMEPAD_WANT_MOUSE_INPUT))) {
        LOG("The mobile gamepad isn't available in this system");
        return;
    }

    /* listen to the back button */
    engine_add_event_listener(ALLEGRO_EVENT_KEY_UP, NULL, a5_handle_back_event);

    /* initialize the interactive radii */
    for(int i = 0; i < NUM_CONTROLS; i++)
        interactive_radius[i] = 0.0f;

    /* validate the animations */
    for(int i = 0; i < NUM_CONTROLS; i++) {
        if(!sprite_animation_exists(SPRITE_NAME[i], 0))
            FATAL("Can't find sprite \"%s\"", SPRITE_NAME[i]);
    }

    /* create the actors */
    for(int i = 0; i < NUM_CONTROLS; i++)
        actor[i] = actor_create();

    enable_linear_filtering();

    /* make it visible */
    is_visible = true;
    alpha = 0.0f; /* make it fade in nicely when initializing */

    /* success! */
    is_available = true;
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
    is_available = false;
}

/*
 * mobilegamepad_update()
 * Updates the mobile gamepad
 */
void mobilegamepad_update()
{
    /* do nothing if uanvailable */
    if(!is_available)
        return;

    /* reset the state */
    current_state = IDLE_STATE;

    /* the back button works regardless of the visibility of the mobile gamepad */
    if(back_pressed) {
        current_state.buttons |= MOBILEGAMEPAD_BUTTON_BACK;
        back_pressed = false;
    }

    /* detect if something is pressed on the screen,
       but only if the mobile gamepad is visible */
    if(is_visible && !level_editmode()) {

        /* reset touch */
        touch_t touch[MAX_TOUCHES];
        for(int j = 0; j < MAX_TOUCHES; j++)
            touch[j] = NO_TOUCH;

        /* read touch input */
        if(flags & MOBILEGAMEPAD_WANT_TOUCH_INPUT) {

            ALLEGRO_TOUCH_INPUT_STATE touch_state;
            al_get_touch_input_state(&touch_state);

            for(int i = 0, j = 0; i < MAX_TOUCHES; i++) {
                if(touch_state.touches[i].id >= 0) {
                    touch[j].down = true;
                    touch[j].position = v2d_new(touch_state.touches[i].x, touch_state.touches[i].y);
                    j++;
                }
            }

        }

        /* read mouse input */
        if(flags & MOBILEGAMEPAD_WANT_MOUSE_INPUT) {

            ALLEGRO_MOUSE_STATE mouse;
            al_get_mouse_state(&mouse);

            if(mouse.buttons & 1) {
                touch[0].down = true;
                touch[0].position = v2d_new(mouse.x, mouse.y);
            }

        }

        /* check if any button is pressed */
        for(int i = 0; i < MAX_TOUCHES; i++) {
            if(touch[i].down) {
                for(int j = 0; j < NUM_CONTROLS; j++) {
                    /* coordinates are given in window space */
                    v2d_t offset = v2d_subtract(touch[i].position, actor[j]->position);
                    float distance = v2d_magnitude(offset);

                    if(distance < interactive_radius[j])
                        trigger(j, offset);
                }
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
    /* do nothing if unavailable */
    if(!is_available)
        return;

    /* skip if in the editor */
    if(level_editmode())
        return;

    /* fading in and fading out */
    handle_fade_effect();

    /* render mobile gamepad */
    render_actors();

    /* render the mouse cursor */
    if(flags & MOBILEGAMEPAD_WANT_MOUSE_INPUT) {

        ALLEGRO_MOUSE_STATE mouse;
        al_get_mouse_state(&mouse);

        float r = video_get_window_size().x * 0.01f;
        image_ellipsefill(mouse.x, mouse.y, r, r, color_rgba(255, 255, 0, 64));

    }
}

/*
 * mobilegamepad_is_available()
 * Checks if the mobile gamepad is available in this system
 */
bool mobilegamepad_is_available()
{
    return is_available;
}

/*
 * mobilegamepad_is_visible()
 * Checks if the mobile gamepad is visible
 */
bool mobilegamepad_is_visible()
{
    return is_visible;
}

/*
 * mobilegamepad_get_state()
 * Reads the current state of the mobile gamepad
 */
void mobilegamepad_get_state(mobilegamepad_state_t* state)
{
    *state = is_available ? current_state : IDLE_STATE;
}

/*
 * mobilegamepad_fadein()
 * Makes the mobile gamepad visible
 */
void mobilegamepad_fadein()
{
    if(is_available)
        is_visible = true;
}

/*
 * mobilegamepad_fadeout()
 * Makes the mobile gamepad invisible
 */
void mobilegamepad_fadeout()
{
    if(is_available)
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
            if(v2d_magnitude(offset) > interactive_radius[DPAD] * DPAD_DEADZONE_THRESHOLD) {

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

        case DPAD_STICK:
            /* do nothing */
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
        [DPAD_STICK] = DPAD_STICK_ANIMATION_NUMBER[(current_state.dpad != MOBILEGAMEPAD_DPAD_CENTER) ? PRESSED : UNPRESSED],
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
    v2d_t window_scale = v2d_new(window_size.x / REFERENCE_RESOLUTION.x, window_size.y / REFERENCE_RESOLUTION.y);
    float scale = max(window_scale.x, window_scale.y);

    /* animate the actors */
    animate_actors();

    /* update the attributes of the actors */
    for(int i = 0; i < NUM_CONTROLS; i++) {
        actor[i]->position = v2d_compmult(RELATIVE_POSITION[i], window_size);
        actor[i]->scale = v2d_new(scale, scale);
        actor[i]->alpha = alpha;
    }

    /* update the interactive radii of the controls based on the scale of the actors */
    for(int i = 0; i < NUM_CONTROLS; i++) {
        const image_t* image = actor_image(actor[i]);
        int width = image_width(image);
        int height = image_height(image);

        int unscaled_diameter = max(width, height);
        float unscaled_radius = unscaled_diameter * 0.5f;

        interactive_radius[i] = unscaled_radius * scale;
    }

    /* reposition the dpad stick */
    v2d_t stick_offset = dpad_stick_offset(scale);
    actor[DPAD_STICK]->position.x += stick_offset.x;
    actor[DPAD_STICK]->position.y += stick_offset.y;
}

void render_actors()
{
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);

    /* render the mobile controls in screen space */
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

/* handle a keyboard ALLEGRO_KEY_BACK event */
void a5_handle_back_event(const ALLEGRO_EVENT* event, void* data)
{
    /* When triggering the back button or performing a back gesture on Android,
       we receive a keyDown event followed by a keyUp event - possibly in the
       same frame. Therefore, let's just focus on the keyUp event. */
    if(event->type == ALLEGRO_EVENT_KEY_UP && event->keyboard.keycode == ALLEGRO_KEY_BACK) {

        /* we clear up this flag in the main loop */
        back_pressed = true;

    }

    (void)data;
}

/* compute the current offset of the dpad stick
   (relative to the center of the dpad) */
v2d_t dpad_stick_offset(float scale)
{
    /* compute a smooth transition and determine the angle of the dpad stick */
    static float transition = 0.0f, angle = 0.0f;
    float ds = timer_get_delta() / DPAD_STICK_MOVEMENT_TIME;

    if(current_state.dpad != MOBILEGAMEPAD_DPAD_CENTER) {
        transition = min(1.0f, transition + ds);
        angle = DPAD_STICK_ANGLE[current_state.dpad & DPAD_STICK_ANGLE_MASK];
    }
    else
        transition = max(0.0f, transition - ds);

    /* compute the offset of the dpad stick using polar coordinates */
    v2d_t action_offset = actor_action_offset(actor[DPAD_STICK]);
    float unscaled_max_length = v2d_magnitude(action_offset);
    float max_length = unscaled_max_length * scale;
    float current_length = max_length * transition;

    v2d_t unit_vector = v2d_new(cosf(angle), sinf(angle));
    v2d_t offset = v2d_multiply(unit_vector, floorf(current_length));

    /* done! */
    return offset;
}