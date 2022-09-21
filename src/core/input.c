/*
 * Open Surge Engine
 * input.c - input management
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

#include "input.h"
#include "util.h"
#include "video.h"
#include "logfile.h"
#include "timer.h"
#include "inputmap.h"
#include "stringutil.h"

/* <base class>: generic input */
struct input_t {
    bool enabled; /* enable input? */
    bool state[IB_MAX], oldstate[IB_MAX]; /* states */
    void (*update)(input_t*); /* update method */
};

/* <derived class>: mouse input */
struct inputmouse_t {
    input_t base;
    int x, y; /* cursor position */
    int dx, dy, dz; /* delta-x, delta-y, delta-z (mouse mickeys) */
};
static void inputmouse_update(input_t* in);

/* <derived class>: computer-generated input */
struct inputcomputer_t {
    input_t base;
};
static void inputcomputer_update(input_t* in);

/* <derived class>: input with user-defined mapping */
struct inputuserdefined_t {
    input_t base;
    const inputmap_t *inputmap; /* input mapping */
};
static void inputuserdefined_update(input_t* in);

/* list of inputs */
typedef struct input_list_t input_list_t;
struct input_list_t {
    input_t *data;
    input_list_t *next;
};



/* keyboard input */
static bool a5_key[ALLEGRO_KEY_MAX] = { false };

/* mouse input */
#define LEFT_MOUSE_BUTTON   1 /* primary button, 1 << 0 */
#define RIGHT_MOUSE_BUTTON  2 /* secondary button, 1 << 1 */
#define MIDDLE_MOUSE_BUTTON 4 /* tertiary button, 1 << 2 */
static struct {
    int x, y, z; /* position of the cursor */
    int dx, dy, dz; /* deltas */
    int b; /* bit vector of active buttons */
} a5_mouse = { 0 };

/* joystick input */
#define MAX_JOYS      8 /* maximum number of joysticks */
#define AXIS_X        0 /* x-axis of a stick */
#define AXIS_Y        1 /* y-axis of a stick */
#define REQUIRED_AXES 2 /* required number of axes of a stick */

static struct {
    float axis[REQUIRED_AXES]; /* -1.0 <= axis[i] <= 1.0 */
    uint32_t button; /* bit vector */
} joy[MAX_JOYS];

static bool ignore_joystick = false;

/* dead-zone for analog input */
static const float DEADZONE_THRESHOLD = 0.2f;

/* analog to digital conversion thresholds for the (x,y) axes */
static const float analog2digital_threshold[REQUIRED_AXES] = {

    /* Pressing up + jump won't make the player jump */
    [AXIS_X] = 0.25f,
    [AXIS_Y] = 0.75f

};

/* private data */
static const char* DEFAULT_INPUTMAP_NAME = "default";
static input_list_t *inlist = NULL;

/* private methods */
static void input_register(input_t *in);
static void input_unregister(input_t *in);
static inline void input_clear(input_t *in);
static void log_joysticks();



/*
 * input_init()
 * Initializes the input module
 */
void input_init()
{
    extern ALLEGRO_EVENT_QUEUE* a5_event_queue;
    logfile_message("Initializing the input system...");

    /* initialize the Allegro input system */
    if(!al_install_keyboard())
        fatal_error("Can't initialize the keyboard");
    al_register_event_source(a5_event_queue, al_get_keyboard_event_source());

    if(!al_install_mouse())
        fatal_error("Can't initialize the mouse");
    al_register_event_source(a5_event_queue, al_get_mouse_event_source());

    if(!al_install_joystick())
        fatal_error("Can't initialize the joystick subsystem");
    al_register_event_source(a5_event_queue, al_get_joystick_event_source());

    /* initialize the input list */
    inlist = NULL;

    /* initialize mouse input */
    a5_mouse.b = 0;
    a5_mouse.x = a5_mouse.y = a5_mouse.z = 0;
    a5_mouse.dx = a5_mouse.dy = a5_mouse.dz = 0;

    /* initialize keyboard input */
    for(int i = 0; i < ALLEGRO_KEY_MAX; i++)
        a5_key[i] = false;

    /* initialize joystick input */
    ignore_joystick = !input_is_joystick_available();
    log_joysticks();

    /* loading custom input mappings */
    inputmap_init();
}

/*
 * input_update()
 * Updates all the registered input objects
 */
