/*
 * Open Surge Engine
 * sprite.c - sprite manager
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

#include <string.h>
#include "sprite.h"
#include "animation.h"
#include "image.h"
#include "logfile.h"
#include "asset.h"
#include "resourcemanager.h"
#include "nanoparser.h"
#include "keyframes.h"
#include "../util/v2d.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/darray.h"
#include "../util/hashtable.h"
#include "../util/dictionary.h"
#include "../util/iterator.h"
#include "../physics/collisionmask.h"

typedef struct animtransition_t animtransition_t;
typedef struct userproperty_t userproperty_t;

/* sprite info */
/* spriteinfo_t represents only ONE sprite (metadata), with several animations */
struct spriteinfo_t {
    char* source_file; /* path to image file */
    int rect_x; /* source rectangle: xpos */
    int rect_y; /* source rectangle: ypos */
    int rect_w; /* source rectangle: width */
    int rect_h; /* source rectangle: height */
    int frame_w; /* frame width */
    int frame_h; /* frame height */
    v2d_t default_hot_spot; /* default hot spot for all animations */
    v2d_t default_action_spot; /* default unflipped action spot for all animations */

    int frame_count; /* every frame related to this sprite */
    image_t** frame_data; /* image_t* vector */
    const image_t* spritesheet; /* reference to the spritesheet */

    int animation_count;
    animation_t** animation_data; /* vector of animation_t* */

    DARRAY(animtransition_t*, transition); /* transitions */
    DARRAY(animtransition_t*, preprocessed_transition); /* transitions without "any" sorted by from_id and then by to_id */
    int* transition_from; /* transition_from[i] is the first index of preprocessed_transition having from_id == i, if it exists */
    int transition_from_length; /* length of transition_from[] */

    dictionary_t* prog_anims; /* keyframe-based animations */
    dictionary_t* user_properties; /* user-defined properties */
};

/* transitions are animations that play between two other animations */
struct animtransition_t
{
    int anim_id; /* ID of the transition animation */
    int from_id; /* ID of the previous animation */
    int to_id; /* ID of the next animation */
    bool is_valid; /* validity flag */
};

static animtransition_t *transition_new(int anim_id, int from_id, int to_id); /* creates a transition */
static animtransition_t *transition_delete(animtransition_t *transition); /* deletes transition */

/* user-defined properties */
struct userproperty_t
{
    DARRAY(char*, data); /* NULL-terminated string array of length >= 1 */
};

static userproperty_t* userproperty_new(); /* create a user-defined property */
static userproperty_t* userproperty_delete(userproperty_t* user_property); /* delete a user-defined property */
static void userproperty_push(userproperty_t* user_property, const char* string); /* add a string to a user-defined property */
static bool userproperty_is_valid_name(const char* name); /* validate the name of a user-defined property */

/* constants & utilities */
#define MAX_ANIMATIONS 256 /* sprites can have at most this number of animations (numbered from 0 to MAX_ANIMATIONS-1) */
#define is_valid_anim_id(id) ((id) >= 0 && (id) < MAX_ANIMATIONS) /* is id a valid animation ID? */
#define is_transition_anim_id(id) ((id) >= MAX_ANIMATIONS) /* is id a transition animation ID? */

static const int DEFAULT_ANIM = 0;
static const char DEFAULT_SPRITE[] = "null";
static const char OVERRIDE_PREFIX[] = "sprites/overrides/"; /* sprites defined in .spr files located in this folder take predecence over sprites defined elsewhere */
static const int OVERRIDE_PREFIX_LENGTH = sizeof(OVERRIDE_PREFIX) - 1;
static const int TRANSITION_ANY_ANIM = -1; /* special value representing a transition to/from any animation */
static const int MAX_FRAMES = 4096; /* maximum number of frames in a spritesheet */

/* private functions */
static void validate_sprite(spriteinfo_t *spr); /* validates the sprite */
static spriteinfo_t *spriteinfo_new(); /* creates a new spriteinfo_t instance */
static spriteinfo_t *spriteinfo_delete(spriteinfo_t *sprite); /* deletes an existing spriteinfo_t instance */
static animation_t *allocate_sprite_anim(spriteinfo_t *sprite, int anim_id); /* allocate an animation ID in a spriteinfo_t instance */
static int transition_cmp(const void *t1, const void *t2); /* comparison function for sorting */
static void validate_transitions(const spriteinfo_t *sprite);
static void preprocess_transitions(spriteinfo_t *sprite);
static void load_sprite_images(spriteinfo_t *spr); /* loads the sprite by reading the spritesheet */
static int scanfile(const char* vpath, void* param); /* file system callback */
static int traverse(const parsetree_statement_t *stmt, void *vpath);
static int traverse_sprite_attributes(const parsetree_statement_t *stmt, void *spriteinfo);
static int traverse_user_properties(const parsetree_statement_t *stmt, void *dict);
static void inspect_transitions(const spriteinfo_t* sprite);
static void destroy_proganim(void* element, void* context);
static void destroy_userproperty(void* element, void* context);

/* hash table that stores the metadata of the sprites */
HASHTABLE_GENERATE_CODE(spriteinfo_t, spriteinfo_destroy);
static HASHTABLE(spriteinfo_t, sprites);




/* public methods */

/*
 * sprite_init()
 * Initializes the sprite system
 */
void sprite_init()
{
    logfile_message("Loading sprites...");
    sprites = hashtable_spriteinfo_t_create();

    /* scan the sprites/ folder */
    asset_foreach_file("sprites", ".spr", scanfile, NULL, true);

    logfile_message("All sprites have been loaded!");
}


/*
 * sprite_release()
 * Releases the sprite system
 */
void sprite_release()
{
    logfile_message("Releasing sprites...");
    sprites = hashtable_spriteinfo_t_destroy(sprites);
}



/*
 * sprite_get_animation()
 * Returns a pointer to an animation corresponding to the
 * specified sprite name and animation number.
 * Tip: to use a default sprite, set sprite_name to NULL
 */
