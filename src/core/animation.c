/*
 * Open Surge Engine
 * animation.c - animation system
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

#include "animation.h"
#include "sprite.h"
#include "keyframes.h"
#include "image.h"
#include "../core/nanoparser.h"
#include "../core/logfile.h"
#include "../util/util.h"
#include "../util/numeric.h"
#include "../util/stringutil.h"
#include "../util/transform.h"

/* animation structure */
struct animation_t {
    const spriteinfo_t* sprite; /* a reference: this animation belongs to this sprite */
    int id; /* id of the animation */
    bool repeat; /* repeat animation? */
    float fps; /* frames per second */
    int frame_count; /* how many frames does this animation have? */
    int* data; /* array of indices of frames of the sprite */
    int frame_width; /* frame width, in pixels */
    int frame_height; /* frame height, in pixels */
    v2d_t hot_spot; /* hot spot */
    v2d_t action_spot; /* action spot */
    int repeat_from; /* if repeat is true, jump back to this frame of the animation. Defaults to zero */
    bool is_transition; /* is this a transition animation? */
    char* prog_anim_name; /* name of a keyframe-based animation (or NULL if none is used) */
    const proganim_t* prog_anim; /* cached pointer (possibly NULL) */
};

/* constants */
static const float DEFAULT_FPS = 8.0f;
static const float MIN_FPS = 1e-5;




/*
 * animation_id()
 * The ID of the animation, as declared in a .spr file (typically)
 */
int animation_id(const animation_t* anim)
{
    return anim->id;
}

/*
 * animation_fps()
 * The FPS rate of the animation (frames per second)
 */
float animation_fps(const animation_t* anim)
{
    return anim->fps;
}

/*
 * animation_frame_count()
 * The number of frames of the animation
 */
int animation_frame_count(const animation_t* anim)
{
    return anim->frame_count;
}

/*
 * animation_frame_width()
 * The width, in pixels, of a frame of the animation
 */
int animation_frame_width(const animation_t* anim)
{
    return anim->frame_width;
}

/*
 * animation_frame_height()
 * The height, in pixels, of a frame of the animation
 */
int animation_frame_height(const animation_t* anim)
{
    return anim->frame_height;
}

/*
 * animation_repeats()
 * Does the animation repeat itself? (loop)
 */
bool animation_repeats(const animation_t* anim)
{
    return anim->repeat;
}

/*
 * animation_repeat_from()
 * Index of repeating frame; typically zero
 */
int animation_repeat_from(const animation_t* anim)
{
    return anim->repeat_from;
}

/*
 * animation_hot_spot()
 * The hot spot of the animation, in pixels
 */
v2d_t animation_hot_spot(const animation_t* anim)
{
    return anim->hot_spot;
}

/*
 * animation_action_spot()
 * The unflipped action spot of the animation, in pixels
 */
v2d_t animation_action_spot(const animation_t* anim)
{
    return anim->action_spot;
}

/*
 * animation_sprite()
 * The sprite to which this animation belongs
 */
const spriteinfo_t* animation_sprite(const animation_t* anim)
{
    return anim->sprite;
}

/*
 * animation_image()
 * Receives an animation and the desired frame number.
 * Returns an image.
 */
const image_t* animation_image(const animation_t *anim, int frame_number)
{
    if(frame_number < 0)
        frame_number = 0;
    else if(frame_number >= anim->frame_count)
        frame_number = anim->frame_count - 1;

    return spriteinfo_get_animation_frame(anim->sprite, anim->data[frame_number]);
}

/*
 * animation_image_at_time()
 * Returns an animation frame given a time in seconds
 * (start time is zero)
 */
const image_t* animation_image_at_time(const animation_t* anim, double seconds)
{
    /* this is a quite common case;
       skip the computations below */
    if(anim->frame_count == 1)
        return spriteinfo_get_animation_frame(anim->sprite, anim->data[0]);

    /* compute animation frame */
    int frame_number = animation_frame_at_time(anim, seconds);
    return spriteinfo_get_animation_frame(anim->sprite, anim->data[frame_number]);
}

/*
 * animation_frame_at_time()
 * The frame number at a given time in seconds
 * (start time is zero)
 */
int animation_frame_at_time(const animation_t* anim, double seconds)
{
    int frame_number = floor((double)anim->fps * seconds);

    if(frame_number >= anim->frame_count) {
        /* let's make sure that
           frame_number < anim->frame_count */
        if(anim->repeat) {
            if(anim->repeat_from == 0) {
                /* regular loop */
                frame_number %= anim->frame_count;
            }
            else if(anim->repeat_from < anim->frame_count) {
                /* repeat_from is not often used */
                frame_number %= anim->frame_count - anim->repeat_from;
                frame_number += anim->repeat_from;
            }
            else {
                /* this shouldn't happen */
                frame_number = anim->frame_count - 1;
            }
        }
        else {
            /* stop the animation */
            frame_number = anim->frame_count - 1;
        }
    }
    else if(frame_number < 0) {
        /* this should never happen, but what
           if variable "seconds" is negative? */
        frame_number = 0;
    }

    /* now we have frame_number in a valid range:
       0 <= frame_number < anim->frame_count */
    return frame_number;
}

