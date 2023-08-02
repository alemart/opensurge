/*
 * Open Surge Engine
 * pause.c - pause menu
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
#include <math.h>
#include "pause.h"
#include "quest.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/input.h"
#include "../core/sprite.h"
#include "../core/audio.h"
#include "../core/timer.h"
#include "../core/font.h"
#include "../core/asset.h"
#include "../core/logfile.h"
#include "../util/numeric.h"
#include "../util/util.h"
#include "../entities/actor.h"
#include "../entities/mobilegamepad.h"
#include "../scenes/level.h"
#include "../scripting/scripting.h"
#include "mobile/util/touch.h"



/* states of the pause menu */
typedef enum pause_state_t pause_state_t;
enum pause_state_t
{
    STATE_APPEARING,    /* the player has just paused the game */
    STATE_WAITING,   /* waiting for player input */
    STATE_DISAPPEARING  /* closing the pause menu */
};

#define INITIAL_STATE STATE_APPEARING
static pause_state_t state = INITIAL_STATE;

static void update_appearing();
static void update_waiting();
static void update_disappearing();
static void (*update[])() = {
    [STATE_APPEARING] = update_appearing,
    [STATE_WAITING] = update_waiting,
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

static void confirm_continue();
static void confirm_restart();
static void confirm_exit();
static void (*confirm[])() = {
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
    [SPRITE_BACKGROUND] =   { [STATE_APPEARING] = 1, [STATE_WAITING] = 2, [STATE_DISAPPEARING] = 3 },
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
    SOUND_CANCEL,           /* the player has pressed the back button */

    SOUND_COUNT             /* number of sound effects */
};

static const char* SOUND_PATH[] = {
    [SOUND_APPEAR] =        "samples/pause_appear.wav",
    [SOUND_HIGHLIGHT] =     "samples/pause_highlight.wav",
    [SOUND_CONFIRM] =       "samples/pause_confirm.wav",
    [SOUND_CANCEL] =        "samples/pause_cancel.wav"
};

static sound_t* sound[SOUND_COUNT] = { NULL };



/* overlay with a drag handle for mobile */
#define DRAG_HANDLE_SPEED               ((float)(VIDEO_SCREEN_H) / 0.25f) /* in px/s */
#define DRAG_HANDLE_RADIUS              (v2d_magnitude(actor_action_offset(drag_handle)))
#define DRAG_HANDLE_MINDIST             (VIDEO_SCREEN_H / 4) /* minimum distance required to open the overlay */
#define DRAG_HANDLE_INITIAL_POSITION    (v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H))
static const char* DRAG_HANDLE_SPRITE_NAME = "Pause Menu - Drag Handle";
static const int DRAG_HANDLE_ANIMATION_NUMBER = 0;
static const float DRAG_HANDLE_FADE_TIME = 0.125f; /* in seconds */

#define OVERLAY_COLOR color_premul_rgba(0, 0, 0, 192)
static enum {
    OVERLAY_CLOSING,    /* move down; not dragging */
    OVERLAY_DRAGGING,   /* dragging the handle */
    OVERLAY_OPENING,    /* move up after dragging */
    OVERLAY_FULLYOPEN,  /* change the scene */
    OVERLAY_FINISHED    /* move down and exit the pause menu */
} overlay_state = OVERLAY_CLOSING;

static bool want_overlay();
static void render_overlay();

static void close_overlay();
static void drag_overlay();
static void open_overlay();
static void fullyopen_overlay();
static void finish_overlay();
static void (*update_overlay[])() = {
    [OVERLAY_CLOSING] = close_overlay,
    [OVERLAY_DRAGGING] = drag_overlay,
    [OVERLAY_OPENING] = open_overlay,
    [OVERLAY_FULLYOPEN] = fullyopen_overlay,
    [OVERLAY_FINISHED] = finish_overlay
};

static void on_touch_start(v2d_t touch_start);
static void on_touch_end(v2d_t touch_start, v2d_t touch_end);
static void on_touch_move(v2d_t touch_start, v2d_t touch_current);

static actor_t* drag_handle = NULL;
static input_t* mouse_input = NULL;



/* private stuff */
#define LOG(...)        logfile_message("Pause Menu - " __VA_ARGS__)
#define changed_scene() (scenestack_top() != storyboard_get_scene(SCENE_PAUSE))
static const float FADEOUT_TIME = 0.5f; /* in seconds */
static bool was_mobilegamepad_visible = false;
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

    /* log */
    LOG("Paused the game");

    /* take a snapshot of the game */
    snapshot = video_take_snapshot();

    /* create an input object */
    input = input_create_user(NULL);
    input_simulate_button_down(input, START_BUTTON); /* this assumes that the start button triggered the opening of this scene */
    input_simulate_button_down(input, BACK_BUTTON);

    /* pause music & scripting */
    music_pause();
    scripting_pause_vm();

    /* enable the mobile gamepad (just in case) */
    was_mobilegamepad_visible = mobilegamepad_is_visible();
    mobilegamepad_fadein();

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

    /* initialize the mobile overlay */
    overlay_state = OVERLAY_CLOSING;
    mouse_input = input_create_mouse();
    drag_handle = actor_create();
    drag_handle->alpha = 0.0f;
    drag_handle->visible = want_overlay();
    drag_handle->position = DRAG_HANDLE_INITIAL_POSITION;
    actor_change_animation(drag_handle, sprite_get_animation(DRAG_HANDLE_SPRITE_NAME, DRAG_HANDLE_ANIMATION_NUMBER));

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
    /* log */
    LOG("Unpaused the game");

    if(!legacy_mode) {

        /* release the actors */
        for(int i = 0; i < SPRITE_COUNT; i++)
            actor_destroy(actor[i]);

        /* release the fonts */
        for(int i = 0; i < TEXT_COUNT; i++)
            font_destroy(font[i]);

        /* release the sound effects */
        /* done automatically by the resource manager */

        /* release the drag handle for mobile */
        input_destroy(mouse_input);
        actor_destroy(drag_handle);

    }

    #if 0

    /* restore the previous visibility of the mobile gamepad */
    /* do we really want this? what if a script incorrectly disabled the controls? */
    if(!was_mobilegamepad_visible)
        mobilegamepad_fadeout(); /* seriously?! */
    else
        mobilegamepad_fadein(); /* just in case... */

    #else

    /* make the mobile gamepad visible no matter what */
    mobilegamepad_fadein();
    (void)was_mobilegamepad_visible;

    #endif

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
    update[state]();
    if(changed_scene())
        return;

    /* animate the sprites */
    #define ANIMATE_SPRITE(s, a) actor_change_animation(actor[s], ANIMATION(s, a))
    #define ANIMATE_OPTION(s, o) do { \
        if(!actor_is_transition_animation_playing(actor[s])) \
            ANIMATE_SPRITE((s), option == (o) && state == STATE_WAITING ? HIGHLIGHTED : UNHIGHLIGHTED); \
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
        font_set_visible(font[i], state == STATE_WAITING);
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

    /* render the mobile overlay */
    if(want_overlay())
        render_overlay();
}



