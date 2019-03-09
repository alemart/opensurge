/*
 * Open Surge Engine
 * input.c - input management
 * Copyright (C) 2008-2011, 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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
    int enabled; /* enable input? */
    int state[IB_MAX], oldstate[IB_MAX]; /* states */
    void (*update)(input_t*); /* update method */
};

/* <derived class>: mouse input */
struct inputmouse_t {
    input_t base;
    int x, y, z; /* cursor position */
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

/* private data */
static const char* DEFAULT_INPUTMAP_NAME = "default";
static input_list_t *inlist;
static int got_joystick;
static int ignore_joystick;
static int plugged_joysticks;

/* private methods */
static int are_all_joysticks_valid();
static void input_register(input_t *in);
static void input_unregister(input_t *in);
static void get_mouse_mickeys_ex(int *mickey_x, int *mickey_y, int *mickey_z);



/*
 * input_init()
 * Initializes the input module
 */
void input_init()
{
#if defined(A5BUILD)
    logfile_message("input_init()");

    /* initializing the input list */
    inlist = NULL;

    /* joypad setup */
    got_joystick = FALSE;
    ignore_joystick = TRUE;
    plugged_joysticks = 0;
    logfile_message("No joysticks have been detected."); /* TODO */

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
    got_joystick = FALSE;
    ignore_joystick = TRUE;
    plugged_joysticks = 0;
    if(install_joystick(JOY_TYPE_AUTODETECT) == 0) {
        if(num_joysticks > 0 && are_all_joysticks_valid()) {
            got_joystick = TRUE;
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

    if(input_joystick_available())
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

    logfile_message("releasing registed input objects...");
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
int input_button_down(input_t *in, inputbutton_t button)
{
    return in->enabled && in->state[(int)button];
}


/*
 * input_button_pressed()
 * Checks if a given button has just been pressed (not held down)
 */
int input_button_pressed(input_t *in, inputbutton_t button)
{
    return in->enabled && (in->state[(int)button] && !in->oldstate[(int)button]);
}


/*
 * input_button_up()
 * Checks if a given button has just been released
 */
int input_button_up(input_t *in, inputbutton_t button)
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
    int i;

    in->update = inputmouse_update;
    in->enabled = TRUE;
    for(i=0; i<IB_MAX; i++)
        in->state[i] = in->oldstate[i] = FALSE;

    me->dx = me->dy = me->x = me->y = 0;

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
    int i;

    in->update = inputcomputer_update;
    in->enabled = TRUE;
    for(i=0; i<IB_MAX; i++)
        in->state[i] = in->oldstate[i] = FALSE;

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
    int i;

    in->update = inputuserdefined_update;
    in->enabled = TRUE;
    for(i=0; i<IB_MAX; i++)
        in->state[i] = in->oldstate[i] = FALSE;

    /* if there isn't such a inputmap_name, the game will exit beautifully */
    inputmap_name = inputmap_name ? inputmap_name : DEFAULT_INPUTMAP_NAME;
    me->inputmap = inputmap_get(inputmap_name);

    /* check joystick stuff */
    if(me->inputmap->joystick.enabled && (!input_joystick_available() || me->inputmap->joystick.id >= input_number_of_plugged_joysticks())) {
        logfile_message(
            "WARNING: inputmap '%s' accepts a joystick (id: %d, plugged joysticks: %d), but %s.",
            inputmap_name, me->inputmap->joystick.id, input_number_of_plugged_joysticks(),
            (input_joystick_available() ? "the joystick id is invalid" : "the user isn't using a joystick")
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
    in->enabled = FALSE;
}


/*
 * input_restore()
 * Restore Control
 */
void input_restore(input_t *in)
{
    in->enabled = TRUE;
}



/*
 * input_is_ignored()
 * Returns TRUE if the input is ignored,
 * or FALSE otherwise
 */
int input_is_ignored(input_t *in)
{
    return !in->enabled;
}




/*
 * input_clear()
 * Clears all the input buttons
 */
void input_clear(input_t *in)
{
    for(int i = 0; i < IB_MAX; i++)
        in->state[i] = in->oldstate[i] = FALSE;
}



/*
 * input_simulate_button_down()
 * Useful for computer-controlled input objects
 */
void input_simulate_button_down(input_t *in, inputbutton_t button)
{
    in->state[(int)button] = TRUE;
}



/*
 * input_simulate_button_up()
 * Useful for computer-controlled input objects
 */
void input_simulate_button_up(input_t *in, inputbutton_t button)
{
    in->state[(int)button] = FALSE;
}



/*
 * input_joystick_available()
 * Is a joystick available?
 */
int input_joystick_available()
{
    return got_joystick && !ignore_joystick;
}


/*
 * input_ignore_joystick()
 * Ignores the input received from a joystick (if available)
 */
void input_ignore_joystick(int ignore)
{
    ignore_joystick = ignore;
}


/*
 * input_is_joystick_ignored()
 * Is the joystick input ignored?
 */
int input_is_joystick_ignored()
{
    return ignore_joystick;
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

/*
 * input_number_of_plugged_joysticks()
 * number of plugged joysticks
 */
int input_number_of_plugged_joysticks()
{
    return plugged_joysticks;
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


/* get mouse mickeys (mouse wheel included) */
void get_mouse_mickeys_ex(int *mickey_x, int *mickey_y, int *mickey_z)
{
#if defined(A5BUILD)
    *mickey_x = *mickey_y = *mickey_z = 0;
#else
    get_mouse_mickeys(mickey_x, mickey_y);
    if(mickey_z != NULL) {
        static int mz, first = TRUE;
        if(first) { mz = mouse_z; first = FALSE; }
        *mickey_z = mz - mouse_z;
        mz = mouse_z;
    }
#endif
}

/* check if all joysticks have at least 2 axis and 4 buttons */
int are_all_joysticks_valid()
{
#if defined(A5BUILD)
    return TRUE;
#else
    int i;

    for(i=0; i<num_joysticks; i++) {
        if( !(joy[i].num_sticks > 0 && joy[i].stick[0].num_axis >= 2 && joy[i].num_buttons >= 4) )
            return FALSE;
    }

    return TRUE;
#endif
}

/* update specific input devices */
void inputmouse_update(input_t* in)
{
#if defined(A5BUILD)
    inputmouse_t *me = (inputmouse_t*)in;
    const int mouse_x = 0, mouse_y = 0, mouse_z = 0, mouse_b = 0; /* FIXME */

    get_mouse_mickeys_ex(&me->dx, &me->dy, &me->dz);
    me->x = mouse_x;
    me->y = mouse_y;
    me->z = mouse_z;
    in->state[IB_UP] = (me->dz < 0);
    in->state[IB_DOWN] = (me->dz > 0);
    in->state[IB_LEFT] = FALSE;
    in->state[IB_RIGHT] = FALSE;
    in->state[IB_FIRE1] = (mouse_b & 1);
    in->state[IB_FIRE2] = (mouse_b & 2);
    in->state[IB_FIRE3] = (mouse_b & 4);
    in->state[IB_FIRE4] = FALSE;
    in->state[IB_FIRE5] = FALSE;
    in->state[IB_FIRE6] = FALSE;
    in->state[IB_FIRE7] = FALSE;
    in->state[IB_FIRE8] = FALSE;
#else
    inputmouse_t *me = (inputmouse_t*)in;

    get_mouse_mickeys_ex(&me->dx, &me->dy, &me->dz);
    me->x = mouse_x;
    me->y = mouse_y;
    me->z = mouse_z;
    in->state[IB_UP] = (me->dz < 0);
    in->state[IB_DOWN] = (me->dz > 0);
    in->state[IB_LEFT] = FALSE;
    in->state[IB_RIGHT] = FALSE;
    in->state[IB_FIRE1] = (mouse_b & 1);
    in->state[IB_FIRE2] = (mouse_b & 2);
    in->state[IB_FIRE3] = (mouse_b & 4);
    in->state[IB_FIRE4] = FALSE;
    in->state[IB_FIRE5] = FALSE;
    in->state[IB_FIRE6] = FALSE;
    in->state[IB_FIRE7] = FALSE;
    in->state[IB_FIRE8] = FALSE;
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

    for(int i = 0; i < IB_MAX; i++)
        in->state[i] = FALSE;
    in->state[IB_RIGHT]=1;
#else
    inputuserdefined_t *me = (inputuserdefined_t*)in;
    const inputmap_t *im = me->inputmap;
    int i, k;

    for(i=0; i<IB_MAX; i++)
        in->state[i] = FALSE;

    if(im->keyboard.enabled) {
        for(i=0; i<IB_MAX; i++)
            in->state[i] |= (im->keyboard.scancode[i] > 0) ? key[ im->keyboard.scancode[i] ] : FALSE;
    }

    if(input_joystick_available() && im->joystick.enabled && im->joystick.id < input_number_of_plugged_joysticks()) {
        k = im->joystick.id;
        in->state[IB_UP] |= joy[k].stick[0].axis[1].d1;
        in->state[IB_DOWN] |= joy[k].stick[0].axis[1].d2;
        in->state[IB_LEFT] |= joy[k].stick[0].axis[0].d1;
        in->state[IB_RIGHT] |= joy[k].stick[0].axis[0].d2;
        in->state[IB_FIRE1] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE1]) ? joy[k].button[ im->joystick.button[IB_FIRE1] ].b : FALSE;
        in->state[IB_FIRE2] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE2]) ? joy[k].button[ im->joystick.button[IB_FIRE2] ].b : FALSE;
        in->state[IB_FIRE3] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE3]) ? joy[k].button[ im->joystick.button[IB_FIRE3] ].b : FALSE;
        in->state[IB_FIRE4] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE4]) ? joy[k].button[ im->joystick.button[IB_FIRE4] ].b : FALSE;
        in->state[IB_FIRE5] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE5]) ? joy[k].button[ im->joystick.button[IB_FIRE5] ].b : FALSE;
        in->state[IB_FIRE6] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE6]) ? joy[k].button[ im->joystick.button[IB_FIRE6] ].b : FALSE;
        in->state[IB_FIRE7] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE7]) ? joy[k].button[ im->joystick.button[IB_FIRE7] ].b : FALSE;
        in->state[IB_FIRE8] |= (joy[k].num_buttons > im->joystick.button[IB_FIRE8]) ? joy[k].button[ im->joystick.button[IB_FIRE8] ].b : FALSE;
    }
#endif
}