/*
 * animation_start_time_of_frame()
 * The time in which the given animation frame starts playing,
 * given in seconds. The start time of the first frame is zero.
 */
double animation_start_time_of_frame(const animation_t* anim, int frame_number)
{
    if(frame_number < 0)
        frame_number = 0;
    else if(frame_number >= anim->frame_count)
        frame_number = anim->frame_count - 1;

    return (double)frame_number / (double)anim->fps;
}

/*
 * animation_frame_index()
 * The index of an animation frame in the spritesheet
 */
int animation_frame_index(const animation_t* anim, int frame_number)
{
    if(frame_number < 0)
        frame_number = 0;
    else if(frame_number >= anim->frame_count)
        frame_number = anim->frame_count - 1;

    return anim->data[frame_number];
}

/*
 * animation_duration()
 * The duration of an animation, in seconds
 */
double animation_duration(const animation_t* anim)
{
    /* The duration of a repeating animation is the same
       as the duration of a non-repeating animation; we
       just repeat it. This is a more useful definition. */

    return (double)anim->frame_count / (double)anim->fps;
}

/*
 * animation_is_over()
 * Checks if an animation at a given time is over
 */
bool animation_is_over(const animation_t* anim, double seconds)
{
    /* animations that loop are never over */
    if(anim->repeat)
        return false;

    /* find the duration */
    double normal_duration = (double)anim->frame_count / (double)anim->fps;
    double programmatic_duration = (anim->prog_anim != NULL) ? proganim_duration(anim->prog_anim) : 0.0;

    /* test if the animation is over */
    return seconds >= max(normal_duration, programmatic_duration);
}

/*
 * animation_find_transition()
 * Gets a transition animation.
 * Returns NULL if there is no such transition
 */
const animation_t* animation_find_transition(const animation_t* from, const animation_t* to)
{
    /* there is no transition to/from another transition */
    if(from->is_transition || to->is_transition)
        return NULL;

    /* transitions are only available within the same sprite */
    if(from->sprite != to->sprite)
        return NULL;

    /* find the transition */
    return spriteinfo_find_transition_animation(from->sprite, from->id, to->id);
}

/*
 * animation_is_transition()
 * is anim a transition animation?
 */
bool animation_is_transition(const animation_t* anim)
{
    return anim->is_transition;
}

/*
 * animation_has_keyframes()
 * Is this a keyframe-based animation?
 */
bool animation_has_keyframes(const animation_t* anim)
{
    return anim->prog_anim != NULL;
}

/*
 * animation_interpolated_transform()
 * The interpolated transform of a keyframe-based animation
 */
transform_t* animation_interpolated_transform(const animation_t* anim, double seconds, transform_t* out_transform)
{
    /* not defined? */
    if(anim->prog_anim == NULL) {
        transform_identity(out_transform);
        return out_transform;
    }

    /* interpolate! */
    return proganim_interpolated_transform(anim->prog_anim, seconds, anim->repeat, out_transform);
}

/*
 * animation_interpolated_opacity()
 * The interpolated opacity of a keyframe-based animation
 */
float animation_interpolated_opacity(const animation_t* anim, double seconds)
{
    /* not defined? */
    if(anim->prog_anim == NULL)
        return 1.0f;

    /* interpolate! */
    return proganim_interpolated_opacity(anim->prog_anim, seconds, anim->repeat);
}

/*
 * animation_user_property()
 * Get a NULL-terminated array with the element(s) of a user-defined custom property,
 * or NULL if no property with the given name exists
 */
const char* const* animation_user_property(const animation_t* anim, const char* name)
{
    return spriteinfo_user_property(anim->sprite, name);
}




/* ----- friend class spriteinfo_t :P ----- */


/*
 * animation_create()
 * Creates a new animation instance
 */
animation_t *animation_create(const spriteinfo_t *sprite, int anim_id, bool is_transition, int frame_width, int frame_height, v2d_t default_hot_spot, v2d_t default_action_spot)
{
    animation_t *anim = mallocx(sizeof *anim);

    anim->sprite = sprite;
    anim->id = anim_id;
    anim->repeat = false;
    anim->fps = DEFAULT_FPS;
    anim->frame_count = 0;
    anim->data = NULL; /* this will be malloc'd later */
    anim->frame_width = frame_width;
    anim->frame_height = frame_height;
    anim->hot_spot = default_hot_spot;
    anim->action_spot = default_action_spot;
    anim->repeat_from = 0;
    anim->is_transition = is_transition;
    anim->prog_anim_name = NULL;
    anim->prog_anim = NULL;

    return anim;
}