const animation_t *sprite_get_animation(const char *sprite_name, int anim_id)
{
    const spriteinfo_t *sprite;

    /* default sprite? */
    if(sprite_name == NULL)
        return sprite_get_animation(DEFAULT_SPRITE, DEFAULT_ANIM);

    /* find the corresponding spriteinfo_t* instance */
    sprite = hashtable_spriteinfo_t_find(sprites, sprite_name);
    if(sprite != NULL) {
        if(anim_id >= 0 && anim_id < sprite->animation_count) {
            if(sprite->animation_data[anim_id] != NULL)
                return sprite->animation_data[anim_id];
        }

        /* can't find the required animation */
        fatal_error("Can't find animation %d of sprite \"%s\"", anim_id, sprite_name);
    }

    /* can't find the required sprite */
    fatal_error("Can't find sprite \"%s\"", sprite_name);
    return NULL;
}


/*
 * sprite_animation_exists()
 * Checks if an animation exists (for a given sprite)
 */
bool sprite_animation_exists(const char* sprite_name, int anim_id)
{
    const spriteinfo_t *info = hashtable_spriteinfo_t_find(sprites, sprite_name);

    return info != NULL && (
        anim_id >= 0 && anim_id < info->animation_count &&
        info->animation_data[anim_id] != NULL
    );
}



/*
 * spriteinfo_create()
 * Creates a spriteinfo_t given a parse tree
 */
spriteinfo_t* spriteinfo_create(const parsetree_program_t *tree)
{
    spriteinfo_t *sprite = spriteinfo_new();
    nanoparser_traverse_program_ex(tree, (void*)sprite, traverse_sprite_attributes);

    validate_sprite(sprite);
    preprocess_transitions(sprite);
    load_sprite_images(sprite);

    (void)inspect_transitions;
    return sprite;
}

/*
 * spriteinfo_destroy()
 * Destroys a spriteinfo_t object
 */
void spriteinfo_destroy(spriteinfo_t *info)
{
    spriteinfo_delete(info);
}

/*
 * spriteinfo_get_animation_frame()
 * Gets an animation frame of the sprite
 */
const image_t* spriteinfo_get_animation_frame(const spriteinfo_t* info, int frame_index)
{
    if(frame_index < 0 || frame_index >= info->frame_count) {
        fatal_error("%s: can't get the frame whose index is %d of sprite \"%s\". Valid range is [0,%d].", __func__, frame_index, info->source_file, info->frame_count-1);
        return NULL;
    }

    return info->frame_data[frame_index];
}

/*
 * spriteinfo_get_animation()
 * Gets an animation of the sprite
 */
const animation_t* spriteinfo_get_animation(const spriteinfo_t* info, int anim_id)
{
    if(anim_id < 0 || anim_id >= info->animation_count || info->animation_data[anim_id] == NULL) {
        fatal_error("Can't get animation %d of sprite \"%s\"", anim_id, info->source_file);
        return NULL;
    }

    return info->animation_data[anim_id];
}

/*
 * spriteinfo_find_transition_animation()
 * Finds a transition animation in a sprite
 * Returns NULL if there isn't any
 */
const animation_t* spriteinfo_find_transition_animation(const spriteinfo_t* info, int from_id, int to_id)
{
    /* skip everything: there are no transitions in this sprite */
    if(darray_length(info->transition) == 0)
        return NULL;

    /* perform a linear search on the set of all transition animations from
       (from->id). Find a transition to (to->id). This set is typically very
       small, UNLESS a declaration such as "transition any to x" is present in
       the .spr file. In the latter case, we perform a binary search because
       the set is ordered by ascending "to_id".

       Note: a "transition any to x" declaration is, in most cases, unnecessary
       and can be replaced by the first few frames of anim x, using repeat_from
       if x repeats. If x must repeat with a different repeat_from index, then
       a transition from any to x is applicable. How rare is that? Very rare.
       It's not just a matter of necessity, however. Such a transition can be
       used for clarity in the script. */
    assertx(from_id >= 0 && from_id + 1 < info->transition_from_length);

    int start = info->transition_from[from_id];
    int end = info->transition_from[from_id + 1];
    int count = end - start; /* zero if there are no transitions from (from_id) */

    if(count < 6) {
        /* linear search */
        for(int i = start; i < end; i++) {
            const animtransition_t* transition = info->preprocessed_transition[i];
            if(transition->to_id == to_id)
                return info->animation_data[transition->anim_id];
        }
    }
    else {
        /* binary search */
        --end;
        while(start <= end) {
            int middle = (start + end) / 2;
            const animtransition_t* transition = info->preprocessed_transition[middle];

            if(transition->to_id == to_id)
                return info->animation_data[transition->anim_id];
            else if(transition->to_id > to_id)
                end = middle - 1;
            else
                start = middle + 1;
        }
    }

    /* not found */
    return NULL;
}

/*
 * spriteinfo_get_proganim()
 * Get a programmatic animation, or NULL if none is defined with the given name
 */
const proganim_t* spriteinfo_get_proganim(const spriteinfo_t* info, const char* name)
{
    const proganim_t* prog_anim = dictionary_get(info->prog_anims, name);
    return prog_anim; /* possibly NULL */
}

/*
 * spriteinfo_user_property()
 * Get a NULL-terminated array with the element(s) of a user-defined custom property,
 * or NULL if no property with the given name exists
 */
const char* const* spriteinfo_user_property(const spriteinfo_t* info, const char* name)
{
    const userproperty_t* user_property = dictionary_get(info->user_properties, name);

    /* no such user-defined custom property? */
    if(user_property == NULL)
        return NULL;

    /* user-property->data is a NULL-terminated array of strings */
    assertx(user_property->data[0] != NULL); /* ensure length(array) > 0 */

    /* done! */
    return (const char* const*)user_property->data;
}