/* private */

/* update logic: appearing state */
void update_appearing()
{
    /* wait for the appearing animation to finish */
    if(!actor_animation_finished(actor[SPRITE_BACKGROUND]))
        return;

    /* change the state */
    state = STATE_WAITING;
}

/* update logic: waiting state */
void update_waiting()
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

    /* update the mobile overlay */
    if(want_overlay()) {
        update_overlay[overlay_state]();
        /*if(changed_scene()) return;*/
    }
}

/* update logic: disappearing state */
void update_disappearing()
{
    /* wait for the disappearing animation to finish */
    if(!actor_animation_finished(actor[SPRITE_BACKGROUND]))
        return;

    /* perform the necessary action */
    confirm[option]();
    /*if(changed_scene()) return;*/
}

/* the player has chosen to continue playing */
void confirm_continue()
{
    /* log */
    LOG("Will continue the game");

    /* back to game */
    scenestack_pop();
    /*return;*/
}

/* the player has chosen to restart the level */
void confirm_restart()
{
    if(fadefx_is_over()) {

        /* log */
        LOG("Will restart the level");

        /* restart the level */
        level_restart();
        scenestack_pop();
        return;

    }

    /* fade out */
    color_t black = color_rgb(0, 0, 0);
    fadefx_out(black, FADEOUT_TIME);
}

