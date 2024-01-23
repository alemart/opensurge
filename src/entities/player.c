/*
 * Open Surge Engine
 * player.c - player module
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
 *
 * Edits by Dalton Sterritt (copyright given to Alexandre):
 * player_enable_roll, player_disable_roll
 */

#include <surgescript.h>
#include <math.h>
#include "actor.h"
#include "player.h"
#include "brick.h"
#include "camera.h"
#include "character.h"
#include "mobilegamepad.h"
#include "sfx.h"
#include "legacy/item.h"
#include "legacy/enemy.h"
#include "../core/global.h"
#include "../core/audio.h"
#include "../core/video.h"
#include "../core/timer.h"
#include "../core/logfile.h"
#include "../core/input.h"
#include "../core/sprite.h"
#include "../core/fadefx.h"
#include "../scenes/level.h"
#include "../util/darray.h"
#include "../util/numeric.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../physics/physicsactor.h"
#include "../physics/obstaclemap.h"
#include "../physics/obstacle.h"
#include "../physics/collisionmask.h"
#include "../scripting/scripting.h"



/* public constants */
const int PLAYER_INITIAL_LIVES = 5;    /* initial lives */

/* private constants */
static const int PLAYER_MAX_STARS = 16;               /* how many invincibility stars */
static const float PLAYER_MAX_BLINK = 2.0f;           /* how long does the player blink if he/she gets hurt, in seconds */
static const float PLAYER_UNDERWATER_TIME = 30.0f;    /* how long can the player stay underwater before drowning, in seconds */
static const float PLAYER_TURBOCHARGE_TIME = 20.0f;   /* turbocharge time, in seconds */
static const float PLAYER_INVINCIBILITY_TIME = 20.0f; /* invincibility time, in seconds */
static const float PLAYER_DEAD_RESTART_TIME = 2.5f;   /* time to restart the level when the player is killed */

/* private data */
static int collectibles = 0;                /* shared collectibles */
static int lives = PLAYER_INITIAL_LIVES;    /* shared lives */
static int score = 0;                       /* shared score */

/* misc */
static void update_shield(player_t *player);
static void update_animation(player_t *player);
static void update_animation_speed(player_t *player);
static void update_underwater_status(player_t* player);
static void physics_adapter(player_t *player, const obstaclemap_t* obstaclemap);
static float smooth_angle(const physicsactor_t* pa, float current_angle);
static bool require_angle_to_be_zero(physicsactorstate_t state, movmode_t movmode);
static inline float delta_angle(float alpha, float beta);
static void hotspot_magic(player_t* player);
static void animate_invincibility_stars(player_t* player);
static void run_dying_logic(player_t* player);
static int fix_angle(int degrees, int threshold);
static int is_head_underwater(const player_t* player);
static void set_default_multipliers(physicsactor_t* pa, const character_t* character);
static void set_turbocharged_multipliers(physicsactor_t* pa, bool turbocharged);
static void set_underwater_multipliers(physicsactor_t* pa, bool underwater);
static void create_bouncing_collectibles(int number_of_collectibles, v2d_t position);
static void on_physics_event(physicsactor_t* pa, physicsactorevent_t event, void* context);


/*
 * player_create()
 * Creates a player
 */
player_t *player_create(int id, const char *character_name)
{
    int i;
    player_t *p = mallocx(sizeof *p);
    const character_t *c = charactersystem_get(character_name);

    logfile_message("player_create(%d, \"%s\")", id, character_name);

    /* initializing... */
    p->id = id;
    p->character = c;
    p->disable_movement = FALSE;
    p->disable_roll = FALSE;
    p->disable_animation_control = FALSE;
    p->invulnerable = FALSE;
    p->immortal = FALSE;
    p->secondary = FALSE;
    p->focusable = TRUE;
    p->aggressive = FALSE;
    p->inoffensive = FALSE;
    p->visible = TRUE;

    /* auxiliary variables */
    p->on_movable_platform = FALSE;
    p->thrown_while_rolling = FALSE;
    p->mirror = IF_NONE;
    p->got_glasses = FALSE;

    /* blink */
    p->blinking = FALSE;
    p->blink_timer = 0.0f;
    p->blink_visibility_timer = 0.0f;

    /* actor */
    p->actor = actor_create();
    p->actor->input = input_create_user(NULL);
    actor_change_animation(p->actor, sprite_get_animation(c->animation.sprite_name, c->animation.stopped));

    /* shield */
    p->shield = actor_create();
    p->shield_type = SH_NONE;

    /* invincibility */
    p->invincible = FALSE;
    p->invincibility_timer = 0.0f;
    p->star = mallocx(PLAYER_MAX_STARS * sizeof(actor_t*));
    for(i = 0; i < PLAYER_MAX_STARS; i++) {
        p->star[i] = actor_create();
        actor_change_animation(p->star[i], sprite_get_animation("Invincibility", 0));
    }

    /* turbo */
    p->turbocharged = FALSE;
    p->turbocharged_timer = 0;

    /* loop system */
    p->layer = BRL_DEFAULT;

    /* physics */
    p->pa = physicsactor_create(p->actor->position);
    set_default_multipliers(p->pa, c);
    physicsactor_subscribe(p->pa, on_physics_event, p);

    /* misc */
    p->underwater = FALSE;
    p->forcibly_underwater = FALSE;
    p->underwater_timer = 0.0f;
    p->breath_time = PLAYER_UNDERWATER_TIME;
    p->dead_timer = 0.0f;

    /* success! */
    collectibles = 0;
    logfile_message("Created player \"%s\"", c->name);
    return p;
}

/*
 * player_destroy()
 * Destroys a player
 */
player_t* player_destroy(player_t *player)
{
    int i;

    /* releasing actors */
    actor_destroy(player->shield);
    actor_destroy(player->actor);
    for(i = 0; i < PLAYER_MAX_STARS; i++)
        actor_destroy(player->star[i]);
    free(player->star);

    /* physics */
    physicsactor_destroy(player->pa);

    /* done */
    free(player);
    return NULL;
}



/*
 * player_update()
 * Updates the player
 */