/*
 * spriteinfo_source_file()
 * The source file (image) of the sprite
 */
const char* spriteinfo_source_file(const spriteinfo_t* info)
{
    return info->source_file;
}

/*
 * spriteinfo_source_rect()
 * The source rect of the sprite
 */
rect_t spriteinfo_source_rect(const spriteinfo_t* info)
{
    return rect_new(info->rect_x, info->rect_y, info->rect_w, info->rect_h);
}

/*
 * spriteinfo_frame_width()
 * The width of a frame of the sprite
 */
int spriteinfo_frame_width(const spriteinfo_t* info)
{
    return info->frame_w;
}

/*
 * spriteinfo_frame_height()
 * The height of a frame of the sprite
 */
int spriteinfo_frame_height(const spriteinfo_t* info)
{
    return info->frame_h;
}

/*
 * spriteinfo_to_collisionmask()
 * Create a collision mask from a frame of the spritesheet
 */
collisionmask_t* spriteinfo_to_collisionmask(const spriteinfo_t* info, int frame_index)
{
    if(frame_index < 0 || frame_index >= info->frame_count) {
        fatal_error("%s: can't get the frame whose index is %d of sprite \"%s\". Valid range is [0,%d].", __func__, frame_index, info->source_file, info->frame_count-1);
        return NULL;
    }

#if 0
    /* apparently, locking doesn't work properly with sub-bitmaps */
    image_t* image = info->frame_data[frame_index];
    image_lock(image, "r");
    collisionmask_t* mask = collisionmask_create(image, 0, 0, info->frame_w, info->frame_h);
    image_unlock(image);
#else
    /* let's lock the entire spritesheet and recompute the coordinates of the frame */
    image_t* spritesheet = (image_t*)info->spritesheet;
    int w = info->rect_w / info->frame_w;
    int x = info->rect_x + (frame_index % w) * info->frame_w;
    int y = info->rect_y + (frame_index / w) * info->frame_h;

    image_lock(spritesheet, "r");
    collisionmask_t* mask = collisionmask_create(spritesheet, x, y, info->frame_w, info->frame_h);
    image_unlock(spritesheet);
#endif

    return mask;
}




/* private methods */

/*
 * scanfile()
 * Called for each .spr file in sprites/
 */
int scanfile(const char *vpath, void *param)
{
    const char* fullpath = asset_path(vpath);

    /* read the .spr file */
    parsetree_program_t* p = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program_ex(p, (void*)vpath, traverse);
    nanoparser_deconstruct_tree(p);

    /* done! */
    return 0;
}

/*
 * spriteinfo_new()
 * Creates a new empty spriteinfo_t instance
 */
spriteinfo_t *spriteinfo_new()
{
    spriteinfo_t *sprite = mallocx(sizeof *sprite);

    sprite->source_file = NULL;
    sprite->rect_x = 0;
    sprite->rect_y = 0;
    sprite->rect_w = 1;
    sprite->rect_h = 1;
    sprite->frame_w = 1;
    sprite->frame_h = 1;
    sprite->default_hot_spot = v2d_new(0, 0);
    sprite->default_action_spot = v2d_new(0, 0);

    sprite->frame_count = 0;
    sprite->frame_data = NULL;
    sprite->spritesheet = NULL;

    sprite->animation_count = 0;
    sprite->animation_data = NULL;

    darray_init(sprite->transition);
    darray_init(sprite->preprocessed_transition);
    sprite->transition_from = NULL; /* lazy allocation */
    sprite->transition_from_length = 0;

    sprite->prog_anims = dictionary_create(true, destroy_proganim, sprite);
    sprite->user_properties = dictionary_create(true, destroy_userproperty, sprite);

    return sprite;
}

/*
 * spriteinfo_delete()
 * Deletes a spriteinfo_t instance
 */
spriteinfo_t *spriteinfo_delete(spriteinfo_t *sprite)
{
    extern animation_t *animation_destroy(animation_t *anim);

    /* delete user-defined properties */
    dictionary_destroy(sprite->user_properties);

    /* delete programmatic animations */
    dictionary_destroy(sprite->prog_anims);

    /* delete transition_from[] */
    if(sprite->transition_from != NULL)
        free(sprite->transition_from);

    /* delete the preprocessed transitions */
    for(int i = 0; i < darray_length(sprite->preprocessed_transition); i++)
        transition_delete(sprite->preprocessed_transition[i]);
    darray_release(sprite->preprocessed_transition);

    /* delete the transitions */
    for(int i = 0; i < darray_length(sprite->transition); i++)
        transition_delete(sprite->transition[i]);
    darray_release(sprite->transition);

    /* delete the animations */
    if(sprite->animation_data != NULL) {
        for(int i = 0; i < sprite->animation_count; i++) {
            if(sprite->animation_data[i] != NULL)
                sprite->animation_data[i] = animation_destroy(sprite->animation_data[i]);
        }
        free(sprite->animation_data);
        sprite->animation_data = NULL;
    }

    /* delete the images */
    if(sprite->frame_data != NULL) {
        for(int i = 0; i < sprite->frame_count; i++)
            image_destroy(sprite->frame_data[i]);
        free(sprite->frame_data);
        sprite->frame_data = NULL;
    }

    /* delete the source_file string */
    if(sprite->source_file != NULL) {
        #if 0
        /* this may crash the editor on reload.
           further investigation is needed. */
        resourcemanager_purge_image(sprite->source_file); /* force reload */
        #endif
        free(sprite->source_file);
        sprite->source_file = NULL;
    }

    /* delete the spriteinfo_t instance */
    free(sprite);

    /* done! */
    return NULL;
}

/*
 * allocate_sprite_anim()
 * Allocate an animation ID in a spriteinfo_t instance
 */
