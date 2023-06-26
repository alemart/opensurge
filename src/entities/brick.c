/*
 * Open Surge Engine
 * brick.c - brick module
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

#include <stdio.h>
#include <math.h>
#include "brick.h"
#include "player.h"
#include "actor.h"
#include "sfx.h"
#include "legacy/item.h"
#include "legacy/enemy.h"
#include "../core/global.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/logfile.h"
#include "../core/asset.h"
#include "../core/timer.h"
#include "../core/audio.h"
#include "../core/sprite.h"
#include "../core/nanoparser.h"
#include "../util/numeric.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../physics/collisionmask.h"
#include "../physics/obstacle.h"
#include "../physics/physicsactor.h"
#include "../scenes/level.h"

/* constants */
#define BRKDATA_MAX             16384 /* up to BRKDATA_MAX bricks per theme are supported */
#define BRICK_MAXVALUES         1
#define BRICKBEHAVIOR_MAXARGS   5

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
    int id; /* brick ID */
    spriteinfo_t *data; /* this is not stored in the main hash */
    const image_t *image; /* pointer to a brick image in the animation */
    int image_width, image_height; /* cached image size */
    char* maskfile; /* collision mask file (may be NULL) */
    collisionmask_t *mask; /* collision mask (may be NULL) */
    image_t* maskimg; /* mask image for rendering (may be NULL) */
    float zindex; /* 0.0 (background) <= z-index <= 1.0 (foreground) */
    bricktype_t type;
    brickbehavior_t behavior;
    float behavior_arg[BRICKBEHAVIOR_MAXARGS];
};

/* brick instances */
struct brick_t { /* a real, concrete brick */
    const brickdata_t *brick_ref; /* brick metadata */
    int x, y; /* current position */
    int sx, sy; /* spawn point */
    brickstate_t state; /* brick state: BRS_* */
    float value[BRICK_MAXVALUES]; /* alterable values */
    float animation_frame; /* controlled by a timer */
    bricklayer_t layer; /* loop system: BRL_* */
    brickflip_t flip; /* flip bitwise flags */
    obstacle_t* obstacle; /* used by the physics system */
    const image_t *image; /* pointer to the current brick image in the animation */
};

/* collision mask (parsed data) */
struct maskdetails_t {
    const char *source_file;
    int x, y, w, h;
};

/* private stuff */
static void animate_brick(brick_t *brk);
static brickdata_t *brickdata_get(int id);
static brickdata_t* brickdata_new(int brick_id);
static brickdata_t* brickdata_delete(brickdata_t *obj);
static void validate_brickdata(const brickdata_t *obj);
static int traverse(const parsetree_statement_t *stmt);
static int traverse_brick_attributes(const parsetree_statement_t *stmt, void *brickdata);
static int traverse_collisionmask(const parsetree_statement_t *stmt, void *maskdetails);
static collisionmask_t *read_collisionmask(const parsetree_program_t *block);
static void create_collisionmasks();
static obstacle_t* create_obstacle(const brick_t* brick);
static obstacle_t* destroy_obstacle(obstacle_t* obstacle);
static inline int get_obstacle_flags(const brick_t* brick);
static inline obstaclelayer_t get_obstacle_layer(const brick_t* brick);
static inline int get_image_flags(const brick_t* brick);
static bool is_player_standing_on_platform(const player_t *player, const brick_t *brk);
static bool can_be_clipped_out(const brick_t* brick, v2d_t topleft);
static surgescript_object_t* create_particle(const brick_t* brick, int source_x, int source_y, int width, int height, v2d_t position, v2d_t velocity);
static int brickdata_count = 0; /* size of brickdata[] */
static brickdata_t* brickdata[BRKDATA_MAX]; /* brick data */

/* utilities */
#define ROUND(x)   (int)(((x)>=0.0f)?((x)+0.5f):((x)-0.5f))
static const float BRICK_FALL_TTL = 1.0f; /* time in seconds before a BRB_FALL gets destroyed */
static const float BRICK_FLOAT_AMPLITUDE = 8.0f; /* if a player touches a floating brick, how deep in pixels should it go? */
static const float BRICK_FLOAT_TIME = 0.25f; /* time in seconds before a floating brick reaches full amplitude */
static const float BRICK_FLOAT_TTL = 0.75f; /* time to live: seconds before a floating brick falls down (if it falls) */