void player_update(player_t *player, const obstaclemap_t* obstaclemap)
{
    actor_t *act = player->actor;
    physicsactor_t *pa = player->pa;
    float padding = 16.0f, eps = 1e-5;
    float dt = timer_get_delta();

    /* if the player movement is enabled... */
    if(!player->disable_movement) {

        /* run physics simulation */
        physics_adapter(player, obstaclemap);

        /* read new position */
        v2d_t position = player_position(player);

        /* enter / leave water */
        update_underwater_status(player);

        /* underwater logic */
        if(player_is_underwater(player)) {
            /* disable turbo */
            player_set_turbocharged(player, FALSE);

            /* disable some shields */
            if(player->shield_type == SH_FIRESHIELD || player->shield_type == SH_THUNDERSHIELD) {
                if(!player_is_invincible(player))
                    player_hit(player, 0.0f);
                else
                    player->shield_type = SH_NONE;
            }

            /* timer countdown */
            if(player->shield_type != SH_WATERSHIELD && !player_is_winning(player) && (
                player_is_forcibly_underwater(player) || /* forcibly underwater via scripting OR... */
                is_head_underwater(player)               /* the head of the player is underwater */
            ))
                player->underwater_timer += dt;
            else
                player->underwater_timer = 0.0f;

            /* drowning */
            if(player_seconds_remaining_to_drown(player) <= 0.0f)
                player_drown(player);
        }

        /* the player is blinking */
        if(player->blinking) {
            player->blink_timer += dt;

            if(player->blink_timer >= player->blink_visibility_timer + 0.06f) {
                player->blink_visibility_timer = player->blink_timer;
                act->visible = !act->visible;
            }

            if(player->blink_timer >= PLAYER_MAX_BLINK)
                player_set_blinking(player, FALSE);
        }

        /* invincibility stars */
        if(player->invincible) {
            /* update timer & finish */
            player->invincibility_timer += dt;
            if(player->invincibility_timer >= PLAYER_INVINCIBILITY_TIME)
                player_set_invincible(player, FALSE);
        }

        /* turbo speed */
        if(player->turbocharged) {
            /* update timer & finish */
            player->turbocharged_timer += dt;
            if(player->turbocharged_timer >= PLAYER_TURBOCHARGE_TIME)
                player_set_turbocharged(player, FALSE);
        }

        /* pitfalls */
        if(position.y >= level_height_at(position.x))
            player_kill(player);

        /* winning pose */
        if(level_has_been_cleared())
            physicsactor_enable_winning_pose(pa);

        /* rolling misc */
        if(!player_is_midair(player))
            player->thrown_while_rolling = FALSE;
        else if(player_ysp(player) < 0.0f && player_is_rolling(player))
            player->thrown_while_rolling = TRUE;

        /* misc */
        player->on_movable_platform = FALSE;

        /* the focused player can't get off the boundaries of the camera
           (when boundaries are enabled) */
        if(player_has_focus(player)) {
            v2d_t cam_topleft = camera_clip(v2d_new(0, 0));
            v2d_t cam_bottomright = camera_clip(level_size());

            /* lock horizontally */
            if(position.x > cam_bottomright.x - padding + eps) {
                player_set_speed(player, player_speed(player) * 0.5f);
                player_set_xpos(player, cam_bottomright.x - padding);
                position = player_position(player); /* update position */
            }
            else if(position.x < cam_topleft.x + padding - eps) {
                player_set_speed(player, player_speed(player) * 0.5f);
                player_set_xpos(player, cam_topleft.x + padding);
                position = player_position(player);
            }

            /* lock on top; won't prevent pits */
            if(!player_is_dying(player)) {
                if(position.y < cam_topleft.y + padding - eps) {
                    player_set_ysp(player, player_ysp(player) * 0.5f);
                    player_set_ypos(player, cam_topleft.y + padding);
                    position = player_position(player);
                }
            }
        }

        /* am I hurt? Gotta have the focus */
        if(player_is_getting_hit(player) || player_is_dying(player))
            player_focus(player);

    }
#if 0
    else {
        /* if the player is frozen...? */
    }
#endif

    /* can't leave the world */
    v2d_t position = player_position(player);

    if(position.x < padding - eps) {
        player_set_speed(player, player_speed(player) * 0.5f);
        player_set_xpos(player, padding);
        position = player_position(player); /* update position */
    }
    else if(position.x > level_size().x - padding + eps) {
        player_set_speed(player, player_speed(player) * 0.5f);
        player_set_xpos(player, level_size().x - padding);
        position = player_position(player);
    }

    if(position.y < padding - eps) {
        player_set_ysp(player, player_ysp(player) * 0.5f);
        player_set_ypos(player, padding);
        position = player_position(player);
    }

    /* invincibility stars */
    if(player->invincible)
        animate_invincibility_stars(player);

    /* shield */
    if(player->shield_type != SH_NONE)
        update_shield(player);

    /* restart the level if dead */
    if(player_is_dying(player))
        run_dying_logic(player);
}


/*
 * player_early_update()
 * Pre-scripting update routine
 */
void player_early_update(player_t *player)
{
    /* update animation */
    if(player->disable_animation_control) {
        player->disable_animation_control = FALSE; /* for set_player_animation (scripting) */
        return;
    }

    if(player->disable_movement)
        return;

    update_animation(player);
}


/*
 * player_render()
 * Rendering function
 */
void player_render(player_t *player, v2d_t camera_position)
{
    actor_t *act = player->actor;
    v2d_t hot_spot = act->hot_spot;

    /* invisible player? */
    if(!player->visible)
        return;

    /* mirroring */
    if(((int)physicsactor_is_facing_right(player->pa)) ^ ((player->mirror & IF_HFLIP) != 0))
        act->mirror &= ~IF_HFLIP;
    else
        act->mirror |= IF_HFLIP;

    if(!(player->mirror & IF_VFLIP))
        act->mirror &= ~IF_VFLIP;
    else
        act->mirror |= IF_VFLIP;

    /* hotspot "gambiarra" */
    hotspot_magic(player);

    /* render the player */
    actor_render(act, camera_position);

    /* render the shield */
    if(player->shield_type != SH_NONE)
        actor_render(player->shield, camera_position);

    /* invincibility stars */
    if(player->invincible) {
        for(int i = 0; i < PLAYER_MAX_STARS; i++)
            actor_render(player->star[i], camera_position);
    }

    /* restore hot spot */
    act->hot_spot = hot_spot;
}

/*
 * player_bounce()
 * Rebound. Returns TRUE if the player actually bounces.
 * tip: direction < 0 - the player is above the hazard, > 0 - the player is below
 */
int player_bounce(player_t *player, float direction, int is_heavy_object)
{
    (void)is_heavy_object; /* obsolete */

    return physicsactor_bounce(player->pa, direction);
}


/*
 * player_bounce_ex()
 * The same as player_bounce(), but you provide an actor as a hazard
 */
int player_bounce_ex(player_t *player, const actor_t *hazard, int is_heavy_object)
{
    int hh = image_height(actor_image(hazard));
    int ph = image_height(actor_image(player->actor));
    float hazard_centre = (hazard->position.y - hazard->hot_spot.y) + hh * 0.5f;
    float player_centre = (player_position(player).y - player->actor->hot_spot.y) + ph * 0.5f;
    return player_bounce(player, player_centre - hazard_centre, is_heavy_object);
}


/*
 * player_detach_from_ground()
 * Ensures the player is not touching the ground
 * (or ceiling/wall if rotated) on the next frame
 */
void player_detach_from_ground(player_t *player)
{
    physicsactor_detach_from_ground(player->pa);
}

/*
 * player_hit()
 * Hits a player. If it has no collectibles, then it must die.
 * tip: direction > 0 is right, < 0 is left, 0 is neutral
 */
void player_hit(player_t *player, float direction)
{
    /* do nothing */
    if(player->invincible || player->blinking || player_is_getting_hit(player) || player_is_dying(player))
        return;

    /* kill the player */
    if(player_get_collectibles() <= 0 && player->shield_type == SH_NONE && !player->invulnerable) {
        player_kill(player);
        return;
    }

    /* get hit */
    physicsactor_hit(player->pa, direction);
}

/*
 * player_hit_ex()
 * The same as player_hit(), but you give an actor as a hazard
 */
void player_hit_ex(player_t *player, const actor_t *hazard)
{
    int hw = image_width(actor_image(hazard));
    int pw = image_width(actor_image(player->actor));
    float hazard_centre = (hazard->position.x - hazard->hot_spot.x) + hw * 0.5f;
    float player_centre = (player_position(player).x - player->actor->hot_spot.x) + pw * 0.5f;
    player_hit(player, player_centre - hazard_centre);
}



/*
 * player_kill()
 * Kills a player
 */
void player_kill(player_t *player)
{
    physicsactor_kill(player->pa);
}


/*
 * player_roll()
 * Rolls
 */
void player_roll(player_t *player)
{
    physicsactor_roll(player->pa);
}

/*
 * player_enable_roll()
 * enables player rolling - OBSOLETE
 */
void player_enable_roll(player_t *player)
{
    physicsactor_t *pa = player->pa;
    if(player->disable_roll) {
        physicsactor_set_rollthreshold(pa, physicsactor_get_rollthreshold(pa) - 1000.0);
        player->disable_roll = FALSE;
    }
}

/*
 * player_disable_roll()
 * disables player rolling - OBSOLETE
 */
