/*
 * Open Surge Engine
 * brick.c - brick module
 * Copyright (C) 2008-2012, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdio.h>
#include <math.h>
#include "brick.h"
#include "player.h"
#include "enemy.h"
#include "item.h"
#include "actor.h"
#include "../physics/collisionmask.h"
#include "../physics/obstacle.h"
#include "../scenes/level.h"
#include "../core/global.h"
#include "../core/video.h"
#include "../core/stringutil.h"
#include "../core/logfile.h"
#include "../core/assetfs.h"
#include "../core/util.h"
#include "../core/timer.h"
#include "../core/audio.h"
#include "../core/sprite.h"
#include "../core/soundfactory.h"
#include "../core/nanoparser/nanoparser.h"

/* private stuff */
#define BRKDATA_MAX             16384 /* up to BRKDATA_MAX bricks per theme are supported */
#define BRICK_MAXVALUES         2
#define BRICKBEHAVIOR_MAXARGS   5
#define BRB_FALL_TIME           1.0 /* time in seconds before a BRB_FALL gets destroyed */

/* types */
typedef enum brickstate_t brickstate_t;
typedef struct brickdata_t brickdata_t;
typedef struct maskdetails_t maskdetails_t;

/* brick state */
enum brickstate_t {
    BRS_IDLE,           /* the brick is alive, idle */
    BRS_DEAD,           /* must be removed from the level */
    BRS_ACTIVE,         /* generic action */
};

/* brick theme: meta data of bricks */
struct brickdata_t {
    spriteinfo_t *data; /* this is not stored in the main hash */
    image_t *image; /* pointer to the current brick image in the animation */
    char* maskfile; /* collision mask file (may be NULL) */
    collisionmask_t *mask; /* collision mask (may be NULL) */
    float zindex; /* 0.0 (background) <= z-index <= 1.0 (foreground) */
    bricktype_t type;
    brickbehavior_t behavior;
    float behavior_arg[BRICKBEHAVIOR_MAXARGS];
};

/* brick instances */
struct brick_t { /* a real, concrete brick */
    brickdata_t *brick_ref; /* brick metadata */
    int x, y; /* current position */
    int sx, sy; /* spawn point */
    brickstate_t state; /* brick state: BRS_* */
    float value[BRICK_MAXVALUES]; /* alterable values */
    float animation_frame; /* controlled by a timer */
    bricklayer_t layer; /* loop system: BRL_* */
    brickflip_t flip; /* flip bitwise flags */
    obstacle_t* obstacle; /* used by the physics system */
};

/* collision mask (parsed data) */
struct maskdetails_t {
    const char *source_file;
    int x, y, w, h;
};

/* private data */
static int brickdata_count = 0; /* size of brickdata[] */
static brickdata_t* brickdata[BRKDATA_MAX]; /* brick data */

/* private functions */
static brickdata_t *brickdata_get(int id);
static void brick_animate(brick_t *brk);
static brickdata_t* brickdata_new();
static brickdata_t* brickdata_delete(brickdata_t *obj);
static void validate_brickdata(const brickdata_t *obj);
static int traverse(const parsetree_statement_t *stmt);
static int traverse_brick_attributes(const parsetree_statement_t *stmt, void *brickdata);
static int traverse_collisionmask(const parsetree_statement_t *stmt, void *maskdetails);
static collisionmask_t *read_collisionmask(const parsetree_program_t *block);
static obstacle_t* create_obstacle(const brick_t* brick);
static obstacle_t* destroy_obstacle(obstacle_t* obstacle);
static inline int get_obstacle_flags(const brick_t* brick);
static inline int get_image_flags(const brick_t* brick);

/* public functions */

/* ========== brick theme interface ================ */

/*
 * brickset_load()
 * Loads a brickset from a file
 */