animation_t *allocate_sprite_anim(spriteinfo_t *sprite, int anim_id)
{
    extern animation_t *animation_create(const spriteinfo_t *sprite, int anim_id, bool is_transition, int frame_width, int frame_height, v2d_t default_hot_spot, v2d_t default_action_spot);
    extern animation_t *animation_destroy(animation_t *anim);
    const int HARD_LIMIT = 2 * MAX_ANIMATIONS; /* transitions are numbered from MAX_ANIMATIONS to HARD_LIMIT-1 */

    /* validate anim_id */
    if(anim_id < 0 || anim_id >= HARD_LIMIT) {
        fatal_error(
            "Can't allocate %sanimation %d: it's outside the range [0, %d]",
            anim_id >= MAX_ANIMATIONS ? "transition " : "", /* wow, really? THAT many transitions? */
            anim_id,
            HARD_LIMIT - 1
        );
    }

    /* delete existing animation, if we're overwriting any */
    if(anim_id >= 0 && anim_id < sprite->animation_count && NULL != sprite->animation_data[anim_id])
        sprite->animation_data[anim_id] = animation_destroy(sprite->animation_data[anim_id]);

    /* resize animation_data[] */
    int old_count = sprite->animation_count;
    sprite->animation_count = max(sprite->animation_count, anim_id + 1);
    sprite->animation_data = reallocx(sprite->animation_data, sizeof(animation_t*) * sprite->animation_count); /* watch this! It may generate garbage in the middle. */
    for(int i = old_count; i < sprite->animation_count; i++)
        sprite->animation_data[i] = NULL;

    /* allocate & return the new animation */
    sprite->animation_data[anim_id] = animation_create(
        sprite,
        anim_id,
        is_transition_anim_id(anim_id),
        sprite->frame_w,
        sprite->frame_h,
        sprite->default_hot_spot,
        sprite->default_action_spot
    );

    return sprite->animation_data[anim_id];
}

/*
 * transition_new()
 * Creates a transition
 */
animtransition_t *transition_new(int anim_id, int from_id, int to_id)
{
    animtransition_t *transition = mallocx(sizeof *transition);

    transition->anim_id = anim_id;
    transition->from_id = from_id;
    transition->to_id = to_id;
    transition->is_valid = false;

    return transition;
}

/*
 * transition_delete()
 * Deletes a list of transitions
 */
animtransition_t *transition_delete(animtransition_t *transition)
{
    free(transition);
    return NULL;
}

/*
 * transition_cmp()
 * Comparison function (sorting)
 */
int transition_cmp(const void *t1, const void *t2)
{
    const animtransition_t *a = *((const animtransition_t**)t1);
    const animtransition_t *b = *((const animtransition_t**)t2);

    /* sort by ascending from_id and then by ascending to_id */
    if(a->from_id != b->from_id)
        return a->from_id - b->from_id;
    else
        return a->to_id - b->to_id;
}

/*
 * validate_transitions()
 * Validate the transition animations of a sprite
 */
void validate_transitions(const spriteinfo_t *sprite)
{
    #define STRINGIFY_FIELD(x,k) (x == TRANSITION_ANY_ANIM ? "any" : str_from_int((x), buf[k], sizeof(buf[k])))
    char buf[2][32];

    for(int i = 0; i < darray_length(sprite->transition); i++) {
        animtransition_t* transition = sprite->transition[i];

        /* validate the from_id and the to_id fields */
        if(!(
        (
            /* require a valid to_id field */
            (transition->to_id == TRANSITION_ANY_ANIM) ||
            (transition->to_id >= 0 && transition->to_id < sprite->animation_count && sprite->animation_data[transition->to_id] != NULL)
        ) && (
            /* require a valid from_id field */
            (transition->from_id == TRANSITION_ANY_ANIM) ||
            (transition->from_id >= 0 && transition->from_id < sprite->animation_count && sprite->animation_data[transition->from_id] != NULL)
        ) && (
            /* can't transition to itself */
            (transition->from_id != transition->to_id)
        ) && (
            /* can't transition any to any */
            /*!(transition->to_id == TRANSITION_ANY_ANIM && transition->from_id == TRANSITION_ANY_ANIM)*/
            1 /* redundant */
        )
        )) {
            logfile_message("WARNING: found an invalid animation transition from %s to %s at \"%s\"", STRINGIFY_FIELD(transition->from_id, 0), STRINGIFY_FIELD(transition->to_id, 1), sprite->source_file);
            transition->is_valid = false;
            continue;
        }

        /* the transition is valid */
        transition->is_valid = true;
    }

    #undef STRINGIFY_FIELD
}


/*
 * preprocess_transitions()
 * Preprocess the animation transitions
 */