void player_disable_roll(player_t *player)
{
    if(!player->disable_roll) {
        physicsactor_t *pa = player->pa;
        physicsactor_set_rollthreshold(pa, physicsactor_get_rollthreshold(pa) + 1000.0);
        player->disable_roll = TRUE;
    }
}

/*
 * player_restore_state()
 * Restore the player to a vulnerable state
 */
void player_restore_state(player_t *player)
{
    physicsactor_restore_state(player->pa);
}

/*
 * player_springify()
 * Springify player
 */
void player_springify(player_t *player)
{
    physicsactor_springify(player->pa);
}

/*
 * player_drown()
 * Drown (underwater). This will be called automatically, internally.
 */
void player_drown(player_t *player)
{
    if(player_is_underwater(player))
        physicsactor_drown(player->pa);
}

/*
 * player_breathe()
 * Breathe (air bubble, underwater)
 */
void player_breathe(player_t *player)
{
    if(player_is_underwater(player)) {
        player_reset_underwater_timer(player);
        physicsactor_breathe(player->pa);
    }
}

/*
 * player_enter_water()
 * Enters the water
 */
void player_enter_water(player_t *player)
{
    if(player_is_dying(player))
        return;

    if(!player_is_underwater(player)) {
        player_set_speed(player, player_speed(player) * 0.5f);
        player_set_ysp(player, player_ysp(player) * 0.25f);

        player->underwater_timer = 0.0f;
        player->underwater = TRUE;

        set_underwater_multipliers(player->pa, true);
        sound_play(SFX_WATERIN);
    }
}

/*
 * player_leave_water()
 * Leaves the water
 */
void player_leave_water(player_t *player)
{
    if(player_is_underwater(player)) {
        if(!player_is_springing(player) && !player_is_dying(player)) {
            float double_ysp = player_ysp(player) * 2.0f;
            player_set_ysp(player, max(double_ysp, -960.0f));
        }

        player->underwater = FALSE;

        set_underwater_multipliers(player->pa, false);
        sound_play(SFX_WATEROUT);
    }
}

/*
 * player_is_underwater()
 * Is the player underwater?
 */
int player_is_underwater(const player_t *player)
{
    return player->underwater;
}

/*
 * player_is_forcibly_underwater()
 * Forcibly underwater? (underwater physics regardless of waterlevel)
 */
int player_is_forcibly_underwater(const player_t* player)
{
    return player->forcibly_underwater;
}

/*
 * player_set_forcibly_underwater()
 * Set forcibly underwater flag (underwater physics regardless of waterlevel)
 */
void player_set_forcibly_underwater(player_t* player, int forcibly_underwater)
{
    player->forcibly_underwater = forcibly_underwater;

    update_underwater_status(player);
}

/*
 * player_reset_underwater_timer()
 * Reset underwater timer
 */
void player_reset_underwater_timer(player_t *player)
{
    player->underwater_timer = 0.0f;
}

/*
 * player_seconds_remaining_to_drown()
 * How many seconds to drown?
 */
float player_seconds_remaining_to_drown(const player_t *player)
{
    if(player_is_underwater(player) && player->shield_type != SH_WATERSHIELD)
        return max(0.0f, player->breath_time - player->underwater_timer);
    else
        return INFINITY;
}

/*
 * player_set_breath_time()
 * Set the maximum time the player can remain underwater without breathing
 */
void player_set_breath_time(player_t* player, float seconds)
{
    player->breath_time = max(0.0f, seconds);
}

/*
 * player_breath_time()
 * The maximum time the player can remain underwater without breathing, in seconds
 */
float player_breath_time(const player_t* player)
{
    return player->breath_time;
}

/*
 * player_lock_horizontally_for()
 * Horizontal control lock timer
 */
void player_lock_horizontally_for(player_t *player, float seconds)
{
    physicsactor_lock_horizontally_for(player->pa, seconds);
}

/*
 * player_collision()
 * Collision detection using axis-aligned bounding boxes
 * Returns TRUE if the player is colliding with an actor
 */
int player_collision(const player_t *player, const actor_t *actor)
{
    int player_box_width, player_box_height;
    v2d_t player_box_center, actor_box_topleft;
    float player_box[4], actor_box[4];
    const image_t *img = actor_image(actor);

    physicsactor_bounding_box(player->pa, &player_box_width, &player_box_height, &player_box_center);
    if(player_is_frozen(player))
        player_box_center = player_position(player);

    player_box[0] = player_box_center.x - player_box_width / 2;
    player_box[1] = player_box_center.y - player_box_height / 2;
    player_box[2] = player_box_center.x + player_box_width / 2;
    player_box[3] = player_box_center.y + player_box_height / 2;

    actor_box_topleft = v2d_subtract(actor->position, actor->hot_spot);
    actor_box[0] = actor_box_topleft.x;
    actor_box[1] = actor_box_topleft.y;
    actor_box[2] = actor_box_topleft.x + image_width(img);
    actor_box[3] = actor_box_topleft.y + image_height(img);

    return bounding_box(player_box, actor_box);
}

/*
 * player_overlaps()
 * Returns TRUE if the player is overlapping the given rectangle,
 * given in world coordinates
 */
int player_overlaps(const player_t *player, int x, int y, int width, int height)
{
    float player_box[4], other_box[4];
    int player_box_width, player_box_height;
    v2d_t player_box_center;

    physicsactor_bounding_box(player->pa, &player_box_width, &player_box_height, &player_box_center);
    if(player_is_frozen(player))
        player_box_center = player_position(player);

    player_box[0] = player_box_center.x - player_box_width / 2;
    player_box[1] = player_box_center.y - player_box_height / 2;
    player_box[2] = player_box_center.x + player_box_width / 2;
    player_box[3] = player_box_center.y + player_box_height / 2;

    other_box[0] = x;
    other_box[1] = y;
    other_box[2] = x + width;
    other_box[3] = y + height;

    return bounding_box(player_box, other_box);
}

/*
 * player_bounding_box()
 * Returns a bounding box of the player
 */
rect_t player_bounding_box(const player_t* player)
{
    /* we only consider the dimensions of the collider;
       not those of the sprite */
    v2d_t center;
    int width, height;

    physicsactor_bounding_box(player->pa, &width, &height, &center);

    return rect_new(center.x - width / 2, center.y - height / 2, width, height);
}

/*
 * player_senses_layer()
 * Returns TRUE if the player is currently capable of
 * sensing the given layer
 */
int player_senses_layer(const player_t* player, bricklayer_t layer)
{
    return (layer == BRL_DEFAULT) || (player->layer == layer);
}

/*
 * player_transform_into()
 * Transforms the player into a different character
 * Returns TRUE on success
 */
int player_transform_into(player_t *player, surgescript_object_t *player_object, const char *character_name)
{
    /* if the player is to be transformed into itself, then we consider
       the transformation to be successful, but we do nothing */
    if(0 == str_icmp(player_name(player), character_name))
        return TRUE;

    /* if the target character doesn't exist, then the transformation
       is not successful */
    if(!charactersystem_exists(character_name))
        return FALSE;

    /* destroy the companion objects */
    surgescript_object_call_function(player_object, "__destroyCompanions", NULL, 0, NULL);

    /* let's change the character and update the parameters of the
       physics model */
    int turbocharged = player_is_turbocharged(player);
    int underwater = player_is_underwater(player);

    player->character = charactersystem_get(character_name);
    set_default_multipliers(player->pa, player->character);

    if(turbocharged)
        set_turbocharged_multipliers(player->pa, true);

    if(underwater)
        set_underwater_multipliers(player->pa, true);

    /* restore the controls (this is probably desirable) */
    player->disable_movement = FALSE;
    input_enable(player->actor->input);

    /* update animation */
    update_animation(player);

    /* reset the Animation object in SurgeScript */
    surgescript_object_call_function(player_object, "__resetAnimation", NULL, 0, NULL);

    /* respawn the companion objects */
    surgescript_object_call_function(player_object, "__spawnCompanions", NULL, 0, NULL);

    /* successful transformation! */
    return TRUE;
}

