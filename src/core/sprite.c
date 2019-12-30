/*
 * Open Surge Engine
 * sprite.c - code for the sprites/animations
 * Copyright (C) 2008-2010, 2012, 2018  Alexandre Martins <alemartf@gmail.com>
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
#include "util.h"
#include "stringutil.h"
#include "image.h"
#include "logfile.h"
#include "assetfs.h"
#include "hashtable.h"
#include "resourcemanager.h"
#include "nanoparser/nanoparser.h"

/* private stuff ;) */
#define SPRITE_MAX_ANIM         1024 /* sprites can have at most SPRITE_MAX_ANIM animations (numbered 0 .. SPRITE_MAX_ANIM-1) */
HASHTABLE_GENERATE_CODE(spriteinfo_t, spriteinfo_destroy);
static HASHTABLE(spriteinfo_t, sprites);
static const char* DEFAULT_SPRITE = "null";
static const int DEFAULT_ANIM = 0;

/* private functions */
static int dirfill(const char *vpath, void *param); /* file system callback */
static void validate_sprite(spriteinfo_t *spr); /* validates the sprite */
static void validate_animation(animation_t *anim); /* validates the animation */
static spriteinfo_t *spriteinfo_new(); /* creates a new spriteinfo_t instance */
static animation_t *animation_new(int anim_id, v2d_t hot_spot); /* creates a new animation_t instance */
static animation_t *animation_delete(animation_t *anim); /* deletes anim */
static void load_sprite_images(spriteinfo_t *spr); /* loads the sprite by reading the spritesheet */
static void fix_sprite_animations(spriteinfo_t *spr); /* fixes the animations of the given sprite */
static int traverse(const parsetree_statement_t *stmt);
static int traverse_sprite_attributes(const parsetree_statement_t *stmt, void *spriteinfo);
static int traverse_animation_attributes(const parsetree_statement_t *stmt, void *animation);


/* public methods */

/*
 * sprite_init()
 * Initializes the sprite module
 */
void sprite_init()
{
    parsetree_program_t *prog = NULL;

    logfile_message("Loading sprites...");
    sprites = hashtable_spriteinfo_t_create();

    /* reading the parse tree */
    assetfs_foreach_file("sprites", ".spr", dirfill, (void*)(&prog), true);
    if(prog == NULL)
        fatal_error("FATAL ERROR: no sprites have been found. Please reinstall the game.");

    /* reading the sprites */
    nanoparser_traverse_program(prog, traverse);

    /* we're done! */
    prog = nanoparser_deconstruct_tree(prog);
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
    spriteinfo_t *info;

    /* default sprite? */
    if(sprite_name == NULL)
        return sprite_get_animation(DEFAULT_SPRITE, DEFAULT_ANIM);

    /* find the corresponding spriteinfo_t* instance */
    info = hashtable_spriteinfo_t_find(sprites, sprite_name);
    if(info != NULL) {
        anim_id = clip(anim_id, 0, info->animation_count-1);
        return info->animation_data[anim_id];
    }
    else {
        fatal_error("Can't find sprite '%s' (animation %d)", sprite_name, anim_id);
        return NULL;
    }
}


/*
 * sprite_get_image()
 * Receives an animation and the desired frame number.
 * Returns an image.
 */
image_t *sprite_get_image(const animation_t *anim, int frame_id)
{
    frame_id = clip(frame_id, 0, anim->frame_count-1);
    return anim->frame_data[ anim->data[frame_id] ];
}


/*
 * sprite_animation_exists()
 * Checks if an animation exists (for a given sprite)
 */
int sprite_animation_exists(const char* sprite_name, int anim_id)
{
    spriteinfo_t *info = hashtable_spriteinfo_t_find(sprites, sprite_name);
    return info != NULL;
}





/*
 * spriteinfo_create()
 * Creates and stores on the memory a spriteinfo_t
 * object by parsing the passed tree
 */
spriteinfo_t* spriteinfo_create(const parsetree_program_t *tree)
{
    spriteinfo_t *sprite;

    sprite = spriteinfo_new();
    nanoparser_traverse_program_ex(tree, (void*)sprite, traverse_sprite_attributes);
    validate_sprite(sprite);
    load_sprite_images(sprite);
    fix_sprite_animations(sprite);

    return sprite;
}

/*
 * spriteinfo_destroy()
 * Destroys a spriteinfo_t object
 */
void spriteinfo_destroy(spriteinfo_t *info)
{
    int i;

    if(info->animation_data != NULL) {
        for(i=0; i<info->animation_count; i++)
            info->animation_data[i] = animation_delete(info->animation_data[i]);
        free(info->animation_data);
    }

    if(info->frame_data != NULL) {
        for(i=0; i<info->frame_count; i++)
            image_destroy(info->frame_data[i]);
        free(info->frame_data);
    }

    if(info->source_file != NULL) {
        #if 0
        /* this may crash the editor on reload.
           further investigation is needed. */
        resourcemanager_purge_image(info->source_file); /* force reload */
        #endif
        free(info->source_file);
    }

    free(info);
}
















/* private methods */

/*
 * dirfill()
 * File system callback
 */
int dirfill(const char *vpath, void *param)
{
    const char* fullpath = assetfs_fullpath(vpath);
    parsetree_program_t** p = (parsetree_program_t**)param;
    *p = nanoparser_append_program(*p, nanoparser_construct_tree(fullpath));
    return 0;
}