void preprocess_transitions(spriteinfo_t *sprite)
{
    int anim_id[MAX_ANIMATIONS];
    int anim_id_length = 0;

    /* skip if there is nothing to do */
    if(darray_length(sprite->transition) == 0)
        return;

    /* set the validity flag of all transitions */
    validate_transitions(sprite);

    /* find all valid animation IDs */
    assertx(MAX_ANIMATIONS <= 256);
    for(int i = 0; i < MAX_ANIMATIONS; i++) {
        if(sprite->animation_data[i] != NULL)
            anim_id[anim_id_length++] = i;
    }

    /* no valid animations; this shouldn't happen */
    if(anim_id_length == 0)
        return;

    /* get the minimum integer greater than the largest animation ID */
    int sup_anim_id = 1 + anim_id[anim_id_length - 1]; /* no more than MAX_ANIMATIONS */

    /* first, let's copy all transitions from x to y (non-any) */
    for(int i = 0; i < darray_length(sprite->transition); i++) {
        const animtransition_t* transition = sprite->transition[i];

        /* skip invalid transitions */
        if(!transition->is_valid)
            continue;

        /* skip transitions from/to "any" */
        else if(transition->from_id == TRANSITION_ANY_ANIM || transition->to_id == TRANSITION_ANY_ANIM)
            continue;

        /* copy transition from x to y */
        animtransition_t* copy = transition_new(transition->anim_id, transition->from_id, transition->to_id);
        copy->is_valid = true;
        darray_push(sprite->preprocessed_transition, copy);
    }

    /* second, preprocess transitions to/from "any" other animation number */
    for(int i = 0; i < darray_length(sprite->transition); i++) {
        const animtransition_t* transition = sprite->transition[i];

        /* skip invalid transitions */
        if(!transition->is_valid)
            continue;

        /* copy transitions from any */
        if(transition->from_id == TRANSITION_ANY_ANIM) {
            for(int j = 0; j < anim_id_length; j++) {
                if(anim_id[j] != transition->to_id) { /* replace "from any" by all valid animations */
                    animtransition_t* new_transition = transition_new(transition->anim_id, anim_id[j], transition->to_id);
                    new_transition->is_valid = true;
                    darray_push(sprite->preprocessed_transition, new_transition);
                }
            }
        }

        /* copy transitions to any */
        else if(transition->to_id == TRANSITION_ANY_ANIM) {
            for(int j = 0; j < anim_id_length; j++) {
                if(anim_id[j] != transition->from_id) { /* replace "to any" by all valid animations */
                    animtransition_t* new_transition = transition_new(transition->anim_id, transition->from_id, anim_id[j]);
                    new_transition->is_valid = true;
                    darray_push(sprite->preprocessed_transition, new_transition);
                }
            }
        }

        /* there are no transitions from any to any */

    }

    /* sort transitions with a stable sorting algorithm, so
       that transitions from/to "any" have lower precedence */
    int n = darray_length(sprite->preprocessed_transition);
    merge_sort(sprite->preprocessed_transition, n, sizeof(*(sprite->preprocessed_transition)), transition_cmp);

    /* allocate transition_from[] and initialize it with -1s */
    assertx(sprite->transition_from == NULL);
    sprite->transition_from_length = 1 + sup_anim_id;
    sprite->transition_from = mallocx((1 + sup_anim_id) * sizeof(int));
    for(int i = 0; i <= sup_anim_id; i++)
        sprite->transition_from[i] = -1;

    /* fill transition_from[] */
    for(int i = n-1; i >= 0; i--) {
        const animtransition_t* transition = sprite->preprocessed_transition[i];
        assertx(transition->from_id >= 0 && transition->from_id < sup_anim_id);
        sprite->transition_from[transition->from_id] = i;
    }

    /* get rid of all -1s */
    sprite->transition_from[sup_anim_id] = n;
    for(int i = sup_anim_id - 1; i >= 0; i--) {
        if(sprite->transition_from[i] == -1)
            sprite->transition_from[i] = sprite->transition_from[i+1]; /* one minus the other (count) is zero */
    }
}


/*
 * userproperty_new()
 * Create a user-defined property
 */
userproperty_t* userproperty_new()
{
    userproperty_t* user_property = mallocx(sizeof *user_property);

    darray_init(user_property->data);
    darray_push(user_property->data, NULL);

    return user_property;
}


/*
 * userproperty_delete()
 * Delete a user-defined property
 */
userproperty_t* userproperty_delete(userproperty_t* user_property)
{
    /* user_property->data is a NULL-terminated array */
    for(int i = darray_length(user_property->data) - 2; i >= 0; i--)
        free(user_property->data[i]);

    darray_release(user_property->data);

    free(user_property);
    return NULL;
}


/*
 * userproperty_push()
 * Add a string to a user-defined property
 */
void userproperty_push(userproperty_t* user_property, const char* string)
{
    int last = darray_length(user_property->data) - 1;

    assertx(user_property->data[last] == NULL);

    user_property->data[last] = str_dup(string);
    darray_push(user_property->data, NULL);
}


/*
 * userproperty_is_valid_name()
 * Validate the name of a user-defined property
 */
bool userproperty_is_valid_name(const char* name)
{
    if(!(isalpha(*name) || *name == '_'))
        return false;

    for(const char* p = name + 1; *p != '\0'; p++) {
        if(!(isalnum(*p) || *p == '_'))
            return false;
    }

    return true;
}


/*
 * validate_sprite()
 * Validate (and possibly fix) the sprite
 */