/*
 * player_has_focus()
 * Does the specified player have the focus?
 */
int player_has_focus(const player_t* player)
{
    return level_player() == player;
}

/*
 * player_focus()
 * Give focus to a player. Return true on success
 */
bool player_focus(player_t* player)
{
    player_t* p;

    /* already focused? (note: a single player is always focused) */
    if(player_has_focus(player))
        return true;

    /* not focusable? */
    if(!player_is_focusable(player))
        return false;

#if 0
    /* error if the target player is midair - is this desirable? */
    if(player_is_midair(player) || player->on_movable_platform)
        return false;
#endif

#if 0
    /* error if the target player is frozen - is this desirable? */
    if(player_is_frozen(player))
        return false;
#endif

    /* error if the current player is inside a locked area - is this desirable? */
    if(camera_is_locked() && camera_clip_test(player_position(level_player())))
        return false;

    /* is any player dying? */
    for(int i = 0; (p = level_get_player_by_id(i)) != NULL; i++) {
        if(player_is_dying(p) && !player_is_immortal(p))
            return false;
    }

    /* level cleared? */
    if(level_has_been_cleared())
        return false;

    /* change focus */
    level_change_player(player);
    return player_has_focus(player);
}

/*
 * player_shield_type()
 * Returns the current shield type of the player
 */
playershield_t player_shield_type(const player_t* player)
{
    return player->shield_type;
}

/*
 * player_grant_shield()
 * Grants the player a shield
 */
void player_grant_shield(player_t* player, playershield_t shield_type)
{
    player->shield_type = shield_type;
}
/*
 * player_layer()
 * The current layer of the player (loop system)
 */
bricklayer_t player_layer(const player_t* player)
{
    return player->layer;
}

/*
 * player_set_layer()
 * Sets the current layer of the player (useful for the loop system)
 */
void player_set_layer(player_t* player, bricklayer_t layer)
{
    player->layer = layer;
}

/*
 * player_mirror_flags()
 * Gets the mirror flags of the player (rendering)
 */
int player_mirror_flags(const player_t* player)
{
    return player->mirror;
}

/*
 * player_set_mirror_flags()
 * Sets the mirror flags of the player (rendering)
 */
void player_set_mirror_flags(player_t* player, int flags)
{
    player->mirror = flags;
}

/*
 * player_is_attacking()
 * Returns true if the player is attacking; false otherwise
 */
int player_is_attacking(const player_t *player)
{
    if(player_is_dying(player))
        return FALSE;

    if(player->aggressive || player->invincible)
        return TRUE;

    if(player->inoffensive)
        return FALSE;

    physicsactorstate_t state = physicsactor_get_state(player->pa);
    return state == PAS_JUMPING || state == PAS_ROLLING || state == PAS_CHARGING;
}

/*
 * player_is_rolling()
 * TRUE iff the player is rolling
 */
int player_is_rolling(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_ROLLING;
}

/*
 * player_is_charging()
 * TRUE iff the player is charging
 */
int player_is_charging(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_CHARGING;
}

/*
 * player_is_getting_hit()
 * TRUE iff the player is getting hit
 */
int player_is_getting_hit(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_GETTINGHIT;
}

/*
 * player_is_dying()
 * TRUE iff the player is dying
 */
int player_is_dying(const player_t *player)
{
    physicsactorstate_t state = physicsactor_get_state(player->pa);
    return state == PAS_DEAD || state == PAS_DROWNED;
}


/*
 * player_is_stopped()
 * TRUE iff the player is stopped
 */
int player_is_stopped(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_STOPPED;
}

/*
 * player_is_walking()
 * TRUE iff the player is walking
 */
int player_is_walking(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_WALKING;
}

/*
 * player_is_running()
 * TRUE iff the player is running
 */
int player_is_running(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_RUNNING;
}

/*
 * player_is_jumping()
 * TRUE iff the player is jumping
 */
int player_is_jumping(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_JUMPING;
}

/*
 * player_is_springing()
 * TRUE iff the player is springing
 */
int player_is_springing(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_SPRINGING;
}

/*
 * player_is_pushing()
 * TRUE iff the player is pushing
 */
int player_is_pushing(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_PUSHING;
}

/*
 * player_is_braking()
 * TRUE iff the player is braking
 */
int player_is_braking(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_BRAKING;
}

/*
 * player_is_at_ledge()
 * TRUE iff the player is at ledge
 */
int player_is_at_ledge(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_LEDGE;
}

/*
 * player_is_drowning()
 * TRUE iff the player is drowning
 */
int player_is_drowning(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_DROWNED;
}

/*
 * player_is_breathing()
 * TRUE iff the player is breathing an air bubble
 */
int player_is_breathing(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_BREATHING;
}

/*
 * player_is_ducking()
 * TRUE iff the player is ducking
 */
int player_is_ducking(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_DUCKING;
}

/*
 * player_is_looking_up()
 * TRUE iff the player is looking up
 */
int player_is_looking_up(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_LOOKINGUP;
}

/*
 * player_is_waiting()
 * TRUE iff the player is waiting
 */
int player_is_waiting(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_WAITING;
}

/*
 * player_is_winning()
 * TRUE iff the player is winning
 */
int player_is_winning(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_WINNING;
}

/*
 * player_is_midair()
 * TRUE iff the player is midair
 */
int player_is_midair(const player_t *player)
{
    return physicsactor_is_midair(player->pa);
}

/*
 * player_is_turbocharged()
 * TRUE if, and only if, the player is turbocharged
 * (i.e., runs faster than normal)
 */
int player_is_turbocharged(const player_t* player)
{
    return player->turbocharged;
}

/*
 * player_set_turbocharged()
 * Enable (or disable) turbo mode
 */
void player_set_turbocharged(player_t* player, int turbocharged)
{
    if(player_is_dying(player))
        return;

    if(turbocharged == player->turbocharged) {
        if(turbocharged)
            player->turbocharged_timer = 0.0f;
        return; /* nothing to do */
    }

    if(turbocharged) {
        player->turbocharged = TRUE;
        player->turbocharged_timer = 0.0f;
        set_turbocharged_multipliers(player->pa, true);
    }
    else {
        player->turbocharged = FALSE;
        set_turbocharged_multipliers(player->pa, false);
    }
}

/*
 * player_is_invincible()
 * TRUE iff the player is invincible
 */
int player_is_invincible(const player_t* player)
{
    return player->invincible;
}

/*
 * player_set_invincible()
 * Make the player invincible (or not invincible)
 */
void player_set_invincible(player_t* player, int invincible)
{
    if(player_is_dying(player))
        return;

    if(invincible)
        player->invincibility_timer = 0.0f;

    player->invincible = invincible;
}

/*
 * player_is_frozen()
 * Is the player frozen (i.e., without movement)?
 */
int player_is_frozen(const player_t* player)
{
    return player->disable_movement;
}

/*
 * player_set_frozen()
 * Enable/disable movement
 */
void player_set_frozen(player_t* player, int frozen)
{
    if(frozen && !player->disable_movement) {
        if(player_is_blinking(player))
            player_set_blinking(player, FALSE);
    }

    player->disable_movement = frozen;
}

/*
 * player_is_visible()
 * Is the player visible? (should it be rendered?)
 */
int player_is_visible(const player_t* player)
{
    return player->visible;
}

/*
 * player_set_visible()
 * Change the visibility of the player
 */
void player_set_visible(player_t* player, int visible)
{
    player->visible = visible;
}

