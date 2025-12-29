/*
 * Open Surge Engine
 * input.c - input management
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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

#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_memfile.h>
#include "input.h"
#include "engine.h"
#include "video.h"
#include "logfile.h"
#include "timer.h"
#include "inputmap.h"
#include "asset.h"
#include "../entities/mobilegamepad.h"
#include "../util/numeric.h"
#include "../util/util.h"
#include "../util/stringutil.h"

/* <base class>: generic input */
struct input_t {
    bool enabled; /* is this input object enabled? */
    bool blocked; /* is this input object blocked for user input? */
    bool state[IB_MAX], oldstate[IB_MAX]; /* state of the buttons */
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




/* private data */
static input_list_t* input_list = NULL; /* list of registered inputs */
static const char DEFAULT_INPUTMAP_NAME[] = "default";
static const char GAME_CONTROLLER_DB_PATH[] = "inputs/gamecontrollerdb.txt";

/* keyboard input */
static bool a5_key[ALLEGRO_KEY_MAX] = { false };

/* mouse input */
enum {
    LEFT_MOUSE_BUTTON   = 1 << 0, /* primary mouse button */
    RIGHT_MOUSE_BUTTON  = 1 << 1, /* secondary mouse button */
    MIDDLE_MOUSE_BUTTON = 1 << 2  /* tertiary mouse button */
};
static struct {
    int x, y, z; /* position of the cursor */
    int dx, dy, dz; /* deltas */
    int b; /* bit vector of pressed buttons */
} a5_mouse = { 0 };

/* joystick input */
#define MAX_JOYS         8 /* maximum number of joysticks */
#define REQUIRED_AXES    2 /* required number of axes of a stick */
#define MIN_BUTTONS      4 /* minimum number of buttons for a joystick to be considered a gamepad */
#define MAX_BUTTONS      MAX_JOYSTICK_BUTTONS /* maximum supported number of buttons */
enum { AXIS_X = 0, AXIS_Y = 1 }; /* axes of a stick */

typedef struct joystick_input_t joystick_input_t;
struct joystick_input_t {
    float axis[REQUIRED_AXES]; /* -1.0 <= axis[i] <= 1.0 */
    uint32_t buttons; /* bit vector of pressed buttons */
};

static joystick_input_t joy[MAX_JOYS];
static joystick_input_t *wanted_joy[MAX_JOYS]; /* remap of joy[] */
static bool ignore_joystick = false;
static const float DEADZONE_THRESHOLD = 0.2f; /* dead-zone for analog input */
static const float ANALOG_SENSITIVITY_THRESHOLD = 0.5f; /* analog sticks: sensitivity threshold, a value in [0,1] */
static const float ANALOG_AXIS_THRESHOLD[REQUIRED_AXES] = { /* analog sticks: thresholds for the (x,y) axes */

    /* Pressing up + jump won't make the player jump */
    [AXIS_X] = 0.609f,/* cos(52.5 degrees) ~ 105 degrees horizontally */
    [AXIS_Y] = 0.752f /* sin(48.75 degrees) ~ 97.5 degrees vertically */
    /*[AXIS_Y] = 0.707f*/ /* sin(45 degrees) ~ 90 degrees vertically */

};

/* XInput button numbers from Allegro's source code at src/win/wjoyxi.c
   Historically, we've been using this layout based on the Xbox controller
   Here I type the numbers explicitly */
enum {
    BUTTON_A              = 0,
    BUTTON_B              = 1,
    BUTTON_X              = 2,
    BUTTON_Y              = 3,
    BUTTON_RIGHT_SHOULDER = 4,
    BUTTON_LEFT_SHOULDER  = 5,
    BUTTON_RIGHT_THUMB    = 6,
    BUTTON_LEFT_THUMB     = 7,
    BUTTON_BACK           = 8,
    BUTTON_START          = 9,
    BUTTON_DPAD_RIGHT     = 10,
    BUTTON_DPAD_LEFT      = 11,
    BUTTON_DPAD_DOWN      = 12,
    BUTTON_DPAD_UP        = 13
};

/*

See also: recommended game actions for gamepad buttons
https://developer.android.com/develop/ui/views/touch-and-input/game-controllers/controller-input#button
https://developer.android.com/training/tv/start/controllers#tv-ui-events

*/

/* Joystick initialization fix for Allegro 5.2.9.x or older on Unix */
#if defined(ALLEGRO_UNIX) && AL_ID(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION, 0) < AL_ID(5,2,10,0)
# define WANT_JOYINIT_QUIRK 1
#endif

/* the new Gamepad system requires Allegro 5.2.11 or newer */
#if AL_ID(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION, 0) >= AL_ID(5,2,11,0)
# define HAVE_GAMEPAD_API 1
#endif

/* remap the new Gamepad button layout of Allegro to the XInput button layout we've been using historically */
#if WANT_BETTER_GAMEPAD && HAVE_GAMEPAD_API
# define WANT_BACKWARDS_COMPATIBLE_GAMEPAD 1
#endif

#if WANT_BETTER_GAMEPAD && !HAVE_GAMEPAD_API
# error "Compile-time option WANT_BETTER_GAMEPAD requires Allegro 5.2.11 or newer"
#endif

#if WANT_BETTER_GAMEPAD && defined(__ANDROID__)
# error "Compile-time option WANT_BETTER_GAMEPAD is currently unavailable for Android"
/* requires Allegro 5.2.x or newer on Android */
#endif

/* the joystick pool is used to keep consistent joystick IDs across reconfigurations */
#define POOL_CAPACITY    MAX_JOYS /* how many different joysticks we can connect */
static ALLEGRO_JOYSTICK* joystick_pool[POOL_CAPACITY];
static ALLEGRO_JOYSTICK* joystick_pool_query(int id);
static int joystick_pool_indexof(ALLEGRO_JOYSTICK* joystick);
static int joystick_pool_next();
static void joystick_pool_clear();
static void joystick_pool_refresh();
static void joystick_pool_recycle();
static int joystick_pool_count();

/* private methods */
static void input_register(input_t *in);
static void input_unregister(input_t *in);
static void input_clear(input_t *in);
static bool remap_joystick_buttons(uint32_t* buttons, ALLEGRO_JOYSTICK* joystick);
static bool remap_joystick_button(int* out, int button_number, ALLEGRO_JOYSTICK* joystick);
static void log_joysticks();
static void log_joystick(ALLEGRO_JOYSTICK* joystick);
static const char* joyguid2str(ALLEGRO_JOYSTICK* joystick);

/* Allegro 5 events */
static void a5_handle_keyboard_event(const ALLEGRO_EVENT* event, void* data);
static void a5_handle_mouse_event(const ALLEGRO_EVENT* event, void* data);
static void a5_handle_joystick_event(const ALLEGRO_EVENT* event, void* data);
static void a5_handle_touch_event(const ALLEGRO_EVENT* event, void* data);
static struct {
    ALLEGRO_EVENT_SOURCE event_source;
    bool initialized;
    int tracked_touch_id;
} emulated_mouse = { .initialized = false, .tracked_touch_id = -1 };






/*
 * public API
 */



/*
 * input_init()
 * Initializes the input module
 */
void input_init()
{
    /* initialize the Allegro input system */
    logfile_message("Initializing the input system...");

    /* initialize the keyboard */
    if(!al_is_keyboard_installed()) {
        if(!al_install_keyboard())
            fatal_error("Can't initialize the keyboard");
    }
    engine_add_event_source(al_get_keyboard_event_source());
    engine_add_event_listener(ALLEGRO_EVENT_KEY_DOWN, NULL, a5_handle_keyboard_event);
    engine_add_event_listener(ALLEGRO_EVENT_KEY_UP, NULL, a5_handle_keyboard_event);

    /* initialize the mouse */
    if(!al_is_mouse_installed()) {
        if(!al_install_mouse())
            fatal_error("Can't initialize the mouse");
    }
    engine_add_event_source(al_get_mouse_event_source());
    engine_add_event_listener(ALLEGRO_EVENT_MOUSE_BUTTON_DOWN, NULL, a5_handle_mouse_event);
    engine_add_event_listener(ALLEGRO_EVENT_MOUSE_BUTTON_UP, NULL, a5_handle_mouse_event);
    engine_add_event_listener(ALLEGRO_EVENT_MOUSE_AXES, NULL, a5_handle_mouse_event);

    /* initialize the joystick */
#if defined(_WIN32)
    /* use the XInput driver on Windows */
    const char* windrv = al_get_config_value(al_get_system_config(), "joystick", "driver");
    if(windrv == NULL || *windrv == '\0')
        al_set_config_value(al_get_system_config(), "joystick", "driver", "XINPUT");
#endif

#if WANT_BACKWARDS_COMPATIBLE_GAMEPAD
    /*

    Let's set up a consistent cross-platform gamepad layout

    al_set_joystick_mappings_f() should be called before al_install_joystick()
    according to the Allegro manual

    */

    /* read a built-in gamecontrollerdb.txt */
    extern const char GAME_CONTROLLER_DB[];
    extern const size_t GAME_CONTROLLER_DB_SIZE;
    ALLEGRO_FILE* db = al_open_memfile((void*)GAME_CONTROLLER_DB, GAME_CONTROLLER_DB_SIZE, "r");

    if(db != NULL) {
        if(!al_set_joystick_mappings_f(db)) {
            logfile_message("Can't use the built-in joystick mappings");
            video_showmessage("Can't use the built-in joystick mappings");
        }
        al_fclose(db);
    }
    else
        logfile_message("Can't read the built-in joystick mappings");

    /* append an external, additional gamecontrollerdb.txt
       Perhaps it's up-to-date. Perhaps it's older than the built-in db. Perhaps it's customized by the user */
    if(asset_exists(GAME_CONTROLLER_DB_PATH)) {
        const char* fullpath = asset_path(GAME_CONTROLLER_DB_PATH);
        logfile_message("Found additional joystick mappings at %s", GAME_CONTROLLER_DB_PATH);
        if(!al_set_joystick_mappings(fullpath)) {
            logfile_message("Can't use the additional joystick mappings");
            video_showmessage("Can't use the additional joystick mappings");
        }
    }
    else
        logfile_message("No additional joystick mappings found at %s", GAME_CONTROLLER_DB_PATH);
#else
    logfile_message("Joystick mappings are unavailable in this build");
    (void)GAME_CONTROLLER_DB_PATH;
#endif

#if WANT_JOYINIT_QUIRK
    /*

    Handle a quirk of Allegro 5.2.9 and below.

    On Linux, Allegro will scan /dev/input when calling al_install_joystick().
    In function ljoy_scan() at src/linux/ljoynu.c, we see that Allegro's file
    system routines are used. If the ALLEGRO_FS_INTERFACE vtable is not the
    default one based on stdio (like the physfs vtable), then the scan will
    fail and the initially connected joysticks will not be detected.

    */
    const ALLEGRO_FS_INTERFACE* fs_interface = al_get_fs_interface();
    al_set_standard_fs_interface();
#endif

    if(!al_is_joystick_installed()) {
        if(!al_install_joystick())
            fatal_error("Can't initialize the joystick subsystem");
    }
    engine_add_event_source(al_get_joystick_event_source());
    engine_add_event_listener(ALLEGRO_EVENT_JOYSTICK_CONFIGURATION, NULL, a5_handle_joystick_event);

#if WANT_JOYINIT_QUIRK
    /* restore the file system interface */
    al_set_fs_interface(fs_interface);
#endif

    /* initialize touch input */
    if(!al_is_touch_input_installed()) {
        if(!al_install_touch_input())
            logfile_message("Can't initialize the multi-touch subsystem");
    }

    if(al_is_touch_input_installed()) {
        logfile_message("Touch input is available");

        logfile_message("Enabling mouse emulation via touch input");
        al_init_user_event_source(&emulated_mouse.event_source);
        emulated_mouse.initialized = true;

        engine_add_event_source(&emulated_mouse.event_source);
        engine_add_event_source(al_get_touch_input_event_source());
        engine_add_event_listener(ALLEGRO_EVENT_TOUCH_BEGIN, NULL, a5_handle_touch_event);
        engine_add_event_listener(ALLEGRO_EVENT_TOUCH_END, NULL, a5_handle_touch_event);
        engine_add_event_listener(ALLEGRO_EVENT_TOUCH_MOVE, NULL, a5_handle_touch_event);
        engine_add_event_listener(ALLEGRO_EVENT_TOUCH_CANCEL, NULL, a5_handle_touch_event);
    }
    else {
        logfile_message("Touch input is unavailable");
        emulated_mouse.initialized = false;
    }

    /* initialize the input list */
    input_list = NULL;

    /* initialize mouse input */
    a5_mouse.b = 0;
    a5_mouse.x = a5_mouse.y = a5_mouse.z = 0;
    a5_mouse.dx = a5_mouse.dy = a5_mouse.dz = 0;

    /* initialize keyboard input */
    for(int i = 0; i < ALLEGRO_KEY_MAX; i++)
        a5_key[i] = false;

    /* initialize joystick input */
    for(int j = 0; j < MAX_JOYS; j++)
        wanted_joy[j] = NULL;

    joystick_pool_clear();
    joystick_pool_refresh();
    log_joysticks();
    ignore_joystick = !input_is_joystick_available();

    /* loading custom input mappings */
    inputmap_init();

#if defined(__ANDROID__)
    /* this seems to be needed on Android TV */
    if(is_tv_device())
        input_reconfigure_joysticks();
#endif
}

/*
 * input_update()
 * Updates all the registered input objects
 */
void input_update()
{
    /* read the number of joysticks */
    int num_joys = al_get_num_joysticks();
    if(num_joys > MAX_JOYS)
        num_joys = MAX_JOYS;

    /* read joystick input */
    for(int j = 0; j < num_joys; j++) {
        ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);
        int num_sticks = al_get_joystick_num_sticks(joystick);
        int num_buttons = al_get_joystick_num_buttons(joystick);

        /* cap the number of buttons */
        if(num_buttons > MAX_BUTTONS)
            num_buttons = MAX_BUTTONS;

        /* read the current state */
        ALLEGRO_JOYSTICK_STATE state;
        al_get_joystick_state(joystick, &state);

        /* read buttons */
        joy[j].buttons = 0u;
        for(int b = 0; b < num_buttons; b++)
            joy[j].buttons |= (uint32_t)(state.button[b] != 0) << b;

        /* remap buttons */
        remap_joystick_buttons(&(joy[j].buttons), joystick);

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

        https://github.com/mdqinc/SDL_GameControllerDB

        Update: in the new Gamepad system introduced in Allegro 5.2.11, the first
        stick (stick_id = 0) is the D-Pad. The second stick (stick_id = 1) is the
        left analog. So we test multiple sticks.

        (*) Yes, it can if we use Allegro's XInput driver on Windows. Stick 0
            is the "Left Thumbstick" and stick 1 is the "Right Thumbstick". See
            src/win/wjoyxi.c in Allegro's source code.

        (**) the division is merely logical in Allegro's Windows XInput driver.
             A XINPUT_STATE structure holds a XINPUT_GAMEPAD structure. The
             latter describes the current state of the controller. The state
             of the digital buttons is described by a single bitmask, which is
             mapped to logical buttons defined in the Allegro driver. The state
             of the analog sticks is mapped to different logical sticks and axes
             that are also defined by Allegro in src/win/wjoyxi.c. Take a look
             at joyxi_convert_state() in that file.

             https://learn.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad

        */

        /* read sticks */
        joy[j].axis[AXIS_X] = 0.0f;
        joy[j].axis[AXIS_Y] = 0.0f;

        for(int stick_id = 0; stick_id < num_sticks; stick_id++) {

            /* safety check */
            /* I use <= because I think that al_get_joystick_num_axes() cannot
               be fully trusted with controllers in DInput mode.
               https://www.allegro.cc/forums/thread/614996/1 */
            if(REQUIRED_AXES <= al_get_joystick_num_axes(joystick, stick_id)) {

                float x = state.stick[stick_id].axis[0];
                float y = state.stick[stick_id].axis[1];

#if HAVE_GAMEPAD_API
                /* ignore the right analog stick */
                if(al_get_joystick_type(joystick) == ALLEGRO_JOYSTICK_TYPE_GAMEPAD) {
                    if(stick_id == ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB)
                        continue;
                }
                else {
#endif

                /* ignore what is probably the right analog stick (stick_id = 1
                   with 2 axes), but not a D-Pad that is mapped to a stick
                   (probably the first stick having stick_id >= 2 and 2 axes). */
                int flags = al_get_joystick_stick_flags(joystick, stick_id);
                if(stick_id == 1 && !(flags & ALLEGRO_JOYFLAG_DIGITAL))
                    continue;

#if HAVE_GAMEPAD_API
                }
#endif

                /* ignore the dead-zone and normalize the data back to [-1,1] */
                const float NORMALIZER = 1.0f - DEADZONE_THRESHOLD;
                if(fabs(x) >= DEADZONE_THRESHOLD)
                    joy[j].axis[AXIS_X] += (x - DEADZONE_THRESHOLD * sign(x)) / NORMALIZER;
                if(fabs(y) >= DEADZONE_THRESHOLD)
                    joy[j].axis[AXIS_Y] += (y - DEADZONE_THRESHOLD * sign(y)) / NORMALIZER;

            }

        }

        /* clamp values to [-1,1] */
        joy[j].axis[AXIS_X] = clip(joy[j].axis[AXIS_X], -1.0f, 1.0f);
        joy[j].axis[AXIS_Y] = clip(joy[j].axis[AXIS_Y], -1.0f, 1.0f);
    }