void validate_sprite(spriteinfo_t *spr)
{
    /* check for fatal errors */
    if(spr->source_file == NULL)
        fatal_error("Sprite error: unspecified source_file");
    else if(spr->spritesheet == NULL)
        fatal_error("Sprite error: invalid spritesheet \"%s\"", spr->source_file);
    else if(spr->animation_count < 1 || spr->animation_data == NULL)
        fatal_error("Sprite error: sprites must contain at least one animation");

    /* validate the source_rect and the frame_size - and adjust if necessary */
    if(spr->rect_x < 0 || spr->rect_y < 0 || spr->rect_w <= 0 || spr->rect_h <= 0) {
        logfile_message("Sprite error: invalid source_rect (%d,%d,%d,%d)", spr->rect_x, spr->rect_y, spr->rect_w, spr->rect_h);
        spr->rect_x = max(0, spr->rect_x);
        spr->rect_y = max(0, spr->rect_y);
        spr->rect_w = max(1, spr->rect_w);
        spr->rect_h = max(1, spr->rect_h);
        logfile_message("Adjusting source_rect to (%d,%d,%d,%d)", spr->rect_x, spr->rect_y, spr->rect_w, spr->rect_h);
    }

    if(spr->rect_x >= image_width(spr->spritesheet) || spr->rect_y >= image_height(spr->spritesheet)) {
        logfile_message("Sprite error: source_rect (%d,%d,%d,%d) is out of bounds of %d x %d image \"%s\"", spr->rect_x, spr->rect_y, spr->rect_w, spr->rect_h, image_width(spr->spritesheet), image_height(spr->spritesheet), spr->source_file);
        spr->rect_x = min(spr->rect_x, image_width(spr->spritesheet) - 1);
        spr->rect_y = min(spr->rect_y, image_height(spr->spritesheet) - 1);
        logfile_message("Adjusting source_rect to (%d,%d,%d,%d)", spr->rect_x, spr->rect_y, spr->rect_w, spr->rect_h);
    }

    if(spr->rect_x + spr->rect_w > image_width(spr->spritesheet) || spr->rect_y + spr->rect_h > image_height(spr->spritesheet)) {
        logfile_message("Sprite error: source_rect (%d,%d,%d,%d) exceeds %d x %d image \"%s\"", spr->rect_x, spr->rect_y, spr->rect_w, spr->rect_h, image_width(spr->spritesheet), image_height(spr->spritesheet), spr->source_file);
        spr->rect_w = min(image_width(spr->spritesheet) - spr->rect_x, spr->rect_w);
        spr->rect_h = min(image_height(spr->spritesheet) - spr->rect_y, spr->rect_h);
        logfile_message("Adjusting source_rect to (%d,%d,%d,%d)", spr->rect_x, spr->rect_y, spr->rect_w, spr->rect_h);
    }

    if(spr->frame_w <= 0 || spr->frame_h <= 0) {
        logfile_message("Sprite error: invalid frame_size (%d,%d)", spr->frame_w, spr->frame_h);
        spr->frame_w = max(1, spr->frame_w);
        spr->frame_h = max(1, spr->frame_h);
        logfile_message("Adjusting frame_size to (%d,%d)", spr->frame_w, spr->frame_h);
    }

    if(spr->frame_w > spr->rect_w || spr->frame_h > spr->rect_h) {
        logfile_message("Sprite error: frame_size (%d,%d) can't be larger than source_rect size (%d,%d)", spr->frame_w, spr->frame_h, spr->rect_w, spr->rect_h);
        spr->frame_w = min(spr->frame_w, spr->rect_w);
        spr->frame_h = min(spr->frame_h, spr->rect_h);
        logfile_message("Adjusting frame_size to (%d,%d)", spr->frame_w, spr->frame_h);
    }

    if(spr->rect_w % spr->frame_w > 0 || spr->rect_h % spr->frame_h > 0) {
        logfile_message("Sprite error: incompatible frame_size (%d,%d) x source_rect size (%d,%d). source_rect size should be a multiple of frame_size.", spr->frame_w, spr->frame_h, spr->rect_w, spr->rect_h);
        spr->rect_w = (spr->rect_w % spr->frame_w > 0) ? (spr->rect_w - spr->rect_w % spr->frame_w + spr->frame_w) : spr->rect_w;
        spr->rect_h = (spr->rect_h % spr->frame_h > 0) ? (spr->rect_h - spr->rect_h % spr->frame_h + spr->frame_h) : spr->rect_h;
        logfile_message("Adjusting source_rect size to (%d,%d)", spr->rect_w, spr->rect_h);
    }

    /* validate the number of frames in the spritesheet */
    int number_of_frames_in_the_sheet = (spr->rect_w / spr->frame_w) * (spr->rect_h / spr->frame_h);
    if(number_of_frames_in_the_sheet > MAX_FRAMES)
        fatal_error("Sprite error: sprites cannot have more than %d frames. Is the frame_size (%d,%d) correct?", MAX_FRAMES, spr->frame_w, spr->frame_h);

    /* validate individual animations */
    extern void animation_validate(animation_t *anim, int number_of_frames_in_the_sheet);
    for(int i = 0; i < spr->animation_count; i++) {
        if(spr->animation_data[i] != NULL)
            animation_validate(spr->animation_data[i], number_of_frames_in_the_sheet);
    }

    /* validate individual keyframe-based animations */
    extern void proganim_validate(proganim_t* prog_anim);
    iterator_t* it = dictionary_keys(spr->prog_anims);
    while(iterator_has_next(it)) {
        const char* name = iterator_next(it);
        logfile_message("Validating keyframe-based animation \"%s\"...", name);

        proganim_t* prog_anim = dictionary_get(spr->prog_anims, name);
        proganim_validate(prog_anim);
    }
    iterator_destroy(it);
}



/*
 * load_sprite_images()
 * Loads the sprite by reading the spritesheet
 */
void load_sprite_images(spriteinfo_t *spr)
{
    int cur_x, cur_y;

    /* allocate frames */
    spr->frame_count = (spr->rect_w / spr->frame_w) * (spr->rect_h / spr->frame_h);
    spr->frame_data = mallocx(spr->frame_count * sizeof(*(spr->frame_data)));

    /* read each frame */
    cur_x = spr->rect_x;
    cur_y = spr->rect_y;
    for(int i = 0; i < spr->frame_count; i++) {
        spr->frame_data[i] = image_create_shared(spr->spritesheet, cur_x, cur_y, spr->frame_w, spr->frame_h);
        cur_x += spr->frame_w;
        if(cur_x >= spr->rect_x + spr->rect_w) {
            cur_x = spr->rect_x;
            cur_y += spr->frame_h;
        }
    }
}


/*
 * destroy_proganim()
 * Destroy a programmatic animation (dictionary)
 */
void destroy_proganim(void* element, void* context)
{
    extern proganim_t* proganim_destroy(proganim_t* prog_anim);
    proganim_t* prog_anim = (proganim_t*)element;
    spriteinfo_t* sprite = (spriteinfo_t*)context;

    proganim_destroy(prog_anim);

    (void)sprite;
}


/*
 * destroy_userproperty()
 * Destroy a user-defined property (dictionary)
 */
void destroy_userproperty(void* element, void* context)
{
    userproperty_t* user_property = (userproperty_t*)element;
    spriteinfo_t* sprite = (spriteinfo_t*)context;

    userproperty_delete(user_property);

    (void)sprite;
}


/*
 * traverse()
 * Sprite block traversal
 */