/*
 * player_is_aggressive()
 * Is the player aggressive?
 */
int player_is_aggressive(const player_t* player)
{
    return player->aggressive;
}

/*
 * player_set_aggressive()
 * If set to true, the attacking flag will be true
 * regardless of the state of the player
 */
void player_set_aggressive(player_t* player, int aggressive)
{
    player->aggressive = aggressive;
}

/*
 * player_is_inoffensive()
 * Is the player inoffensive?
 */
int player_is_inoffensive(const player_t* player)
{
    return player->inoffensive;
}

/*
 * player_set_inoffensive()
 * If set to true, the attacking flag will be false regardless of the
 * state of the player, unless it is also aggressive or invincible
 */
void player_set_inoffensive(player_t* player, int inoffensive)
{
    player->inoffensive = inoffensive;
}

/*
 * player_is_invulnerable()
 * Is the player invulnerable? An invulnerable player won't take damage
 */
int player_is_invulnerable(const player_t* player)
{
    return player->invulnerable;
}

/*
 * player_set_invulnerable()
 * Set the invulnerability flag
 */
void player_set_invulnerable(player_t* player, int invulnerable)
{
    player->invulnerable = invulnerable;
}

/*
 * player_is_immortal()
 * Is the player immortal? If an immortal player appears to be killed, it
 * will appear to be resurrected on its spawn point without losing a life
 */
int player_is_immortal(const player_t* player)
{
    return player->immortal;
}

/*
 * player_set_immortal()
 * Set the immortality flag
 */
void player_set_immortal(player_t* player, int immortal)
{
    player->immortal = immortal;
}

/*
 * player_is_secondary()
 * Is the player secondary? A secondary player plays a secondary role and
 * interacts with items in different ways. It cannot smash item boxes, activate
 * goal signs, etc. These differences are specified in the scripting layer.
 */
int player_is_secondary(const player_t* player)
{
    return player->secondary;
}

/*
 * player_set_secondary()
 * Set the secondary flag
 */
void player_set_secondary(player_t* player, int secondary)
{
    player->secondary = secondary;
}

/*
 * player_is_focusable()
 * Is the player focusable? That is, can this player receive
 * focus? If only a single player exists in the level, then
 * that player will have focus regardless of the value of
 * this flag.
 */
int player_is_focusable(const player_t* player)
{
    return player->focusable;
}

/*
 * player_set_focusable()
 * Set the focusable flag
 */
void player_set_focusable(player_t* player, int focusable)
{
    player->focusable = focusable;
}

/*
 * player_is_blinking()
 * Is the player blinking? (happens after getting hit)
 */
int player_is_blinking(const player_t *player)
{
    return player->blinking;
}

/*
 * player_set_blinking()
 * Will make the player blink (or stop blinking)
 */
void player_set_blinking(player_t* player, int blink)
{
    if(blink) {
        player->blinking = TRUE;
        player->blink_timer = 0.0f;
        player->blink_visibility_timer = 0.0f;
    }
    else {
        player->blinking = FALSE;
        player->actor->visible = true;
    }
}

/*
 * player_speed()
 * Get the speed of the player (gsp or xsp), in pixels per second
 */
float player_speed(const player_t* player)
{
    if(player_is_midair(player) || player_is_getting_hit(player) || player_is_dying(player))
        return player_xsp(player);
    else
        return player_gsp(player);
}

/*
 * player_set_speed()
 * Set the speed of the player (gsp or xsp), in pixels per second
 */
void player_set_speed(player_t* player, float value)
{
    if(player_is_midair(player) || player_is_getting_hit(player) || player_is_dying(player))
        return player_set_xsp(player, value);
    else
        return player_set_gsp(player, value);
}

/*
 * player_gsp()
 * Get the ground speed of the player, in pixels per second
 */
float player_gsp(const player_t* player)
{
    return physicsactor_get_gsp(player->pa);
}

/*
 * player_set_gsp()
 * Set the ground speed of the player, in pixels per second
 */
void player_set_gsp(player_t* player, float value)
{
    physicsactor_set_gsp(player->pa, value);
}

/*
 * player_xsp()
 * Get the x-speed of the player, in pixels per second
 */
float player_xsp(const player_t* player)
{
    return physicsactor_get_xsp(player->pa);
}

/*
 * player_set_xsp()
 * Set the x-speed of the player, in pixels per second
 */
void player_set_xsp(player_t* player, float value)
{
    if(!player_is_midair(player) && !nearly_zero(value)) {
        movmode_t movmode = physicsactor_get_movmode(player->pa);
        if((movmode == MM_RIGHTWALL && value < 0.0f) || (movmode == MM_LEFTWALL && value > 0.0f))
            player_detach_from_ground(player);
    }

    physicsactor_set_xsp(player->pa, value);
}

/*
 * player_ysp()
 * Get the y-speed of the player, in pixels per second
 */
float player_ysp(const player_t* player)
{
    return physicsactor_get_ysp(player->pa);
}

/*
 * player_set_ysp()
 * Set the y-speed of the player, in pixels per second
 */
void player_set_ysp(player_t* player, float value)
{
    if(!player_is_midair(player) && !nearly_zero(value)) {
        movmode_t movmode = physicsactor_get_movmode(player->pa);
        if((movmode == MM_FLOOR && value < 0.0f) || (movmode == MM_CEILING && value > 0.0f))
            player_detach_from_ground(player);
    }

    physicsactor_set_ysp(player->pa, value);
}

/*
 * player_position()
 * The position of the player in world space
 */
v2d_t player_position(const player_t* player)
{
    return physicsactor_get_position(player->pa);
}

/*
 * player_set_position()
 * The position of the player in world space
 */
void player_set_position(player_t* player, v2d_t position)
{
    physicsactor_set_position(player->pa, position);
    player->actor->position = position;
}

/*
 * player_set_xpos()
 * A helper that sets the x position of the player in world space
 */
void player_set_xpos(player_t* player, float xpos)
{
    v2d_t position = player_position(player);
    position.x = xpos;
    player_set_position(player, position);
}

/*
 * player_set_ypos()
 * A helper that sets the y position of the player in world space
 */
void player_set_ypos(player_t* player, float ypos)
{
    v2d_t position = player_position(player);
    position.y = ypos;
    player_set_position(player, position);
}

/*
 * player_angle()
 * The display angle of the player (in radians)
 */
float player_angle(const player_t* player)
{
    return player->actor->angle;
}

/*
 * player_set_angle()
 * Set the display angle of the player (in radians)
 */
void player_set_angle(player_t* player, float radians)
{
    player->actor->angle = radians;
}

/*
 * player_scale()
 * The scale of the player in world space
 */
v2d_t player_scale(const player_t* player)
{
    return player->actor->scale;
}

/*
 * player_set_scale()
 * Set the scale of the player in world space. (1,1) is the default scale.
 */
void player_set_scale(player_t* player, v2d_t scale)
{
    player->actor->scale = scale;
}

/*
 * player_id()
 * A number that uniquely identifies the player in the Level
 */
int player_id(const player_t* player)
{
    return player->id;
}

/*
 * player_name()
 * The name of the (character associated with the) player
 */
const char* player_name(const player_t* player)
{
    return player->character->name;
}

/*
 * player_animation()
 * The current animation
 */
const animation_t* player_animation(const player_t* player)
{
    return player->actor->animation;
}

/*
 * player_override_animation()
 * Overrides the animation of the player
 */
void player_override_animation(player_t* player, const animation_t* animation)
{
    actor_change_animation(player->actor, animation);
    player->disable_animation_control = TRUE;
}

/*
 * player_sprite_name()
 * The sprite name associated with the player
 */
const char* player_sprite_name(const player_t* player)
{
    return player->character->animation.sprite_name;
}