void brickset_load(const char *filename)
{
    int i;
    const char* fullpath;
    parsetree_program_t *tree;

    if(brickset_loaded()) {
        fatal_error("Can't load brickset \"%s\": another brickset is already loaded.", filename);
        return;
    }

    logfile_message("brickset_load(\"%s\")", filename);
    fullpath = assetfs_fullpath(filename);

    brickdata_count = 0;
    for(i=0; i<BRKDATA_MAX; i++) 
        brickdata[i] = NULL;

    tree = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(tree, traverse);
    tree = nanoparser_deconstruct_tree(tree);

    if(brickdata_count == 0)
        fatal_error("FATAL ERROR: no bricks have been defined in \"%s\"", filename);

    logfile_message("Creating collision masks...");
    for(i=0; i<brickdata_count; i++) {
        if(brickdata[i] != NULL && brickdata[i]->type != BRK_PASSABLE && brickdata[i]->mask == NULL) {
            const char* maskfile = brickdata[i]->maskfile ? brickdata[i]->maskfile : brickdata[i]->data->source_file;
            spriteinfo_t* sprite = brickdata[i]->data;
            image_t* maskimg = image_load(maskfile);
            brickdata[i]->mask = collisionmask_create(
                maskimg,
                sprite->rect_x,
                sprite->rect_y,
                sprite->frame_w,
                sprite->frame_h
            );
            image_unload(maskimg);
        }
    }

    logfile_message("The brickset has been loaded.");
}



/*
 * brickset_unload()
 * Unloads the current brickset
 */
void brickset_unload()
{
    int i;

    logfile_message("brickset_unload()");

    for(i = 0; i < brickdata_count; i++)
        brickdata[i] = brickdata_delete(brickdata[i]);
    brickdata_count = 0;

    logfile_message("The brickset has been unloaded.");
}


/*
 * brickset_size()
 * How many bricks are there (in this brickset)?
 */
int brickset_size()
{
    return brickdata_count;
}

/*
 * brickset_loaded()
 * Checks if a brickset is currently loaded
 */
int brickset_loaded()
{
    return brickset_size() > 0;
}




/* ========= brick interface ============== */

/*
 * brick_create()
 * Spawns a new brick
 */
brick_t* brick_create(int id, v2d_t position, bricklayer_t layer, brickflip_t flip_flags)
{
    brick_t *b = mallocx(sizeof *b);
    int i;

    b->brick_ref = brickdata_get(id);
    if(b->brick_ref == NULL)
        fatal_error("Can't create brick %d: brick not found.", id);

    b->x = b->sx = (int)position.x;
    b->y = b->sy = (int)position.y;
    b->animation_frame = 0;
    b->state = BRS_IDLE;
    b->layer = layer;
    b->flip = flip_flags;
    b->obstacle = create_obstacle(b);

    for(i=0; i<BRICK_MAXVALUES; i++)
        b->value[i] = 0.0f;

    return b;
}


/*
 * brick_destroy()
 * Destroys an existing brick instace
 */
brick_t* brick_destroy(brick_t *brk)
{
    destroy_obstacle(brk->obstacle);
    free(brk);
    return NULL;
}


/*
 * brick_update()
 * Updates a brick
 */