void input_update()
{
    ALLEGRO_MOUSE_STATE state;
    int num_joys = min(al_get_num_joysticks(), MAX_JOYS);

    /* read mouse input */
    al_get_mouse_state(&state);
    a5_mouse.dx = state.x - a5_mouse.x;
    a5_mouse.dy = state.y - a5_mouse.y;
    a5_mouse.dz = state.z - a5_mouse.z;
    a5_mouse.x = state.x;
    a5_mouse.y = state.y;
    a5_mouse.z = state.z;
    /*a5_mouse.b is received from the event queue */

    /* read joystick input */
    for(int j = 0; j < num_joys; j++) {
        ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);
        int num_sticks = al_get_joystick_num_sticks(joystick);
        int num_buttons = min(al_get_joystick_num_buttons(joystick), MAX_JOYSTICK_BUTTONS);

        /* read the current state */
        ALLEGRO_JOYSTICK_STATE state;
        al_get_joystick_state(joystick, &state);

        /* read buttons */
        joy[j].button = 0;
        for(int b = 0; b < num_buttons; b++)
            joy[j].button |= (state.button[b] != 0) << b;

        /*

        In order to read directional input from the analog sticks, we use the
        following heuristic: read the first reported stick that has two axes.
        Such stick likely corresponds to the left analog stick of the connected
        gamepad - if it exists.

        If we take a look at SDL_GameControllerDB, a community sourced database
        of game controller mappings, we'll see that, in most controllers, entry
        "leftx" is mapped to "a0" (axis 0) and that entry "lefty" is mapped to
        "a1" (axis 1). Entries "rightx" and "righty" aren't mapped as uniformly.
        I infer that axis 0 and 1 likely correspond to the two axes of stick 0
        reported by the Allegro API.

        We only read one analog stick at this time. Other sticks may correspond
        to shoulder buttons acting as analog sticks with a single axis. I don't
        know if the second stick that has two axes (which may be stick 1, 2...),
        as reported by Allegro, can be reliably associated with the right analog
        stick*. Further testing is desirable. How does Allegro divide the analog
        input in sticks**? jstest only reports axes and buttons.

        https://github.com/gabomdq/SDL_GameControllerDB

        (*) Yes, it can if we use Allegro's XInput driver on Windows. Stick 0
            is the "Left Thumbstick" and stick 1 is the "Right Thumbstick". See
            src/win/wjoyxi.c in Allegro's source code.

        (**) the division is merely logical in Allegro's Windows XInput driver.
             A XINPUT_STATE structure holds a XINPUT_GAMEPAD structure. The
             latter describes the current state of the controller. The state
             of the digital buttons is described by a single bitmask, which is
             mapped to logical buttons defined in the Allegro driver. The state
             of the analog sticks is mapped to different logical sticks and axes
             that are also defined by Allegro in joyxi_convert_state().

             https://learn.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad

        */

        /* read sticks */
        joy[j].axis[AXIS_X] = 0.0f;
        joy[j].axis[AXIS_Y] = 0.0f;

        for(int stick_id = 0; stick_id < num_sticks; stick_id++) {

            /* safety check */
            /* I use <= because I think al_get_joystick_num_axes() cannot be
               fully trusted with controllers in DInput mode.
               https://www.allegro.cc/forums/thread/614996/1 */
            if(REQUIRED_AXES <= al_get_joystick_num_axes(joystick, stick_id)) {

                float x = state.stick[stick_id].axis[0];
                float y = state.stick[stick_id].axis[1];

                /* ignore the dead-zone and normalize the data back to [-1,1] */
                const float NORMALIZER = 1.0f - DEADZONE_THRESHOLD;
                if(fabs(x) >= DEADZONE_THRESHOLD)
                    joy[j].axis[AXIS_X] += (x - DEADZONE_THRESHOLD * sign(x)) / NORMALIZER;
                if(fabs(y) >= DEADZONE_THRESHOLD)
                    joy[j].axis[AXIS_Y] += (y - DEADZONE_THRESHOLD * sign(y)) / NORMALIZER;

                break;

            }

        }

        joy[j].axis[AXIS_X] = clip(joy[j].axis[AXIS_X], -1.0f, 1.0f);
        joy[j].axis[AXIS_Y] = clip(joy[j].axis[AXIS_Y], -1.0f, 1.0f);
    }

    /* update the input objects */
    for(input_list_t* it = inlist; it; it = it->next) {
        for(int i = 0; i < IB_MAX; i++)
            it->data->oldstate[i] = it->data->state[i];
        it->data->update(it->data);
    }
}


/*
 * input_release()
 * Releases the input module
 */
