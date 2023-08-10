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



/* Smoothing the angle (the greater the value, the faster it rotates) */
#define ANGLE_SMOOTHING 3

/* macros */
#define ON_STATE(player, s) \
    if((player)->pa_old_state != (s) && physicsactor_get_state((player)->pa) == (s))

#define CHANGE_ANIM(player, id) do { \
    const animation_t *an = sprite_get_animation((player)->character->animation.sprite_name, (player)->character->animation.id); \
    float sf = (player)->actor->animation_speed_factor; \
    actor_change_animation((player)->actor, an); \
    actor_change_animation_speed_factor((player)->actor, sf); \
} while(0)

#define ANIM_SPEED_FACTOR(k, spd) \
    1.5f * min(1, (max((spd), 100)) / (k)) /* 24 / 16 */

/* public constants */
const int PLAYER_INITIAL_LIVES = 5;    /* initial lives */

/* private constants */
static const int PLAYER_MAX_STARS = 16;               /* how many invincibility stars */
static const float PLAYER_MAX_BLINK = 2.0f;           /* how long does the player blink if he/she gets hurt, in seconds */
static const float PLAYER_UNDERWATER_BREATH = 30.0f;  /* how long can the player stay underwater before drowning, in seconds */
static const float PLAYER_TURBO_TIME = 20.0f;         /* super speed time, in seconds */
static const float PLAYER_INVINCIBILITY_TIME = 20.0f; /* invincibility time, in seconds */
static const float PLAYER_DEAD_RESTART_TIME = 2.5f;   /* time to restart the level when the player is killed */

/* private data */
static int collectibles = 0;                /* shared collectibles */
static int lives = PLAYER_INITIAL_LIVES;    /* shared lives */
static int score = 0;                       /* shared score */
static playermode_t mode = PM_COOPERATIVE;  /* mode of gameplay */

