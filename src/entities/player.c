/*
 * Open Surge Engine
 * player.c - player module
 * Copyright (C) 2008-2012, 2018, 2019  Alexandre Martins <alemartf@gmail.com>
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
#include "sfx.h"
#include "legacy/item.h"
#include "legacy/enemy.h"
#include "../core/global.h"
#include "../core/audio.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/timer.h"
#include "../core/logfile.h"
#include "../core/input.h"
#include "../core/sprite.h"
#include "../scenes/level.h"
#include "../physics/physicsactor.h"
#include "../physics/obstaclemap.h"
#include "../physics/obstacle.h"
#include "../physics/collisionmask.h"
#include "../scripting/scripting.h"



/* Smoothing the angle (the greater the value, the faster it rotates) */
#define ANGLE_SMOOTHING 3

/* macros */
#define ON_STATE(player, s) \
    if((player)->pa_old_state != (s) && physicsactor_get_state((player)->pa) == (s))

#define CHANGE_ANIM(player, id) do { \
    animation_t *an = sprite_get_animation((player)->character->animation.sprite_name, (player)->character->animation.id); \
    float sf = (player)->actor->animation_speed_factor; \
    actor_change_animation((player)->actor, an); \
    actor_change_animation_speed_factor((player)->actor, sf); \
} while(0)

#define ANIM_SPEED_FACTOR(k, spd) \
    1.5f * min(1, (max((spd), 100)) / (k)) /* 24 / 16 */

#define DEG2RAD(x) \
    ((x) / 57.2957795131f)

/* public constants */
const int PLAYER_INITIAL_LIVES = 5;           /* initial lives */
const float PLAYER_MAX_TURBO = 23.0f;         /* turbo timer */
const float PLAYER_MAX_INVINCIBILITY = 23.0f; /* invincibility timer */

/* private constants */
static const int PLAYER_MAX_STARS = 16;              /* how many invincibility stars */
static const float PLAYER_MAX_BLINK = 2.0f;          /* how many seconds does the player blink if he/she gets hurt? */
static const float PLAYER_UNDERWATER_BREATH = 30.0f; /* how many seconds can the player stay underwater before drowning? */

/* private data */
static int hundred_collectibles; /* counter (extra lives) */
static int collectibles;         /* shared collectibles */
static int lives;                /* shared lives */
static int score;                /* shared score */

/* misc */
static void update_shield(player_t *player);
static void update_animation(player_t *player);
static void play_sounds(player_t *player);
static void physics_adapter(player_t *player, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list, surgescript_object_t* (*get_bricklike_object)(int));
static obstacle_t* item2obstacle(const item_t* item);
static obstacle_t* object2obstacle(const object_t* object);
static obstacle_t* bricklike2obstacle(const surgescript_object_t* object);
static inline int ignore_obstacle(const player_t* player, bricklayer_t brick_layer);
static inline float delta_angle(float alpha, float beta);
static void hotspot_magic(player_t* player);
static void animate_invincibility_stars(player_t* player);
static int fix_angle(int degrees, int threshold);
static int is_head_underwater(const player_t* player);
static void turbinate_player(player_t* player, float multiplier);


/*
 * player_create()
 * Creates a player
 */