void input_release()
{
    input_list_t *it, *next;

    logfile_message("input_release()");
    inputmap_release();

    logfile_message("Releasing registered input objects...");
    for(it = inlist; it; it=next) {
        next = it->next;
        free(it->data);
        free(it);
    }
}


/*
 * input_button_down()
 * Checks if a given button is down
 */
bool input_button_down(input_t *in, inputbutton_t button)
{
    return in->enabled && in->state[(int)button];
}


/*
 * input_button_pressed()
 * Checks if a given button has just been pressed (not held down)
 */
bool input_button_pressed(input_t *in, inputbutton_t button)
{
    return in->enabled && (in->state[(int)button] && !in->oldstate[(int)button]);
}


/*
 * input_button_up()
 * Checks if a given button has just been released
 */
bool input_button_up(input_t *in, inputbutton_t button)
{
    return in->enabled && (!in->state[(int)button] && in->oldstate[(int)button]);
}




/* 
 * input_create_mouse()
 * Creates an input object based on the mouse
 */
input_t *input_create_mouse()
{
    inputmouse_t *me = mallocx(sizeof *me);
    input_t *in = (input_t*)me;

    in->update = inputmouse_update;
    in->enabled = true;
    me->dx = 0;
    me->dy = 0;
    me->x = 0;
    me->y = 0;
    input_clear(in);

    input_register(in);
    return in;
}




/*
 * input_create_computer()
 * Creates an object that receives "input" from
 * the computer
 */
input_t *input_create_computer()
{
    inputcomputer_t *me = mallocx(sizeof *me);
    input_t *in = (input_t*)me;

    in->update = inputcomputer_update;
    in->enabled = true;
    input_clear(in);

    input_register(in);
    return in;
}

/*
 * input_create_user()
 * Creates an user's custom input device
 */
input_t *input_create_user(const char* inputmap_name)
{
    inputuserdefined_t *me = mallocx(sizeof *me);
    input_t *in = (input_t*)me;

    in->update = inputuserdefined_update;
    in->enabled = true;
    input_clear(in);

    /* if there isn't such a inputmap_name, the game will exit beautifully */
    inputmap_name = inputmap_name ? inputmap_name : DEFAULT_INPUTMAP_NAME;
    me->inputmap = inputmap_get(inputmap_name);

    /* check joystick stuff */
    if(me->inputmap->joystick.enabled && (!input_is_joystick_enabled() || me->inputmap->joystick.id >= input_number_of_joysticks())) {
        logfile_message(
            "WARNING: inputmap '%s' accepts a joystick (id: %d, plugged joysticks: %d), but %s.",
            inputmap_name, me->inputmap->joystick.id, input_number_of_joysticks(),
            (input_is_joystick_enabled() ? "the joystick id is invalid" : "the user isn't using a joystick")
        );
    }

    input_register(in);
    return in;
}


/*
 * input_destroy()
 * Destroys an input object
 */
void input_destroy(input_t *in)
{
    input_unregister(in);
    free(in);
}


/*
 * input_ignore()
 * Ignore Control
 */
void input_ignore(input_t *in)
{
    in->enabled = false;
}


/*
 * input_restore()
 * Restore Control
 */
void input_restore(input_t *in)
{
    in->enabled = true;
}



/*
 * input_is_ignored()
 * Checks if the input device is ignored
 */
bool input_is_ignored(input_t *in)
{
    return !in->enabled;
}





/*
 * input_simulate_button_down()
 * Useful for computer-controlled input objects
 */
void input_simulate_button_down(input_t *in, inputbutton_t button)
{
    in->oldstate[(int)button] = in->state[(int)button];
    in->state[(int)button] = true;
}



/*
 * input_simulate_button_up()
 * Useful for computer-controlled input objects
 */
void input_simulate_button_up(input_t *in, inputbutton_t button)
{
    in->oldstate[(int)button] = in->state[(int)button];
    in->state[(int)button] = false;
}



/*
 * input_reset()
 * Resets the input object like if nothing is being held down
 */
void input_reset(input_t *in)
{
    for(int i = 0; i < IB_MAX; i++)
        input_simulate_button_up(in, (inputbutton_t)i);
}



/*
 * input_is_joystick_available()
 * Checks if there is a plugged joystick
 */
bool input_is_joystick_available()
{
    return input_number_of_joysticks() > 0;
}

/*
 * input_is_joystick_enabled()
 * Is the joystick input enabled?
 */
bool input_is_joystick_enabled()
{
    return !ignore_joystick && input_is_joystick_available();
}