void brick_update(brick_t *brk, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, enemy_list_t *enemy_list)
{
    int i;

    if((brk == NULL) || (brk->brick_ref == NULL))
        return;

    switch(brk->brick_ref->behavior) {
        /* breakable bricks */
        case BRB_BREAKABLE: {
            int brkw = image_width(brk->brick_ref->image);
            int brkh = image_height(brk->brick_ref->image);
            float a[4], b[4] = { brk->x, brk->y, brk->x + brkw, brk->y + brkh };

            for(i=0; i<team_size; i++) {
                a[0] = team[i]->actor->position.x - team[i]->actor->hot_spot.x - 3;
                a[1] = team[i]->actor->position.y - team[i]->actor->hot_spot.y - 3;
                a[2] = a[0] + image_width(actor_image(team[i]->actor)) + 6;
                a[3] = a[1] + image_height(actor_image(team[i]->actor)) + 6;

                if((team[i]->attacking || player_is_rolling(team[i])) && bounding_box(a,b)) {
                    /* particles */
                    int bi, bj, bh, bw;
                    bw = max(brk->brick_ref->behavior_arg[0], 1);
                    bh = max(brk->brick_ref->behavior_arg[1], 1);
                    for(bi=0; bi<bw; bi++) {
                        for(bj=0; bj<bh; bj++) {
                            v2d_t brkpos = v2d_new(brk->x + (bi*brkw)/bw, brk->y + (bj*brkh)/bh);
                            v2d_t brkspeed = v2d_new(-team[i]->actor->speed.x*0.3, -100-random(50));
                            image_t *brkimg = image_create(brkw/bw, brkh/bh);

                            if(fabs(brkspeed.x) > EPSILON)
                                brkspeed.x += (brkspeed.x>0?1:-1) * random(50);

                            image_blit(brk->brick_ref->image, brkimg, (bi*brkw)/bw, (bj*brkh)/bh, 0, 0, brkw/bw, brkh/bh);
                            level_create_particle(brkimg, brkpos, brkspeed, FALSE);
                        }
                    }

                    /* destroy brick */
                    sound_play(SFX_BREAK);
                    brk->state = BRS_DEAD;
                }
            }
            break;
        }

        /* falling bricks */
        case BRB_FALL: {
            int i;
            int brkw = image_width(brk->brick_ref->image);
            int brkh = image_height(brk->brick_ref->image);
            float a[4], b[4] = { brk->x, brk->y, brk->x + brkw, brk->y + brkh/2 };
            int bb = FALSE;

            for(i=0; i<team_size; i++) {
                a[0] = team[i]->actor->position.x - team[i]->actor->hot_spot.x - 3;
                a[1] = team[i]->actor->position.y - team[i]->actor->hot_spot.y + image_height(actor_image(team[i]->actor))/2;
                a[2] = a[0] + image_width(actor_image(team[i]->actor)) + 6;
                a[3] = a[1] + image_height(actor_image(team[i]->actor))/2 + 6;
                bb = bb || bounding_box(a, b);
            }
            
            if(brk->state == BRS_IDLE && bb)
                brk->state = BRS_ACTIVE;

            if((brk->state == BRS_ACTIVE) && ((brk->value[1] += timer_get_delta()) >= BRB_FALL_TIME)) {
                int bi, bj, bw, bh;
                int right_oriented = ((int)brk->brick_ref->behavior_arg[2] != 0);
                image_t *brkimg = brk->brick_ref->image;

                /* particles */
                bw = max(brk->brick_ref->behavior_arg[0], 1);
                bh = max(brk->brick_ref->behavior_arg[1], 1);
                for(bi=0; bi<bw; bi++) {
                    for(bj=0; bj<bh; bj++) {
                        v2d_t piecepos = v2d_new(brk->x + (bi*image_width(brkimg))/bw, brk->y + (bj*image_height(brkimg))/bh);
                        v2d_t piecespeed = v2d_new(0, 20+bj*20+ (right_oriented?bi:bw-bi)*20);
                        image_t *piece = image_create(image_width(brkimg)/bw, image_height(brkimg)/bh);

                        image_blit(brkimg, piece, (bi*image_width(brkimg))/bw, (bj*image_height(brkimg))/bh, 0, 0, image_width(piece), image_height(piece));
                        level_create_particle(piece, piecepos, piecespeed, FALSE);
                    }
                }

                /* destroy brick */
                sound_play(SFX_BREAK);
                brk->state = BRS_DEAD;
            }
            break;
        }

        /* movable bricks */
        case BRB_CIRCULAR: {
            int brkw = image_width(brk->brick_ref->image);
            int brkh = image_height(brk->brick_ref->image);
            float rx, ry, sx, sy, ph, t;
            float a[4], b[4];
            int i;

            t = (brk->value[0] += timer_get_delta()); /* elapsed time */
            rx = brk->brick_ref->behavior_arg[0]; /* x-dist */
            ry = brk->brick_ref->behavior_arg[1]; /* y-dist */
            sx = brk->brick_ref->behavior_arg[2] * (2.0f * PI); /* x-speed */
            sy = brk->brick_ref->behavior_arg[3] * (2.0f * PI); /* x-speed */
            ph = brk->brick_ref->behavior_arg[4] * PI/180.0f; /* initial phase */

            brk->x = brk->sx + round(rx*cos(sx*t+ph));
            brk->y = brk->sy + round(ry*sin(sy*t+ph));

            if(brk->brick_ref->type == BRK_PASSABLE)
                break;

            if(brk->obstacle != NULL)
                obstacle_set_position(brk->obstacle, brk->x, brk->y);

            for(i=0; i<team_size; i++) {
                image_t* actor = actor_image(team[i]->actor);
                v2d_t box_size = v2d_new(image_width(actor), image_height(actor));
                v2d_t position = v2d_subtract(team[i]->actor->position, team[i]->actor->hot_spot);
                v2d_t offset = v2d_new(4.0f, 4.0f);

                a[0] = position.x + box_size.x / 2 - offset.x;
                a[1] = position.y + box_size.y - offset.y;
                a[2] = position.x + box_size.x / 2 + offset.x;
                a[3] = position.y + box_size.y + offset.y;

                b[0] = brk->x;
                b[1] = brk->y;
                b[2] = b[0] + brkw;
                b[3] = b[1] + brkh;

                if(!player_is_dying(team[i]) && !player_is_getting_hit(team[i]) && bounding_box(a,b)) {
                    team[i]->on_movable_platform = TRUE;
                    team[i]->actor->position = v2d_add(team[i]->actor->position, v2d_multiply(brick_movable_platform_offset(brk), timer_get_delta()));
                }
                else
                    team[i]->on_movable_platform = FALSE;
            }
            break;
        }

        /* static bricks */
        default: {
            break;
        }
    }
}