player_t *player_create(const char *character_name)
{
    int i;
    player_t *p = mallocx(sizeof *p);
    const character_t *c = charactersystem_get(character_name);

    logfile_message("player_create(\"%s\")", character_name);

    /* initializing... */
    p->name = str_dup(character_name);
    p->character = c;
    p->disable_movement = FALSE;
    p->disable_roll = FALSE;
    p->disable_collectible_loss = FALSE;
    p->disable_animation_control = FALSE;
    p->aggressive = FALSE;
    p->visible = TRUE;
    p->actor = actor_create();
    p->actor->input = input_create_user(NULL);
    CHANGE_ANIM(p, stopped);

    /* auxiliary variables */
    p->on_movable_platform = FALSE;
    p->got_glasses = FALSE;
    p->thrown_while_rolling = FALSE;

    /* blink */
    p->blinking = FALSE;
    p->blink_timer = 0.0f;
    p->blink_visibility_timer = 0.0f;

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
    p->turbo = FALSE;
    p->turbo_timer = 0;

    /* loop system */
    p->layer = BRL_DEFAULT;

    /* physics */
    p->pa = physicsactor_create(p->actor->position);
    p->pa_old_state = physicsactor_get_state(p->pa);
    p->obstaclemap = obstaclemap_create();
    darray_init_ex(p->mock_obstacles, 32);

    /* misc */
    p->underwater = FALSE;
    p->underwater_timer = 0.0f;
    p->breath_time = PLAYER_UNDERWATER_BREATH;

    /* character system: setting the multipliers */
    physicsactor_set_acc(p->pa, physicsactor_get_acc(p->pa) * c->multiplier.acc);
    physicsactor_set_dec(p->pa, physicsactor_get_dec(p->pa) * c->multiplier.dec);
    physicsactor_set_frc(p->pa, physicsactor_get_frc(p->pa) * c->multiplier.frc);
    physicsactor_set_grv(p->pa, physicsactor_get_grv(p->pa) * c->multiplier.grv);
    physicsactor_set_slp(p->pa, physicsactor_get_slp(p->pa) * c->multiplier.slp);
    physicsactor_set_jmp(p->pa, physicsactor_get_jmp(p->pa) * c->multiplier.jmp);
    physicsactor_set_chrg(p->pa, physicsactor_get_chrg(p->pa) * c->multiplier.chrg);
    physicsactor_set_jmprel(p->pa, physicsactor_get_jmprel(p->pa) * c->multiplier.jmp);
    physicsactor_set_topspeed(p->pa, physicsactor_get_topspeed(p->pa) * c->multiplier.topspeed);
    physicsactor_set_rolluphillslp(p->pa, physicsactor_get_rolluphillslp(p->pa) * c->multiplier.slp);
    physicsactor_set_rolldownhillslp(p->pa, physicsactor_get_rolldownhillslp(p->pa) * c->multiplier.slp);
    physicsactor_set_rollfrc(p->pa, physicsactor_get_rollfrc(p->pa) * c->multiplier.frc);
    physicsactor_set_rolldec(p->pa, physicsactor_get_rolldec(p->pa) * c->multiplier.dec);
    physicsactor_set_air(p->pa, physicsactor_get_air(p->pa) * c->multiplier.airacc);
    physicsactor_set_airdrag(p->pa, physicsactor_get_airdrag(p->pa) / max(c->multiplier.airdrag, 0.001f));

    /* character system: configuring the abilities */
    if(!c->ability.roll)
        physicsactor_set_rollthreshold(p->pa, 20000.0f);
    if(!c->ability.brake)
        physicsactor_set_brakingthreshold(p->pa, 20000.0f);
    if(!c->ability.charge)
        physicsactor_set_chrg(p->pa, 0.0f);

    /* success! */
    hundred_collectibles = collectibles = 0;
    logfile_message("Created player \"%s\"", p->name);
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

    /* obstacle map */
    obstaclemap_destroy(player->obstaclemap);
    for(i = 0; i < darray_length(player->mock_obstacles); i++)
        obstacle_destroy(player->mock_obstacles[i]);
    darray_release(player->mock_obstacles);

    /* done */
    free(player->name);
    free(player);
    return NULL;
}



/*
 * player_update()
 * Updates the player
 */