/*
 * input_ignore_joystick()
 * Ignores the input received from a joystick (if available)
 */
void input_ignore_joystick(bool ignore)
{
    ignore_joystick = ignore;
    if(!ignore_joystick && input_number_of_joysticks() == 0) {
        video_showmessage("No joysticks have been found!");
        ignore_joystick = true;
    }
}


/*
 * input_is_joystick_ignored()
 * Is the joystick input ignored?
 */
bool input_is_joystick_ignored()
{
    return ignore_joystick;
}


/*
 * input_number_of_joysticks()
 * number of plugged joysticks
 */
int input_number_of_joysticks()
{
    return al_get_num_joysticks();
}



/*
 * input_get_xy()
 * Gets the xy coordinates (this will only work for a mouse device)
 */
v2d_t input_get_xy(inputmouse_t *in)
{
    return v2d_new(in->x, in->y);
}

/*
 * input_change_mapping()
 * Changes the input mapping of an user-defined input device
 */
void input_change_mapping(inputuserdefined_t *in, const char* inputmap_name)
{
    if(str_icmp(inputmap_name, input_get_mapping_name(in)) != 0) {
        input_clear((input_t*)in);
        in->inputmap = inputmap_get(inputmap_name ? inputmap_name : DEFAULT_INPUTMAP_NAME);
        ((input_t*)in)->update((input_t*)in);
    }
}

/*
 * input_get_mapping_name()
 * Returns the mapping name associated to this user-defined input device
 */
const char* input_get_mapping_name(inputuserdefined_t *in)
{
    return in->inputmap->name;
}









/* private methods */


/* registers an input device */
void input_register(input_t *in)
{
    input_list_t *node = mallocx(sizeof *node);

    node->data = in;
    node->next = inlist;
    inlist = node;
}

/* unregisters the given input device */
void input_unregister(input_t *in)
{
    input_list_t *node, *next;

    if(!inlist)
        return;

    if(inlist->data == in) {
        next = inlist->next;
        free(inlist);
        inlist = next;
    }
    else {
        node = inlist;
        while(node->next && node->next->data != in)
            node = node->next;
        if(node->next) {
            next = node->next->next; 
            free(node->next);
            node->next = next;
        }
    }
}

/* clears all the input buttons */
void input_clear(input_t *in)
{
    for(int i = 0; i < IB_MAX; i++)
        in->state[i] = in->oldstate[i] = false;
}

/* update specific input devices */
void inputmouse_update(input_t* in)
{
    inputmouse_t *mouse = (inputmouse_t*)in;

    mouse->x = a5_mouse.x;
    mouse->y = a5_mouse.y;
    mouse->dx = a5_mouse.dx;
    mouse->dy = a5_mouse.dy;
    mouse->dz = a5_mouse.dz;

    in->state[IB_UP] = (mouse->dz > 0);
    in->state[IB_DOWN] = (mouse->dz < 0);
    in->state[IB_LEFT] = false;
    in->state[IB_RIGHT] = false;
    in->state[IB_FIRE1] = (a5_mouse.b & LEFT_MOUSE_BUTTON);
    in->state[IB_FIRE2] = (a5_mouse.b & RIGHT_MOUSE_BUTTON);
    in->state[IB_FIRE3] = (a5_mouse.b & MIDDLE_MOUSE_BUTTON);
    in->state[IB_FIRE4] = false;
    in->state[IB_FIRE5] = false;
    in->state[IB_FIRE6] = false;
    in->state[IB_FIRE7] = false;
    in->state[IB_FIRE8] = false;
}

void inputcomputer_update(input_t* in)
{
    ;
}

void inputuserdefined_update(input_t* in)
{
    inputuserdefined_t *me = (inputuserdefined_t*)in;
    const inputmap_t *im = me->inputmap;
    inputbutton_t button;

    for(button = 0; button < IB_MAX; button++)
        in->state[button] = false;

    if(im->keyboard.enabled) {
        for(button = 0; button < IB_MAX; button++)
            in->state[button] = (im->keyboard.scancode[button] > 0) && a5_key[im->keyboard.scancode[button]];
    }

    if(im->joystick.enabled && input_is_joystick_enabled()) {
        int num_joysticks = min(input_number_of_joysticks(), MAX_JOYS);
        if(im->joystick.id < num_joysticks) {
            in->state[IB_UP] = in->state[IB_UP] || (joy[im->joystick.id].axis[AXIS_Y] <= -analog2digital_threshold[AXIS_Y]);
            in->state[IB_DOWN] = in->state[IB_DOWN] || (joy[im->joystick.id].axis[AXIS_Y] >= analog2digital_threshold[AXIS_Y]);
            in->state[IB_LEFT] = in->state[IB_LEFT] || (joy[im->joystick.id].axis[AXIS_X] <= -analog2digital_threshold[AXIS_X]);
            in->state[IB_RIGHT] = in->state[IB_RIGHT] || (joy[im->joystick.id].axis[AXIS_X] >= analog2digital_threshold[AXIS_X]);
            for(button = 0; button < IB_MAX; button++) {
                uint32_t button_mask = im->joystick.button_mask[(int)button];
                in->state[button] = in->state[button] || ((joy[im->joystick.id].button & button_mask) != 0);
            }
        }
    }
}