/*
 * player_companion_name()
 * The name of the i-th companion object, or NULL if there is no such companion
 * index = 0, 1, 2...
 */
const char* player_companion_name(const player_t* player, int index)
{
    if(index >= 0 && index < darray_length(player->character->companion_name))
        return player->character->companion_name[index];
    else
        return NULL;
}





/*
 * player_get_collectibles()
 * Returns the amount of collectibles
 * the player has got so far
 */
int player_get_collectibles()
{
    return collectibles;
}

/*
 * player_set_collectibles()
 * Sets a new amount of collectibles
 */
void player_set_collectibles(int value)
{
    collectibles = max(0, value);
}

/*
 * player_get_lives()
 * How many lives does the player have?
 */
int player_get_lives()
{
    return lives;
}

/*
 * player_set_lives()
 * Sets the number of lives
 */
void player_set_lives(int value)
{
    lives = max(0, value);
}

/*
 * player_get_score()
 * Returns the score
 */
int player_get_score()
{
    return score;
}

/*
 * player_set_score()
 * Sets the score
 */
void player_set_score(int value)
{
    score = max(0, value);
}





/* private functions */

/* updates the current shield */
void update_shield(player_t *player)
{
    actor_t *sh = player->shield;

    v2d_t position = player_position(player);
    float angle = player_angle(player);
    v2d_t scale = player_scale(player);

    v2d_t offset = v2d_new(0,0); /* no rotation */
    sh->position = v2d_add(position, v2d_rotate(offset, -angle));
    sh->scale = scale;

    switch(player->shield_type) {
        case SH_SHIELD:
            actor_change_animation(sh, sprite_get_animation("Shield", 0));
            break;

        case SH_FIRESHIELD:
            actor_change_animation(sh, sprite_get_animation("Fire shield", 0));
            break;

        case SH_THUNDERSHIELD:
            actor_change_animation(sh, sprite_get_animation("Thunder shield", 0));
            break;

        case SH_WATERSHIELD:
            actor_change_animation(sh, sprite_get_animation("Water shield", 0));
            break;

        case SH_ACIDSHIELD:
            actor_change_animation(sh, sprite_get_animation("Acid shield", 0));
            break;

        case SH_WINDSHIELD:
            actor_change_animation(sh, sprite_get_animation("Wind shield", 0));
            break;

        case SH_NONE:
            break;
    }
}

/* updates the animation of the player */
void update_animation(player_t *player)
{
    #define CHANGE_ANIM(id) do { \
        const character_t* character = player->character; \
        const animation_t* anim = sprite_get_animation(character->animation.sprite_name, character->animation.id); \
        actor_change_animation(player->actor, anim); \
    } while(0)

    physicsactorstate_t state = physicsactor_get_state(player->pa);

    /* change animation */
    switch(state) {
        case PAS_STOPPED:    CHANGE_ANIM(stopped);    break;
        case PAS_WALKING:    CHANGE_ANIM(walking);    break;
        case PAS_RUNNING:    CHANGE_ANIM(running);    break;
        case PAS_JUMPING:    CHANGE_ANIM(jumping);    break;
        case PAS_SPRINGING:  CHANGE_ANIM(springing);  break;
        case PAS_ROLLING:    CHANGE_ANIM(rolling);    break;
        case PAS_CHARGING:   CHANGE_ANIM(charging);   break;
        case PAS_PUSHING:    CHANGE_ANIM(pushing);    break;
        case PAS_GETTINGHIT: CHANGE_ANIM(gettinghit); break;
        case PAS_DEAD:       CHANGE_ANIM(dead);       break;
        case PAS_BRAKING:    CHANGE_ANIM(braking);    break;
        case PAS_LEDGE:      CHANGE_ANIM(ledge);      break;
        case PAS_DROWNED:    CHANGE_ANIM(drowned);    break;
        case PAS_BREATHING:  CHANGE_ANIM(breathing);  break;
        case PAS_WAITING:    CHANGE_ANIM(waiting);    break;
        case PAS_DUCKING:    CHANGE_ANIM(ducking);    break;
        case PAS_LOOKINGUP:  CHANGE_ANIM(lookingup);  break;
        case PAS_WINNING:    CHANGE_ANIM(winning);    break;
    }

    /* handle variable animation speeds */
    update_animation_speed(player);

    #undef CHANGE_ANIM
}

/* variable animation speeds */
void update_animation_speed(player_t *player)
{
    physicsactorstate_t state = physicsactor_get_state(player->pa);
    const animation_t* anim = player->actor->animation;
    int frame_count = animation_frame_count(anim);
    float original_fps = animation_fps(anim);
    float desired_fps, fps_multiplier;

    /* piecewise linear functions on |gsp| are simple and look good */
    float gsp = fabsf(player_gsp(player));
    switch(state) {
        case PAS_WALKING:
        case PAS_RUNNING:
            desired_fps = clip(30.0f * (gsp / 480.0f), 7.5f, 30.0f);
            fps_multiplier = frame_count / 8.0f; /* the animation should have a single walk/run cycle */
            break;

        case PAS_JUMPING:
        case PAS_ROLLING:
            desired_fps = clip(60.0f * (gsp / 360.0f), 15.0f, 60.0f); /* also: |gsp| / 300 */
            fps_multiplier = frame_count / 8.0f;
            break;

        default:
            desired_fps = original_fps; /* non-variable */
            fps_multiplier = 1.0f;
            break;
    }

    if(!animation_is_transition(anim)) {
        float fps_ratio = desired_fps / original_fps;
        float speed_factor = fps_ratio * max(1.0f, fps_multiplier);
        actor_change_animation_speed_factor(player->actor, speed_factor);
    }
}

/* update underwater status */
void update_underwater_status(player_t* player)
{
    /* check if the player is forcibly underwater */
    if(player_is_forcibly_underwater(player)) {
        if(!player_is_underwater(player))
            player_enter_water(player);
    }

    /* if not, adopt the regular logic: check the waterlevel */
    else if(player_position(player).y >= level_waterlevel()) {
        if(!player_is_underwater(player))
            player_enter_water(player);
    }
    else {
        if(player_is_underwater(player))
            player_leave_water(player);
    }
}

/* the interface between player_t and physicsactor_t */
void physics_adapter(player_t *player, const obstaclemap_t* obstaclemap)
{
    physicsactor_t *pa = player->pa;
    actor_t *act = player->actor;

    /* capture input */
    physicsactor_capture_input(pa, act->input);

    /* set the layer of the physics actor */
    if(player->layer == BRL_GREEN)
        physicsactor_set_layer(pa, OL_GREEN);
    else if(player->layer == BRL_YELLOW)
        physicsactor_set_layer(pa, OL_YELLOW);
    else
        physicsactor_set_layer(pa, OL_DEFAULT);

    /* physics update */
    physicsactor_update(pa, obstaclemap);

    /* update position */
    act->position = physicsactor_get_position(pa);

    /* smooth the angle */
    act->angle = smooth_angle(pa, act->angle);
}

/* angle interpolation */
float smooth_angle(const physicsactor_t* pa, float current_angle)
{
    const float ninety = 90.0f * DEG2RAD;
    const float min_t = 0.15f, max_t = 1.0f;
    movmode_t movmode = physicsactor_get_movmode(pa);
    physicsactorstate_t state = physicsactor_get_state(pa);

    if(!require_angle_to_be_zero(state, movmode)) {
        float new_angle = fix_angle(physicsactor_get_angle(pa), 15) * DEG2RAD;
        float delta = delta_angle(new_angle, current_angle);

        if(delta < ninety) {
            float t = min_t + (delta / ninety) * (max_t - min_t);
            return lerp_angle(current_angle, new_angle, t);
        }

        return new_angle;
    }

    return 0.0f;
}