void player_update(player_t *player, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, enemy_list_t *enemy_list, surgescript_object_t* (*get_bricklike_object)(int))
{
    actor_t *act = player->actor;
    physicsactor_t *pa = player->pa;
    float padding = 16.0f, eps = 1e-5;
    float dt = timer_get_delta();

    /* is it a CPU controlled player? */
    if(player != level_player())
        input_reset(act->input);

    /* physics */
    if(!player->disable_movement) {
        player->pa_old_state = physicsactor_get_state(pa);
        physics_adapter(player, team, team_size, brick_list, item_list, enemy_list, get_bricklike_object);
    }

    /* the player is blinking */
    if(player->blinking) {
        player->blink_timer += timer_get_delta();

        if(player->blink_timer - player->blink_visibility_timer >= 0.067f) {
            player->blink_visibility_timer = player->blink_timer;
            act->visible = !act->visible;
        }

        if(player->blink_timer >= PLAYER_MAX_BLINK)
            player_set_blinking(player, FALSE);
    }

    if(physicsactor_get_state(pa) != PAS_GETTINGHIT && player->pa_old_state == PAS_GETTINGHIT)
        player_set_blinking(player, TRUE);

    /* shield */
    if(player->shield_type != SH_NONE)
        update_shield(player);

    /* underwater logic */
    if(!player->underwater && act->position.y >= level_waterlevel())
        player_enter_water(player);
    else if(player->underwater && act->position.y < level_waterlevel())
        player_leave_water(player);
    if(player->underwater) {
        /* disable turbo */
        player_set_turbo(player, FALSE);

        /* disable some shields */
        if(player->shield_type == SH_FIRESHIELD || player->shield_type == SH_THUNDERSHIELD)
            player_hit(player, 0.0f);

        /* timer countdown */
        if(player->shield_type != SH_WATERSHIELD && !player_is_winning(player) && (
            act->position.y < level_waterlevel() || /* forced underwater via scripting OR... */
            is_head_underwater(player)              /* the head of the player is underwater */
        ))
            player->underwater_timer += dt;
        else
            player->underwater_timer = 0.0f;

        /* drowning */
        if(player_seconds_remaining_to_drown(player) <= 0.0f)
            player_drown(player);
    }

    /* invincibility stars */
    if(player->invincible) {
        /* animate */
        animate_invincibility_stars(player);

        /* update timer & finish */
        player->invincibility_timer += dt;
        if(player->invincibility_timer >= PLAYER_MAX_INVINCIBILITY)
            player_set_invincible(player, FALSE);
    }

    /* turbo speed */
    if(player->turbo) {
        /* update timer & finish */
        player->turbo_timer += dt;
        if(player->turbo_timer >= PLAYER_MAX_TURBO)
            player_set_turbo(player, FALSE);
    }

    /* winning pose */
    if(level_has_been_cleared())
        physicsactor_enable_winning_pose(pa);

    /* animation */
    update_animation(player);

    /* play sounds */
    play_sounds(player);

    /* can't leave the world */
    if(act->position.x < padding - eps) {
        act->position.x = padding;
        act->speed.x *= 0.5f;
    }
    else if(act->position.x > level_size().x - padding + eps) {
        act->position.x = level_size().x - padding;
        act->speed.x *= 0.5f;
    }

    if(act->position.y < padding - eps) {
        act->position.y = padding;
        act->speed.y *= 0.5f;
    }

    /* pitfalls */
    if(act->position.y >= level_height_at(act->position.x))
        player_kill(player);

    /* smashed / crushed */
    if(!physicsactor_is_midair(player->pa) && physicsactor_is_inside_wall(player->pa))
        player_kill(player);

    /* active player can't get off camera */
    if(player == level_player() && !player->disable_movement) {
        v2d_t cam_topleft = camera_clip(v2d_new(0, 0));
        v2d_t cam_bottomright = camera_clip(level_size());

        /* lock horizontally */
        if(act->position.x > cam_bottomright.x - padding + eps) {
            act->position.x = cam_bottomright.x - padding;
            act->speed.x *= 0.5f;
        }
        else if(act->position.x < cam_topleft.x + padding - eps) {
            act->position.x = cam_topleft.x + padding;
            act->speed.x *= 0.5f;
        }

        /* lock on top; won't prevent pits */
        if(!player_is_dying(player)) {
            if(act->position.y < cam_topleft.y + padding - eps) {
                act->position.y = cam_topleft.y + padding;
                act->speed.y *= 0.5f;
            }
        }
    }

    /* rolling misc */
    if(!player_is_midair(player))
        player->thrown_while_rolling = FALSE;
    else if(physicsactor_get_ysp(pa) < 0.0f && player_is_rolling(player))
        player->thrown_while_rolling = TRUE;

    /* misc */
    player->on_movable_platform = FALSE;
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
    if(player_is_midair(player) && !player_is_dying(player)) {
        actor_t *act = player->actor;
        
        if(direction <= 0.0f && act->speed.y >= 0.0f)
            act->speed.y = -fabs(act->speed.y);
        else if(is_heavy_object)
            act->speed.y = fabs(act->speed.y) / 2.0f;
        else
            act->speed.y += 60.0f * sign(act->speed.y);

        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_bounce(player->pa);

        return TRUE;
    }

    return FALSE;
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
    float player_centre = (player->actor->position.y - player->actor->hot_spot.y) + ph * 0.5f;
    return player_bounce(player, player_centre - hazard_centre, is_heavy_object);
}


/*
 * player_detach_from_ground()
 * Ensures the player is not touching the ground
 * (or ceiling if rotated) on the next frame
 */
void player_detach_from_ground(player_t *player)
{
    /* this is meant to counter the "sticky physics" */
    if(!player_is_midair(player)) {
        if(physicsactor_get_movmode(player->pa) == MM_FLOOR) {
            if(!player_is_rolling(player))
                player->actor->position.y -= 2;
            else
                player->actor->position.y -= 5;
        }
        else if(physicsactor_get_movmode(player->pa) == MM_CEILING) {
            if(!player_is_rolling(player))
                player->actor->position.y += 2;
            else
                player->actor->position.y += 5;
        }
    }
}