/*
 * brick_render()
 * Renders a brick
 */
void brick_render(brick_t *brk, v2d_t camera_position)
{
    brick_animate(brk);

    if(brk->layer == BRL_DEFAULT || !level_editmode())
        image_draw(brick_image(brk), video_get_backbuffer(), brk->x-((int)camera_position.x-VIDEO_SCREEN_W/2), brk->y-((int)camera_position.y-VIDEO_SCREEN_H/2), get_image_flags(brk));
    else
        image_draw_translit(brick_image(brk), video_get_backbuffer(), brk->x-((int)camera_position.x-VIDEO_SCREEN_W/2), brk->y-((int)camera_position.y-VIDEO_SCREEN_H/2), brick_util_layercolor(brk->layer), 0.5f, get_image_flags(brk));
}







/*
 * brick_render_path()
 * Renders the path of a brick (if it's a movable platform)
 */
void brick_render_path(const brick_t *brk, v2d_t camera_position)
{
    float oldx = 0.0f, oldy = 0.0f, x = 0.0f, y = 0.0f, t = 0.0f;
    float rx, ry, sx, sy, ph, off;
    int w = brick_size(brk).x;
    int h = brick_size(brk).y;
    v2d_t topleft = v2d_subtract(camera_position, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));

    switch(brk->brick_ref->behavior) {
        case BRB_CIRCULAR:
            rx = brk->brick_ref->behavior_arg[0];             /* x-dist */
            ry = brk->brick_ref->behavior_arg[1];             /* y-dist */
            sx = brk->brick_ref->behavior_arg[2] * (2*PI);    /* x-speed */
            sy = brk->brick_ref->behavior_arg[3] * (2*PI);    /* y-speed */
            ph = brk->brick_ref->behavior_arg[4] * PI/180.0;  /* initial phase */

            off = sx*t+ph;
            while(sx*t+ph < 2*PI + off) {
                x = brk->sx + round(rx*cos(sx*t+ph));
                y = brk->sy + round(ry*sin(sy*t+ph));

                if(t > 0.0f)
                    image_line(video_get_backbuffer(), (int)(oldx-topleft.x+w/2), (int)(oldy-topleft.y+h/2), (int)(x-topleft.x+w/2), (int)(y-topleft.y+h/2), image_rgb(255,0,0));

                oldx = x;
                oldy = y;
                t += 2*PI / 60.0f;
            }

            t = 0.0f;
            x = brk->sx + round(rx*cos(sx*t+ph));
            y = brk->sy + round(ry*sin(sy*t+ph));
            image_line(video_get_backbuffer(), (int)(oldx-topleft.x+w/2), (int)(oldy-topleft.y+h/2), (int)(x-topleft.x+w/2), (int)(y-topleft.y+h/2), image_rgb(255,0,0));

        default:
            break;
    }
}