/* handle an ALLEGRO_KEYBOARD_EVENT */
void a5_handle_keyboard_event(const ALLEGRO_EVENT* event)
{
    switch(event->type) {

        case ALLEGRO_EVENT_KEY_DOWN:
            a5_key[event->keyboard.keycode] = true;
            break;

        case ALLEGRO_EVENT_KEY_UP:
            a5_key[event->keyboard.keycode] = false;
            break;

    }
}

/* handle an ALLEGRO_MOUSE_EVENT */
void a5_handle_mouse_event(const ALLEGRO_EVENT* event)
{
    switch(event->type) {

        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            a5_mouse.b |= 1 << (event->mouse.button - 1);
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            a5_mouse.b &= ~(1 << (event->mouse.button - 1));
            break;

    }
}

/* handle an ALLEGRO_JOYSTICK_EVENT */
void a5_handle_joystick_event(const ALLEGRO_EVENT* event)
{
    switch(event->type) {
        /*
         * Joystick input based on ALLEGRO_JOYSTICK_STATE
         * seems to work better according to several users
         * 
         * tested with Allegro 5.2.5 on Windows using
         * DirectInput devices
         */
#if 0
        case ALLEGRO_EVENT_JOYSTICK_AXIS:
            break;

        case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            break;

        case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
            break;
#endif
        /* hot plugging */
        case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION: {
            int num_joysticks;
            al_reconfigure_joysticks();

            num_joysticks = al_get_num_joysticks();
            if(num_joysticks > 0) {
                /* display message as soon as new joysticks are plugged */
                video_showmessage("Found %d joystick%s:", num_joysticks, num_joysticks == 1 ? "" : "s");
                for(int j = 0; j < num_joysticks; j++) {
                    ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);
                    video_showmessage("%s", al_get_joystick_name(joystick));
                }

                /* activate input */
                input_ignore_joystick(false); /* the user probably wants this (automatic joystick detection) */
            }
            else {
                video_showmessage("No joysticks have been detected");
                input_ignore_joystick(true);
            }

            /* log joysticks */
            log_joysticks();

            /* done! */
            break;
        }
    }
}

/* log joysticks */
void log_joysticks()
{
    int num_joysticks = al_get_num_joysticks();

    if(num_joysticks == 0) {
        logfile_message("No joysticks have been found");
        return;
    }

    logfile_message("Found %d joystick%s", num_joysticks, num_joysticks == 1 ? "" : "s");
    for(int j = 0; j < num_joysticks; j++) {
        ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);

        logfile_message("- Joystick %d (\"%s\"):", j, al_get_joystick_name(joystick));
        logfile_message("-- %d sticks, %d buttons", al_get_joystick_num_sticks(joystick), al_get_joystick_num_buttons(joystick));

        for(int s = 0; s < al_get_joystick_num_sticks(joystick); s++) {
            static const char* joy_flag[4] = { "", "digital", "analog", "" };

            logfile_message("-- stick %d (\"%s\")", s, al_get_joystick_stick_name(joystick, s));
            logfile_message("--- flags: 0x%X %s", al_get_joystick_stick_flags(joystick, s), joy_flag[al_get_joystick_stick_flags(joystick, s) & 0x3]);
            logfile_message("--- number of axes: %d", al_get_joystick_num_axes(joystick, s));

            for(int a = 0; a < al_get_joystick_num_axes(joystick, s); a++)
                logfile_message("---- axis %d (\"%s\")", a, al_get_joystick_axis_name(joystick, s, a));
        }

        for(int b = 0; b < al_get_joystick_num_buttons(joystick); b++)
            logfile_message("-- button %d (\"%s\")", b, al_get_joystick_button_name(joystick, b));
    }
}