    /* remap joystick IDs. The first joystick (if any) must be a valid one!
       This is especially important on Android, which reports the first
       joystick as an accelerometer. Also, we ensure that joystick IDs
       remain consistent across reconfigurations. */
    for(int j = 0; j < MAX_JOYS; j++) {
        wanted_joy[j] = NULL;

        const ALLEGRO_JOYSTICK* target = joystick_pool_query(j);
        if(target != NULL) {
            for(int k = 0; k < num_joys; k++) {
                if(al_get_joystick(k) == target) {
                    wanted_joy[j] = &joy[k];
                    break;
                }
            }
        }
    }

    /* update the input objects */
    for(input_list_t* it = input_list; it; it = it->next) {
        input_t* in = it->data;

        /* save the previous state of the buttons */
        memcpy(in->oldstate, in->state, sizeof(in->oldstate));

        /* clear the current state of the buttons */
        for(inputbutton_t button = 0; button < IB_MAX; button++)
            in->state[button] = false;

        /* accept user input */
        if(!in->blocked)
            in->update(in);
    }
}


/*
 * input_release()
 * Releases the input module
 */
void input_release()
{
    logfile_message("input_release()");

    logfile_message("Releasing registered input objects...");
    for(input_list_t *next, *it = input_list; it; it = next) {
        next = it->next;
        free(it->data);
        free(it);
    }

    inputmap_release();

    if(emulated_mouse.initialized) {
        logfile_message("Disabling mouse emulation via touch input");
        engine_remove_event_source(&emulated_mouse.event_source);
        al_destroy_user_event_source(&emulated_mouse.event_source);
        emulated_mouse.initialized = false;
    }
}


