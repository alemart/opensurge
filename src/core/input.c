/*
 * Open Surge Engine
 * input.c - input management
 * Copyright (C) 2008-2011, 2019-2020  Alexandre Martins <alemartf@gmail.com>
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

#if defined(A5BUILD)
#include <allegro5/allegro.h>
#else
#include <allegro.h>
#endif

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

#if defined(A5BUILD)
/* mouse struct */
static struct {
    int x, y, z, dx, dy, dz, b;
} a5_mouse = { 0 };

/* joysticks */
#define MAX_JOYS 8 /* maximum number of joysticks */
#define MAX_AXES 2 /* maximum number of axes */
typedef char __joy_max[-(!(MAX_JOYS && !(MAX_JOYS & (MAX_JOYS-1))))]; /* MAX_JOYS must be a power of 2 */
typedef char __joy_axis[-(MAX_AXES < 2)]; /* MAX_AXES must be at least 2 */

static bool ignore_joystick = false;
static struct {
    float axis[MAX_AXES]; /* -1.0 <= axis <= 1.0 */
    uint32_t button; /* bit vector */
} joy[MAX_JOYS];
#else
/* joysticks */
static bool got_joystick;
static bool ignore_joystick;
static bool plugged_joysticks;
static bool are_all_joysticks_valid();

/* misc */
static void get_mouse_mickeys_ex(int *mickey_x, int *mickey_y, int *mickey_z);
#endif

/* private data */
static const char* DEFAULT_INPUTMAP_NAME = "default";
static const float joy_analog_threshold = 0.6f; /* pressing up + jump won't make the player jump */
static input_list_t *inlist;

/* private methods */
static void input_register(input_t *in);
static void input_unregister(input_t *in);
static inline void input_clear(input_t *in);



/*
 * input_init()
 * Initializes the input module
 */
void input_init()
{
#if defined(A5BUILD)
    extern ALLEGRO_EVENT_QUEUE* a5_event_queue;
    logfile_message("Initializing the input system...");

    /* initializing Allegro */
    if(!al_install_keyboard())
        fatal_error("Can't initialize the keyboard");
    al_register_event_source(a5_event_queue, al_get_keyboard_event_source());

    if(!al_install_mouse())
        fatal_error("Can't initialize the mouse");
    al_register_event_source(a5_event_queue, al_get_mouse_event_source());

    if(!al_install_joystick())
        fatal_error("Can't initialize the joystick subsystem");
    al_register_event_source(a5_event_queue, al_get_joystick_event_source());

    /* joystick config */
    if(input_is_joystick_available()) {
        logfile_message("Found %d joystick(s)", input_number_of_joysticks());
        ignore_joystick = false;
    }
    else {
        logfile_message("No joysticks have been found");
        ignore_joystick = true;
    }

    /* initializing the input list */
    inlist = NULL;

    /* loading custom input mappings */
    inputmap_init();
#else
    logfile_message("input_init()");

    /* installing Allegro stuff */
    logfile_message("Installing Allegro input devices...");
    if(install_keyboard() != 0)
        logfile_message("install_keyboard() failed: %s", allegro_error);
    if(install_mouse() == -1)
        logfile_message("install_mouse() failed: %s", allegro_error);

    /* initializing */
    inlist = NULL;

    /* joystick */
    got_joystick = false;
    ignore_joystick = true;
    plugged_joysticks = 0;
    if(install_joystick(JOY_TYPE_AUTODETECT) == 0) {
        if(num_joysticks > 0 && are_all_joysticks_valid()) {
            got_joystick = true;
            ignore_joystick = false;
            plugged_joysticks = num_joysticks;
            logfile_message("Joystick(s) installed successfully! Number of plugged joysticks: %d", plugged_joysticks);
        }
        else if(num_joysticks <= 0) {
            logfile_message("No joystick has been detected.");
        }
        else {
            logfile_message(
                "Invalid joystick! Please make sure all the connected (digital) "
                "joysticks have at least 4 buttons and 2 axis."
            );
        }
    }
    else
        logfile_message("install_joystick() failed: %s", allegro_error);

    /* loading custom input mappings */
    inputmap_init();
#endif
}

/*
 * input_update()
 * Updates all the registered input objects
 */