int traverse(const parsetree_statement_t *stmt, void *vpath)
{
    const char *identifier, *sprite_name;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    const spriteinfo_t *sprite;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "sprite") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1); /* first parameter = sprite name */
        p2 = nanoparser_get_nth_parameter(param_list, 2); /* second parameter = block */

        nanoparser_expect_string(p1, "Must provide sprite name");
        nanoparser_expect_program(p2, "Must provide sprite attributes");

        sprite_name = nanoparser_get_string(p1);
        nanoparser_warn(stmt, "Loading sprite \"%s\"", sprite_name);

        if(NULL == (sprite = hashtable_spriteinfo_t_find(sprites, sprite_name))) {
            /* register a new sprite */
            spriteinfo_t *new_sprite = spriteinfo_create(nanoparser_get_program(p2));
            hashtable_spriteinfo_t_add(sprites, sprite_name, new_sprite);
        }
        else {
            /* solve conflicting definitions for the same sprite */
            bool must_override = (str_incmp((const char*)vpath, OVERRIDE_PREFIX, OVERRIDE_PREFIX_LENGTH) == 0);

            if(must_override) {
                nanoparser_warn(stmt, "OVERRIDE: redefining sprite \"%s\"", sprite_name);

                spriteinfo_t *new_sprite = spriteinfo_create(nanoparser_get_program(p2));
                if(hashtable_spriteinfo_t_replace(sprites, sprite_name, new_sprite))
                    return 0; /* the sprite has been successfully redefined */

                nanoparser_warn(stmt, "Can't override sprite \"%s\"", sprite_name); /* shouldn't happen */
                spriteinfo_destroy(new_sprite);
            }
            else
                nanoparser_warn(stmt, "Can't redefine sprite \"%s\"", sprite_name);
        }
    }
    else
        nanoparser_crash(stmt, "Unknown identifier \"%s\"", identifier);

    return 0;
}


/*
 * traverse_sprite_attributes()
 * Sprite attributes traversal
 */
int traverse_sprite_attributes(const parsetree_statement_t *stmt, void *spriteinfo)
{
    extern int traverse_animation_attributes(const parsetree_statement_t *stmt, void *animation);
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4;
    spriteinfo_t *s = (spriteinfo_t*)spriteinfo;
    int anim_id = 0;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    /* sanity check */
    if(s->animation_data != NULL && str_icmp(identifier, "animation") != 0 && str_icmp(identifier, "transition") != 0 && str_icmp(identifier, "keyframes") != 0)
        nanoparser_crash(stmt, "Can't load sprite. Animations, transitions and keyframes must be declared after the other parameters");

    /* read parameters */
    if(str_icmp(identifier, "source_file") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Must provide path to the source_file");
        if(s->source_file != NULL)
            free(s->source_file);
        s->source_file = str_dup(nanoparser_get_string(p1));
        s->spritesheet = image_load(s->source_file);
    }
    else if(str_icmp(identifier, "source_rect") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);
        p4 = nanoparser_get_nth_parameter(param_list, 4);
        nanoparser_expect_string(p1, "Must provide four numbers to source_rect: xpos, ypos, width, height");
        nanoparser_expect_string(p2, "Must provide four numbers to source_rect: xpos, ypos, width, height");
        nanoparser_expect_string(p3, "Must provide four numbers to source_rect: xpos, ypos, width, height");
        nanoparser_expect_string(p4, "Must provide four numbers to source_rect: xpos, ypos, width, height");
        s->rect_x = max(0, atoi(nanoparser_get_string(p1)));
        s->rect_y = max(0, atoi(nanoparser_get_string(p2)));
        s->rect_w = max(1, atoi(nanoparser_get_string(p3)));
        s->rect_h = max(1, atoi(nanoparser_get_string(p4)));
    }
    else if(str_icmp(identifier, "frame_size") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "Must provide two numbers to frame_size: width, height");
        nanoparser_expect_string(p2, "Must provide two numbers to frame_size: width, height");
        s->frame_w = max(1, atoi(nanoparser_get_string(p1)));
        s->frame_h = max(1, atoi(nanoparser_get_string(p2)));
    }
    else if(str_icmp(identifier, "hot_spot") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "Must provide two numbers to hot_spot: xpos, ypos");
        nanoparser_expect_string(p2, "Must provide two numbers to hot_spot: xpos, ypos");
        s->default_hot_spot.x = (float)atoi(nanoparser_get_string(p1));
        s->default_hot_spot.y = (float)atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "action_spot") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "Must provide two numbers to action_spot: xpos, ypos");
        nanoparser_expect_string(p2, "Must provide two numbers to action_spot: xpos, ypos");
        s->default_action_spot.x = (float)atoi(nanoparser_get_string(p1));
        s->default_action_spot.y = (float)atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "animation") == 0) {
        int n = nanoparser_get_number_of_parameters(param_list);

        if(n == 2) {
            p1 = nanoparser_get_nth_parameter(param_list, 1);
            p2 = nanoparser_get_nth_parameter(param_list, 2);
            nanoparser_expect_string(p1, "Must provide animation number");
            nanoparser_expect_program(p2, "Must provide animation attributes");
            anim_id = atoi(nanoparser_get_string(p1));
        }
        else if(n == 1) {
            p1 = nanoparser_get_nth_parameter(param_list, 1);
            nanoparser_expect_program(p1, "Must provide animation attributes");
            anim_id = 0;
            p2 = p1;
        }
        else {
            p1 = p2 = NULL;
            nanoparser_crash(stmt, "Undefined animation block");
        }

        /* validate anim_id */
        if(!is_valid_anim_id(anim_id))
            nanoparser_crash(stmt, "Animation numbers must be in the range [0, %d]", MAX_ANIMATIONS - 1);

        /* allocate animation */
        allocate_sprite_anim(s, anim_id);

        /* read the animation */
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), s->animation_data[anim_id], traverse_animation_attributes);
    }
    else if(str_icmp(identifier, "transition") == 0) {
        int anim_id, from_id, to_id;

        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);
        p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "Expected an animation number or \"any\"");
        nanoparser_expect_string(p2, "Expected the keyword \"to\"");
        nanoparser_expect_string(p3, "Expected an animation number or \"any\"");
        nanoparser_expect_program(p4, "Expected a transition animation block");

        /* read & validate animation IDs */
        if(str_icmp("any", nanoparser_get_string(p1)) != 0) {
            from_id = atoi(nanoparser_get_string(p1));
            if(!is_valid_anim_id(from_id))
                nanoparser_crash(stmt, "Invalid transition. Animation numbers must be in the range [0, %d]", MAX_ANIMATIONS - 1);
        }
        else
            from_id = TRANSITION_ANY_ANIM;

        if(str_icmp("any", nanoparser_get_string(p3)) != 0) {
            to_id = atoi(nanoparser_get_string(p3));
            if(!is_valid_anim_id(to_id))
                nanoparser_crash(stmt, "Invalid transition. Animation numbers must be in the range [0, %d]", MAX_ANIMATIONS - 1);
        }
        else
            to_id = TRANSITION_ANY_ANIM;

        if(from_id == TRANSITION_ANY_ANIM && to_id == TRANSITION_ANY_ANIM)
            nanoparser_crash(stmt, "Can't set a transition from \"any\" to \"any\"");
        else if(from_id == to_id)
            nanoparser_crash(stmt, "Can't set a transition from itself to itself");

        /* validate syntax */
        if(str_icmp(nanoparser_get_string(p2), "to") != 0)
            nanoparser_crash(stmt, "Syntax error: expected keyword \"to\"");

        /* allocate transition animation */
        anim_id = MAX_ANIMATIONS + darray_length(s->transition); /* allocate after MAX_ANIMATIONS */
        allocate_sprite_anim(s, anim_id);
        darray_push(s->transition, transition_new(anim_id, from_id, to_id)); /* register the transition */

        /* read the animation */
        nanoparser_traverse_program_ex(nanoparser_get_program(p4), s->animation_data[anim_id], traverse_animation_attributes);
    }
    else if(str_icmp(identifier, "keyframes") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "Must provide the name of the keyframe-based animation");
        nanoparser_expect_program(p2, "Must provide the declaration of a keyframe-based animation");

        /* validate name */
        const char* name = nanoparser_get_string(p1);
        if(*name == '\0')
            nanoparser_crash(stmt, "Invalid name for a keyframe-based animation");

        /* allocate a keyframe-based animation */
        extern proganim_t* proganim_create();
        proganim_t* prog_anim = proganim_create();

        /* read the keyframes */
        extern int traverse_keyframes(const parsetree_statement_t* stmt, void* context);
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), prog_anim, traverse_keyframes);

        /* add the keyframe-based animation to the dictionary */
        dictionary_put(s->prog_anims, name, prog_anim);
    }
    else if(str_icmp(identifier, "custom_properties") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Expected a block of user-defined properties");

        /* read the user-defined properties */
        nanoparser_traverse_program_ex(nanoparser_get_program(p1), s->user_properties, traverse_user_properties);
    }
    else
        nanoparser_crash(stmt, "Unknown identifier \"%s\"", identifier);

    return 0;
}