/* misc */
static void update_shield(player_t *player);
static void update_animation(player_t *player);
static void play_sounds(player_t *player);
static void physics_adapter(player_t *player, const obstaclemap_t* obstaclemap);
static inline float delta_angle(float alpha, float beta);
static void hotspot_magic(player_t* player);
static void animate_invincibility_stars(player_t* player);
static int fix_angle(int degrees, int threshold);
static int is_head_underwater(const player_t* player);
static void set_default_multipliers(physicsactor_t* pa, const character_t* character);
static void set_turbocharged_multipliers(physicsactor_t* pa, bool turbocharged);
static void set_underwater_multipliers(physicsactor_t* pa, bool underwater);
static void create_bouncing_collectibles(int number_of_collectibles, v2d_t position);


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
    set_default_multipliers(p->pa, c);

    /* misc */
    p->underwater = FALSE;
    p->underwater_timer = 0.0f;
    p->breath_time = PLAYER_UNDERWATER_BREATH;
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
        player->pa_old_state = physicsactor_get_state(pa);
        physics_adapter(player, obstaclemap);

        /* enter / leave water */
        /* FIXME scripting flag */
        if(act->position.y >= level_waterlevel()) {
            if(!player->underwater)
                player_enter_water(player);
        }
        else {
            if(player->underwater)
                player_leave_water(player);
        }

        /* underwater logic */
        if(player->underwater) {
            /* disable turbo */
            player_set_turbo(player, FALSE);

            /* disable some shields */
            if(player->shield_type == SH_FIRESHIELD || player->shield_type == SH_THUNDERSHIELD) {
                if(!player_is_invincible(player))
                    player_hit(player, 0.0f);
                else
                    player->shield_type = SH_NONE;
            }

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

        /* the player is blinking */
        if(player->blinking) {
            player->blink_timer += timer_get_delta();

            if(player->blink_timer >= player->blink_visibility_timer + 0.067f) {
                player->blink_visibility_timer = player->blink_timer;
                act->visible = !act->visible;
            }

            if(player->blink_timer >= PLAYER_MAX_BLINK)
                player_set_blinking(player, FALSE);
        }

        if(physicsactor_get_state(pa) != PAS_GETTINGHIT && player->pa_old_state == PAS_GETTINGHIT)
            player_set_blinking(player, TRUE);

        /* invincibility stars */
        if(player->invincible) {
            /* update timer & finish */
            player->invincibility_timer += dt;
            if(player->invincibility_timer >= PLAYER_INVINCIBILITY_TIME)
                player_set_invincible(player, FALSE);
        }

        /* turbo speed */
        if(player->turbo) {
            /* update timer & finish */
            player->turbo_timer += dt;
            if(player->turbo_timer >= PLAYER_TURBO_TIME)
                player_set_turbo(player, FALSE);
        }

        /* pitfalls */
        if(act->position.y >= level_height_at(act->position.x))
            player_kill(player);

        /* smashed / crushed */
        if(physicsactor_is_smashed(player->pa))
            player_kill(player);

        /* winning pose */
        if(level_has_been_cleared())
            physicsactor_enable_winning_pose(pa);

        /* rolling misc */
        if(!player_is_midair(player))
            player->thrown_while_rolling = FALSE;
        else if(physicsactor_get_ysp(pa) < 0.0f && player_is_rolling(player))
            player->thrown_while_rolling = TRUE;

        /* misc */
        player->on_movable_platform = FALSE;

        /* the focused player can't get off the boundaries of the camera
           (when boundaries are enabled) */
        if(player_has_focus(player)) {
            v2d_t cam_topleft = camera_clip(v2d_new(0, 0));
            v2d_t cam_bottomright = camera_clip(level_size());

            /* lock horizontally */
            if(act->position.x > cam_bottomright.x - padding + eps) {
                act->position.x = cam_bottomright.x - padding;
                player_set_speed(player, player_speed(player) * 0.5f);
            }
            else if(act->position.x < cam_topleft.x + padding - eps) {
                act->position.x = cam_topleft.x + padding;
                player_set_speed(player, player_speed(player) * 0.5f);
            }

            /* lock on top; won't prevent pits */
            if(!player_is_dying(player)) {
                if(act->position.y < cam_topleft.y + padding - eps) {
                    act->position.y = cam_topleft.y + padding;
                    player_set_ysp(player, player_ysp(player) * 0.5f);
                }
            }
        }

        /* modes of gameplay */
        switch(mode) {

            /* cooperative play */
            case PM_COOPERATIVE: {
                /* am I hurt? Gotta have the focus */
                if(player_is_getting_hit(player) || player_is_dying(player)) {
                    if(!player_has_focus(player))
                        player_focus(player);
                }
                break;
            }

            /* classic mode */
            case PM_CLASSIC: {
                /* make non-focused players invulnerable, immortal and secondary.
                   we continuously update the flags (both on and off) because we
                   take character switching into account. */
                int has_focus = player_has_focus(player);
                player_set_invulnerable(player, !has_focus);
                player_set_immortal(player, !has_focus);
                player_set_secondary(player, !has_focus);
                break;
            }

        }
    }
    /* else if player is frozen, blinking and invisible ...? */

    /* can't leave the world */
    if(act->position.x < padding - eps) {
        act->position.x = padding;
        player_set_speed(player, player_speed(player) * 0.5f);
    }
    else if(act->position.x > level_size().x - padding + eps) {
        act->position.x = level_size().x - padding;
        player_set_speed(player, player_speed(player) * 0.5f);
    }

    if(act->position.y < padding - eps) {
        act->position.y = padding;
        player_set_ysp(player, player_ysp(player) * 0.5f);
    }

    /* invincibility stars */
    if(player->invincible)
        animate_invincibility_stars(player);

    /* shield */
    if(player->shield_type != SH_NONE)
        update_shield(player);

    /* play sounds */
    play_sounds(player);

    /* restart the level if dead */