/*
 * brick_movable_platform_offset()
 * movable platforms must move actors on top of them.
 * Returns a delta_space vector.
 */
v2d_t brick_movable_platform_offset(const brick_t *brk)
{
    float t, rx, ry, sx, sy, ph;

    if((brk == NULL) || (brk->brick_ref == NULL))
        return v2d_new(0,0);

    t = brk->value[0]; /* elapsed time */
    switch(brk->brick_ref->behavior) {
        case BRB_CIRCULAR:
            rx = brk->brick_ref->behavior_arg[0];             /* x-dist */
            ry = brk->brick_ref->behavior_arg[1];             /* y-dist */
            sx = brk->brick_ref->behavior_arg[2] * (2*PI);    /* x-speed */
            sy = brk->brick_ref->behavior_arg[3] * (2*PI);    /* y-speed */
            ph = brk->brick_ref->behavior_arg[4] * PI/180.0;  /* initial phase */

            /* take the derivative. e.g.,
               d[ sx + A*cos(PI*t) ]/dt = -A*PI*sin(PI*t) */
            return v2d_new( (-rx*sx)*sin(sx*t+ph), (ry*sy)*cos(sy*t+ph) );

        default:
            return v2d_new(0,0);
    }
}

/*
 * brick_id()
 * Returns the brick ID, i.e., its number in the brickset
 */
int brick_id(const brick_t* brk)
{
    for(int i = 0; i < brickdata_count; i++) {
        if(brk->brick_ref == brickdata[i])
            return i;
    }

    return -1; /* not found */
}

/*
 * brick_type()
 * Returns the type of the brick
 */
bricktype_t brick_type(const brick_t* brk)
{
    if(brk->brick_ref)
        return brk->brick_ref->type;
    else
        return BRK_OBSTACLE;
}


/*
 * brick_behavior()
 * Returns the behavior of the brick
 */
brickbehavior_t brick_behavior(const brick_t* brk)
{
    if(brk->brick_ref)
        return brk->brick_ref->behavior;
    else
        return BRB_DEFAULT;
}

/*
 * brick_layer()
 * Returns the layer of the brick (green, yellow, default)
 */
bricklayer_t brick_layer(const brick_t* brk)
{
    return brk->layer;
}

/*
 * brick_flip()
 * Returns the flip (mirroring) status of the brick
 */
brickflip_t brick_flip(const brick_t* brk)
{
    return brk->flip;
}

/*
 * brick_image()
 * Returns the image of an (animated?) brick
 */
const image_t *brick_image(const brick_t *brk)
{
    if(brk->brick_ref)
        return brk->brick_ref->image;
    else
        return NULL;
}

/*
 * brick_obstacle()
 * Returns the obstacle associated with this brick
 * WARNING: will be NULL if the brick is passable!
 */
const obstacle_t* brick_obstacle(const brick_t* brk)
{
    return brk->obstacle;
}

/*
 * brick_zindex()
 * Returns the zindex of the brick
 */
float brick_zindex(const brick_t* brk)
{
    if(brk->brick_ref)
        return brk->brick_ref->zindex;
    else
        return 0.5f;
}

/*
 * brick_position()
 * Returns the position of the
 * (topleft corner of the) brick
 */
v2d_t brick_position(const brick_t* brk)
{
    return v2d_new(brk->x, brk->y);
}

/*
 * brick_spawnpoint()
 * Returns the spawn point of the brick
 */
v2d_t brick_spawnpoint(const brick_t* brk)
{
    return v2d_new(brk->sx, brk->sy);
}

/*
 * brick_size()
 * Returns the size of the brick
 */
v2d_t brick_size(const brick_t* brk)
{
    if(brk->brick_ref && brk->brick_ref->image)
        return v2d_new(image_width(brk->brick_ref->image), image_height(brk->brick_ref->image));
    else
        return v2d_new(0, 0);
}