void input_update()
{
#if defined(A5BUILD)
    int num_joys = min(al_get_num_joysticks(), MAX_JOYS);
    extern int a5_mouse_b;
    ALLEGRO_MOUSE_STATE state;

    /* read mouse input */
    al_get_mouse_state(&state);
    a5_mouse.dx = state.x - a5_mouse.x;
    a5_mouse.dy = state.y - a5_mouse.y;
    a5_mouse.dz = state.z - a5_mouse.z;
    a5_mouse.x = state.x;
    a5_mouse.y = state.y;
    a5_mouse.z = state.z;
    a5_mouse.b = a5_mouse_b; /* received from the event queue */

    /* read joystick input */
    for(int j = 0; j < num_joys; j++) {
        ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);
        int num_sticks = al_get_joystick_num_sticks(joystick);
        int num_buttons = min(al_get_joystick_num_buttons(joystick), MAX_JOYSTICK_BUTTONS);

        /* read current state */
        ALLEGRO_JOYSTICK_STATE state;
        al_get_joystick_state(joystick, &state);

        /* read sticks */
        for(int a = 0; a < MAX_AXES; a++)
            joy[j].axis[a] = 0.0f;

        for(int stick_id = 0; stick_id < num_sticks; stick_id++) {
            int num_axes = min(MAX_AXES, al_get_joystick_num_axes(joystick, stick_id));
            for(int a = 0; a < num_axes; a++) {
                if(fabs(state.stick[stick_id].axis[a]) >= joy_analog_threshold)
                    joy[j].axis[a] += state.stick[stick_id].axis[a];
            }
        }

        for(int a = 0; a < MAX_AXES; a++)
            joy[j].axis[a] = clip(joy[j].axis[a], -1.0f, 1.0f);

        /* read buttons */
        joy[j].button = 0;
        for(int b = 0; b < num_buttons; b++)
            joy[j].button |= (state.button[b] ? 1 : 0) << b;
    }

    /* updating the input objects */
    for(input_list_t* it = inlist; it; it = it->next) {
        for(int i = 0; i < IB_MAX; i++)
            it->data->oldstate[i] = it->data->state[i];
        it->data->update(it->data);
    }
#else
    int i;
    static int old_f6 = 0;
    input_list_t *it;

    /* polling devices */
    if(keyboard_needs_poll())
        poll_keyboard();

    if(mouse_needs_poll())
        poll_mouse();

    if(input_is_joystick_enabled())
        poll_joystick();

    /* updating input objects */
    for(it = inlist; it; it=it->next) {

        /* updating the old states */
        for(i=0; i<IB_MAX; i++)
            it->data->oldstate[i] = it->data->state[i];

        /* update the appropriate input device */
        it->data->update(it->data);

    }

    /* ignore/restore joystick */
    if(!old_f6 && key[KEY_F6]) {
        input_ignore_joystick(!input_is_joystick_ignored());
        video_showmessage("%s joystick input", input_is_joystick_ignored() ? "Ignored" : "Restored");
    }
    old_f6 = key[KEY_F6];

    /* quit game */
    if(key[KEY_ALT] && key[KEY_F4])
        game_quit();
#endif
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
#if defined(A5BUILD)
    return !ignore_joystick && input_is_joystick_available();
#else
    return got_joystick && !ignore_joystick;
#endif
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
#if defined(A5BUILD)
    return al_get_num_joysticks();
#else
    return plugged_joysticks;
#endif
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
#if defined(A5BUILD)
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
    in->state[IB_FIRE1] = (a5_mouse.b & 1);
    in->state[IB_FIRE2] = (a5_mouse.b & 2);
    in->state[IB_FIRE3] = (a5_mouse.b & 4);
    in->state[IB_FIRE4] = false;
    in->state[IB_FIRE5] = false;
    in->state[IB_FIRE6] = false;
    in->state[IB_FIRE7] = false;
    in->state[IB_FIRE8] = false;
#else
    inputmouse_t *me = (inputmouse_t*)in;

    get_mouse_mickeys_ex(&me->dx, &me->dy, &me->dz);
    me->x = mouse_x;
    me->y = mouse_y;
    in->state[IB_UP] = (me->dz < 0);
    in->state[IB_DOWN] = (me->dz > 0);
    in->state[IB_LEFT] = false;
    in->state[IB_RIGHT] = false;
    in->state[IB_FIRE1] = (mouse_b & 1);
    in->state[IB_FIRE2] = (mouse_b & 2);
    in->state[IB_FIRE3] = (mouse_b & 4);
    in->state[IB_FIRE4] = false;
    in->state[IB_FIRE5] = false;
    in->state[IB_FIRE6] = false;
    in->state[IB_FIRE7] = false;
    in->state[IB_FIRE8] = false;
#endif
}

void inputcomputer_update(input_t* in)
{
    ;
}