/* public functions */

/* ========== brick theme interface ================ */

/*
 * brickset_load()
 * Loads a brickset from a file
 */
void brickset_load(const char* filename)
{
    int i;
    const char* fullpath;
    parsetree_program_t* tree;

    if(brickset_loaded()) {
        fatal_error("Can't load brickset \"%s\": another brickset is already loaded.", filename);
        return;
    }

    logfile_message("Loading brickset \"%s\"...", filename);
    fullpath = asset_path(filename);

    brickdata_count = 0;
    for(i=0; i<BRKDATA_MAX; i++) 
        brickdata[i] = NULL;

    tree = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(tree, traverse);
    tree = nanoparser_deconstruct_tree(tree);

    if(brickdata_count == 0)
        fatal_error("FATAL ERROR: no bricks have been defined in \"%s\"", filename);

    logfile_message("Creating collision masks...");
    create_collisionmasks();

    logfile_message("The brickset has been loaded.");
}



/*
 * brickset_unload()
 * Unloads the current brickset
 */
void brickset_unload()
{
    int i;

    logfile_message("Unloading the brickset...");

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
    b->image = b->brick_ref->image;
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
void brick_update(brick_t *brk, player_t** team, int team_size)
{
    int i, brk_width, brk_height;

    /* do we still need this? */
    if((brk == NULL) || (brk->brick_ref == NULL))
        return;

    /* animate the brick */
    animate_brick(brk);

    /* move the brick */
    brk_width = image_width(brk->brick_ref->image);
    brk_height = image_height(brk->brick_ref->image);
    switch(brk->brick_ref->behavior) {
        /* breakable bricks */
        case BRB_BREAKABLE: {
            for(i=0; i<team_size; i++) {
                if(
                    (player_is_aggressive(team[i]) || player_is_charging(team[i])) ||
                    (player_is_rolling(team[i]) /*&& fabs(team[i]->actor->speed.x) >= 240.0f*/)
                ) {
                    if(
                        player_senses_layer(team[i], brk->layer) &&
                        player_overlaps(team[i], brk->x - 16, brk->y - 4, brk_width + 32, brk_height)
                    ) {
                        int bw = clip(brk->brick_ref->behavior_arg[0], 1, brk_width);
                        int bh = clip(brk->brick_ref->behavior_arg[1], 1, brk_height);
                        float dx = team[i]->actor->position.x - brk->x;

                        /* create particles */
                        for(int bi=0; bi<bw; bi++) {
                            for(int bj=0; bj<bh; bj++) {
                                v2d_t brk_pos = v2d_new(brk->x + (bi*brk_width)/bw, brk->y + (bj*brk_height)/bh);
                                v2d_t brk_vel = v2d_new(
                                    dx >= 0 ? (90 + 60*(1+bi)/bw) : -(90 + 60*(bw-bi)/bw),
                                    -(120 + 60*(bh-bj)/bh)
                                );

                                create_particle(
                                    brk,
                                    (bi * brk_width) / bw,
                                    (bj * brk_height) / bh,
                                    brk_width / bw,
                                    brk_height / bh,
                                    brk_pos,
                                    brk_vel
                                );
                            }
                        }

                        /* destroy brick */
                        sound_play(SFX_BREAK);
                        brk->state = BRS_DEAD;
                    }
                }
            }
            
            break;
        }

        /* falling bricks */
        case BRB_FALL: {
            int collision = FALSE;

            for(i = 0; i < team_size && !collision; i++) {
                if(player_senses_layer(team[i], brk->layer)) {
                    if(is_player_standing_on_platform(team[i], brk))
                        collision = TRUE;
                }
            }
            
            if(brk->state == BRS_IDLE && collision)
                brk->state = BRS_ACTIVE;

            if((brk->state == BRS_ACTIVE) && ((brk->value[0] += timer_get_delta()) >= BRICK_FALL_TTL)) {
                int bw = clip(brk->brick_ref->behavior_arg[0], 1, brk_width);
                int bh = clip(brk->brick_ref->behavior_arg[1], 1, brk_height);
                int right_oriented = ((int)brk->brick_ref->behavior_arg[2] >= 0);

                /* create particles */
                for(int bi=0; bi<bw; bi++) {
                    for(int bj=0; bj<bh; bj++) {
                        v2d_t piece_pos = v2d_new(brk->x + (bi*brk_width)/bw, brk->y + (bj*brk_height)/bh);
                        v2d_t piece_speed = v2d_new(0, (1+bj)*15 + (right_oriented?bi:bw-bi)*15);

                        create_particle(
                            brk,
                            (bi * brk_width) / bw,
                            (bj * brk_height) / bh,
                            brk_width / bw,
                            brk_height / bh,
                            piece_pos,
                            piece_speed
                        );
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
            int dx, dy, old_x, old_y;

            /* get the parameters */
            float t = (brk->value[0] = level_time()); /* elapsed time */
            float rx = max(brk->brick_ref->behavior_arg[0], 0.0f); /* x-dist */
            float ry = max(brk->brick_ref->behavior_arg[1], 0.0f); /* y-dist */
            float sx = TWO_PI * brk->brick_ref->behavior_arg[2]; /* x-speed */
            float sy = TWO_PI * brk->brick_ref->behavior_arg[3]; /* y-speed */
            float ph = DEG2RAD * brk->brick_ref->behavior_arg[4]; /* initial phase */

            /* compute the position */
            old_x = brk->x; old_y = brk->y;
            brk->x = brk->sx + ROUND(rx * cosf(sx * t + ph));
            brk->y = brk->sy + ROUND(ry * sinf(sy * t + ph));
            dx = brk->x - old_x; dy = brk->y - old_y;

            /* passable bricks do not affect the player */
            if(brk->brick_ref->type == BRK_PASSABLE)
                break;

            /* move the player(s) */
            for(i = 0; i < team_size; i++) {
                if(!player_is_dying(team[i]) && !player_is_getting_hit(team[i]) && !player_is_midair(team[i])) {
                    if(player_senses_layer(team[i], brk->layer)) {
                        if(is_player_standing_on_platform(team[i], brk)) {
                            team[i]->on_movable_platform = TRUE;
                            team[i]->actor->position.x += dx;
                            team[i]->actor->position.y += dy;
                        }
                    }
                }
            }

            /* move the obstacle after moving the player(s) */
            if(brk->obstacle != NULL)
                obstacle_set_position(brk->obstacle, point_new(brk->x, brk->y));

            /* done */
            break;
        }

        /* pendular bricks */
        case BRB_PENDULAR: {
            int dx, dy, old_x, old_y;

            /* get the parameters */
            float t = (brk->value[0] = level_time()); /* elapsed time */
            float r = max(brk->brick_ref->behavior_arg[0], 0.0f); /* radius */
            float f = TWO_PI * brk->brick_ref->behavior_arg[1]; /* cycles per second */
            float ph = DEG2RAD * brk->brick_ref->behavior_arg[2]; /* initial phase */
            float off = DEG2RAD * (90.0f + brk->brick_ref->behavior_arg[3]); /* angular offset */
            float a = DEG2RAD * (fmodf(180.0f + brk->brick_ref->behavior_arg[4], 360.0f)); /* angular amplitude */

            /* compute the angle */
            float ang = (a / 2.0f) * cosf(f * t + ph) + off;

            /* compute the position */
            old_x = brk->x; old_y = brk->y;
            brk->x = brk->sx + ROUND(r * cosf(ang));
            brk->y = brk->sy + ROUND(r * sinf(ang));
            dx = brk->x - old_x; dy = brk->y - old_y;

            /* passable bricks do not affect the player */
            if(brk->brick_ref->type == BRK_PASSABLE)
                break;

            /* move the player(s) */
            for(i = 0; i < team_size; i++) {
                if(!player_is_dying(team[i]) && !player_is_getting_hit(team[i]) && !player_is_midair(team[i])) {
                    if(player_senses_layer(team[i], brk->layer)) {
                        if(is_player_standing_on_platform(team[i], brk)) {
                            team[i]->on_movable_platform = TRUE;
                            team[i]->actor->position.x += dx;
                            team[i]->actor->position.y += dy;
                        }
                    }
                }
            }

            /* move the obstacle after moving the player(s) */
            if(brk->obstacle != NULL)
                obstacle_set_position(brk->obstacle, point_new(brk->x, brk->y));

            /* done */
            break;
        }

        /* smashable bricks */
        case BRB_SMASHABLE: {
            int w, h, feet;
            v2d_t center;

            /* check collision & conditions */
            for(i=0; i<team_size; i++) {
                /* get the y-position of the feet of the player */
                physicsactor_bounding_box(team[i]->pa, &w, &h, &center);
                if(player_is_frozen(team[i]))
                    center = team[i]->actor->position;
                feet = center.y + h/2 - 2;

                /* check collision */
                if(
                    (
                        (player_is_jumping(team[i]) &&
                        physicsactor_get_ysp(team[i]->pa) > 0.0f) ||
                        ((player_is_charging(team[i]) || player_is_rolling(team[i])) &&
                        feet < brk->y)
                    ) &&
                    player_senses_layer(team[i], brk->layer) &&
                    player_overlaps(team[i], brk->x, brk->y - 10, brk_width, min(8, brk_height))
                ) {
                    /* create particles */
                    int bw = clip(brk->brick_ref->behavior_arg[0], 1, brk_width);
                    int bh = clip(brk->brick_ref->behavior_arg[1], 1, brk_height);

                    for(int bi = 0; bi < bw; bi++) {
                        for(int bj = 0; bj < bh; bj++) {
                            v2d_t brk_pos = v2d_new(brk->x + (bi*brk_width)/bw, brk->y + (bj*brk_height)/bh);
                            v2d_t brk_vel = v2d_new(
                                60.0f * ((bi + 0.5f) / bw - 0.5f),
                                -(120 + 60*(bh-bj)/bh)
                            );

                            create_particle(
                                brk,
                                (bi * brk_width) / bw,
                                (bj * brk_height) / bh,
                                brk_width / bw,
                                brk_height / bh,
                                brk_pos,
                                brk_vel
                            );
                        }
                    }

                    /* bounce player */
                    if(player_bounce(team[i], -1.0f, FALSE))
                        team[i]->actor->speed.y = -180.0f;

                    /* destroy brick */
                    sound_play(SFX_BREAK);
                    brk->state = BRS_DEAD;
                }
            }
            break;
        }

        /* floating bricks */
        case BRB_FLOAT: {
            const float ninety = 1.57079632679f; /* radians */
            player_t* player = NULL; /* player on top of the platform */
            float seconds_til_fall;
            int dy, old_y;

            /* get the parameters */
            int modifier = (int)(brk->brick_ref->behavior_arg[0]); /* seconds AFTER the player lands on the brick */
            if(modifier == 1)
                seconds_til_fall = BRICK_FLOAT_TTL;
            else /* 0 is the default value */
                seconds_til_fall = INFINITY;

            /* passable bricks do not affect the player */
            if(brk->brick_ref->type == BRK_PASSABLE)
                break;

            /* check for collisions */
            for(i = 0; i < team_size && !player; i++) {
                if(!player_is_dying(team[i]) && !player_is_getting_hit(team[i]) && !player_is_midair(team[i])) {
                    if(player_senses_layer(team[i], brk->layer)) {
                        if(is_player_standing_on_platform(team[i], brk))
                            player = team[i];
                    }
                }
            }
            
            /* change states */
            if(brk->state == BRS_ACTIVE && !player) {
                if(!isfinite(seconds_til_fall)) {
                    brk->state = BRS_IDLE;
                    brk->value[0] = min(BRICK_FLOAT_TIME, brk->value[0]);
                }
            }
            else if(brk->state == BRS_IDLE && player)
                brk->state = BRS_ACTIVE;

            /* time logic */
            if(brk->state == BRS_ACTIVE)
                brk->value[0] += timer_get_delta();
            else if(brk->state == BRS_IDLE) {
                brk->value[0] -= timer_get_delta();
                if(brk->value[0] < 0.0f)
                    brk->value[0] = 0.0f;
            }

            /* compute the position */
            old_y = brk->y;
            brk->y = brk->sy + BRICK_FLOAT_AMPLITUDE * sinf(
                min(BRICK_FLOAT_TIME, brk->value[0]) * (ninety / BRICK_FLOAT_TIME)
            );
            dy = brk->y - old_y;

            /* set the obstacle */
            if(brk->obstacle != NULL)
                obstacle_set_position(brk->obstacle, point_new(brk->x, brk->y));

            /* move the player */
            /* NOTE: handle multiple players at once? is it really needed? */
            if(player != NULL)
                player->actor->position.y += dy;

            /* should the brick fall? */
            if(brk->state == BRS_ACTIVE && brk->value[0] >= seconds_til_fall) {
                /* create particle */
                v2d_t brk_pos = v2d_new(brk->x, brk->y);
                v2d_t brk_vel = v2d_new(0, 120);

                create_particle(
                    brk,
                    0,
                    0,
                    image_width(brk->image),
                    image_height(brk->image),
                    brk_pos,
                    brk_vel
                );

                /* destroy brick */
                brk->state = BRS_DEAD;
            }
            
            /* done */
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
void brick_render(const brick_t *brk, v2d_t camera_position)
{
    if(brk->brick_ref->behavior != BRB_MARKER) {
        v2d_t topleft = v2d_subtract(camera_position, v2d_multiply(video_get_screen_size(), 0.5f));

        if(!can_be_clipped_out(brk, topleft))
            image_draw(brk->image, brk->x - (int)topleft.x, brk->y - (int)topleft.y, get_image_flags(brk));
    }
}

/*
 * brick_render_debug()
 * Renders a brick (editor)
 */
void brick_render_debug(const brick_t *brk, v2d_t camera_position)
{
    v2d_t topleft = v2d_subtract(camera_position, v2d_multiply(video_get_screen_size(), 0.5f));

    if(!can_be_clipped_out(brk, topleft)) {
        if(brk->layer != BRL_DEFAULT)
            image_draw_lit(brk->image, brk->x - (int)topleft.x, brk->y - (int)topleft.y, brick_util_layercolor(brk->layer), get_image_flags(brk));
        else
            image_draw(brk->image, brk->x - (int)topleft.x, brk->y - (int)topleft.y, get_image_flags(brk));
    }
}

/*
 * brick_render_mask()
 * Renders the mask of a brick (if any)
 */
void brick_render_mask(const brick_t *brk, v2d_t camera_position)
{
    if(brk->brick_ref->maskimg != NULL) {
        v2d_t topleft = v2d_subtract(camera_position, v2d_multiply(video_get_screen_size(), 0.5f));

        if(!can_be_clipped_out(brk, topleft))
            image_draw(brk->brick_ref->maskimg, brk->x - (int)topleft.x, brk->y - (int)topleft.y, get_image_flags(brk));
    }
}

/*
 * brick_render_path()
 * Renders the path of a moving brick (editor)
 */
void brick_render_path(const brick_t *brk, v2d_t camera_position)
{
    v2d_t topleft = v2d_subtract(camera_position, v2d_multiply(video_get_screen_size(), 0.5f));
    v2d_t size = brick_size(brk);
    color_t color = color_rgb(255, 0, 0);
    int w = size.x;
    int h = size.y;

    switch(brk->brick_ref->behavior) {
        case BRB_CIRCULAR: {
            float rx = fabs(brk->brick_ref->behavior_arg[0]); /* x-dist */
            float ry = fabs(brk->brick_ref->behavior_arg[1]); /* y-dist */
            if(rx < 1)
                image_line(brk->sx - topleft.x + w/2, brk->sy - topleft.y - ry + h/2, brk->sx - topleft.x + w/2, brk->sy - topleft.y + ry + h/2, color);
            else if(ry < 1)
                image_line(brk->sx - topleft.x - rx + w/2, brk->sy - topleft.y + h/2, brk->sx - topleft.x + rx + w/2, brk->sy - topleft.y + h/2, color);
            else
                image_ellipse(brk->sx - topleft.x + w/2, brk->sy - topleft.y + h/2, rx, ry, color);
            break;
        }

        case BRB_PENDULAR: {
            float r = fabs(brk->brick_ref->behavior_arg[0]); /* radius */
            image_ellipse(brk->sx - topleft.x + w/2, brk->sy - topleft.y + h/2, r, r, color);
            break;
        }

        default:
            break;
    }
}

/*
 * brick_id()
 * Returns the brick ID, i.e., its number in the brickset
 */
int brick_id(const brick_t* brk)
{
    if(brk->brick_ref == NULL)
        return -1; /* not found */

    return brk->brick_ref->id;
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
        return BRK_SOLID;
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
    return brk->image;
}

/*
 * brick_obstacle()
 * Returns the obstacle associated with this brick
 * WARNING: will be NULL if the brick is passable!
 */
const obstacle_t* brick_obstacle(const brick_t* brk)
{
    if(brk->state != BRS_DEAD)
        return brk->obstacle;
    else
        return NULL;
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
    return brk->brick_ref ? v2d_new(brk->brick_ref->image_width, brk->brick_ref->image_height) : v2d_new(0, 0);
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
 * brick_is_moving()
 * Checks if a brick has a movement path
 */
bool brick_has_movement_path(const brick_t* brk)
{
    if(brk->brick_ref != NULL)
        return brk->brick_ref->behavior == BRB_CIRCULAR || brk->brick_ref->behavior == BRB_PENDULAR;
    else
        return false;
}

/*
 * brick_has_mask()
 * Checks if a brick has a collision mask
 */
bool brick_has_mask(const brick_t* brk)
{
    if(brk->brick_ref != NULL)
        return brk->brick_ref->mask != NULL;
    else
        return false;
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

        case BRK_SOLID:
            return "SOLID";

        case BRK_CLOUD:
            return "CLOUD";
    }

    return "UNKNOWN";
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

        case BRB_SMASHABLE:
            return "SMASHABLE";

        case BRB_FLOAT:
            return "FLOAT";

        case BRB_PENDULAR:
            return "PENDULAR";

        case BRB_MARKER:
            return "MARKER";
    }

    return "UNKNOWN";
}

/* utilities */
color_t brick_util_layercolor(bricklayer_t layer)
{
    switch(layer) {
        case BRL_GREEN:     return color_rgb(0, 255, 0);
        case BRL_YELLOW:    return color_rgb(255, 255, 0);
        default:            return color_rgb(255, 255, 255);
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
 * brick_image_flags()
 * Convert flags: brick flip to image flip
 */
int brick_image_flags(brickflip_t flip)
{
    brick_t b = { .flip = flip }; /* hackish */
    return get_image_flags(&b);
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
 * brick_type_preview()
 * The type of the brick having the given id
 */
bricktype_t brick_type_preview(int id)
{
    if(id >= 0 && id < brickdata_count) {
        if(brickdata[id] != NULL)
            return brickdata[id]->type;
    }
    return BRK_PASSABLE;
}

/*
 * brick_behavior_preview()
 * The behavior of the brick having the given id
 */
brickbehavior_t brick_behavior_preview(int id)
{
    if(id >= 0 && id < brickdata_count) {
        if(brickdata[id] != NULL)
            return brickdata[id]->behavior;
    }
    return BRB_DEFAULT;
}

/*
 * brick_zindex_preview()
 * The zindex of the brick having the given id
 */
float brick_zindex_preview(int id)
{
    if(id >= 0 && id < brickdata_count) {
        if(brickdata[id] != NULL)
            return brickdata[id]->zindex;
    }
    return -1.0f;
}





/* === private stuff === */

/* Animates a brick */
void animate_brick(brick_t *brk)
{
    const spriteinfo_t *sprite = brk->brick_ref->data;

    /* fake brick? */
    if(sprite == NULL)
        return;

    /* does this brick have only one animation frame?
       skip everything (this is probably the case!) */
    if(sprite->animation_data[0]->frame_count == 1)
        return;

    /* animate */
    bool loop = sprite->animation_data[0]->repeat;
    int c = sprite->animation_data[0]->frame_count;

    if(!loop)
        brk->animation_frame = min(c-1, brk->animation_frame + sprite->animation_data[0]->fps * timer_get_delta());
    else
        brk->animation_frame = (int)(sprite->animation_data[0]->fps * timer_get_elapsed()) % c;

    int f = clip((int)brk->animation_frame, 0, c-1);
    brk->image = sprite->frame_data[ sprite->animation_data[0]->data[f] ];
}

/* Checks if the player is standing on top of a platform */
bool is_player_standing_on_platform(const player_t *player, const brick_t *brk)
{
    if(brk->obstacle == NULL)
        return false;

    return physicsactor_is_standing_on_platform(player->pa, brk->obstacle);
}

/* Checks if the brick can be clipped out (rendering) */
bool can_be_clipped_out(const brick_t* brick, v2d_t topleft)
{
    int x = brick->x - (int)topleft.x;
    int y = brick->y - (int)topleft.y;

    const image_t* img = brick->image;
    int w = image_width(img);
    int h = image_height(img);

    const image_t* backbuffer = video_get_backbuffer();
    int sw = image_width(backbuffer);
    int sh = image_height(backbuffer);

    return (x + w <= 0 || x >= sw || y + h <= 0 || y >= sh);
}

/* create a brick particle */
surgescript_object_t* create_particle(const brick_t* brick, int source_x, int source_y, int width, int height, v2d_t position, v2d_t velocity)
{
    /* create the particle */
    surgescript_object_t* particle = level_create_object("BrickParticle", position);

    /* create auxiliary variables */
    surgescript_var_t* id = surgescript_var_create();
    surgescript_var_t* x = surgescript_var_create();
    surgescript_var_t* y = surgescript_var_create();
    surgescript_var_t* w = surgescript_var_create();
    surgescript_var_t* h = surgescript_var_create();

    /* call particle.setBrick(brick_id, source_x, source_y, width, height) */
    const surgescript_var_t* args1[] = {
        surgescript_var_set_number(id, brick_id(brick)),
        surgescript_var_set_number(x, source_x),
        surgescript_var_set_number(y, source_y),
        surgescript_var_set_number(w, width),
        surgescript_var_set_number(h, height)
    };
    surgescript_object_call_function(particle, "setBrick", args1, 5, NULL);

    /* call particle.setVelocity(xvel, yvel) */
    const surgescript_var_t* args2[] = {
        surgescript_var_set_number(x, velocity.x),
        surgescript_var_set_number(y, velocity.y)
    };
    surgescript_object_call_function(particle, "setVelocity", args2, 2, NULL);

    /* call particle.set_zindex(zindex) */
    /*
    // the zindex of the brick is already set by setBrick()
    const surgescript_var_t* args3[] = {
        surgescript_var_set_number(x, 1.0)
    };
    surgescript_object_call_function(particle, "set_zindex", args3, 1, NULL);
    */

    /* release auxiliary variables */
    surgescript_var_destroy(h);
    surgescript_var_destroy(w);
    surgescript_var_destroy(y);
    surgescript_var_destroy(x);
    surgescript_var_destroy(id);

    /* return the particle */
    return particle;
}



/* new brick theme */
brickdata_t* brickdata_new(int brick_id)
{
    brickdata_t *obj = mallocx(sizeof *obj);

    obj->id = brick_id;
    obj->data = NULL;
    obj->image = NULL;
    obj->image_width = 0;
    obj->image_height = 0;
    obj->mask = NULL;
    obj->maskfile = NULL;
    obj->maskimg = NULL;
    obj->type = BRK_PASSABLE;
    obj->behavior = BRB_DEFAULT;
    obj->zindex = 0.5f;

    for(int i = 0; i < BRICKBEHAVIOR_MAXARGS; i++)
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
        if(obj->maskimg != NULL)
            image_destroy(obj->maskimg);
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
        obstaclelayer_t layer = get_obstacle_layer(brick);
        point_t position = point_new(brick->x, brick->y);

        return obstacle_create(mask, position, layer, flags);
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
        ((brick_type(brick) == BRK_SOLID) ? OF_SOLID : OF_CLOUD) |
        ((brick->flip & BRF_HFLIP) ? OF_HFLIP : 0) |
        ((brick->flip & BRF_VFLIP) ? OF_VFLIP : 0)
    ;
}

obstaclelayer_t get_obstacle_layer(const brick_t* brick)
{
    return (
        (brick->layer == BRL_GREEN) ? OL_GREEN :
        ((brick->layer == BRL_YELLOW) ? OL_YELLOW : OL_DEFAULT)
    );
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
        brickdata[brick_id] = brickdata_new(brick_id);
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)brickdata[brick_id], traverse_brick_attributes);
        validate_brickdata(brickdata[brick_id]);

        /* setup preview image */
        brickdata[brick_id]->image = brickdata[brick_id]->data->frame_data[
            brickdata[brick_id]->data->animation_data[0]->data[0]
        ];

        /* cache preview image size */
        brickdata[brick_id]->image_width = image_width(brickdata[brick_id]->image);
        brickdata[brick_id]->image_height = image_height(brickdata[brick_id]->image);
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

        if(str_icmp(type, "SOLID") == 0 || str_icmp(type, "OBSTACLE") == 0)
            dat->type = BRK_SOLID;
        else if(str_icmp(type, "CLOUD") == 0)
            dat->type = BRK_CLOUD;
        else if(str_icmp(type, "PASSABLE") == 0)
            dat->type = BRK_PASSABLE;
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
        else if(str_icmp(type, "SMASHABLE") == 0)
            dat->behavior = BRB_SMASHABLE;
        else if(str_icmp(type, "FLOAT") == 0)
            dat->behavior = BRB_FLOAT;
        else if(str_icmp(type, "PENDULAR") == 0)
            dat->behavior = BRB_PENDULAR;
        else if(str_icmp(type, "MARKER") == 0)
            dat->behavior = BRB_MARKER;
        else
            fatal_error("Can't read brick attributes: unknown brick type '%s'", type);

        for(j = 0; j < BRICKBEHAVIOR_MAXARGS; j++) {
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
        nanoparser_expect_string(p1, "Can't read brick attributes: zindex should be a number between 0.0 and 1.0");
        dat->zindex = atof(nanoparser_get_string(p1));
        dat->zindex = clip(dat->zindex, 0.0f, 1.0f);
    }
    else if(str_icmp(identifier, "mask") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Can't read brick attrbitues: mask must be a filename");
        if(dat->maskfile != NULL)
            free(dat->maskfile);
        dat->maskfile = str_dup(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "collision_mask") == 0) { /* deprecated */
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        logfile_message("WARNING: brick parameter collision_mask is deprecated. Use mask instead.");
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
        fatal_error("Can't read brick attributes: unknown identifier '%s'", identifier);

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

/* read a collision mask from a file (deprecated) */
collisionmask_t *read_collisionmask(const parsetree_program_t *block)
{
    maskdetails_t s = { NULL, 0, 0, 0, 0 };
    collisionmask_t* mask = NULL;
    image_t* maskimg = NULL;

    nanoparser_traverse_program_ex(block, (void*)(&s), traverse_collisionmask);
    if(s.source_file == NULL)
        fatal_error("collision_mask: a source_file must be specified");

    /* specify a custom collision mask for this particular brick (deprecated) */
    maskimg = image_load(s.source_file);
    image_lock(maskimg);
    mask = collisionmask_create(maskimg, s.x, s.y, s.w, s.h);
    image_unlock(maskimg);
    image_unload(maskimg);
    return mask;
}

/* creates the collision masks of all bricks */
void create_collisionmasks()
{
    int i;
    image_t* mask = NULL;
    const char* prev_maskfile = "";

    /* creates the collision masks */
    for(i = 0; i < brickdata_count; i++) {
        if(brickdata[i] != NULL && brickdata[i]->type != BRK_PASSABLE && brickdata[i]->mask == NULL) {
            const char* maskfile = brickdata[i]->maskfile ? brickdata[i]->maskfile : brickdata[i]->data->source_file;
            spriteinfo_t* sprite = brickdata[i]->data;

            if(mask == NULL || 0 != str_icmp(prev_maskfile, maskfile)) {
                if(mask != NULL) {
                    image_unlock(mask);
                    image_unload(mask);
                }
                mask = image_load(maskfile);
                image_lock(mask);
                prev_maskfile = maskfile;
            }

            brickdata[i]->mask = collisionmask_create(
                mask,
                sprite->rect_x,
                sprite->rect_y,
                sprite->frame_w,
                sprite->frame_h
            );
        }
    }

    if(mask != NULL) {
        image_unlock(mask);
        image_unload(mask);
    }

    /* creates the images of the masks */
    for(i = 0; i < brickdata_count; i++) {
        if(brickdata[i] != NULL && brickdata[i]->mask != NULL) {
            bricktype_t type = brickdata[i]->type;
            color_t color = (type == BRK_SOLID) ? color_rgb(255, 0, 0) : color_rgb(255, 255, 255);
            brickdata[i]->maskimg = collisionmask_to_image(brickdata[i]->mask, color);
        }
    }
}