/*
 * brick_kill()
 * Kills a brick
 */
void brick_kill(brick_t* brk)
{
    brk->state = BRS_DEAD;
}

/*
 * brick_is_alive()
 * Checks if a brick is alive
 */
int brick_is_alive(const brick_t* brk)
{
    return brk->state != BRS_DEAD;
}

/*
 * brick_util_typename()
 * Returns the name of a given brick type
 */
const char* brick_util_typename(bricktype_t type)
{
    switch(type) {
        case BRK_PASSABLE:
            return "PASSABLE";

        case BRK_OBSTACLE:
            return "OBSTACLE";

        case BRK_CLOUD:
            return "CLOUD";

        default:
            return "Unknown";
    }
}



/*
 * brick_util_behaviorname()
 * Returns the name of a given brick behavior
 */
const char* brick_util_behaviorname(brickbehavior_t behavior)
{
    switch(behavior) {
        case BRB_DEFAULT:
            return "DEFAULT";

        case BRB_CIRCULAR:
            return "CIRCULAR";

        case BRB_BREAKABLE:
            return "BREAKABLE";

        case BRB_FALL:
            return "FALL";

        default:
            return "Unknown";
    }
}

/* utilities */
uint32 brick_util_layercolor(bricklayer_t layer)
{
    switch(layer) {
        case BRL_GREEN:     return image_rgb(0,255,0);
        case BRL_YELLOW:    return image_rgb(255,255,0);
        default:            return image_rgb(255,255,255);
    }
}

const char* brick_util_layername(bricklayer_t layer)
{
    switch(layer) {
        case BRL_GREEN:     return "green";
        case BRL_YELLOW:    return "yellow";
        default:            return "default";
    }
}

bricklayer_t brick_util_layercode(const char *name)
{
    if(str_icmp(name, "green") == 0)
        return BRL_GREEN;
    else if(str_icmp(name, "yellow") == 0)
        return BRL_YELLOW;
    else
        return BRL_DEFAULT;
}

const char* brick_util_flipstr(brickflip_t flip)
{
    switch(flip) {
        case BRF_HFLIP:     return "hflip";
        case BRF_VFLIP:     return "vflip";
        case BRF_VHFLIP:    return "vhflip";
        default:            return "noflip";
    }
}

brickflip_t brick_util_flipcode(const char* str)
{
    if(str_icmp(str, "hflip") == 0)
        return BRF_HFLIP;
    else if(str_icmp(str, "vflip") == 0)
        return BRF_VFLIP;
    else if(str_icmp(str, "vhflip") == 0)
        return BRF_VHFLIP;
    else
        return BRF_NOFLIP;
}

/*
 * brick_exists()
 * Does a brick with the given id exist?
 */
int brick_exists(int id)
{
    if(id >= 0 && id < brickdata_count)
        return brickdata[id] != NULL;
    else
        return FALSE;
}

/*
 * brick_image_preview()
 * Image of the brick with the given id (may be NULL)
 */
const image_t* brick_image_preview(int id)
{
    if(id >= 0 && id < brickdata_count) {
        if(brickdata[id] != NULL)
            return brickdata[id]->image;
    }
    return NULL;
}

/*
 * brick_image_flags()
 * Convert flags: brick flip to image flip
 */
int brick_image_flags(brickflip_t flip)
{
    brick_t b = { flip: flip }; /* hackish */
    return get_image_flags(&b);
}



/* === private stuff === */

/* Animates a brick */
void brick_animate(brick_t *brk)
{
    spriteinfo_t *sprite = brk->brick_ref->data;

    if(sprite != NULL) { /* if brk is not a fake brick */
        int loop = sprite->animation_data[0]->repeat;
        int f, c = sprite->animation_data[0]->frame_count;

        if(!loop)
            brk->animation_frame = min(c-1, brk->animation_frame + sprite->animation_data[0]->fps * timer_get_delta());
        else
            brk->animation_frame = (int)(sprite->animation_data[0]->fps * (timer_get_ticks() * 0.001f)) % c;

        f = clip((int)brk->animation_frame, 0, c-1);
        brk->brick_ref->image = sprite->frame_data[ sprite->animation_data[0]->data[f] ];
    }
}