void inputuserdefined_update(input_t* in)
{
#if defined(A5BUILD)
    inputuserdefined_t *me = (inputuserdefined_t*)in;
    const inputmap_t *im = me->inputmap;
    extern bool a5_key[];
    inputbutton_t button;

    for(button = 0; button < IB_MAX; button++)
        in->state[button] = false;

    if(im->keyboard.enabled) {
        for(button = 0; button < IB_MAX; button++)
            in->state[button] = (im->keyboard.scancode[button] > 0) && a5_key[im->keyboard.scancode[button]];
    }

    if(im->joystick.enabled) {
        int num_joysticks = min(input_number_of_joysticks(), MAX_JOYS);
        if(input_is_joystick_enabled() && im->joystick.id < num_joysticks) {
            in->state[IB_UP] = in->state[IB_UP] || (joy[im->joystick.id].axis[1] <= -joy_analog_threshold);
            in->state[IB_DOWN] = in->state[IB_DOWN] || (joy[im->joystick.id].axis[1] >= joy_analog_threshold);
            in->state[IB_LEFT] = in->state[IB_LEFT] || (joy[im->joystick.id].axis[0] <= -joy_analog_threshold);
            in->state[IB_RIGHT] = in->state[IB_RIGHT] || (joy[im->joystick.id].axis[0] >= joy_analog_threshold);
            for(button = IB_FIRE1; button <= IB_FIRE8; button++) {
                uint32_t button_mask = im->joystick.button_mask[(int)button];
                in->state[button] = in->state[button] || ((joy[im->joystick.id].button & button_mask) != 0);
            }
        }
    }
#else
    inputuserdefined_t *me = (inputuserdefined_t*)in;
    const inputmap_t *im = me->inputmap;
    int i, k;

    for(i=0; i<IB_MAX; i++)
        in->state[i] = false;

    if(im->keyboard.enabled) {
        for(i=0; i<IB_MAX; i++)
            in->state[i] = (im->keyboard.scancode[i] > 0) && key[ im->keyboard.scancode[i] ];
    }

    /* this needs update (Allegro 4) */
    /*
    if(input_is_joystick_enabled() && im->joystick.enabled && im->joystick.id < input_number_of_joysticks()) {
        k = im->joystick.id;
        in->state[IB_UP] = in->state[IB_UP] || joy[k].stick[0].axis[1].d1;
        in->state[IB_DOWN] = in->state[IB_DOWN] || joy[k].stick[0].axis[1].d2;
        in->state[IB_LEFT] = in->state[IB_LEFT] || joy[k].stick[0].axis[0].d1;
        in->state[IB_RIGHT] = in->state[IB_RIGHT] || joy[k].stick[0].axis[0].d2;
        in->state[IB_FIRE1] = in->state[IB_FIRE1] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE1]) && joy[k].button[ im->joystick.button[IB_FIRE1] ].b);
        in->state[IB_FIRE2] = in->state[IB_FIRE2] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE2]) && joy[k].button[ im->joystick.button[IB_FIRE2] ].b);
        in->state[IB_FIRE3] = in->state[IB_FIRE3] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE3]) && joy[k].button[ im->joystick.button[IB_FIRE3] ].b);
        in->state[IB_FIRE4] = in->state[IB_FIRE4] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE4]) && joy[k].button[ im->joystick.button[IB_FIRE4] ].b);
        in->state[IB_FIRE5] = in->state[IB_FIRE5] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE5]) && joy[k].button[ im->joystick.button[IB_FIRE5] ].b);
        in->state[IB_FIRE6] = in->state[IB_FIRE6] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE6]) && joy[k].button[ im->joystick.button[IB_FIRE6] ].b);
        in->state[IB_FIRE7] = in->state[IB_FIRE7] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE7]) && joy[k].button[ im->joystick.button[IB_FIRE7] ].b);
        in->state[IB_FIRE8] = in->state[IB_FIRE8] || ((joy[k].num_buttons > im->joystick.button[IB_FIRE8]) && joy[k].button[ im->joystick.button[IB_FIRE8] ].b);
    }
    */
#endif
}

#if defined(A5BUILD)
/* handle an ALLEGRO_JOYSTICK_EVENT */
void a5_handle_joystick_event(const ALLEGRO_EVENT* event)
{
    switch(event->type) {
#if 0
        /*
         * Joystick input based on ALLEGRO_JOYSTICK_STATE
         * seems to work better according to several users
         * 
         * tested with Allegro 5.2.5 on Windows using
         * DirectInput devices
         */
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

            /* log it */
            logfile_message("Found %d joystick%s", num_joysticks, num_joysticks == 1 ? "" : "s");
            for(int j = 0; j < num_joysticks; j++) {
                ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);
                logfile_message("Joystick %d: \"%s\"", j, al_get_joystick_name(joystick));
            }

            /* done! */
            break;
        }
    }
}
#else
/* get mouse mickeys (mouse wheel included) */
void get_mouse_mickeys_ex(int *mickey_x, int *mickey_y, int *mickey_z)
{
    get_mouse_mickeys(mickey_x, mickey_y);
    if(mickey_z != NULL) {
        static int mz, first = TRUE;
        if(first) { mz = mouse_z; first = FALSE; }
        *mickey_z = mz - mouse_z;
        mz = mouse_z;
    }
}

/* check if all joysticks have at least 2 axis and 4 buttons */
bool are_all_joysticks_valid()
{
    int i;

    for(i=0; i<num_joysticks; i++) {
        if( !(joy[i].num_sticks > 0 && joy[i].stick[0].num_axis >= 2 && joy[i].num_buttons >= 4) )
            return false;
    }

    return true;
}
#endif