#if 0
    if(!player->disable_movement && player_is_dying(player)) {
#else
    if(player_is_dying(player)) {
#endif
        bool can_ressurrect = player->immortal;
        const float FADEOUT_TIME = 1.0f;

        if(!can_ressurrect) {
            /* fade out the music */
            const float MUSIC_FADEOUT_TIME = 0.5f;
            float new_volume = 1.0f - min(player->dead_timer, MUSIC_FADEOUT_TIME) / MUSIC_FADEOUT_TIME;
            float current_volume = music_get_volume();
            if(new_volume < current_volume)
                music_set_volume(new_volume);

            /* hide the mobile gamepad */
            mobilegamepad_fadeout();
        }

        /* decide what to do next */
        if(player->dead_timer >= PLAYER_DEAD_RESTART_TIME) {

            if(can_ressurrect) {
                /* ressurrect */
                v2d_t ressurrected_position = level_spawnpoint();
                physicsactor_ressurrect(player->pa, ressurrected_position);
                player->actor->position = ressurrected_position;
                player->dead_timer = 0.0f;
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
}

/*
 * player_early_update()
 * Pre-scripting update routine
 */
void player_early_update(player_t *player)
{
    /* update animation */
    do {
        if(player->disable_animation_control) {
            player->disable_animation_control = FALSE; /* for set_player_animation (scripting) */
            break;
        }

        if(player->disable_movement)
            break;

        update_animation(player);
    } while(0);
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
    (void)is_heavy_object; /* obsolete */

    player->pa_old_state = physicsactor_get_state(player->pa);
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
    float player_centre = (player->actor->position.y - player->actor->hot_spot.y) + ph * 0.5f;
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
    player->pa_old_state = physicsactor_get_state(player->pa);
    physicsactor_hit(player->pa, direction);

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
        int number_of_collectibles = min(32, player_get_collectibles());
        create_bouncing_collectibles(number_of_collectibles, player->actor->position);
        player_set_collectibles(0);
        sound_play(SFX_GETHIT);
    }
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
        player_set_invulnerable(player, FALSE);
        player->shield_type = SH_NONE;

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
 * enables player rolling - OBSOLETE
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
 * disables player rolling - OBSOLETE
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
    if(player->underwater && player->shield_type != SH_WATERSHIELD)
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
 * player_transform_into()
 * Transforms the player into a different character
 * Returns TRUE on success
 */
int player_transform_into(player_t *player, surgescript_object_t *player_object, const char *character_name)
{
    /* if the player must be transformed to itself, then we consider
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
    int turbocharged = player->turbo;
    int underwater = player->underwater;

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
 * Give focus to a player
 */
void player_focus(player_t* player)
{
    if(!player_has_focus(player))
        level_change_player(player);
}

/*
 * player_is_attacking()
 * Returns TRUE if a given player is attacking;
 * FALSE otherwise
 */
int player_is_attacking(const player_t *player)
{
    if(!player_is_dying(player)) {
        physicsactorstate_t state = physicsactor_get_state(player->pa);
        return player->aggressive || player->invincible || state == PAS_JUMPING || state == PAS_ROLLING || state == PAS_CHARGING;
    }

    return FALSE;
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
        set_turbocharged_multipliers(player->pa, true);
    }
    else {
        player->turbo = FALSE;
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
    if(frozen && !player->disable_movement) {
        if(player_is_blinking(player))
            player_set_blinking(player, FALSE);
    }

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
 * will appear to be ressurrected on its spawn point without losing a life
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

/*
 * player_set_mode()
 * Set the mode of gameplay
 */
void player_set_mode(playermode_t new_mode)
{
    mode = new_mode;
}

/*
 * player_get_mode()
 * Get the current mode of gameplay
 */
playermode_t player_get_mode()
{
    return mode;
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
    physicsactorstate_t state = physicsactor_get_state(player->pa);
    float xsp = fabs(physicsactor_get_xsp(player->pa));
    float gsp = fabs(physicsactor_get_gsp(player->pa));
    bool midair = physicsactor_is_midair(player->pa);

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
        actor_change_animation_speed_factor(player->actor, ANIM_SPEED_FACTOR(480, midair ? xsp : gsp));
    else if(state == PAS_ROLLING && !midair)
        actor_change_animation_speed_factor(player->actor, ANIM_SPEED_FACTOR(300, max(gsp, xsp)));
    else if(!((state == PAS_JUMPING) || (state == PAS_ROLLING && midair)))
        actor_change_animation_speed_factor(player->actor, 1.0f);
    else if(state == PAS_JUMPING && player->actor->animation_speed_factor < 1.0f)
        actor_change_animation_speed_factor(player->actor, 1.0f);
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
void physics_adapter(player_t *player, const obstaclemap_t* obstaclemap)
{
    actor_t *act = player->actor;
    physicsactor_t *pa = player->pa;

    /* set position
       TODO remove */
    physicsactor_set_position(pa, act->position);

    /* capturing input */
    if(input_button_down(act->input, IB_RIGHT))
        physicsactor_walk_right(pa);
    if(input_button_down(act->input, IB_LEFT))
        physicsactor_walk_left(pa);
    if(input_button_down(act->input, IB_DOWN))
        physicsactor_duck(pa);
    if(input_button_down(act->input, IB_UP))
        physicsactor_look_up(pa);
    if(input_button_down(act->input, IB_FIRE1))
        physicsactor_jump(pa);

    /* set the layer of the physics actor */
    if(player->layer == BRL_GREEN)
        physicsactor_set_layer(pa, OL_GREEN);
    else if(player->layer == BRL_YELLOW)
        physicsactor_set_layer(pa, OL_YELLOW);
    else
        physicsactor_set_layer(pa, OL_DEFAULT);

    /* physics update */
    physicsactor_update(pa, obstaclemap);

    /* mirroring */
    if(physicsactor_is_facing_right(pa))
        act->mirror &= ~IF_HFLIP;
    else
        act->mirror |= IF_HFLIP;

    /* update position */
    act->position = physicsactor_get_position(pa);

    /* smoothing the angle */
    if((physicsactor_get_movmode(pa) != MM_FLOOR || !(
        player_is_stopped(player) || player_is_waiting(player) ||
        player_is_ducking(player) || player_is_looking_up(player) ||
        player_is_jumping(player) || player_is_pushing(player) ||
        player_is_rolling(player) || player_is_at_ledge(player)
    )) && !player_is_dying(player)) {
        float new_angle = DEG2RAD * fix_angle(physicsactor_get_angle(pa), 15);
        if(delta_angle(new_angle, act->angle) < 1.6f) {
            float t = (ANGLE_SMOOTHING * PI) * timer_get_delta();
            act->angle = lerp_angle(act->angle, new_angle, t);
        }
        else
            act->angle = new_angle;
    }
    else
        act->angle = 0.0f;
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
        player_box_center = player->actor->position;

    top = player_box_center.y - player_box_height / 2.0f;
    bottom = player_box_center.y + player_box_height / 2.0f;
    return (int)lerp(bottom, top, head_factor) >= level_waterlevel();
}

/* turbocharged physics */
void set_turbocharged_multipliers(physicsactor_t* pa, bool turbocharged)
{
    float multiplier = turbocharged ? 2.0f : 0.5f;

    physicsactor_set_acc(pa, physicsactor_get_acc(pa) * multiplier);
    physicsactor_set_frc(pa, physicsactor_get_frc(pa) * multiplier);
    physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * multiplier);
    physicsactor_set_air(pa, physicsactor_get_air(pa) * multiplier);
    physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * multiplier);
}

/* underwater physics */
void set_underwater_multipliers(physicsactor_t* pa, bool underwater)
{
    float multiplier = underwater ? 0.5f : 2.0f;

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
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) / 3.5f);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) / 1.85f);
    }
    else {
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) * 3.5f);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) * 1.85f);
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
    physicsactor_set_airdrag(pa, physicsactor_get_airdrag(pa) / max(character->multiplier.airdrag, 0.001f));

    /* configure the abilities */
    if(!character->ability.roll)
        physicsactor_set_rollthreshold(pa, 20000.0f);
    if(!character->ability.brake)
        physicsactor_set_brakingthreshold(pa, 20000.0f);
    if(!character->ability.charge)
        physicsactor_set_chrg(pa, 0.0f);
}

/* create bouncing collectibles at the specified position */
void create_bouncing_collectibles(int number_of_collectibles, v2d_t position)
{
    const char* object_name = "Bouncing Collectible";
    const int collectibles_per_circle = 16;
    const float angle_increment = 360.0f / (float)collectibles_per_circle;
    float angle = 101.25f, speed = 240.0f;

    surgescript_var_t* x = surgescript_var_create();
    surgescript_var_t* y = surgescript_var_create();
    const surgescript_var_t* param[2] = { x, y };

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
        if(i % collectibles_per_circle == 0) {
            speed *= 0.5f;
            angle -= 180.0f;
        }
    }

    surgescript_var_destroy(y);
    surgescript_var_destroy(x);
}