/*
 * player_hit()
 * Hits a player. If it has no collectibles, then it must die.
 * tip: direction > 0 is right, < 0 is left, 0 is neutral
 */
void player_hit(player_t *player, float direction)
{
    if(player->invincible || physicsactor_get_state(player->pa) == PAS_GETTINGHIT || player->blinking || player_is_dying(player))
        return;

    if(player_get_collectibles() > 0 || player->shield_type != SH_NONE) {
        if(direction != 0.0f)
            player->actor->speed.x = fabs(physicsactor_get_hitjmp(player->pa) * 0.5f) * sign(direction);
        player->actor->speed.y = physicsactor_get_hitjmp(player->pa);
        player_detach_from_ground(player);

        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_hit(player->pa);

        if(player->shield_type != SH_NONE) {
            player->shield_type = SH_NONE;
            sound_play(SFX_DAMAGE);
        }
        else if(!player->disable_collectible_loss) {
            float a = 101.25f, spd = 240.0f;
            int r = min(32, player_get_collectibles());

            /* create collectibles */
            for(int i = 0; i < r; i++) {
                v2d_t velocity = v2d_new(
                    -sinf(DEG2RAD(a)) * spd * (1 - 2 * (i % 2)),
                    cosf(DEG2RAD(a)) * spd
                );
                surgescript_object_t* collectible = level_create_object("Bouncing Collectible", player->actor->position);

                if(collectible != NULL) {
                    surgescript_var_t* x = surgescript_var_create();
                    surgescript_var_t* y = surgescript_var_create();
                    const surgescript_var_t* param[2] = {
                        surgescript_var_set_number(x, velocity.x),
                        surgescript_var_set_number(y, velocity.y),
                    };
                    surgescript_object_call_function(collectible, "setVelocity", param, 2, NULL);
                    surgescript_var_destroy(y);
                    surgescript_var_destroy(x);
                }
                else {
                    item_t* b = level_create_legacy_item(IT_BOUNCINGCOLLECT, player->actor->position);
                    bouncingcollectible_set_velocity(b, velocity);
                }

                a += 22.5f * (i % 2);
                if(i == 16) {
                    spd *= 0.5f;
                    a -= 180.0f;
                }
            }

            player_set_collectibles(0);
            sound_play(SFX_GETHIT);
        }
        else
            sound_play(SFX_DAMAGE);
    }
    else
        player_kill(player);
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
    float player_centre = (player->actor->position.x - player->actor->hot_spot.x) + pw * 0.5f;
    player_hit(player, player_centre - hazard_centre);
}



/*
 * player_kill()
 * Kills a player
 */
void player_kill(player_t *player)
{
    if(!player_is_dying(player)) {
        player_set_invincible(player, FALSE);
        player_set_turbo(player, FALSE);
        player_set_blinking(player, FALSE);
        player_set_aggressive(player, FALSE);
        player->shield_type = SH_NONE;
        player->actor->speed = v2d_new(0, physicsactor_get_diejmp(player->pa));

        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_kill(player->pa);
        sound_play(player->character->sample.death);
    }
}


/*
 * player_roll()
 * Rolls
 */
void player_roll(player_t *player)
{
    if(!player_is_dying(player)) {
        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_roll(player->pa);
    }
}

/*
 * player_enable_roll()
 * enables player rolling
 */
void player_enable_roll(player_t *player)
{
    physicsactor_t *pa = player->pa;
    if(player->disable_roll) {
        physicsactor_set_rollthreshold(pa, physicsactor_get_rollthreshold(pa) - 1000.0f);
        player->disable_roll = FALSE;
    }
}

/*
 * player_disable_roll()
 * disables player rolling
 */
void player_disable_roll(player_t *player)
{
    if(!player->disable_roll) {
        physicsactor_t *pa = player->pa;
        physicsactor_set_rollthreshold(pa, physicsactor_get_rollthreshold(pa) + 1000.0f);
        player->disable_roll = TRUE;
    }
}

/*
 * player_spring()
 * Springfy player
 */
void player_spring(player_t *player)
{
    if(!player_is_dying(player)) {
        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_spring(player->pa);
    }
}

/*
 * player_drown()
 * Drown (underwater). This will be
 * called automatically, internally.
 */
void player_drown(player_t *player)
{
    if(player_is_underwater(player) && !player_is_dying(player)) {
        player->actor->speed = v2d_new(0, 0);
        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_drown(player->pa);
        sound_play(SFX_DROWN);
    }
}