/* the player has chosen to exit the game */
void confirm_exit()
{
    if(fadefx_is_over()) {

        /* log */
        LOG("Will exit the game");

        /* exit the game */
        scenestack_pop();
        scenestack_pop();
        quest_abort();
        return;

    }

    /* fade out */
    color_t black = color_rgb(0, 0, 0);
    fadefx_out(black, FADEOUT_TIME);
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




/* overlay with a drag handle for mobile */


/* should we enable the mobile overlay */
bool want_overlay()
{
    return mobilegamepad_is_available();
}

/* render the drag handle */
void render_overlay()
{
    v2d_t camera = v2d_multiply(video_get_screen_size(), 0.5f);
    float dt = timer_get_delta();

    /* fade-in & fade-out */
    if(state == STATE_WAITING)
        drag_handle->alpha = min(1.0f, drag_handle->alpha + dt / DRAG_HANDLE_FADE_TIME);
    else
        drag_handle->alpha = max(0.0f, drag_handle->alpha - dt / DRAG_HANDLE_FADE_TIME);

    /* render */
    image_rectfill(0, drag_handle->position.y, VIDEO_SCREEN_W, VIDEO_SCREEN_H, OVERLAY_COLOR);
    actor_render(drag_handle, camera);
}

/* overlay logic: closing */
void close_overlay()
{
    float dt = timer_get_delta();
    float v = DRAG_HANDLE_SPEED;

    drag_handle->position.y += v * dt;
    if(drag_handle->position.y >= VIDEO_SCREEN_H)
        drag_handle->position.y = VIDEO_SCREEN_H;

    handle_touch_input(mouse_input, on_touch_start, NULL, NULL);
}

void on_touch_start(v2d_t touch_start)
{
    v2d_t handle_location = v2d_add(drag_handle->position, actor_action_offset(drag_handle));
    v2d_t ds = v2d_subtract(touch_start, handle_location);
    float r = DRAG_HANDLE_RADIUS;

    bool is_dragging = (max(fabs(ds.x), fabs(ds.y)) <= r);
    if(is_dragging) {
        overlay_state = OVERLAY_DRAGGING;
        mobilegamepad_fadeout();
    }
}

/* overlay logic: dragging */
void drag_overlay()
{
    handle_touch_input(mouse_input, NULL, on_touch_end, on_touch_move);
}

void on_touch_move(v2d_t touch_start, v2d_t touch_current)
{
    v2d_t ds = v2d_subtract(touch_current, touch_start);

    /* drag the handle */
    float dy = min(ds.y, 0.0f);
    drag_handle->position.y = DRAG_HANDLE_INITIAL_POSITION.y + dy;
}

void on_touch_end(v2d_t touch_start, v2d_t touch_end)
{
    v2d_t ds = v2d_subtract(touch_end, touch_start);
    float drag_distance = -ds.y;

    /* open or close the overlay */
    if(drag_distance < DRAG_HANDLE_MINDIST) {
        overlay_state = OVERLAY_CLOSING;
        mobilegamepad_fadein();
    }
    else
        overlay_state = OVERLAY_OPENING;
}

/* overlay logic: opening */
void open_overlay()
{
    float dt = timer_get_delta();
    float v = DRAG_HANDLE_SPEED;

    drag_handle->position.y -= v * dt;
    if(drag_handle->position.y <= 0.0f) {
        drag_handle->position.y = 0.0f;
        overlay_state = OVERLAY_FULLYOPEN;
    }
}

/* overlay logic: fully open */
void fullyopen_overlay()
{
    /* log */
    LOG("Will load the mobile menu");

    /* move to the finished state */
    overlay_state = OVERLAY_FINISHED;

    /* change the scene */
    scenestack_push(storyboard_get_scene(SCENE_MOBILEMENU), snapshot);
    /*return;*/
}

/* overlay logic: finish */
void finish_overlay()
{
    float dt = timer_get_delta();
    float v = DRAG_HANDLE_SPEED;

    drag_handle->position.y += v * dt;
    if(drag_handle->position.y >= VIDEO_SCREEN_H) {
        drag_handle->position.y = VIDEO_SCREEN_H;

        /* reset the state */
        overlay_state = OVERLAY_CLOSING;
        mobilegamepad_fadein();

        /* resume game */
        option = OPTION_CONTINUE;
        state = STATE_DISAPPEARING;
        sound_play(sound[SOUND_CANCEL]);
    }
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

    /* unpause in mobile mode: accept fire1 */
    if(mobilegamepad_is_visible() && input_button_pressed(input, IB_FIRE1)) {
        scenestack_pop();
        return;
    }
}

/* legacy pause screen: render */
void legacy_render()
{
    if(!sprite_animation_exists(LEGACY_SPRITE_NAME, 0))
        return;

    const image_t *icon = animation_image(sprite_get_animation(LEGACY_SPRITE_NAME, 0), 0);
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

    if(!sprite_animation_exists(DRAG_HANDLE_SPRITE_NAME, DRAG_HANDLE_ANIMATION_NUMBER))
        return true;

    return false;
}