/*
 * animation_destroy()
 * Destroys an existing animation instance
 */
animation_t* animation_destroy(animation_t *anim)
{
    if(anim->data != NULL)
        free(anim->data);

    if(anim->prog_anim_name != NULL)
        free(anim->prog_anim_name);

    free(anim);
    return NULL;
}

/*
 * animation_validate()
 * Validate (and possibly fix) the animation
 */
void animation_validate(animation_t *anim, int number_of_frames_in_the_sheet)
{
    assertx(number_of_frames_in_the_sheet >= 1);

    if(anim->frame_width <= 0 || anim->frame_height <= 0)
        fatal_error("Animation error: invalid frame size %dx%d in animation %d", anim->frame_width, anim->frame_height, anim->id);

    if(anim->frame_count == 0 || anim->data == NULL)
        fatal_error("Animation error: unspecified 'data' frames in animation %d", anim->id);

    for(int i = 0; i < anim->frame_count; i++) {
        if(anim->data[i] < 0 || anim->data[i] >= number_of_frames_in_the_sheet) {
            logfile_message("Animation warning: 'data' frame %d is outside of the valid range [0,%d] in animation %d", anim->data[i], number_of_frames_in_the_sheet-1, anim->id);
            anim->data[i] = clip(anim->data[i], 0, number_of_frames_in_the_sheet-1);
        }
    }

    if(anim->fps < MIN_FPS) {
        logfile_message("Animation warning: 'fps' value %f is invalid.", anim->fps);
        anim->fps = MIN_FPS;
    }

    if(!anim->repeat && anim->repeat_from != 0) {
        logfile_message("Animation warning: 'repeat_from' has been set, but animation %d does not repeat", anim->id);
        anim->repeat_from = 0;
    }

    if(anim->repeat_from < 0 || anim->repeat_from >= anim->frame_count) {
        logfile_message("Animation warning: 'repeat_from' has been set to %d, a value outside of the valid range [0,%d] in animation %d", anim->repeat_from, anim->frame_count-1, anim->id);
        anim->repeat_from = clip(anim->repeat_from, 0, anim->frame_count-1);
    }

    if(anim->is_transition && anim->repeat) {
        logfile_message("Animation warning: transition animations must not repeat");
        anim->repeat = false;
    }

    if(anim->prog_anim_name != NULL) {
        const proganim_t* prog_anim = spriteinfo_get_proganim(anim->sprite, anim->prog_anim_name);
        if(prog_anim == NULL)
            fatal_error("Animation error: undefined keyframe-based animation \"%s\"", anim->prog_anim_name);

        anim->prog_anim = prog_anim; /* cache the entry */
    }
}

/*
 * traverse_animation_attributes()
 * Animation attributes traversal
 */
int traverse_animation_attributes(const parsetree_statement_t *stmt, void *animation)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *pj;
    animation_t *anim = (animation_t*)animation;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "repeat") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat must be true or false");
        anim->repeat = atob(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "fps") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "fps must be a positive number");
        anim->fps = atof(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "repeat_from") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat_from must be a non-negative number");
        anim->repeat_from = atoi(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "hot_spot") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "hot_spot receives two numbers: xpos, ypos");
        nanoparser_expect_string(p2, "hot_spot receives two numbers: xpos, ypos");
        anim->hot_spot.x = (float)atoi(nanoparser_get_string(p1));
        anim->hot_spot.y = (float)atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "action_spot") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "action_spot receives two numbers: xpos, ypos");
        nanoparser_expect_string(p2, "action_spot receives two numbers: xpos, ypos");
        anim->action_spot.x = (float)atoi(nanoparser_get_string(p1));
        anim->action_spot.y = (float)atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "data") == 0) {
        anim->frame_count = nanoparser_get_number_of_parameters(param_list);
        if(anim->frame_count < 1)
            nanoparser_crash(stmt, "Missing animation 'data' field");

        anim->data = reallocx(anim->data, anim->frame_count * sizeof(int));
        for(int j = 1; j <= anim->frame_count; j++) {
            pj = nanoparser_get_nth_parameter(param_list, j);
            nanoparser_expect_string(pj, "Animation 'data' field is a list of frame numbers");
            anim->data[j-1] = atoi(nanoparser_get_string(pj));
        }
    }
    else if(str_icmp(identifier, "play") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "play receives a string: name");

        if(anim->prog_anim_name != NULL)
            free(anim->prog_anim_name);

        const char* name = nanoparser_get_string(p1);
        anim->prog_anim_name = str_dup(name);
    }
    else
        nanoparser_crash(stmt, "Unknown identifier \"%s\"", identifier);

    return 0;
}