/*
 * player_breathe()
 * Breathe (air bubble, underwater)
 */
void player_breathe(player_t *player)
{
    if(player_is_underwater(player) && physicsactor_get_state(player->pa) != PAS_BREATHING && !player_is_dying(player)) {
        player_reset_underwater_timer(player);
        player->actor->speed = v2d_new(0, 0);
        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_breathe(player->pa);
        sound_play(SFX_BREATHE);
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
        physicsactor_t *pa = player->pa;

        player->actor->speed.x /= 2.0f;
        player->actor->speed.y /= 4.0f;

        physicsactor_set_acc(pa, physicsactor_get_acc(pa) / 2.0f);
        physicsactor_set_dec(pa, physicsactor_get_dec(pa) / 2.0f);
        physicsactor_set_frc(pa, physicsactor_get_frc(pa) / 2.0f);
        physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) / 2.0f);
        physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) / 2.0f);
        physicsactor_set_air(pa, physicsactor_get_air(pa) / 2.0f);
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) / 3.5f);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) / 1.85f);
        physicsactor_set_jmprel(pa, physicsactor_get_jmprel(pa) / 2.0f);
        physicsactor_set_diejmp(pa, physicsactor_get_diejmp(pa) / 2.0f);
        physicsactor_set_hitjmp(pa, physicsactor_get_hitjmp(pa) / 2.0f);

        sound_play(SFX_WATERIN);
        player->underwater_timer = 0.0f;
        player->underwater = TRUE;
    }
}

/*
 * player_leave_water()
 * Leaves the water
 */
void player_leave_water(player_t *player)
{
    if(player_is_underwater(player)) {
        physicsactor_t *pa = player->pa;

        if(!player_is_springing(player) && !player_is_dying(player))
            player->actor->speed.y *= 2.0f;

        physicsactor_set_acc(pa, physicsactor_get_acc(pa) * 2.0f);
        physicsactor_set_dec(pa, physicsactor_get_dec(pa) * 2.0f);
        physicsactor_set_frc(pa, physicsactor_get_frc(pa) * 2.0f);
        physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * 2.0f);
        physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * 2.0f);
        physicsactor_set_air(pa, physicsactor_get_air(pa) * 2.0f);
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) * 3.5f);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) * 1.85f);
        physicsactor_set_jmprel(pa, physicsactor_get_jmprel(pa) * 2.0f);
        physicsactor_set_diejmp(pa, physicsactor_get_diejmp(pa) * 2.0f);
        physicsactor_set_hitjmp(pa, physicsactor_get_hitjmp(pa) * 2.0f);

        sound_play(SFX_WATEROUT);
        player->underwater = FALSE;
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
    return player->underwater && player->shield_type != SH_WATERSHIELD ? max(0.0f, player->breath_time - player->underwater_timer) : INFINITY;
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
    image_t *img = actor_image(actor);

    physicsactor_bounding_box(player->pa, &player_box_width, &player_box_height, &player_box_center);
    if(player_is_frozen(player))
        player_box_center = player->actor->position;

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
        player_box_center = player->actor->position;

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
 * player_senses_layer()
 * Returns TRUE if the player is currently capable of
 * sensing the given layer
 */
int player_senses_layer(const player_t* player, bricklayer_t layer)
{
    return (layer == BRL_DEFAULT) || (player->layer == layer);
}

/*
 * player_is_attacking()
 * Returns TRUE if a given player is attacking;
 * FALSE otherwise
 */
int player_is_attacking(const player_t *player)
{
    return !player_is_dying(player) && (player->aggressive || player->invincible || physicsactor_get_state(player->pa) == PAS_JUMPING || physicsactor_get_state(player->pa) == PAS_ROLLING || physicsactor_get_state(player->pa) == PAS_CHARGING);
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
    return physicsactor_get_state(player->pa) == PAS_DEAD || physicsactor_get_state(player->pa) == PAS_DROWNED;
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
    return player->turbo;
}

/*
 * player_set_turbo()
 * Enable (or disable) turbo mode
 */