/* angle interpolation: helper function */
bool require_angle_to_be_zero(physicsactorstate_t state, movmode_t movmode)
{
    switch(state) {
        case PAS_WALKING:
        case PAS_RUNNING:
        case PAS_BRAKING:
        case PAS_SPRINGING: /* springing is used for various things (scripting) */
            return false;

        case PAS_DEAD:
        case PAS_DROWNED:
            return true;

        default:
            return movmode == MM_FLOOR;
    }
}

/* hotspot "gambiarra" */
void hotspot_magic(player_t* player)
{
    actor_t* act = player->actor;
    physicsactor_t* pa = player->pa;
    int angle = physicsactor_get_angle(pa);

    if(!player_is_rolling(player) && !player_is_charging(player)) {
        const float angthr = sinf(DEG2RAD * 11.25f);
        if(angle % 90 == 0 || player_is_at_ledge(player) || fabs(sinf(act->angle)) < angthr) {
            switch(physicsactor_get_movmode(pa)) {
                case MM_FLOOR: act->hot_spot.y += 1; break;
                case MM_LEFTWALL: act->hot_spot.y += 2; break;
                case MM_RIGHTWALL: act->hot_spot.y += 1; break;
                case MM_CEILING: act->hot_spot.y += 2; break;
            }
        }
        else if(!physicsactor_is_midair(pa)) {
            physicsactorstate_t state = physicsactor_get_state(pa);
            if(!(
                state == PAS_STOPPED || state == PAS_WAITING ||
                state == PAS_DUCKING || state == PAS_LOOKINGUP ||
                state == PAS_PUSHING || state == PAS_WINNING
            )) {
                act->hot_spot.y -= 1;
            }
        }
    }
    else if(player_is_rolling(player)) {
        int roll_delta = physicsactor_roll_delta(pa);

        /* adjust hot spot */
        switch(physicsactor_get_movmode(pa)) {
            case MM_FLOOR:
                act->hot_spot.y += roll_delta + 1;
                if(player->thrown_while_rolling) {
                    if(physicsactor_is_facing_right(pa))
                        act->hot_spot.x -= 5 - roll_delta;
                    else
                        act->hot_spot.x += 4 - roll_delta;
                }
                break;

            case MM_LEFTWALL:
                act->hot_spot.y += roll_delta;
                act->hot_spot.x += 4 - roll_delta;
                if(angle > 270) {
                    act->hot_spot.x += 6 * sinf(act->angle);
                    act->hot_spot.y += 4 * sinf(act->angle);
                }
                break;

            case MM_RIGHTWALL:
                act->hot_spot.y += roll_delta + 1;
                act->hot_spot.x -= 5 - roll_delta;
                if(angle < 90) {
                    act->hot_spot.x += 6 * sinf(act->angle);
                    act->hot_spot.y -= 4 * sinf(act->angle);
                }
                break;

            case MM_CEILING:
                act->hot_spot.x -= (6 - roll_delta) * sinf(act->angle);
                act->hot_spot.y += 4 - roll_delta - 6 * cosf(act->angle);
                break;
        }

        /* disable angle */
        act->angle = 0.0f;
    }
    else {
        act->hot_spot.y += 1;
        act->angle = 0.0f;
    }
}

/* sets the position of the invincibility stars */
void animate_invincibility_stars(player_t* player)
{
    const float magic = PLAYER_MAX_STARS * PLAYER_MAX_STARS * 1.5f;
    const float angpi = TWO_PI / PLAYER_MAX_STARS;
    float x, angle, distance, max_distance;
    int i, width, height;
    v2d_t center;

    /* get coordinates & dimensions */
    physicsactor_bounding_box(player->pa, &width, &height, &center);
    max_distance = min(width, height);
    if(player_is_frozen(player))
        center = player_position(player);

    /* animate */
    for(i = 0; i < PLAYER_MAX_STARS; i++) {
        x = 1.0f - fmodf((float)timer_get_elapsed() * 1000.0f + (magic * i), 1000.0f) * 0.001f;
        distance = max_distance * (1.0f - x * x * x);
        angle = -i * angpi;
        player->star[i]->alpha = x * x;
        player->star[i]->position = v2d_add(center, v2d_rotate(v2d_new(distance, 0.0f), angle));
        actor_change_animation_speed_factor(player->star[i], 1.0f + i * 0.25f);
    }
}

/* restart the level, display the game over scene or resurrect the player */
void run_dying_logic(player_t* player)
{
    const float FADEOUT_TIME = 1.0f, MUSIC_FADEOUT_TIME = 0.5f;
    bool can_resurrect = player->immortal;
    float dt = timer_get_delta();

    if(!can_resurrect) {
        /* fade out the music */
        float new_volume = 1.0f - min(player->dead_timer, MUSIC_FADEOUT_TIME) / MUSIC_FADEOUT_TIME;
        float current_volume = music_get_volume();
        if(new_volume < current_volume)
            music_set_volume(new_volume);

        /* hide the mobile gamepad */
        mobilegamepad_fadeout();
    }

    /* decide what to do next */
    if(player->dead_timer >= PLAYER_DEAD_RESTART_TIME) {

        if(can_resurrect) {
            /* resurrect */
            player->dead_timer = 0.0f;
            player_reset_underwater_timer(player);
            physicsactor_resurrect(player->pa);
            return;
        }
        else if(player_get_lives() <= 1) {
            /* game over */
            level_quit_with_gameover();
        }
        /*else if(fadefx_is_over()) { // skips a frame */
        else if(player->dead_timer + dt >= PLAYER_DEAD_RESTART_TIME + FADEOUT_TIME) {
            /* restart the level */
            player_set_lives(player_get_lives() - 1);
            level_restart();
        }
        else {
            /* fade out */
            color_t black = color_rgb(0, 0, 0);
            fadefx_out(black, FADEOUT_TIME);
        }

    }

    /* update the dead timer */
    player->dead_timer += dt;
}

/* given two angles in [0, 2pi], return their difference */
float delta_angle(float alpha, float beta)
{
    float diff = fmod(fabs(alpha - beta), TWO_PI);
    return min(TWO_PI - diff, diff);
}

/* truncates the angle within a given threshold, assuming 0 <= degrees < 360 */
int fix_angle(int degrees, int threshold)
{
    int t = threshold / 2;
    if(degrees <= t || degrees >= 360 - t)
        return 0;
    else if(degrees <= 90 + t && degrees >= 90 - t)
        return 90;
    else if(degrees <= 180 + t && degrees >= 180 - t)
        return 180;
    else if(degrees <= 270 + t && degrees >= 270 - t)
        return 270;
    else
        return degrees;
}

/* is the head of the player underwater? */
int is_head_underwater(const player_t* player)
{
    const float head_factor = 0.8f;
    float top, bottom;
    int player_box_width, player_box_height;
    v2d_t player_box_center;

    physicsactor_bounding_box(player->pa, &player_box_width, &player_box_height, &player_box_center);
    if(player_is_frozen(player))
        player_box_center = player_position(player);

    top = player_box_center.y - player_box_height / 2.0f;
    bottom = player_box_center.y + player_box_height / 2.0f;
    return (int)lerp(bottom, top, head_factor) >= level_waterlevel();
}

/* turbocharged physics */
void set_turbocharged_multipliers(physicsactor_t* pa, bool turbocharged)
{
    double multiplier = turbocharged ? 2.0 : 0.5;

    physicsactor_set_acc(pa, physicsactor_get_acc(pa) * multiplier);
    physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * multiplier);
    physicsactor_set_air(pa, physicsactor_get_air(pa) * multiplier);