/* new brick theme */
brickdata_t* brickdata_new()
{
    int i;
    brickdata_t *obj = mallocx(sizeof *obj);

    obj->data = NULL;
    obj->image = NULL;
    obj->mask = NULL;
    obj->maskfile = NULL;
    obj->type = BRK_PASSABLE;
    obj->behavior = BRB_DEFAULT;
    obj->zindex = 0.5f;

    for(i=0; i<BRICKBEHAVIOR_MAXARGS; i++)
        obj->behavior_arg[i] = 0.0f;

    return obj;
}

/* delete brick theme */
brickdata_t* brickdata_delete(brickdata_t *obj)
{
    if(obj != NULL) {
        if(obj->data != NULL)
            spriteinfo_destroy(obj->data);
        if(obj->mask != NULL)
            collisionmask_destroy(obj->mask);
        if(obj->maskfile != NULL)
            free(obj->maskfile);
        free(obj);
    }

    return NULL;
}

/* gets a brickdata object */
brickdata_t *brickdata_get(int id)
{
    id = clip(id, 0, brickdata_count-1);
    return brickdata[id];
}

/* validates a brick theme */
void validate_brickdata(const brickdata_t *obj)
{
    if(obj->data == NULL)
        fatal_error("Can't load bricks: all bricks must have a sprite!");
}

/* creates an obstacle (for the physics engine) corresponding to the brick */
obstacle_t* create_obstacle(const brick_t* brick)
{
    if(brick->brick_ref && brick->brick_ref->type != BRK_PASSABLE && brick->brick_ref->mask) {
        const collisionmask_t* mask = brick->brick_ref->mask;
        int flags = get_obstacle_flags(brick);
        return obstacle_create(mask, brick->x, brick->y, flags);
    }
    else
        return NULL;
}

/* destroys an obstacle associated to a brick */
obstacle_t* destroy_obstacle(obstacle_t* obstacle)
{
    return (obstacle != NULL) ? obstacle_destroy(obstacle) : NULL;
}

int get_obstacle_flags(const brick_t* brick)
{
    return
        ((brick_type(brick) == BRK_OBSTACLE) ? OF_SOLID : OF_CLOUD) |
        ((brick->flip & BRF_HFLIP) ? OF_HFLIP : 0) |
        ((brick->flip & BRF_VFLIP) ? OF_VFLIP : 0)
    ;
}

int get_image_flags(const brick_t* brick)
{
    return ((brick->flip & BRF_HFLIP) ? IF_HFLIP : 0) | ((brick->flip & BRF_VFLIP) ? IF_VFLIP : 0);
}

/* traverses a .brk file */
int traverse(const parsetree_statement_t *stmt)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    int brick_id;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "brick") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "Can't load bricks: brick number must be provided");
        nanoparser_expect_program(p2, "Can't load bricks: brick attributes must be provided");

        brick_id = atoi(nanoparser_get_string(p1));
        if(brick_id < 0 || brick_id >= BRKDATA_MAX)
            fatal_error("Can't load bricks: brick number must be in range 0..%d", BRKDATA_MAX-1);

        if(brickdata[brick_id] != NULL)
            brickdata[brick_id] = brickdata_delete(brickdata[brick_id]);

        brickdata_count = max(brickdata_count, brick_id+1);
        brickdata[brick_id] = brickdata_new();
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)brickdata[brick_id], traverse_brick_attributes);
        validate_brickdata(brickdata[brick_id]);
        brickdata[brick_id]->image = brickdata[brick_id]->data->frame_data[0];
    }
    else
        fatal_error("Can't load bricks: unknown identifier '%s'", identifier);

    return 0;
}

