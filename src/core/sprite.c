/*
 * Open Surge Engine
 * sprite.c - code for the sprites/animations
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
#include "image.h"
#include "logfile.h"
#include "asset.h"
#include "resourcemanager.h"
#include "nanoparser.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/hashtable.h"

/* private stuff ;) */
HASHTABLE_GENERATE_CODE(spriteinfo_t, spriteinfo_destroy);
static HASHTABLE(spriteinfo_t, sprites);
static const int MAX_ANIMATIONS = 256; /* sprites can have at most this number of animations (numbered from 0 to MAX_ANIMATIONS-1) */
static const int DEFAULT_ANIM = 0;
static const float DEFAULT_FPS = 8.0f;
static const char DEFAULT_SPRITE[] = "null";
static const char OVERRIDE_PREFIX[] = "sprites/overrides/"; /* sprites defined in .spr files located in this folder take predecence over sprites defined elsewhere */
static const int OVERRIDE_PREFIX_LENGTH = sizeof(OVERRIDE_PREFIX) - 1;
static const int TRANSITION_ANY_ANIM = -1; /* special value representing a transition to/from any animation */
typedef struct animtransition_t animtransition_t;

/* private functions */
#define is_valid_anim_id(id) ((id) >= 0 && (id) < MAX_ANIMATIONS) /* is id a valid animation ID? */
#define is_transition_animation(anim) ((anim)->next != NULL) /* is anim a transition animation? */
static void validate_sprite(spriteinfo_t *spr); /* validates the sprite */
static void validate_animation(animation_t *anim); /* validates the animation */
static spriteinfo_t *spriteinfo_new(); /* creates a new spriteinfo_t instance */
static spriteinfo_t *spriteinfo_delete(spriteinfo_t *sprite); /* deletes an existing spriteinfo_t instance */
static animation_t *allocate_sprite_anim(spriteinfo_t *sprite, int anim_id); /* allocate an animation ID in a spriteinfo_t instance */
static animation_t *animation_new(const spriteinfo_t *sprite, int anim_id); /* creates a new animation_t instance */
static animation_t *animation_delete(animation_t *anim); /* deletes anim */
static animtransition_t *transition_new(int anim_id, int from_id, int to_id); /* creates a transition */
static animtransition_t *transition_delete(animtransition_t *transition); /* deletes transition */
static int transition_cmp(const void *t1, const void *t2); /* comparison function for sorting */
static void setup_transitions(spriteinfo_t *sprite);
static void load_sprite_images(spriteinfo_t *spr); /* loads the sprite by reading the spritesheet */
static int scanfile(const char* vpath, void* param); /* file system callback */
static int traverse(const parsetree_statement_t *stmt, void *vpath);
static int traverse_sprite_attributes(const parsetree_statement_t *stmt, void *spriteinfo);
static int traverse_animation_attributes(const parsetree_statement_t *stmt, void *animation);

/* animation transition */
/* it's an animation that plays between two other animations: from anim[from_id] to anim[to_id] */
struct animtransition_t
{
    int anim_id; /* ID of the transition animation */
    int from_id; /* ID of the previous animation */
    int to_id; /* ID of the next animation */
};




/* public methods */

/*
 * sprite_init()
 * Initializes the sprite module
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
 * Releases the sprite module
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
animation_t *sprite_get_animation(const char *sprite_name, int anim_id)
{
    spriteinfo_t *sprite;

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
int sprite_animation_exists(const char* sprite_name, int anim_id)
{
    spriteinfo_t *info = hashtable_spriteinfo_t_find(sprites, sprite_name);
    return info != NULL && (
        anim_id >= 0 && anim_id < info->animation_count &&
        info->animation_data[anim_id] != NULL
    );
}


/*
 * sprite_get_image()
 * Receives an animation and the desired frame number.
 * Returns an image.
 */
image_t *sprite_get_image(const animation_t *anim, int frame_id)
{
    frame_id = clip(frame_id, 0, anim->frame_count-1);
    return anim->sprite->frame_data[ anim->data[frame_id] ];
}


/*
 * sprite_get_transition()
 * Gets a transition animation.
 * Returns NULL if there is no such transition
 */
animation_t* sprite_get_transition(const animation_t* from, const animation_t* to)
{
    /* skip everything: there are no transitions in this sprite */
    if(!from || !to || darray_length(from->sprite->transition) == 0)
        return NULL;

    /* transitions are only available within the same sprite definition */
    if(from->sprite != to->sprite)
        return NULL;

    /* linear search */
    const spriteinfo_t* sprite = from->sprite;
    for(int i = 0; i < darray_length(sprite->transition); i++) {
        if((
            sprite->transition[i]->from_id == from->id ||
            (
                /* match any animation, except a transition animation */
                sprite->transition[i]->from_id == TRANSITION_ANY_ANIM &&
                !is_transition_animation(from)
            )
        ) && (
            sprite->transition[i]->to_id == to->id ||
            (
                /* match any animation, except a transition animation */
                sprite->transition[i]->to_id == TRANSITION_ANY_ANIM &&
                !is_transition_animation(to)
            )
        )) {
            /* validate */
            animation_t* anim = sprite->animation_data[ sprite->transition[i]->anim_id ];
            if(is_transition_animation(anim))
                return anim;
        }
    }

    /* not found */
    return NULL;
}