/*
 * spriteinfo_new()
 * Creates a new empty spriteinfo_t instance
 */
spriteinfo_t *spriteinfo_new()
{
    spriteinfo_t *info = mallocx(sizeof *info);

    info->source_file = NULL;
    info->rect_x = 0;
    info->rect_y = 0;
    info->rect_w = 0;
    info->rect_h = 0;
    info->frame_w = info->frame_h = 0;
    info->hot_spot = v2d_new(0,0);
    info->frame_count = 0;
    info->frame_data = NULL;
    info->animation_count = 0;
    info->animation_data = NULL;

    return info;
}

/*
 * animation_new()
 * Creates a new empty animation_t instance
 */
animation_t *animation_new(int anim_id, v2d_t hot_spot)
{
    animation_t *anim = mallocx(sizeof *anim);

    anim->id = anim_id;
    anim->repeat = FALSE;
    anim->fps = 8.0f;
    anim->frame_count = 0;
    anim->data = NULL; /* this will be malloc'd later */
    anim->hot_spot = hot_spot; /* default hot spot */
    anim->repeat_from = 0;
    anim->frame_data = NULL;

    return anim;
}


/*
 * animation_delete()
 * Deletes an existing animation_t instance
 */
animation_t* animation_delete(animation_t *anim)
{
    if(anim != NULL) {
        if(anim->data != NULL)
            free(anim->data);
        free(anim);
    }

    return NULL;
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
                logfile_message("Sprite error: invalid frame '%d' of animation %d. Animation frames must be in range %d..%d", spr->animation_data[i]->data[j], i, 0, n-1);
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
    image_t *sheet;

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
 * fix_sprite_animations()
 * Fix the animations of the given sprite
 */
void fix_sprite_animations(spriteinfo_t *spr)
{
    for(int i=0; i<spr->animation_count; i++) {
        if(spr->animation_data[i] != NULL)
            spr->animation_data[i]->frame_data = spr->frame_data;
    }
}


/*
 * traverse()
 * Sprite list traversal
 */
int traverse(const parsetree_statement_t *stmt)
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
            spriteinfo_t *new_sprite = spriteinfo_create(nanoparser_get_program(p2));
            if(new_sprite->animation_count > sprite->animation_count) {
                logfile_message("WARNING: redefining sprite \"%s\" in \"%s\" near line %d", sprite_name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
                hashtable_spriteinfo_t_replace(sprites, sprite_name, new_sprite);
            }
            else {
                logfile_message("WARNING: can't redefine sprite \"%s\" in \"%s\" near line %d", sprite_name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
                spriteinfo_destroy(new_sprite);
            }
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
    if(s->animation_data != NULL && str_icmp(identifier, "animation") != 0)
        fatal_error("Can't load sprite. Animations must be declared after the other parameters\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

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
        s->hot_spot.x = (float)atoi(nanoparser_get_string(p1));
        s->hot_spot.y = (float)atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "animation") == 0) {
        int i, old_count = 0;
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        if(p1 && p2) {
            nanoparser_expect_string(p1, "Must provide animation number");
            nanoparser_expect_program(p2, "Must provide animation attributes");
            anim_id = atoi(nanoparser_get_string(p1));
            if(anim_id < 0 || anim_id >= SPRITE_MAX_ANIM)
                fatal_error("Can't load sprites. Animation number must be in range 0..%d\nin \"%s\" near line %d", SPRITE_MAX_ANIM-1, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        }
        else if(p1) {
            nanoparser_expect_program(p1, "Must provide animation attributes");
            anim_id = 0;
            p2 = p1;
        }
        else
            fatal_error("No attributes provided to 'animation' block\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        if(anim_id < s->animation_count && NULL != s->animation_data[anim_id])
            s->animation_data[anim_id] = animation_delete(s->animation_data[anim_id]);

        old_count = s->animation_count;
        s->animation_count = max(s->animation_count, anim_id+1);
        if(s->animation_count > SPRITE_MAX_ANIM) /* sanity check */
            fatal_error("Can't exceed %d animations\nin \"%s\" near line %d", SPRITE_MAX_ANIM, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        s->animation_data = reallocx(s->animation_data, sizeof(animation_t*) * s->animation_count); /* watch this! It may generate garbage in the middle. */
        for(i = old_count; i < s->animation_count; i++)
            s->animation_data[i] = NULL;
        s->animation_data[anim_id] = animation_new(anim_id, s->hot_spot);
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), s->animation_data[anim_id], traverse_animation_attributes);
        validate_animation(s->animation_data[anim_id]);
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
    else if(str_icmp(identifier, "data") == 0) {
        anim->frame_count = nanoparser_get_number_of_parameters(param_list);
        if(anim->frame_count < 1)
            fatal_error("Can't load sprites. Animation 'data' field is missing\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        
        anim->data = mallocx(anim->frame_count * sizeof(*(anim->data)));
        for(j=1; j<=anim->frame_count; j++) {
            pj = nanoparser_get_nth_parameter(param_list, j);
            nanoparser_expect_string(pj, "Animation 'data' field is a list of frame numbers");
            anim->data[j-1] = max(0, atoi(nanoparser_get_string(pj)));
        }
    }

    return 0;
}