/* traverses a brick { ... } block */
int traverse_brick_attributes(const parsetree_statement_t *stmt, void *brickdata)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *pj;
    brickdata_t *dat = (brickdata_t*)brickdata;
    const char *type;
    int j;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "type") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read brick attributes: must specify brick type");
        type = nanoparser_get_string(p1);

        if(str_icmp(type, "OBSTACLE") == 0)
            dat->type = BRK_OBSTACLE;
        else if(str_icmp(type, "PASSABLE") == 0)
            dat->type = BRK_PASSABLE;
        else if(str_icmp(type, "CLOUD") == 0)
            dat->type = BRK_CLOUD;
        else
            fatal_error("Can't read brick attributes: unknown brick type '%s'", type);
    }
    else if(str_icmp(identifier, "behavior") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read brick attributes: must specify brick behavior");
        type = nanoparser_get_string(p1);

        if(str_icmp(type, "DEFAULT") == 0)
            dat->behavior = BRB_DEFAULT;
        else if(str_icmp(type, "CIRCULAR") == 0)
            dat->behavior = BRB_CIRCULAR;
        else if(str_icmp(type, "BREAKABLE") == 0)
            dat->behavior = BRB_BREAKABLE;
        else if(str_icmp(type, "FALL") == 0)
            dat->behavior = BRB_FALL;
        else
            fatal_error("Can't read brick attributes: unknown brick type '%s'", type);

        for(j=0; j<BRICKBEHAVIOR_MAXARGS; j++) {
            pj = nanoparser_get_nth_parameter(param_list, 2+j);
            dat->behavior_arg[j] = atof(nanoparser_get_string(pj));
        }
    }
    else if(str_icmp(identifier, "angle") == 0) {
        /* brick angle is obsolete, but this section has been kept for compatibility purposes */
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read brick attributes: must specify brick angle, a number between 0 and 359");
    }
    else if(str_icmp(identifier, "zindex") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read brick attributes: zindex must be a number between 0.0 and 1.0");
        dat->zindex = clip(atof(nanoparser_get_string(p1)), 0.0f, 1.0f);
    }
    else if(str_icmp(identifier, "mask") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read brick attrbitues: mask must be a filename");
        if(dat->maskfile != NULL)
            free(dat->maskfile);
        dat->maskfile = str_dup(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "collision_mask") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Can't read brick attributes: collision_mask expects a block");
        if(dat->mask != NULL)
            collisionmask_destroy(dat->mask);
        dat->mask = read_collisionmask(nanoparser_get_program(p1));
    }
    else if(str_icmp(identifier, "sprite") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Can't read brick attributes: a sprite block must be specified");
        if(dat->data != NULL)
            spriteinfo_destroy(dat->data);
        dat->data = spriteinfo_create(nanoparser_get_program(p1));
    }
    else
        fatal_error("Can't read brick attributes: unkown identifier '%s'", identifier);

    return 0;
}

/* this will read a collision_mask { ... } block */
int traverse_collisionmask(const parsetree_statement_t *stmt, void *maskdetails)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4;
    maskdetails_t *s = (maskdetails_t*)maskdetails;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "source_file") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "collision_mask: must provide path to source_file");
        s->source_file = nanoparser_get_string(p1);
    }
    else if(str_icmp(identifier, "source_rect") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);
        p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");
        nanoparser_expect_string(p2, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");
        nanoparser_expect_string(p3, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");
        nanoparser_expect_string(p4, "collision_mask: must provide four numbers to source_rect - xpos, ypos, width, height");

        s->x = max(0, atoi(nanoparser_get_string(p1)));
        s->y = max(0, atoi(nanoparser_get_string(p2)));
        s->w = max(1, atoi(nanoparser_get_string(p3)));
        s->h = max(1, atoi(nanoparser_get_string(p4)));
    }

    return 0;
}

/* read a collision mask from a file */
collisionmask_t *read_collisionmask(const parsetree_program_t *block)
{
    maskdetails_t s = { NULL, 0, 0, 0, 0 };
    collisionmask_t* mask = NULL;
    image_t* maskimg = NULL;

    nanoparser_traverse_program_ex(block, (void*)(&s), traverse_collisionmask);
    if(s.source_file == NULL)
        fatal_error("collision_mask: a source_file must be specified");

    maskimg = image_load(s.source_file);
    mask = collisionmask_create(maskimg, s.x, s.y, s.w, s.h);
    image_unload(maskimg);
    return mask;
}