/*
 * animation_is_transition()
 * is anim a transition animation?
 */
bool animation_is_transition(const animation_t* anim)
{
    return anim != NULL && is_transition_animation(anim);
    /*return anim != NULL && anim->id >= MAX_ANIMATIONS;*/
}



/*
 * spriteinfo_create()
 * Creates and stores on the memory a spriteinfo_t
 * object by parsing the passed tree
 */
spriteinfo_t* spriteinfo_create(const parsetree_program_t *tree)
{
    spriteinfo_t *sprite = spriteinfo_new();

    nanoparser_traverse_program_ex(tree, (void*)sprite, traverse_sprite_attributes);
    setup_transitions(sprite);
    validate_sprite(sprite);
    load_sprite_images(sprite);

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
    sprite->animation_count = 0;
    sprite->animation_data = NULL;
    darray_init(sprite->transition);

    return sprite;
}

/*
 * spriteinfo_delete()
 * Deletes a spriteinfo_t instance
 */
spriteinfo_t *spriteinfo_delete(spriteinfo_t *sprite)
{
    int i;

    /* delete the transitions */
    for(i = 0; i < darray_length(sprite->transition); i++)
        sprite->transition[i] = transition_delete(sprite->transition[i]);
    darray_release(sprite->transition);

    /* delete the animations */
    if(sprite->animation_data != NULL) {
        for(i = 0; i < sprite->animation_count; i++) {
            if(sprite->animation_data[i] != NULL)
                sprite->animation_data[i] = animation_delete(sprite->animation_data[i]);
        }
        free(sprite->animation_data);
        sprite->animation_data = NULL;
    }

    /* delete the images */
    if(sprite->frame_data != NULL) {
        for(i = 0; i < sprite->frame_count; i++)
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
        sprite->animation_data[anim_id] = animation_delete(sprite->animation_data[anim_id]);

    /* resize animation_data[] */
    int old_count = sprite->animation_count;
    sprite->animation_count = max(sprite->animation_count, anim_id + 1);
    sprite->animation_data = reallocx(sprite->animation_data, sizeof(animation_t*) * sprite->animation_count); /* watch this! It may generate garbage in the middle. */
    for(int i = old_count; i < sprite->animation_count; i++)
        sprite->animation_data[i] = NULL;

    /* allocate & return the new animation */
    sprite->animation_data[anim_id] = animation_new(sprite, anim_id);
    return sprite->animation_data[anim_id];
}

/*
 * animation_new()
 * Creates a new empty animation_t instance
 */
animation_t *animation_new(const spriteinfo_t *sprite, int anim_id)
{
    animation_t *anim = mallocx(sizeof *anim);

    anim->sprite = sprite;
    anim->id = anim_id;
    anim->repeat = false;
    anim->fps = DEFAULT_FPS;
    anim->frame_count = 0;
    anim->data = NULL; /* this will be malloc'd later */
    anim->hot_spot = sprite->default_hot_spot;
    anim->action_spot = sprite->default_action_spot;
    anim->repeat_from = 0;
    anim->next = NULL;

    return anim;
}


/*
 * animation_delete()
 * Deletes an existing animation_t instance
 */
animation_t* animation_delete(animation_t *anim)
{
    if(anim->data != NULL)
        free(anim->data);

    free(anim);
    return NULL;
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
 * Compare two animtransition_t instances,
 * for sorting purposes
 */
int transition_cmp(const void *t1, const void *t2)
{
    const animtransition_t *a = *((const animtransition_t**)t1);
    const animtransition_t *b = *((const animtransition_t**)t2);
    bool a_any = (a->from_id == TRANSITION_ANY_ANIM || a->to_id == TRANSITION_ANY_ANIM);
    bool b_any = (b->from_id == TRANSITION_ANY_ANIM || b->to_id == TRANSITION_ANY_ANIM);

    /* transitions from/to "any" animation have less precedence than others */
    if(!a_any && b_any)
        return -1;
    if(a_any && !b_any)
        return 1;
    if(a_any && b_any)
        return 0;

    /* sort by from_id and then by to_id */
    if(a->from_id != b->from_id)
        return a->from_id - b->from_id;
    else
        return a->to_id - b->to_id;
}

/*
 * setup_transitions()
 * Setup the transition animations of a sprite
 */
void setup_transitions(spriteinfo_t *sprite)
{
    int i, n = darray_length(sprite->transition);

    /* sort transitions */
    merge_sort(sprite->transition, n, sizeof(*(sprite->transition)), transition_cmp);

    /* setup anim->next pointers */
    for(i = 0; i < n; i++) {
        animation_t *anim = sprite->animation_data[ sprite->transition[i]->anim_id ];
        int to_id = sprite->transition[i]->to_id;

        if(to_id == TRANSITION_ANY_ANIM) {
            /* This is a transition to "any". We'll change
               the ->next pointer at the appropriate time.
               For now, we just don't want it to be NULL,
               so we know this is a transition animation. */
            anim->next = anim;
        }
        else if(to_id >= 0 && to_id < sprite->animation_count) {
            /* regular transition */
            anim->next = sprite->animation_data[to_id];
        }
        else {
            /* this shouldn't happen */
            logfile_message("WARNING: found an incorrect transition to anim %d (valid range: [0, %d]) at \"%s\"", to_id, sprite->animation_count - 1, sprite->source_file);
            anim->next = NULL;
        }
    }
}


/*
 * validate_sprite()
 * Validates the sprite
 */
void validate_sprite(spriteinfo_t *spr)
{
    int i, j, n;

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

    if(spr->animation_count < 1 || spr->animation_data == NULL)
        fatal_error("Sprite error: sprites must contain at least one animation");

    n = (spr->rect_w / spr->frame_w) * (spr->rect_h / spr->frame_h);
    for(i=0; i<spr->animation_count; i++) {
        if(spr->animation_data[i] == NULL) continue;
        for(j=0; j<spr->animation_data[i]->frame_count; j++) {
            if(!(spr->animation_data[i]->data[j] >= 0 && spr->animation_data[i]->data[j] < n)) {
                logfile_message("Sprite error: invalid frame '%d' of animation %d. Animation frames must be in the range [%d, %d]", spr->animation_data[i]->data[j], i, 0, n-1);
                spr->animation_data[i]->data[j] = clip(spr->animation_data[i]->data[j], 0, n-1);
                logfile_message("Adjusting animation frame to %d", spr->animation_data[i]->data[j]);
            }
        }
    }
}


/*
 * validate_animation()
 * Validates the animation
 */
void validate_animation(animation_t *anim)
{
    if(anim->frame_count == 0)
        fatal_error("Animation error: invalid 'data' field. You must specify the frames of the animation");

    if(anim->repeat_from < 0 || anim->repeat_from >= anim->frame_count) {
        anim->repeat_from = clip(anim->repeat_from, 0, anim->frame_count-1);
        logfile_message("Animation error: the 'repeat_from' field has been adjusted to %d", anim->repeat_from);
    }
}


/*
 * load_sprite_images()
 * Loads the sprite by reading the spritesheet
 */
void load_sprite_images(spriteinfo_t *spr)
{
    int i, cur_x, cur_y;
    const image_t *sheet;

    spr->frame_count = (spr->rect_w / spr->frame_w) * (spr->rect_h / spr->frame_h);
    spr->frame_data = mallocx(spr->frame_count * sizeof(*(spr->frame_data)));

    /* reading the images... */
    if(NULL == (sheet = image_load(spr->source_file)))
        fatal_error("FATAL ERROR: couldn't load spritesheet \"%s\"", spr->source_file);

    cur_x = spr->rect_x;
    cur_y = spr->rect_y;
    for(i=0; i<spr->frame_count; i++) {
        spr->frame_data[i] = image_create_shared(sheet, cur_x, cur_y, spr->frame_w, spr->frame_h);
        cur_x += spr->frame_w;
        if(cur_x >= spr->rect_x+spr->rect_w) {
            cur_x = spr->rect_x;
            cur_y += spr->frame_h;
        }
    }

    image_unload(sheet);
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
        logfile_message("Loading sprite \"%s\" defined in \"%s\"", sprite_name, nanoparser_get_file(stmt));

        if(NULL == (sprite = hashtable_spriteinfo_t_find(sprites, sprite_name))) {
            /* register a new sprite */
            spriteinfo_t *new_sprite = spriteinfo_create(nanoparser_get_program(p2));
            hashtable_spriteinfo_t_add(sprites, sprite_name, new_sprite);
        }
        else {
            /* solve conflicting definitions for the same sprite */
            bool must_override = (str_incmp((const char*)vpath, OVERRIDE_PREFIX, OVERRIDE_PREFIX_LENGTH) == 0);

            if(must_override) {
                logfile_message("OVERRIDE: redefining sprite \"%s\" in \"%s\" near line %d", sprite_name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

                spriteinfo_t *new_sprite = spriteinfo_create(nanoparser_get_program(p2));
                if(hashtable_spriteinfo_t_replace(sprites, sprite_name, new_sprite))
                    return 0; /* the sprite has been successfully redefined */

                logfile_message("Can't override sprite \"%s\"", sprite_name); /* shouldn't happen */
                spriteinfo_destroy(new_sprite);
            }

            logfile_message("WARNING: can't redefine sprite \"%s\" in \"%s\" near line %d", sprite_name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        }
    }
    else
        fatal_error("Can't load sprite. Unknown identifier \"%s\" in \"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}


/*
 * traverse_sprite_attributes()
 * Sprite attributes traversal
 */
int traverse_sprite_attributes(const parsetree_statement_t *stmt, void *spriteinfo)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4;
    spriteinfo_t *s = (spriteinfo_t*)spriteinfo;
    int anim_id = 0;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    /* sanity check */
    if(s->animation_data != NULL && str_icmp(identifier, "animation") != 0 && str_icmp(identifier, "transition") != 0)
        fatal_error("Can't load sprite. Animations and transitions must be declared after the other parameters\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    /* read parameters */
    if(str_icmp(identifier, "source_file") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Must provide path to the source_file");
        if(s->source_file != NULL)
            free(s->source_file);
        s->source_file = str_dup(nanoparser_get_string(p1));
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
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        if(p1 && p2) {
            nanoparser_expect_string(p1, "Must provide animation number");
            nanoparser_expect_program(p2, "Must provide animation attributes");
            anim_id = atoi(nanoparser_get_string(p1));
        }
        else if(p1) {
            nanoparser_expect_program(p1, "Must provide animation attributes");
            anim_id = 0;
            p2 = p1;
        }
        else
            fatal_error("No attributes provided to 'animation' block\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        /* validate anim_id */
        if(!is_valid_anim_id(anim_id))
            fatal_error("Can't load sprites. Animation number must be in the range [0, %d]\nin \"%s\" near line %d", MAX_ANIMATIONS - 1, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        /* allocate animation */
        allocate_sprite_anim(s, anim_id);

        /* read & validate animation */
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), s->animation_data[anim_id], traverse_animation_attributes);
        validate_animation(s->animation_data[anim_id]);
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
                fatal_error("Invalid transition. Animation numbers must be in the range [0, %d]\nin \"%s\" near line %d", MAX_ANIMATIONS - 1, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        }
        else
            from_id = TRANSITION_ANY_ANIM;

        if(str_icmp("any", nanoparser_get_string(p3)) != 0) {
            to_id = atoi(nanoparser_get_string(p3));
            if(!is_valid_anim_id(to_id))
                fatal_error("Invalid transition. Animation numbers must be in the range [0, %d]\nin \"%s\" near line %d", MAX_ANIMATIONS - 1, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        }
        else
            to_id = TRANSITION_ANY_ANIM;

        if(from_id == TRANSITION_ANY_ANIM && to_id == TRANSITION_ANY_ANIM)
            fatal_error("Can't set a transition from \"any\" to \"any\"\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        else if(from_id == to_id)
            fatal_error("Can't set a transition from itself to itself\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        /* validate syntax */
        if(str_icmp(nanoparser_get_string(p2), "to") != 0)
            fatal_error("Syntax error: expected keyword \"to\"\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        /* allocate transition animation */
        anim_id = MAX_ANIMATIONS + darray_length(s->transition); /* allocate after MAX_ANIMATIONS */
        allocate_sprite_anim(s, anim_id);
        darray_push(s->transition, transition_new(anim_id, from_id, to_id)); /* register the transition */

        /* read & validate animation */
        nanoparser_traverse_program_ex(nanoparser_get_program(p4), s->animation_data[anim_id], traverse_animation_attributes);
        validate_animation(s->animation_data[anim_id]);

        /* transition animations shouldn't repeat... */
        if(s->animation_data[anim_id]->repeat)
            logfile_message("WARNING: a transition animation is set to repeat\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
    }
    else
        fatal_error("Can't load sprites. Unknown identifier '%s'\nin \"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
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
    int j;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "repeat") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat flag must be a boolean (true or false)");
        anim->repeat = atob(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "fps") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "fps must be a positive number");
        anim->fps = max(1e-5, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "repeat_from") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "repeat_from must be a non-negative number");
        anim->repeat_from = max(0, atoi(nanoparser_get_string(p1)));
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
            fatal_error("Can't load sprites. Animation 'data' field is missing\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        
        anim->data = reallocx(anim->data, anim->frame_count * sizeof(*(anim->data)));
        for(j=1; j<=anim->frame_count; j++) {
            pj = nanoparser_get_nth_parameter(param_list, j);
            nanoparser_expect_string(pj, "Animation 'data' field is a list of frame numbers");
            anim->data[j-1] = max(0, atoi(nanoparser_get_string(pj)));
        }
    }

    return 0;
}