/*
 * input_button_down()
 * Checks if a given button is down
 */
bool input_button_down(const input_t *in, inputbutton_t button)
{
    return in->enabled && in->state[button];
}


/*
 * input_button_pressed()
 * Checks if a given button has just been pressed (not held down)
 */
bool input_button_pressed(const input_t *in, inputbutton_t button)
{
    return in->enabled && (in->state[button] && !in->oldstate[button]);
}


/*
 * input_button_released()
 * Checks if a given button has just been released
 */
bool input_button_released(const input_t *in, inputbutton_t button)
{
    return in->enabled && (!in->state[button] && in->oldstate[button]);
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
    in->blocked = false;
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
    in->blocked = false;
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
    in->blocked = false;
    input_clear(in);

    /* if there isn't such a inputmap_name, the game will exit beautifully */
    inputmap_name = inputmap_name ? inputmap_name : DEFAULT_INPUTMAP_NAME;
    me->inputmap = inputmap_get(inputmap_name);

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
 * input_disable()
 * Disables an input object
 */
void input_disable(input_t *in)
{
    in->enabled = false;
}


/*
 * input_enable()
 * Enables an input object
 */
void input_enable(input_t *in)
{
    in->enabled = true;
}



/*
 * input_is_enabled()
 * Checks if an input object is enabled
 */
bool input_is_enabled(const input_t *in)
{
    return in->enabled;
}


/*
 * input_is_blocked()
 * Checks if an input object is blocked for user input
 */
bool input_is_blocked(const input_t *in)
{
    return in->blocked;
}


/*
 * input_block()
 * Blocks an input object for user input, but not necessarily for simulated input
 */
void input_block(input_t *in)
{
    in->blocked = true;
}


/*
 * input_unblock()
 * Unblocks an input object for user input
 */
void input_unblock(input_t *in)
{
    in->blocked = false;
}



/*
 * input_simulate_button_down()
 * Useful for computer-controlled input objects
 */
void input_simulate_button_down(input_t *in, inputbutton_t button)
{
    /*in->oldstate[button] = in->state[button];*/ /* this logic creates issues between frames */
    in->state[button] = true;
}



/*
 * input_simulate_button_up()
 * Useful for computer-controlled input objects
 */
void input_simulate_button_up(input_t *in, inputbutton_t button)
{
    in->state[button] = false;
}


/*
 * input_simulate_button_press()
 * Simulate that a button is first pressed;
 * useful for computer-controlled input objects
 */
void input_simulate_button_press(input_t *in, inputbutton_t button)
{
    in->oldstate[button] = false;
    in->state[button] = true;
}


/*
 * input_reset()
 * Resets the input object like if nothing is being held down
 */
void input_reset(input_t *in)
{
    for(inputbutton_t button = 0; button < IB_MAX; button++)
        input_simulate_button_up(in, button);
}


/*
 * input_copy()
 * Copy the state of the buttons of src to dest
 */
void input_copy(input_t *dest, const input_t *src)
{
    /* we just copy the buttons, not the enabled/blocked flags */
    if(src->enabled) {
        memcpy(dest->state, src->state, sizeof(src->state));
        memcpy(dest->oldstate, src->oldstate, sizeof(src->oldstate));
    }
    else {
        memset(dest->state, 0, sizeof(dest->state));
        memset(dest->oldstate, 0, sizeof(dest->oldstate));
    }
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
    return !input_is_joystick_ignored() && input_is_joystick_available();
}


/*
 * input_ignore_joystick()
 * Ignores the input received from joysticks
 */
void input_ignore_joystick(bool ignore)
{
    ignore_joystick = ignore;
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
 * Number of connected and valid joysticks
 */
int input_number_of_joysticks()
{
    return joystick_pool_count();
}


/*
 * input_reconfigure_joysticks()
 * Reconfigures the joysticks. Called when hotplugging, but also may be called manually
 */
void input_reconfigure_joysticks()
{
    logfile_message("Reconfiguring joysticks...");

    al_reconfigure_joysticks();
    joystick_pool_refresh();
    log_joysticks();
}


/*
 * input_print_joysticks()
 * Prints the connected & valid joysticks to the screen
 */
void input_print_joysticks()
{
    int n = input_number_of_joysticks();

    if(n == 0) {
        video_showmessage("No joysticks have been detected");
        return;
    }

    video_showmessage("Found %d joystick%s", n, n != 1 ? "s" : "");
    for(int j = 0; j < POOL_CAPACITY; j++) {
        ALLEGRO_JOYSTICK* joystick = joystick_pool_query(j);

        if(joystick != NULL) {
            int joystick_num = j+1;
            video_showmessage("#%d %s", joystick_num, al_get_joystick_name(joystick));
        }
    }
}


/*
 * input_get_xy()
 * Gets the xy coordinates (this will only work for a mouse device)
 */
v2d_t input_get_xy(const inputmouse_t *in)
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
const char* input_get_mapping_name(const inputuserdefined_t *in)
{
    return in->inputmap->name;
}




/*
 * private input methods
 */

/* registers an input device */
void input_register(input_t *in)
{
    input_list_t *node = mallocx(sizeof *node);

    node->data = in;
    node->next = input_list;
    input_list = node;
}

/* unregisters the given input device */
void input_unregister(input_t *in)
{
    input_list_t *node, *next;

    if(!input_list)
        return;

    if(input_list->data == in) {
        next = input_list->next;
        free(input_list);
        input_list = next;
    }
    else {
        node = input_list;
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
    for(inputbutton_t button = 0; button < IB_MAX; button++)
        in->state[button] = in->oldstate[button] = false;
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

    /* read keyboard input */
    if(im->keyboard.enabled) {
        for(inputbutton_t button = 0; button < IB_MAX; button++)
            in->state[button] = (im->keyboard.scancode[button] > 0) && a5_key[im->keyboard.scancode[button]];
    }

    /* read joystick input */
    if(im->joystick.enabled && input_is_joystick_enabled()) {
        int joy_id = im->joystick.number - 1;
        if(joy_id >= 0 && joy_id < MAX_JOYS && wanted_joy[joy_id] != NULL) {
            v2d_t axis = v2d_new(wanted_joy[joy_id]->axis[AXIS_X], wanted_joy[joy_id]->axis[AXIS_Y]);
            v2d_t abs_axis = v2d_new(fabsf(axis.x), fabsf(axis.y));
            float norm_inf = max(abs_axis.x, abs_axis.y);

            if(norm_inf >= ANALOG_SENSITIVITY_THRESHOLD) {
                v2d_t normalized_axis = v2d_normalize(axis);

                in->state[IB_UP] = in->state[IB_UP] || (normalized_axis.y <= -ANALOG_AXIS_THRESHOLD[AXIS_Y]);
                in->state[IB_DOWN] = in->state[IB_DOWN] || (normalized_axis.y >= ANALOG_AXIS_THRESHOLD[AXIS_Y]);
                in->state[IB_LEFT] = in->state[IB_LEFT] || (normalized_axis.x <= -ANALOG_AXIS_THRESHOLD[AXIS_X]);
                in->state[IB_RIGHT] = in->state[IB_RIGHT] || (normalized_axis.x >= ANALOG_AXIS_THRESHOLD[AXIS_X]);

                /*video_showmessage("%f,%f => %f", normalized_axis.x, normalized_axis.y, RAD2DEG * atan2f(normalized_axis.y, normalized_axis.x));*/
            }

            for(inputbutton_t button = 0; button < IB_MAX; button++) {
                uint32_t button_mask = im->joystick.button_mask[(int)button];
                in->state[button] = in->state[button] || ((wanted_joy[joy_id]->buttons & button_mask) != 0);
            }
        }
    }

    /* read the mobile gamepad as the first joystick (always enabled) */
    if(im->joystick.enabled && im->joystick.number == 1) {
        mobilegamepad_state_t mobile;
        mobilegamepad_get_state(&mobile);

        in->state[IB_UP] = in->state[IB_UP] || ((mobile.dpad & MOBILEGAMEPAD_DPAD_UP) != 0);
        in->state[IB_DOWN] = in->state[IB_DOWN] || ((mobile.dpad & MOBILEGAMEPAD_DPAD_DOWN) != 0);
        in->state[IB_LEFT] = in->state[IB_LEFT] || ((mobile.dpad & MOBILEGAMEPAD_DPAD_LEFT) != 0);
        in->state[IB_RIGHT] = in->state[IB_RIGHT] || ((mobile.dpad & MOBILEGAMEPAD_DPAD_RIGHT) != 0);

        in->state[IB_FIRE1] = in->state[IB_FIRE1] || ((mobile.buttons & MOBILEGAMEPAD_BUTTON_ACTION) != 0);
        in->state[IB_FIRE4] = in->state[IB_FIRE4] || ((mobile.buttons & MOBILEGAMEPAD_BUTTON_BACK) != 0);
    }
}




/*
 * joystick pool
 */

/* clear the joystick pool */
void joystick_pool_clear()
{
    for(int i = 0; i < POOL_CAPACITY; i++)
        joystick_pool[i] = NULL;
}

/* clear all inactive joysticks from the pool */
void joystick_pool_recycle()
{
    for(int i = 0; i < POOL_CAPACITY; i++) {
        if(joystick_pool[i] != NULL) {
            if(!al_get_joystick_active(joystick_pool[i]))
                joystick_pool[i] = NULL;
        }
    }
}

/* index of the next joystick in the pool */
int joystick_pool_next()
{
    int index = 0;

    while(index < POOL_CAPACITY && joystick_pool[index] != NULL)
        index++;

    return index;
}

/* find the index of a joystick in the pool, or -1 if not found */
int joystick_pool_indexof(ALLEGRO_JOYSTICK* joystick)
{
    for(int i = 0; i < POOL_CAPACITY; i++) {
        if(joystick_pool[i] == joystick)
            return i;
    }

    return -1;
}

/* refresh the joystick pool. Call after al_reconfigure_joysticks() */
void joystick_pool_refresh()
{
    /* clear up inactive joysticks from the pool */
    joystick_pool_recycle();

    /* after recycling, get the next id */
    int next = joystick_pool_next();

    /* get the current number of joysticks */
    int num_joys = al_get_num_joysticks();
    if(num_joys > MAX_JOYS)
        num_joys = MAX_JOYS;

    /* we'll try to insert new joysticks into the pool */
    for(int j = 0; j < num_joys; j++) {
        ALLEGRO_JOYSTICK* joystick = al_get_joystick(j);
        int num_buttons = al_get_joystick_num_buttons(joystick);
        int num_sticks = al_get_joystick_num_sticks(joystick);
        int num_axes = num_sticks > 0 ? al_get_joystick_num_axes(joystick, 0) : 0;

        /* filter out devices that are not gamepads
           (an accelerometer is reported as the first joystick on Android) */
        if(num_buttons < MIN_BUTTONS || num_axes < REQUIRED_AXES)
            continue;

        /* is this needed? */
        if(!al_get_joystick_active(joystick))
            continue;

        /* the pool is full (really?!) */
        if(next >= POOL_CAPACITY)
            break;

        /* if the joystick is not in the pool, add it */
        if(joystick_pool_indexof(joystick) < 0) {
            joystick_pool[next] = joystick;
            next = joystick_pool_next();
        }
    }

    /*

    From the Allegro manual:
    https://liballeg.org/a5docs/trunk/joystick.html#al_reconfigure_joysticks

    After a call to al_reconfigure_joysticks(), "the number returned by
    al_get_num_joysticks may be different, and the handles returned by
    al_get_joystick may be different or be ordered differently."

    "All ALLEGRO_JOYSTICK handles remain valid, but handles for disconnected
    devices become inactive: their states will no longer update, and
    al_get_joystick will not return the handle. Handles for devices which
    **remain connected** (emphasis added) will continue to represent the same
    devices. Previously inactive handles may become active again, being reused
    to represent newly connected devices."

    */
}

/* get a joystick from the joystick pool. This ensures consistent IDs after reconfigurations */
ALLEGRO_JOYSTICK* joystick_pool_query(int id)
{
    /* validate ID */
    if(id < 0 || id >= POOL_CAPACITY)
        return NULL;

    /* select by ID */
    ALLEGRO_JOYSTICK* candidate = joystick_pool[id];

    /* no joystick with the given ID exists in the pool */
    if(candidate == NULL)
        return NULL;

    /* the candidate was disconnected */
    if(!al_get_joystick_active(candidate))
        return NULL;

    /* success! return the candidate */
    return candidate;
}

/* count the number of active joysticks in the pool */
int joystick_pool_count()
{
    int count = 0;

    for(int i = 0; i < POOL_CAPACITY; i++) {
        if(joystick_pool[i] != NULL) {
            if(al_get_joystick_active(joystick_pool[i]))
                count++;
        }
    }

    return count;
}




/*
 * event handlers
 */

/* handle a keyboard event */
void a5_handle_keyboard_event(const ALLEGRO_EVENT* event, void* data)
{
    switch(event->type) {

        case ALLEGRO_EVENT_KEY_DOWN:
            a5_key[event->keyboard.keycode] = true;
            break;

        case ALLEGRO_EVENT_KEY_UP:
            a5_key[event->keyboard.keycode] = false;
            break;

    }

    (void)data;
}

/* handle a mouse event */
void a5_handle_mouse_event(const ALLEGRO_EVENT* event, void* data)
{
    #define update_mouse_position() do { \
        a5_mouse.dx = event->mouse.x - a5_mouse.x; \
        a5_mouse.dy = event->mouse.y - a5_mouse.y; \
        a5_mouse.dz = event->mouse.z - a5_mouse.z; \
        a5_mouse.x = event->mouse.x; \
        a5_mouse.y = event->mouse.y; \
        a5_mouse.z = event->mouse.z; \
    } while(0)

    switch(event->type) {

        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            a5_mouse.b |= 1 << (event->mouse.button - 1);
            update_mouse_position();
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            a5_mouse.b &= ~(1 << (event->mouse.button - 1));
            update_mouse_position();
            break;

        case ALLEGRO_EVENT_MOUSE_AXES:
            /* we track the position of the mouse using events and not
               ALLEGRO_MOUSE_STATE because we emit mouse events in order
               to emulate mouse input using touch input */
            update_mouse_position();
            break;

    }

    (void)data;
    #undef update_mouse_position
}

/* handle a joystick event */
void a5_handle_joystick_event(const ALLEGRO_EVENT* event, void* data)
{
    switch(event->type) {
        /*
         * Joystick input based on ALLEGRO_JOYSTICK_STATE
         * seems to work better according to several users
         * 
         * tested with Allegro 5.2.5 on Windows using
         * DirectInput devices
         */
        /*
        case ALLEGRO_EVENT_JOYSTICK_AXIS:
            break;

        case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            break;

        case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
            break;
        */

        /* hot plugging */
        case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION: {
            input_reconfigure_joysticks();
            input_print_joysticks();

            if(input_number_of_joysticks() > 0) {
                /* the user probably wants the joystick input to be enabled
                   (automatic joystick detection) */
                input_ignore_joystick(false);
            }

            break;
        }
    }

    (void)data;
}

/* emulate mouse input using touch input */
void a5_handle_touch_event(const ALLEGRO_EVENT* event, void* data)
{
    ALLEGRO_EVENT my_event;
    static int num_touches = 0;
    const unsigned int left_mouse_button = 1u;

    /* validate */
    if(!emulated_mouse.initialized)
        return;

    /* fill mouse event */
    memset(&my_event, 0, sizeof(my_event));
    switch(event->type) {
        case ALLEGRO_EVENT_TOUCH_BEGIN:
            num_touches++;
            my_event.type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
            my_event.mouse.x = event->touch.x;
            my_event.mouse.y = event->touch.y;
            my_event.mouse.button = left_mouse_button;
            my_event.mouse.pressure = 1.0f;
            break;

        case ALLEGRO_EVENT_TOUCH_MOVE:
            if(event->touch.id != emulated_mouse.tracked_touch_id)
                return; /* skip */

            my_event.type = ALLEGRO_EVENT_MOUSE_AXES;
            my_event.mouse.x = event->touch.x;
            my_event.mouse.y = event->touch.y;
            my_event.mouse.dx = event->touch.dx;
            my_event.mouse.dy = event->touch.dy;
            break;

        case ALLEGRO_EVENT_TOUCH_END:
        case ALLEGRO_EVENT_TOUCH_CANCEL:
            num_touches = max(0, num_touches - 1);
            if(!(event->touch.id == emulated_mouse.tracked_touch_id || num_touches == 0))
                return; /* skip */

            my_event.type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
            my_event.mouse.x = event->touch.x;
            my_event.mouse.y = event->touch.y;
            my_event.mouse.button = left_mouse_button;
            my_event.mouse.pressure = 1.0f;
            break;

        default:
            return; /* skip */
    }

    /* emit mouse event */
    bool is_valid_touch = (event->touch.id >= 0);
    /*bool is_target_touch = (event->touch.primary);*/
    bool is_target_touch = true; /* do not assume that touch.id is monotonically increasing */

    if(is_valid_touch && is_target_touch) {
        ALLEGRO_EVENT_SOURCE* my_event_source = &emulated_mouse.event_source;
        al_emit_user_event(my_event_source, &my_event, NULL);

        /*
            According to the Allegro manual, al_emit_user_event():

            "Events are copied in and out of event queues, so after this function
            returns the memory pointed to by event may be freed or reused."
        */

        /* track the touch ID */
        if(event->type == ALLEGRO_EVENT_TOUCH_BEGIN)
            emulated_mouse.tracked_touch_id = event->touch.id;
        else if(event->type == ALLEGRO_EVENT_TOUCH_END || event->type == ALLEGRO_EVENT_TOUCH_CANCEL)
            emulated_mouse.tracked_touch_id = -1;
    }
}




/*
 * misc
 */

/* remap joystick buttons according to the underlying platform
   we want to maintain consistency across platforms
   argument buttons is an input and output parameter */
bool remap_joystick_buttons(uint32_t* buttons, ALLEGRO_JOYSTICK* joystick)
{
    /* bit vector of remapped buttons */
    uint32_t remapped_buttons = 0u;

    /* for each button b */
    for(int c, b = 0; b < MAX_BUTTONS; b++) {

        /* is b pressed? */
        if((*buttons & (1u << b)) != 0u) {

            /* can we remap b? */
            if(remap_joystick_button(&c, b, joystick)) {

                /* remap b to c */
                remapped_buttons |= 1u << c;

            }

        }

    }

    /* exit if we have remapped nothing: either no buttons are pressed
       or we couldn't remap anything (e.g., if joystick type != gamepad ?) */
    if(!remapped_buttons)
        return false;

    /* save the changed bit vector if we have remapped something */
    *buttons = remapped_buttons;
    return true;
}

/* returns true if there is a remapping for a button of the target joystick
   the output parameter is undefined if this function returns false */
bool remap_joystick_button(int* out, int button_number, ALLEGRO_JOYSTICK* joystick)
{
#if WANT_BACKWARDS_COMPATIBLE_GAMEPAD

    /*

    Remap Allegro's new Gamepad buttons to the buttons of the Windows XInput
    driver we've been using historically - for backwards compatibility.

    */

    static const int remap[] = {
        [ALLEGRO_GAMEPAD_BUTTON_A] = BUTTON_A,
        [ALLEGRO_GAMEPAD_BUTTON_B] = BUTTON_B,
        [ALLEGRO_GAMEPAD_BUTTON_X] = BUTTON_X,
        [ALLEGRO_GAMEPAD_BUTTON_Y] = BUTTON_Y,
        [ALLEGRO_GAMEPAD_BUTTON_LEFT_SHOULDER] = BUTTON_LEFT_SHOULDER,
        [ALLEGRO_GAMEPAD_BUTTON_RIGHT_SHOULDER] = BUTTON_RIGHT_SHOULDER,
        [ALLEGRO_GAMEPAD_BUTTON_BACK] = BUTTON_BACK,
        [ALLEGRO_GAMEPAD_BUTTON_START] = BUTTON_START,
        [ALLEGRO_GAMEPAD_BUTTON_GUIDE] = -1, /* unused; unreliable according to the Allegro manual */
        [ALLEGRO_GAMEPAD_BUTTON_LEFT_THUMB] = BUTTON_LEFT_THUMB,
        [ALLEGRO_GAMEPAD_BUTTON_RIGHT_THUMB] = BUTTON_RIGHT_THUMB
    };

    /* skip if remapping isn't applicable */
    if(al_get_joystick_type(joystick) != ALLEGRO_JOYSTICK_TYPE_GAMEPAD)
        return false;

#elif defined(__ANDROID__)

    /*

    Remap Allegro's JS_* button constants to Allegro's buttons of the Windows XInput driver.

    The following JS_* constants are defined in the source code of MODIFIED Allegro 5.2.9 at:
    android/gradle_project/allegro/src/main/java/org/liballeg/android/AllegroActivity.java

    My joystick-related modifications to Allegro 5.2.9:
    1) https://patch-diff.githubusercontent.com/raw/liballeg/allegro5/pull/1483.patch
    2) https://patch-diff.githubusercontent.com/raw/liballeg/allegro5/pull/1507.patch (apply with fuzz=3)

    */
    enum {
        JS_A = 0,
        JS_B = 1,
        JS_X = 2,
        JS_Y = 3,
        JS_L1 = 4,
        JS_R1 = 5,
        JS_DPAD_L = 6,
        JS_DPAD_R = 7,
        JS_DPAD_U = 8,
        JS_DPAD_D = 9,
        JS_START = 10,
        JS_SELECT = 11,
        JS_MODE = 12,
        JS_THUMBL = 13,
        JS_THUMBR = 14,
        JS_L2 = 15,
        JS_R2 = 16,
        JS_C = 17,
        JS_Z = 18,
        JS_DPAD_CENTER = 19
    };

    static const int remap[] = {
        [JS_A] = BUTTON_A,
        [JS_B] = BUTTON_B,
        [JS_X] = BUTTON_X,
        [JS_Y] = BUTTON_Y,
        [JS_L1] = BUTTON_LEFT_SHOULDER,
        [JS_R1] = BUTTON_RIGHT_SHOULDER,
        [JS_DPAD_L] = BUTTON_DPAD_LEFT,
        [JS_DPAD_R] = BUTTON_DPAD_RIGHT,
        [JS_DPAD_U] = BUTTON_DPAD_UP,
        [JS_DPAD_D] = BUTTON_DPAD_DOWN,
        [JS_START] = BUTTON_START,
        [JS_SELECT] = BUTTON_BACK,
        [JS_MODE] = -1, /* unused */
        [JS_THUMBL] = BUTTON_LEFT_THUMB,
        [JS_THUMBR] = BUTTON_RIGHT_THUMB,
        [JS_L2] = -1, /* unused */
        [JS_R2] = -1, /* unused */
        [JS_C] = BUTTON_RIGHT_THUMB,
        [JS_Z] = BUTTON_LEFT_THUMB,
        [JS_DPAD_CENTER] = BUTTON_A
    };

#else

    /* nothing to do */
    static const int remap[] = { };

#endif

    const int n = sizeof(remap) / sizeof(int);

    /* invalid button number? */
    if(button_number < 0 || button_number >= n)
        return false;

    /* unused button? */
    if(remap[button_number] < 0)
        return false;

    /* remap button */
    *out = remap[button_number];
    return true;
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
        logfile_message("joystick[%d]:", j);
        log_joystick(joystick);
    }
}

/* log joystick info */
void log_joystick(ALLEGRO_JOYSTICK* joystick)
{
#if HAVE_GAMEPAD_API
    ALLEGRO_JOYSTICK_TYPE type = al_get_joystick_type(joystick);
    bool is_gamepad = (type == ALLEGRO_JOYSTICK_TYPE_GAMEPAD);
#else
    int type = 0; /* ALLEGRO_JOYSTICK_TYPE_UNKNOWN */
    bool is_gamepad = false;
#endif

    /* header */
    logfile_message("  name: \"%s\"", al_get_joystick_name(joystick));
    logfile_message("  guid: %s", joyguid2str(joystick));
    logfile_message("  type: %s (%d)", is_gamepad ? "gamepad" : "unknown", type);

    /* list the buttons and their mappings as expected in input\*.in files */
    logfile_message("  buttons%s:", is_gamepad ? " mapped to the Xbox layout" : "");

    int num_buttons = al_get_joystick_num_buttons(joystick);
    if(num_buttons > MAX_BUTTONS)
        num_buttons = MAX_BUTTONS;
    else if(num_buttons == 0)
        logfile_message("    none");

    for(int c, b = 0; b < num_buttons; b++) {
        if(remap_joystick_button(&c, b, joystick))
            logfile_message("    \"%s\" => BUTTON_%d", al_get_joystick_button_name(joystick, b), 1+c);
        else if(!is_gamepad)
            logfile_message("    \"%s\" => BUTTON_%d", al_get_joystick_button_name(joystick, b), 1+b);
        /* else: unused button of a gamepad; ignore */
    }

    /* list the sticks and their axes */
    logfile_message("  sticks%s:", is_gamepad ? " mapped to the Xbox layout" : "");

    int num_sticks = al_get_joystick_num_sticks(joystick);
    if(num_sticks == 0)
        logfile_message("    none");

    for(int s = 0; s < num_sticks; s++) {
        static const char* stick_type_name[4] = { "", "digital", "analog", "" };
        int flags = al_get_joystick_stick_flags(joystick, s);

        logfile_message("    \"%s\":", al_get_joystick_stick_name(joystick, s));
        logfile_message("      type: %s (%d)", stick_type_name[flags & 0x3], flags);
        logfile_message("      axes: %d", al_get_joystick_num_axes(joystick, s));
    }
}

/* convert the GUID of a joystick to a statically allocated string */
const char* joyguid2str(ALLEGRO_JOYSTICK* joystick)
{
#if HAVE_GAMEPAD_API

    ALLEGRO_JOYSTICK_GUID guid = al_get_joystick_guid(joystick);
    STATIC_ASSERTX(sizeof(guid.val) == 16);

    static const char nibble2char[] = "0123456789abcdef";
    static char buffer[33] = { 0 };
    char* p = buffer;

    for(int i = 0; i < 16; i++) {
        uint8_t byte = guid.val[i];
        *p++ = nibble2char[byte >> 4];
        *p++ = nibble2char[byte & 0xF];
    }
    *p = '\0'; /* unnecessary due to initialization, but why not make it explicit? */

    return buffer;

#else

    return "<unavailable>";

#endif
}