/*
 * traverse_user_properties()
 * Read user-defined properties
 */
int traverse_user_properties(const parsetree_statement_t *stmt, void *dict)
{
    /* validate the name of the user-defined property */
    const char* identifier = nanoparser_get_identifier(stmt);
    if(!userproperty_is_valid_name(identifier))
        nanoparser_crash(stmt, "Not a valid user-defined property name \"%s\"", identifier);

    /* validate the content of the user-defined property */
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);
    int number_of_parameters = nanoparser_get_number_of_parameters(param_list);
    if(number_of_parameters == 0)
        nanoparser_crash(stmt, "Unspecified user-defined property \"%s\"", identifier);

    /* allocate a user-defined property */
    dictionary_t* user_properties = (dictionary_t*)dict;
    userproperty_t* user_property = userproperty_new();
    dictionary_put(user_properties, identifier, user_property);

    /* read the element(s) of the user-defined property */
    for(int i = 1; i <= number_of_parameters; i++) {
        const parsetree_parameter_t* param_i = nanoparser_get_nth_parameter(param_list, i);
        const char* string = nanoparser_get_string(param_i);

        userproperty_push(user_property, string);
    }

    /* done! */
    return 0;
}

/*
 * inspect_transitions()
 * Debug only
 */
void inspect_transitions(const spriteinfo_t* sprite)
{
#if 0
    printf("Inspecting %s\n",sprite->source_file);

    printf("\n-- TRANSITIONS --\n");
    for(int i = 0; i < darray_length(sprite->transition); i++) {
        const animtransition_t* transition = sprite->transition[i];
        printf("[%d] from %d to %d == %d\n", i, transition->from_id, transition->to_id, transition->anim_id);
    }

    printf("\n-- PREPROCESSED TRANSITIONS --\n");
    for(int i = 0; i < darray_length(sprite->preprocessed_transition); i++) {
        const animtransition_t* transition = sprite->preprocessed_transition[i];
        printf("[%d] from %d to %d == %d\n", i, transition->from_id, transition->to_id, transition->anim_id);
    }

    if(sprite->transition_from == NULL)
        return;

    printf("\n-- START / COUNT --\n");
    for(int i = 0; i < sprite->transition_from_length-1; i++) {
        int start = sprite->transition_from[i];
        int end = sprite->transition_from[i+1];
        int count = end - start;

        printf("from %d:\t\t%d transitions\t\t(indexed from %d to %d)\n", i, count, start, end-1);
        for(int j = start; j < end; j++) {
            const animtransition_t* transition = sprite->preprocessed_transition[j];
            printf("-- from %d to %d play %d\n", transition->from_id, transition->to_id, transition->anim_id);
        }
    }
#else
    (void)sprite;
#endif
}