void player_set_turbo(player_t* player, int turbo)
{
    if(player_is_dying(player))
        return;

    if(turbo == player->turbo) {
        if(turbo)
            player->turbo_timer = 0.0f;
        return; /* nothing to do */
    }

    if(turbo) {
        player->turbo = TRUE;
        player->turbo_timer = 0.0f;
        turbinate_player(player, 2.0f);
    }
    else {
        player->turbo = FALSE;
        turbinate_player(player, 0.5f);
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
    player->disable_movement = frozen;
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
 * Is the player aggressive? (i.e., it hits baddies regardless if jumping or not)
 */
int player_is_aggressive(const player_t* player)
{
    return player->aggressive;
}

/*
 * player_set_aggressive()
 * If set to true, player_is_attacking() will be true and the player
 *  will be able to hit baddies regardless if jumping or not
 */
void player_set_aggressive(player_t* player, int aggressive)
{
    player->aggressive = aggressive;
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
        player->actor->visible = TRUE;
    }
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
void player_set_collectibles(int c)
{
    collectibles = clip(c, 0, 999);

    /* (100+) * k collectibles (k integer) = new life! */
    if(c/100 > hundred_collectibles) {
        hundred_collectibles = c/100;
        player_set_lives( player_get_lives()+1 );
        level_override_music(SFX_1UP);
    }
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
void player_set_lives(int l)
{
    lives = max(0, l);
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
void player_set_score(int s)
{
    score = max(0, s);
}


/*
 * player_name()
 * The name of the (character associated with the) player
 */
const char* player_name(const player_t* player)
{
    return player->name;
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


/* private functions */

/* updates the current shield */
void update_shield(player_t *player)
{
    actor_t *sh = player->shield, *act = player->actor;
    v2d_t off = v2d_new(0,0);
    sh->position = v2d_add(act->position, v2d_rotate(off, -act->angle));
    sh->scale = act->scale;

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
    /* animations */
    if(!player->disable_animation_control) {
        physicsactorstate_t state = physicsactor_get_state(player->pa);
        float xsp = fabs(physicsactor_get_xsp(player->pa));
        float gsp = fabs(physicsactor_get_gsp(player->pa));

        switch(state) {
            case PAS_STOPPED:    CHANGE_ANIM(player, stopped);    break;
            case PAS_WALKING:    CHANGE_ANIM(player, walking);    break;
            case PAS_RUNNING:    CHANGE_ANIM(player, running);    break;
            case PAS_JUMPING:    CHANGE_ANIM(player, jumping);    break;
            case PAS_SPRINGING:  CHANGE_ANIM(player, springing);  break;
            case PAS_ROLLING:    CHANGE_ANIM(player, rolling);    break;
            case PAS_CHARGING:   CHANGE_ANIM(player, charging);   break;
            case PAS_PUSHING:    CHANGE_ANIM(player, pushing);    break;
            case PAS_GETTINGHIT: CHANGE_ANIM(player, gettinghit); break;
            case PAS_DEAD:       CHANGE_ANIM(player, dead);       break;
            case PAS_BRAKING:    CHANGE_ANIM(player, braking);    break;
            case PAS_LEDGE:      CHANGE_ANIM(player, ledge);      break;
            case PAS_DROWNED:    CHANGE_ANIM(player, drowned);    break;
            case PAS_BREATHING:  CHANGE_ANIM(player, breathing);  break;
            case PAS_WAITING:    CHANGE_ANIM(player, waiting);    break;
            case PAS_DUCKING:    CHANGE_ANIM(player, ducking);    break;
            case PAS_LOOKINGUP:  CHANGE_ANIM(player, lookingup);  break;
            case PAS_WINNING:    CHANGE_ANIM(player, winning);    break;
        }

        if(state == PAS_WALKING || state == PAS_RUNNING)
            actor_change_animation_speed_factor(player->actor, ANIM_SPEED_FACTOR(480, gsp));
        else if(state == PAS_ROLLING && !physicsactor_is_midair(player->pa))
            actor_change_animation_speed_factor(player->actor, ANIM_SPEED_FACTOR(300, max(gsp, xsp)));
        else if(!((state == PAS_JUMPING) || (state == PAS_ROLLING && physicsactor_is_midair(player->pa))))
            actor_change_animation_speed_factor(player->actor, 1.0f);
        else if(state == PAS_JUMPING && player->actor->animation_speed_factor < 1.0f)
            actor_change_animation_speed_factor(player->actor, 1.0f);
    }
    else
        player->disable_animation_control = FALSE; /* for set_player_animation (scripting) */
}

/* play sounds as needed */
void play_sounds(player_t* player)
{
    ON_STATE(player, PAS_JUMPING) {
        sound_play(player->character->sample.jump);
    }

    ON_STATE(player, PAS_BRAKING) {
        sound_play(player->character->sample.brake);
    }

    ON_STATE(player, PAS_CHARGING) {
        sound_play(player->character->sample.charge);
    }

    ON_STATE(player, PAS_ROLLING) {
        if(player->pa_old_state != PAS_CHARGING)
            sound_play(player->character->sample.roll);
        else
            sound_play(player->character->sample.release);
    }

    if(physicsactor_get_state(player->pa) == PAS_CHARGING) {
        if(input_button_pressed(player->actor->input, IB_FIRE1)) {
            sound_t* sample = player->character->sample.charge;
            float max_pitch = player->character->sample.charge_pitch;
            float freq = lerp(1.0f, max_pitch, physicsactor_charge_intensity(player->pa) - 0.25f);
            sound_play_ex(sample, 1.0f, 0.0f, freq);
        }
    }
}

/* the interface between player_t and physicsactor_t */
void physics_adapter(player_t *player, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list, surgescript_object_t* (*get_bricklike_object)(int))
{
    actor_t *act = player->actor;
    physicsactor_t *pa = player->pa;
    obstaclemap_t* obstaclemap = player->obstaclemap;
    surgescript_object_t* bricklike_object = NULL;
    int i;

    /* converting variables */
    physicsactor_set_position(pa, act->position);
    if(!(player_is_midair(player) || player_is_getting_hit(player) || player_is_dying(player)))
        physicsactor_set_gsp(pa, act->speed.x);
    physicsactor_set_xsp(pa, act->speed.x);
    physicsactor_set_ysp(pa, act->speed.y);

    /* capturing input */
    if(input_button_down(act->input, IB_RIGHT))
        physicsactor_walk_right(pa);
    if(input_button_down(act->input, IB_LEFT))
        physicsactor_walk_left(pa);
    if(input_button_down(act->input, IB_DOWN))
        physicsactor_duck(pa);
    if(input_button_down(act->input, IB_UP))
        physicsactor_look_up(pa);
    if(input_button_pressed(act->input, IB_FIRE1))
        physicsactor_1stjump(pa);
    else if(input_button_down(act->input, IB_FIRE1))
        physicsactor_jump(pa);

    /* clearing the obstacle map &
       removing previous mock obstacles */
    obstaclemap_clear(obstaclemap);
    for(i = 0; i < darray_length(player->mock_obstacles); i++)
        obstacle_destroy(player->mock_obstacles[i]);
    darray_clear(player->mock_obstacles);

    /* creating the obstacle map */
    for(; brick_list; brick_list = brick_list->next) {
        if(brick_obstacle(brick_list->data) != NULL && !ignore_obstacle(player, brick_layer(brick_list->data)))
            obstaclemap_add_obstacle(obstaclemap, brick_obstacle(brick_list->data));
    }
    for(; item_list; item_list = item_list->next) {
        if(item_list->data->obstacle && item_list->data->mask && !ignore_obstacle(player, BRL_DEFAULT)) {
            obstacle_t* mock_obstacle = item2obstacle(item_list->data);
            obstaclemap_add_obstacle(obstaclemap, mock_obstacle);
            darray_push(player->mock_obstacles, mock_obstacle);
        }
    }
    for(; object_list; object_list = object_list->next) {
        if(object_list->data->obstacle && object_list->data->mask && !ignore_obstacle(player, BRL_DEFAULT)) {
            obstacle_t* mock_obstacle = object2obstacle(object_list->data);
            obstaclemap_add_obstacle(obstaclemap, mock_obstacle);
            darray_push(player->mock_obstacles, mock_obstacle);
        }
    }
    for(i = 0; (bricklike_object = get_bricklike_object(i)) != NULL; i++) {
        if(!surgescript_object_is_killed(bricklike_object)) {
            if(scripting_brick_enabled(bricklike_object) && scripting_brick_mask(bricklike_object) && !ignore_obstacle(player, scripting_brick_layer(bricklike_object))) {
                obstacle_t* mock_obstacle = bricklike2obstacle(bricklike_object);
                obstaclemap_add_obstacle(obstaclemap, mock_obstacle);
                darray_push(player->mock_obstacles, mock_obstacle);
            }
        }
    }

    /* updating the physics actor */
    physicsactor_update(pa, obstaclemap);

    /* unconverting variables */
    act->position = physicsactor_get_position(pa);
    if(player_is_midair(player) || player_is_getting_hit(player) || player_is_dying(player))
        act->speed = v2d_new(physicsactor_get_xsp(pa), physicsactor_get_ysp(pa));
    else
        act->speed = v2d_new(physicsactor_get_gsp(pa), 0.0f);

    /* smoothing the angle */
    if((physicsactor_get_movmode(pa) != MM_FLOOR || !(
        player_is_stopped(player) || player_is_waiting(player) ||
        player_is_ducking(player) || player_is_looking_up(player) ||
        player_is_jumping(player) || player_is_pushing(player) ||
        player_is_rolling(player) || player_is_at_ledge(player)
    )) && !player_is_dying(player)) {
        float new_angle = DEG2RAD(fix_angle(physicsactor_get_angle(pa), 15));
        if(delta_angle(new_angle, act->angle) < 1.6f) {
            float t = (ANGLE_SMOOTHING * PI) * timer_get_delta();
            act->angle = lerp_angle(act->angle, new_angle, t);
        }
        else
            act->angle = new_angle;
    }
    else
        act->angle = 0.0f;

    /* mirroring */
    act->mirror = !physicsactor_is_facing_right(pa) ? IF_HFLIP : IF_NONE;
}

/* converts a built-in item to an obstacle */
obstacle_t* item2obstacle(const item_t* item)
{
    const collisionmask_t* mask = item->mask;
    v2d_t position = v2d_subtract(item->actor->position, item->actor->hot_spot);
    return obstacle_create(mask, position.x, position.y, OF_SOLID);
}

/* converts a legacy object to an obstacle */
obstacle_t* object2obstacle(const object_t* object)
{
    const collisionmask_t* mask = object->mask;
    v2d_t position = v2d_subtract(object->actor->position, object->actor->hot_spot);
    return obstacle_create(mask, position.x, position.y, OF_SOLID);
}

/* converts a brick-like SurgeScript object to an obstacle */
obstacle_t* bricklike2obstacle(const surgescript_object_t* object)
{
    const collisionmask_t* mask = scripting_brick_mask(object); /* this is assumed to not be NULL */
    v2d_t position = v2d_subtract(scripting_util_world_position(object), scripting_brick_hotspot(object));
    int flags = (scripting_brick_type(object) == BRK_SOLID) ? OF_SOLID : OF_CLOUD;
    return obstacle_create(mask, position.x, position.y, flags);
}

/* ignore the obstacle? */
int ignore_obstacle(const player_t* player, bricklayer_t brick_layer)
{
    return !player_senses_layer(player, brick_layer);
}

/* hotspot "gambiarra" */
void hotspot_magic(player_t* player)
{
    actor_t* act = player->actor;
    physicsactor_t* pa = player->pa;
    int angle = physicsactor_get_angle(pa);

    if(!player_is_rolling(player) && !player_is_charging(player)) {
        const float angthr = sinf(DEG2RAD(11.25f));
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
    const float angpi = (2.0f * PI) / PLAYER_MAX_STARS;
    float x, angle, distance, max_distance;
    int i, width, height;
    v2d_t center;

    /* get coordinates & dimensions */
    physicsactor_bounding_box(player->pa, &width, &height, &center);
    max_distance = min(width, height);
    if(player_is_frozen(player))
        center = player->actor->position;

    /* animate */
    for(i = 0; i < PLAYER_MAX_STARS; i++) {
        x = 1.0f - fmodf(timer_get_ticks() + (magic * i), 1000.0f) * 0.001f;
        distance = max_distance * (1.0f - x * x * x);
        angle = -i * angpi;
        player->star[i]->alpha = x * x;
        player->star[i]->position = v2d_add(center, v2d_rotate(v2d_new(distance, 0.0f), angle));
        actor_change_animation_speed_factor(player->star[i], 1.0f + i * 0.25f);
    }
}



/* given two angles in [0, 2pi], return their difference */
float delta_angle(float alpha, float beta)
{
    static const float twopi = PI * 2;
    float diff = fmod(fabs(alpha - beta), twopi);
    return min(twopi - diff, diff);
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
        player_box_center = player->actor->position;

    top = player_box_center.y - player_box_height / 2.0f;
    bottom = player_box_center.y + player_box_height / 2.0f;
    return (int)lerp(bottom, top, head_factor) >= level_waterlevel();
}

/* turbinate player physics based on some multiplier */
void turbinate_player(player_t* player, float multiplier)
{
    physicsactor_t* pa = player->pa;
    multiplier = max(0.0f, multiplier);

    physicsactor_set_acc(pa, physicsactor_get_acc(pa) * multiplier);
    physicsactor_set_frc(pa, physicsactor_get_frc(pa) * multiplier);
    physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * multiplier);
    physicsactor_set_air(pa, physicsactor_get_air(pa) * multiplier);
    physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * multiplier);
}