#if 1
    /* do we *really* want to increase the friction? This causes undesirable
       behavior. Example: when running very fast, beyond the default capspeed
       and on flat ground, we'll move *slower* when turbocharged because we'll
       lose speed due to the increased friction (frc and rollfrc) */
    physicsactor_set_frc(pa, physicsactor_get_frc(pa) * multiplier);
    physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * multiplier);

    /* a turbocharged player may get locked on steep slopes due to the slope
       factor being nullified by the increased friction and the hlock_timer
       being set due to a nearly stopped player. This mitigates but doesn't
       solve the issue, which is caused by the increased friction (frc) */
    if(turbocharged)
        physicsactor_set_falloffthreshold(pa, physicsactor_get_falloffthreshold(pa) * 0.125);
    else
        physicsactor_set_falloffthreshold(pa, physicsactor_get_falloffthreshold(pa) * 8.0);
#endif
}

/* underwater physics */
void set_underwater_multipliers(physicsactor_t* pa, bool underwater)
{
    double multiplier = underwater ? 0.5 : 2.0;

    physicsactor_set_acc(pa, physicsactor_get_acc(pa) * multiplier);
    physicsactor_set_dec(pa, physicsactor_get_dec(pa) * multiplier);
    physicsactor_set_frc(pa, physicsactor_get_frc(pa) * multiplier);
    physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * multiplier);
    physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * multiplier);
    physicsactor_set_air(pa, physicsactor_get_air(pa) * multiplier);
    physicsactor_set_jmprel(pa, physicsactor_get_jmprel(pa) * multiplier);
    physicsactor_set_diejmp(pa, physicsactor_get_diejmp(pa) * multiplier);
    physicsactor_set_hitjmp(pa, physicsactor_get_hitjmp(pa) * multiplier);

    if(underwater) {
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) / 3.5);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) / 1.85);
    }
    else {
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) * 3.5);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) * 1.85);
    }
}

/* initialize the character multipliers (physics) */
void set_default_multipliers(physicsactor_t* pa, const character_t* character)
{
    /* reset the parameters of the physics model */
    physicsactor_reset_model_parameters(pa);

    /* set the multipliers */
    physicsactor_set_acc(pa, physicsactor_get_acc(pa) * character->multiplier.acc);
    physicsactor_set_dec(pa, physicsactor_get_dec(pa) * character->multiplier.dec);
    physicsactor_set_frc(pa, physicsactor_get_frc(pa) * character->multiplier.frc);
    physicsactor_set_grv(pa, physicsactor_get_grv(pa) * character->multiplier.grv);
    physicsactor_set_slp(pa, physicsactor_get_slp(pa) * character->multiplier.slp);
    physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) * character->multiplier.jmp);
    physicsactor_set_chrg(pa, physicsactor_get_chrg(pa) * character->multiplier.chrg);
    physicsactor_set_jmprel(pa, physicsactor_get_jmprel(pa) * character->multiplier.jmp);
    physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * character->multiplier.topspeed);
    physicsactor_set_rolluphillslp(pa, physicsactor_get_rolluphillslp(pa) * character->multiplier.slp);
    physicsactor_set_rolldownhillslp(pa, physicsactor_get_rolldownhillslp(pa) * character->multiplier.slp);
    physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * character->multiplier.frc);
    physicsactor_set_rolldec(pa, physicsactor_get_rolldec(pa) * character->multiplier.dec);
    physicsactor_set_air(pa, physicsactor_get_air(pa) * character->multiplier.airacc);
    physicsactor_set_airdrag(pa, physicsactor_get_airdrag(pa) / max(character->multiplier.airdrag, 0.001));

    /* configure the abilities */
    if(!character->ability.roll)
        physicsactor_set_rollthreshold(pa, 20000.0);
    if(!character->ability.brake)
        physicsactor_set_brakingthreshold(pa, 20000.0);
    if(!character->ability.charge)
        physicsactor_set_chrg(pa, 0.0);
}

/* create bouncing collectibles at the specified position */
void create_bouncing_collectibles(int number_of_collectibles, v2d_t position)
{
    const char* object_name = "Bouncing Collectible";
    const int max_circles = 2, max_collectibles_per_circle = 16;
    const int max_collectibles = max_collectibles_per_circle * max_circles;
    const float angle_increment = 360.0f / (float)max_collectibles_per_circle;
    float angle = 101.25f, speed = 240.0f;

    surgescript_var_t* x = surgescript_var_create();
    surgescript_var_t* y = surgescript_var_create();
    const surgescript_var_t* param[2] = { x, y };

    if(number_of_collectibles > max_collectibles)
        number_of_collectibles = max_collectibles;

    for(int i = 1; i <= number_of_collectibles; i++) {
        int k = 1 - (i % 2);
        float radians = DEG2RAD * angle, s = 1 - 2 * k;
        v2d_t velocity = v2d_new(cosf(radians) * speed * s, -sinf(radians) * speed);

        surgescript_object_t* collectible = level_create_object(object_name, position);
        if(collectible == NULL) {
            video_showmessage("Can't find object \"%s\"", object_name);
            break;
        }

        surgescript_var_set_number(x, velocity.x);
        surgescript_var_set_number(y, velocity.y);
        surgescript_object_call_function(collectible, "setVelocity", param, 2, NULL);

        angle += angle_increment * k;
        if(i % max_collectibles_per_circle == 0) {
            speed *= 0.5f;
            angle -= 180.0f;
        }
    }

    surgescript_var_destroy(y);
    surgescript_var_destroy(x);
}

/* handle physics event */
void on_physics_event(physicsactor_t* pa, physicsactorevent_t event, void* context)
{
    player_t* player = (player_t*)context;

    switch(event) {
        case PAE_JUMP:
            sound_play(player->character->sample.jump);
            break;

        case PAE_ROLL:
            sound_play(player->character->sample.roll);
            break;

        case PAE_CHARGE:
        case PAE_RECHARGE: {
            sound_t* sample = player->character->sample.charge;
            float max_pitch = player->character->sample.charge_pitch;
            float t = physicsactor_charge_intensity(player->pa) - 0.25f;
            float freq = lerp(1.0f, max_pitch, t);
            sound_play_ex(sample, 1.0f, 0.0f, freq);
            break;
        }

        case PAE_RELEASE:
            sound_play(player->character->sample.release);
            break;

        case PAE_BRAKE:
            sound_play(player->character->sample.brake);
            break;

        case PAE_BREATHE:
            sound_play(SFX_BREATHE);
            break;

        case PAE_BLINK:
            player_set_blinking(player, TRUE);
            break;

        case PAE_HIT:
            if(player->invulnerable) {
                ; /* do nothing */
                sound_play(SFX_DAMAGE);
            }
            else if(player->shield_type != SH_NONE) {
                /* lose shield */
                player->shield_type = SH_NONE;
                sound_play(SFX_DAMAGE);
            }
            else {
                /* create collectibles */
                int number_of_collectibles = player_get_collectibles();
                create_bouncing_collectibles(number_of_collectibles, player_position(player));
                player_set_collectibles(0);
                sound_play(SFX_GETHIT);
            }

            break;

        case PAE_KILL:
            player_set_invincible(player, FALSE);
            player_set_turbocharged(player, FALSE);
            player_set_blinking(player, FALSE);
            player_set_aggressive(player, FALSE);
            player_set_invulnerable(player, FALSE);
            player->shield_type = SH_NONE;
            sound_play(player->character->sample.death);
            break;

        case PAE_DROWN:
            player_set_invincible(player, FALSE);
            player_set_turbocharged(player, FALSE);
            player_set_blinking(player, FALSE);
            player_set_aggressive(player, FALSE);
            player_set_invulnerable(player, FALSE);
            sound_play(SFX_DROWN);
            break;

        case PAE_SMASH:
            break;

        case PAE_RESURRECT:
            player_set_position(player, level_spawnpoint());
            break;
    }
}