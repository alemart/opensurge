/*
 * Open Surge Engine
 * on_event.c - Events: if an event is true, then the state is changed
 * Copyright (C) 2010-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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
 * Edits by Dalton Sterritt (the copyright was given to the project founder):
 * on_player_invincible
 */

#include <string.h>
#include "on_event.h"
#include "../object_vm.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/video.h"
#include "../../core/timer.h"
#include "../../core/input.h"
#include "../../core/audio.h"
#include "../../scenes/level.h"
#include "../player.h"

/* forward declarations */
typedef struct objectdecorator_onevent_t objectdecorator_onevent_t;
typedef struct eventstrategy_t eventstrategy_t;
typedef struct onalways_t onalways_t;
typedef struct ontimeout_t ontimeout_t;
typedef struct oncollision_t oncollision_t;
typedef struct onanimationfinished_t onanimationfinished_t;
typedef struct onrandomevent_t onrandomevent_t;
typedef struct onlevelcleared_t onlevelcleared_t;
typedef struct onplayercollision_t onplayercollision_t;
typedef struct onplayerattack_t onplayerattack_t;
typedef struct onplayerrectcollision_t onplayerrectcollision_t;
typedef struct onobservedplayer_t onobservedplayer_t;
typedef struct onplayerevent_t onplayerevent_t;
typedef struct onplayershield_t onplayershield_t;
typedef struct onbrickcollision_t onbrickcollision_t;
typedef struct onfloorcollision_t onfloorcollision_t;
typedef struct onceilingcollision_t onceilingcollision_t;
typedef struct onleftwallcollision_t onleftwallcollision_t;
typedef struct onrightwallcollision_t onrightwallcollision_t;
typedef struct onbutton_t onbutton_t;
typedef struct oncameraevent_t oncameraevent_t;
typedef struct oncameralock_t oncameralock_t;
typedef struct onmusicplay_t onmusicplay_t;

/* objectdecorator_onevent_t class */
struct objectdecorator_onevent_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *new_state_name; /* state name */
    eventstrategy_t *strategy; /* strategy pattern */
};

/* <<interface>> eventstrategy_t */
struct eventstrategy_t {
    void (*init)(eventstrategy_t*); /* initializes the strategy object */
    void (*release)(eventstrategy_t*); /* releases the strategy object */
    int (*should_trigger_event)(eventstrategy_t*,object_t*,player_t**,int,brick_list_t*,item_list_t*,object_list_t*); /* returns TRUE iff the event should be triggered */
};

/* onalways_t concrete strategy */
struct onalways_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onalways_new();
static void onalways_init(eventstrategy_t *event);
static void onalways_release(eventstrategy_t *event);
static int onalways_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* ontimeout_t concrete strategy */
struct ontimeout_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    expression_t *timeout; /* timeout value */
    float timer; /* time accumulator */
};
static eventstrategy_t* ontimeout_new(expression_t *timeout);
static void ontimeout_init(eventstrategy_t *event);
static void ontimeout_release(eventstrategy_t *event);
static int ontimeout_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* oncollision_t concrete strategy */
struct oncollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    char *target_name; /* object name */
};
static eventstrategy_t* oncollision_new(const char *target_name);
static void oncollision_init(eventstrategy_t *event);
static void oncollision_release(eventstrategy_t *event);
static int oncollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onanimationfinished_t concrete strategy */
struct onanimationfinished_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onanimationfinished_new();
static void onanimationfinished_init(eventstrategy_t *event);
static void onanimationfinished_release(eventstrategy_t *event);
static int onanimationfinished_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onrandomevent_t concrete strategy */
struct onrandomevent_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    expression_t *probability; /* 0.0 <= probability <= 1.0 */
};
static eventstrategy_t* onrandomevent_new(expression_t *probability);
static void onrandomevent_init(eventstrategy_t *event);
static void onrandomevent_release(eventstrategy_t *event);
static int onrandomevent_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onlevelcleared_t concrete strategy */
struct onlevelcleared_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onlevelcleared_new();
static void onlevelcleared_init(eventstrategy_t *event);
static void onlevelcleared_release(eventstrategy_t *event);
static int onlevelcleared_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onplayercollision_t concrete strategy */
struct onplayercollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onplayercollision_new();
static void onplayercollision_init(eventstrategy_t *event);
static void onplayercollision_release(eventstrategy_t *event);
static int onplayercollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onplayerattack_t concrete strategy */
struct onplayerattack_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onplayerattack_new();
static void onplayerattack_init(eventstrategy_t *event);
static void onplayerattack_release(eventstrategy_t *event);
static int onplayerattack_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onplayerrectcollision_t concrete strategy */
struct onplayerrectcollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    expression_t *x1, *y1, *x2, *y2; /* rectangle offsets (related to the hotspot of the holder object */
};
static eventstrategy_t* onplayerrectcollision_new(expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2);
static void onplayerrectcollision_init(eventstrategy_t *event);
static void onplayerrectcollision_release(eventstrategy_t *event);
static int onplayerrectcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onobservedplayer_t concrete strategy */
struct onobservedplayer_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    char *player_name; /* the event will be triggered if the observed player is called player_name */
};
static eventstrategy_t* onobservedplayer_new(const char *player_name);
static void onobservedplayer_init(eventstrategy_t *event);
static void onobservedplayer_release(eventstrategy_t *event);
static int onobservedplayer_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onplayerevent_t concrete strategy */
struct onplayerevent_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    int (*callback)(const player_t*);
};
static eventstrategy_t* onplayerevent_new(int (*callback)(const player_t*));
static void onplayerevent_init(eventstrategy_t *event);
static void onplayerevent_release(eventstrategy_t *event);
static int onplayerevent_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

static eventstrategy_t* onplayerstop_new() { return onplayerevent_new(player_is_stopped); }
static eventstrategy_t* onplayerwalk_new() { return onplayerevent_new(player_is_walking); }
static eventstrategy_t* onplayerrun_new() { return onplayerevent_new(player_is_running); }
static eventstrategy_t* onplayerjump_new() { return onplayerevent_new(player_is_jumping); }
static eventstrategy_t* onplayerspring_new() { return onplayerevent_new(player_is_springing); }
static eventstrategy_t* onplayerroll_new() { return onplayerevent_new(player_is_rolling); }
static eventstrategy_t* onplayerpush_new() { return onplayerevent_new(player_is_pushing); }
static eventstrategy_t* onplayergethit_new() { return onplayerevent_new(player_is_getting_hit); }
static eventstrategy_t* onplayerdeath_new() { return onplayerevent_new(player_is_dying); }
static eventstrategy_t* onplayerbrake_new() { return onplayerevent_new(player_is_braking); }
static eventstrategy_t* onplayerledge_new() { return onplayerevent_new(player_is_at_ledge); }
static eventstrategy_t* onplayerdrown_new() { return onplayerevent_new(player_is_drowning); }
static eventstrategy_t* onplayerbreathe_new() { return onplayerevent_new(player_is_breathing); }
static eventstrategy_t* onplayerduck_new() { return onplayerevent_new(player_is_ducking); }
static eventstrategy_t* onplayerlookup_new() { return onplayerevent_new(player_is_lookingup); }
static eventstrategy_t* onplayerwait_new() { return onplayerevent_new(player_is_waiting); }
static eventstrategy_t* onplayerwin_new() { return onplayerevent_new(player_is_winning); }
static eventstrategy_t* onplayerintheair_new() { return onplayerevent_new(player_is_in_the_air); }
static eventstrategy_t* onplayerunderwater_new() { return onplayerevent_new(player_is_underwater); }
static eventstrategy_t* onplayerspeedshoes_new() { return onplayerevent_new(player_is_ultrafast); }
static eventstrategy_t* onplayerinvincible_new() { return onplayerevent_new(player_is_invincible); }

/* onplayershield_t concrete strategy */
struct onplayershield_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    int shield_type; /* a SH_* constant defined at player.h */
};
static eventstrategy_t* onplayershield_new(int shield_type);
static void onplayershield_init(eventstrategy_t *event);
static void onplayershield_release(eventstrategy_t *event);
static int onplayershield_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onbrickcollision_t concrete strategy */
struct onbrickcollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onbrickcollision_new();
static void onbrickcollision_init(eventstrategy_t *event);
static void onbrickcollision_release(eventstrategy_t *event);
static int onbrickcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onfloorcollision_t concrete strategy */
struct onfloorcollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onfloorcollision_new();
static void onfloorcollision_init(eventstrategy_t *event);
static void onfloorcollision_release(eventstrategy_t *event);
static int onfloorcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onceilingcollision_t concrete strategy */
struct onceilingcollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onceilingcollision_new();
static void onceilingcollision_init(eventstrategy_t *event);
static void onceilingcollision_release(eventstrategy_t *event);
static int onceilingcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onleftwallcollision_t concrete strategy */
struct onleftwallcollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onleftwallcollision_new();
static void onleftwallcollision_init(eventstrategy_t *event);
static void onleftwallcollision_release(eventstrategy_t *event);
static int onleftwallcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onrightwallcollision_t concrete strategy */
struct onrightwallcollision_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onrightwallcollision_new();
static void onrightwallcollision_init(eventstrategy_t *event);
static void onrightwallcollision_release(eventstrategy_t *event);
static int onrightwallcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onbutton_t concrete strategy */
struct onbutton_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    inputbutton_t button;
    int (*check)(input_t*,inputbutton_t);
};
static eventstrategy_t* onbutton_new(const char *button_name, int (*check)(input_t*,inputbutton_t));
static void onbutton_init(eventstrategy_t *event);
static void onbutton_release(eventstrategy_t *event);
static int onbutton_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* oncameraevent_t concrete strategy */
struct oncameraevent_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    const actor_t* (*multiplexer)(object_t*); /* returns the correct actor: this object or the observed player? */
};
static eventstrategy_t* oncameraevent_new(const actor_t* (*mux)(object_t*));
static void oncameraevent_init(eventstrategy_t *event);
static void oncameraevent_release(eventstrategy_t *event);
static int oncameraevent_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

static const actor_t* oncameraevent_mux_object(object_t *o) { return o->actor; }
static const actor_t* oncameraevent_mux_observedplayer(object_t *o) { return enemy_get_observed_player(o)->actor; }

static eventstrategy_t* oncamerafocus_new() { return oncameraevent_new(oncameraevent_mux_object); }
static eventstrategy_t* oncamerafocusplayer_new() { return oncameraevent_new(oncameraevent_mux_observedplayer); }

/* oncameralock_t concrete strategy */
struct oncameralock_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* oncameralock_new();
static void oncameralock_init(eventstrategy_t *event);
static void oncameralock_release(eventstrategy_t *event);
static int oncameralock_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* onmusicplay_t concrete strategy */
struct onmusicplay_t {
    eventstrategy_t base; /* implements eventstrategy_t */
};
static eventstrategy_t* onmusicplay_new();
static void onmusicplay_init(eventstrategy_t *event);
static void onmusicplay_release(eventstrategy_t *event);
static int onmusicplay_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);


/* ------------------------------------- */


/* private methods */
static objectmachine_t *make_decorator(objectmachine_t *decorated_machine, const char *new_state_name, eventstrategy_t *strategy);
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);



/* ---------------------------------- */

/* public methods */

objectmachine_t* objectdecorator_onalways_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onalways_new());
}

objectmachine_t* objectdecorator_ontimeout_new(objectmachine_t *decorated_machine, expression_t *timeout, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, ontimeout_new(timeout));
}

objectmachine_t* objectdecorator_oncollision_new(objectmachine_t *decorated_machine, const char *target_name, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, oncollision_new(target_name));
}

objectmachine_t* objectdecorator_onanimationfinished_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onanimationfinished_new());
}

objectmachine_t* objectdecorator_onrandomevent_new(objectmachine_t *decorated_machine, expression_t *probability, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onrandomevent_new(probability));
}

objectmachine_t* objectdecorator_onlevelcleared_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onlevelcleared_new());
}

objectmachine_t* objectdecorator_onplayercollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayercollision_new());
}

objectmachine_t* objectdecorator_onplayerattack_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerattack_new());
}

objectmachine_t* objectdecorator_onplayerrectcollision_new(objectmachine_t *decorated_machine, expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerrectcollision_new(x1,y1,x2,y2));
}

objectmachine_t* objectdecorator_onobservedplayer_new(objectmachine_t *decorated_machine, const char *player_name, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onobservedplayer_new(player_name));
}

objectmachine_t* objectdecorator_onplayerstop_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerstop_new());
}

objectmachine_t* objectdecorator_onplayerwalk_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerwalk_new());
}

objectmachine_t* objectdecorator_onplayerrun_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerrun_new());
}

objectmachine_t* objectdecorator_onplayerjump_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerjump_new());
}


objectmachine_t* objectdecorator_onplayerroll_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerroll_new());
}

objectmachine_t* objectdecorator_onplayerspring_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerspring_new());
}


objectmachine_t* objectdecorator_onplayerpush_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerpush_new());
}


objectmachine_t* objectdecorator_onplayergethit_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayergethit_new());
}


objectmachine_t* objectdecorator_onplayerdeath_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerdeath_new());
}


objectmachine_t* objectdecorator_onplayerbrake_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerbrake_new());
}


objectmachine_t* objectdecorator_onplayerledge_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerledge_new());
}


objectmachine_t* objectdecorator_onplayerdrown_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerdrown_new());
}


objectmachine_t* objectdecorator_onplayerbreathe_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerbreathe_new());
}


objectmachine_t* objectdecorator_onplayerduck_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerduck_new());
}

objectmachine_t* objectdecorator_onplayerlookup_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerlookup_new());
}

objectmachine_t* objectdecorator_onplayerwait_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerwait_new());
}


objectmachine_t* objectdecorator_onplayerwin_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerwin_new());
}

objectmachine_t* objectdecorator_onplayerintheair_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerintheair_new());
}

objectmachine_t* objectdecorator_onplayerunderwater_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerunderwater_new());
}

objectmachine_t* objectdecorator_onplayerspeedshoes_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerspeedshoes_new());
}

objectmachine_t* objectdecorator_onplayerinvincible_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayerinvincible_new());
}

objectmachine_t* objectdecorator_onnoshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_NONE));
}

objectmachine_t* objectdecorator_onshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_SHIELD));
}

objectmachine_t* objectdecorator_onfireshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_FIRESHIELD));
}

objectmachine_t* objectdecorator_onthundershield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_THUNDERSHIELD));
}

objectmachine_t* objectdecorator_onwatershield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_WATERSHIELD));
}

objectmachine_t* objectdecorator_onacidshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_ACIDSHIELD));
}

objectmachine_t* objectdecorator_onwindshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_WINDSHIELD));
}

objectmachine_t* objectdecorator_onbrickcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onbrickcollision_new());
}

objectmachine_t* objectdecorator_onfloorcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onfloorcollision_new());
}

objectmachine_t* objectdecorator_onceilingcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onceilingcollision_new());
}

objectmachine_t* objectdecorator_onleftwallcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onleftwallcollision_new());
}

objectmachine_t* objectdecorator_onrightwallcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onrightwallcollision_new());
}

objectmachine_t* objectdecorator_onbuttondown_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onbutton_new(button_name, input_button_down));
}

objectmachine_t* objectdecorator_onbuttonpressed_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onbutton_new(button_name, input_button_pressed));
}

objectmachine_t* objectdecorator_onbuttonup_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onbutton_new(button_name, input_button_up));
}

objectmachine_t* objectdecorator_onmusicplay_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, onmusicplay_new());
}

objectmachine_t* objectdecorator_oncamerafocus_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, oncamerafocus_new());
}

objectmachine_t* objectdecorator_oncamerafocusplayer_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, oncamerafocusplayer_new());
}

objectmachine_t* objectdecorator_oncameralock_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return make_decorator(decorated_machine, new_state_name, oncameralock_new());
}

/* ---------------------------------- */

/* private methods */

objectmachine_t *make_decorator(objectmachine_t *decorated_machine, const char *new_state_name, eventstrategy_t *strategy)
{
    objectdecorator_onevent_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->new_state_name = str_dup(new_state_name);
    me->strategy = strategy;

    return obj;
}

void init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_onevent_t *me = (objectdecorator_onevent_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    me->strategy->init(me->strategy);

    decorated_machine->init(decorated_machine);
}

void release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_onevent_t *me = (objectdecorator_onevent_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    me->strategy->release(me->strategy);
    free(me->strategy);
    free(me->new_state_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_onevent_t *me = (objectdecorator_onevent_t*)obj;
    object_t *object = obj->get_object_instance(obj);

    if(me->strategy->should_trigger_event(me->strategy, object, team, team_size, brick_list, item_list, object_list))
        objectvm_set_current_state(object->vm, me->new_state_name);
    else
        decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}


/* ---------------------------------- */

/* onalways_t strategy */
eventstrategy_t* onalways_new()
{
    onalways_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onalways_init;
    e->release = onalways_release;
    e->should_trigger_event = onalways_should_trigger_event;

    return e;
}

void onalways_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onalways_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onalways_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    return TRUE;
}

/* ontimeout_t strategy */
eventstrategy_t* ontimeout_new(expression_t *timeout)
{
    ontimeout_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = ontimeout_init;
    e->release = ontimeout_release;
    e->should_trigger_event = ontimeout_should_trigger_event;

    x->timeout = timeout;
    x->timer = 0.0f;

    return e;
}

void ontimeout_init(eventstrategy_t *event)
{
    ; /* empty */
}

void ontimeout_release(eventstrategy_t *event)
{
    ontimeout_t *x = (ontimeout_t*)event;
    expression_destroy(x->timeout);
}

int ontimeout_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    ontimeout_t *x = (ontimeout_t*)event;
    float timeout = expression_evaluate(x->timeout);

    x->timer += timer_get_delta();
    if(x->timer >= timeout) {
        x->timer = 0.0f;
        return TRUE;
    }

    return FALSE;
}

/* oncollision_t strategy */
eventstrategy_t* oncollision_new(const char *target_name)
{
    oncollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = oncollision_init;
    e->release = oncollision_release;
    e->should_trigger_event = oncollision_should_trigger_event;
    x->target_name = str_dup(target_name);

    return e;
}

void oncollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void oncollision_release(eventstrategy_t *event)
{
    oncollision_t *e = (oncollision_t*)event;
    free(e->target_name);
}

int oncollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    oncollision_t *x = (oncollision_t*)event;
    object_list_t *it;

    for(it = object_list; it != NULL; it = it->next) {
        if(strcmp(it->data->name, x->target_name) == 0) {
            if(actor_pixelperfect_collision(it->data->actor, object->actor))
                return TRUE;
        }
    }

    return FALSE;
}


/* onanimationfinished_t strategy */
eventstrategy_t* onanimationfinished_new()
{
    onanimationfinished_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onanimationfinished_init;
    e->release = onanimationfinished_release;
    e->should_trigger_event = onanimationfinished_should_trigger_event;

    return e;
}

void onanimationfinished_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onanimationfinished_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onanimationfinished_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    return actor_animation_finished(object->actor);
}

/* onrandomevent_t strategy */
eventstrategy_t* onrandomevent_new(expression_t *probability)
{
    onrandomevent_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onrandomevent_init;
    e->release = onrandomevent_release;
    e->should_trigger_event = onrandomevent_should_trigger_event;

    x->probability = probability;

    return e;
}

void onrandomevent_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onrandomevent_release(eventstrategy_t *event)
{
    onrandomevent_t *x = (onrandomevent_t*)event;
    expression_destroy(x->probability);
}

int onrandomevent_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    onrandomevent_t *x = (onrandomevent_t*)event;
    float probability = clip(expression_evaluate(x->probability), 0.0f, 1.0f);
    return (int)(100000 * probability) > random(100000);
}


/* onlevelcleared_t strategy */
eventstrategy_t* onlevelcleared_new()
{
    onlevelcleared_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onlevelcleared_init;
    e->release = onlevelcleared_release;
    e->should_trigger_event = onlevelcleared_should_trigger_event;

    return e;
}

void onlevelcleared_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onlevelcleared_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onlevelcleared_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    return level_has_been_cleared();
}



/* onplayercollision_t strategy */
eventstrategy_t* onplayercollision_new()
{
    onplayercollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onplayercollision_init;
    e->release = onplayercollision_release;
    e->should_trigger_event = onplayercollision_should_trigger_event;

    return e;
}

void onplayercollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onplayercollision_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onplayercollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    player_t *player = enemy_get_observed_player(object);
    return actor_pixelperfect_collision(object->actor, player->actor);
}


/* onplayerattack_t strategy */
eventstrategy_t* onplayerattack_new()
{
    onplayerattack_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onplayerattack_init;
    e->release = onplayerattack_release;
    e->should_trigger_event = onplayerattack_should_trigger_event;

    return e;
}

void onplayerattack_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onplayerattack_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onplayerattack_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    player_t *player = enemy_get_observed_player(object);
    return player_is_attacking(player) && actor_pixelperfect_collision(object->actor, player->actor);
}


/* onplayerrectcollision_t strategy */
eventstrategy_t* onplayerrectcollision_new(expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2)
{
    onplayerrectcollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onplayerrectcollision_init;
    e->release = onplayerrectcollision_release;
    e->should_trigger_event = onplayerrectcollision_should_trigger_event;

    x->x1 = x1;
    x->y1 = y1;
    x->x2 = x2;
    x->y2 = y2;

    return e;
}

void onplayerrectcollision_init(eventstrategy_t *event)
{
    onplayerrectcollision_t *x = (onplayerrectcollision_t*)event;
    int x1 = (int)expression_evaluate(x->x1);
    int x2 = (int)expression_evaluate(x->x2);
    int y1 = (int)expression_evaluate(x->y1);
    int y2 = (int)expression_evaluate(x->y2);

    if(!(x2 > x1 && y2 > y1))
        fatal_error("The rectangle (x1,y1,x2,y2) given to on_player_rect_collision must be such that x2 > x1 and y2 > y1");
}

void onplayerrectcollision_release(eventstrategy_t *event)
{
    onplayerrectcollision_t *x = (onplayerrectcollision_t*)event;
    expression_destroy(x->x1);
    expression_destroy(x->x2);
    expression_destroy(x->y1);
    expression_destroy(x->y2);
}

int onplayerrectcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    onplayerrectcollision_t *me = (onplayerrectcollision_t*)event;
    actor_t *act = object->actor;
    player_t *player = enemy_get_observed_player(object);
    actor_t *pa = player->actor;
    image_t *pi = actor_image(pa);
    int x1 = (int)expression_evaluate(me->x1);
    int x2 = (int)expression_evaluate(me->x2);
    int y1 = (int)expression_evaluate(me->y1);
    int y2 = (int)expression_evaluate(me->y2);
    float a[4], b[4];

    a[0] = act->position.x + x1;
    a[1] = act->position.y + y1;
    a[2] = act->position.x + x2;
    a[3] = act->position.y + y2;

    b[0] = pa->position.x - pa->hot_spot.x;
    b[1] = pa->position.y - pa->hot_spot.y;
    b[2] = pa->position.x - pa->hot_spot.x + image_width(pi);
    b[3] = pa->position.y - pa->hot_spot.y + image_height(pi);

    return !player_is_dying(player) && bounding_box(a, b);
}

/* onobservedplayer_t strategy */
eventstrategy_t* onobservedplayer_new(const char *player_name)
{
    onobservedplayer_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onobservedplayer_init;
    e->release = onobservedplayer_release;
    e->should_trigger_event = onobservedplayer_should_trigger_event;

    x->player_name = str_dup(player_name);

    return e;
}

void onobservedplayer_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onobservedplayer_release(eventstrategy_t *event)
{
    onobservedplayer_t *x = (onobservedplayer_t*)event;
    free(x->player_name);
}

int onobservedplayer_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    onobservedplayer_t *x = (onobservedplayer_t*)event;
    player_t *player = enemy_get_observed_player(object);
    return str_icmp(player->name, x->player_name) == 0;
}

/* onplayerevent_t strategy */
eventstrategy_t* onplayerevent_new(int (*callback)(const player_t*))
{
    onplayerevent_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    x->callback = callback;
    e->init = onplayerevent_init;
    e->release = onplayerevent_release;
    e->should_trigger_event = onplayerevent_should_trigger_event;

    return e;
}

void onplayerevent_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onplayerevent_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onplayerevent_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    player_t *player = enemy_get_observed_player(object);
    return ((onplayerevent_t*)event)->callback(player);
}


/* onplayershield_t strategy */
eventstrategy_t* onplayershield_new(int shield_type)
{
    onplayershield_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onplayershield_init;
    e->release = onplayershield_release;
    e->should_trigger_event = onplayershield_should_trigger_event;
    x->shield_type = shield_type;

    return e;
}

void onplayershield_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onplayershield_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onplayershield_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    onplayershield_t *me = (onplayershield_t*)event;
    player_t *player = enemy_get_observed_player(object);

    return player->shield_type == me->shield_type;
}


/* onbrickcollision_t strategy */
eventstrategy_t* onbrickcollision_new()
{
    onbrickcollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onbrickcollision_init;
    e->release = onbrickcollision_release;
    e->should_trigger_event = onbrickcollision_should_trigger_event;

    return e;
}

void onbrickcollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onbrickcollision_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onbrickcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    actor_t *act = object->actor;
    brick_t *up, *upright, *right, *downright, *down, *downleft, *left, *upleft;

    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    return
        (up != NULL && up->brick_ref->property == BRK_OBSTACLE) ||
        (upright != NULL && upright->brick_ref->property == BRK_OBSTACLE) ||
        (right != NULL && right->brick_ref->property == BRK_OBSTACLE) ||
        (downright != NULL && downright->brick_ref->property != BRK_NONE) ||
        (down != NULL && down->brick_ref->property != BRK_NONE) ||
        (downleft != NULL && downleft->brick_ref->property != BRK_NONE) ||
        (left != NULL && left->brick_ref->property == BRK_OBSTACLE) ||
        (upleft != NULL && upleft->brick_ref->property == BRK_OBSTACLE)
    ;
}

/* onfloorcollision_t strategy */
eventstrategy_t* onfloorcollision_new()
{
    onfloorcollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onfloorcollision_init;
    e->release = onfloorcollision_release;
    e->should_trigger_event = onfloorcollision_should_trigger_event;

    return e;
}

void onfloorcollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onfloorcollision_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onfloorcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    actor_t *act = object->actor;
    brick_t *up, *upright, *right, *downright, *down, *downleft, *left, *upleft;

    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    return
        (downright != NULL && downright->brick_ref->property != BRK_NONE) ||
        (down != NULL && down->brick_ref->property != BRK_NONE) ||
        (downleft != NULL && downleft->brick_ref->property != BRK_NONE)
    ;
}

/* onceilingcollision_t strategy */
eventstrategy_t* onceilingcollision_new()
{
    onceilingcollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onceilingcollision_init;
    e->release = onceilingcollision_release;
    e->should_trigger_event = onceilingcollision_should_trigger_event;

    return e;
}

void onceilingcollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onceilingcollision_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onceilingcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    actor_t *act = object->actor;
    brick_t *up, *upright, *right, *downright, *down, *downleft, *left, *upleft;

    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    return
        (upleft != NULL && upleft->brick_ref->property == BRK_OBSTACLE) ||
        (up != NULL && up->brick_ref->property == BRK_OBSTACLE) ||
        (upright != NULL && upright->brick_ref->property == BRK_OBSTACLE)
    ;
}

/* onleftwallcollision_t strategy */
eventstrategy_t* onleftwallcollision_new()
{
    onleftwallcollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onleftwallcollision_init;
    e->release = onleftwallcollision_release;
    e->should_trigger_event = onleftwallcollision_should_trigger_event;

    return e;
}

void onleftwallcollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onleftwallcollision_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onleftwallcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    actor_t *act = object->actor;
    brick_t *up, *upright, *right, *downright, *down, *downleft, *left, *upleft;

    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    return
        (left != NULL && left->brick_ref->property == BRK_OBSTACLE) ||
        (upleft != NULL && upleft->brick_ref->property == BRK_OBSTACLE)
    ;
}

/* onrightwallcollision_t strategy */
eventstrategy_t* onrightwallcollision_new()
{
    onrightwallcollision_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onrightwallcollision_init;
    e->release = onrightwallcollision_release;
    e->should_trigger_event = onrightwallcollision_should_trigger_event;

    return e;
}

void onrightwallcollision_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onrightwallcollision_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onrightwallcollision_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    actor_t *act = object->actor;
    brick_t *up, *upright, *right, *downright, *down, *downleft, *left, *upleft;

    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    return
        (right != NULL && right->brick_ref->property == BRK_OBSTACLE) ||
        (upright != NULL && upright->brick_ref->property == BRK_OBSTACLE)
    ;
}

/* onbutton_t strategy */
eventstrategy_t* onbutton_new(const char *button_name, int (*check)(input_t*,inputbutton_t))
{
    onbutton_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onbutton_init;
    e->release = onbutton_release;
    e->should_trigger_event = onbutton_should_trigger_event;

    x->check = check;
    x->button = IB_UP;

    if(str_icmp(button_name, "up") == 0)
        x->button = IB_UP;
    else if(str_icmp(button_name, "right") == 0)
        x->button = IB_RIGHT;
    else if(str_icmp(button_name, "down") == 0)
        x->button = IB_DOWN;
    else if(str_icmp(button_name, "left") == 0)
        x->button = IB_LEFT;
    else if(str_icmp(button_name, "fire1") == 0)
        x->button = IB_FIRE1;
    else if(str_icmp(button_name, "fire2") == 0)
        x->button = IB_FIRE2;
    else if(str_icmp(button_name, "fire3") == 0)
        x->button = IB_FIRE3;
    else if(str_icmp(button_name, "fire4") == 0)
        x->button = IB_FIRE4;
    else if(str_icmp(button_name, "fire5") == 0)
        x->button = IB_FIRE5;
    else if(str_icmp(button_name, "fire6") == 0)
        x->button = IB_FIRE6;
    else if(str_icmp(button_name, "fire7") == 0)
        x->button = IB_FIRE7;
    else if(str_icmp(button_name, "fire8") == 0)
        x->button = IB_FIRE8;
    else
        fatal_error(
            "Invalid button '%s' in on_button_%s event",
            button_name,
            ((check == input_button_down) ? "down" : ((check == input_button_pressed) ? "pressed" : "up"))
        );

    return e;
}

void onbutton_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onbutton_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onbutton_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    onbutton_t *me = (onbutton_t*)event;
    player_t *player = enemy_get_observed_player(object);
    return me->check(player->actor->input, me->button);
}

/* oncameraevent_t strategy */
eventstrategy_t* oncameraevent_new(const actor_t* (*mux)(object_t*))
{
    oncameraevent_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    x->multiplexer = mux;
    e->init = oncameraevent_init;
    e->release = oncameraevent_release;
    e->should_trigger_event = oncameraevent_should_trigger_event;

    return e;
}

void oncameraevent_init(eventstrategy_t *event)
{
    ; /* empty */
}

void oncameraevent_release(eventstrategy_t *event)
{
    ; /* empty */
}

int oncameraevent_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    return level_get_camera_focus() == ((oncameraevent_t*)event)->multiplexer(object);
}


/* oncameralock_t strategy */
eventstrategy_t* oncameralock_new()
{
    oncameralock_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = oncameralock_init;
    e->release = oncameralock_release;
    e->should_trigger_event = oncameralock_should_trigger_event;

    return e;
}

void oncameralock_init(eventstrategy_t *event)
{
    ; /* empty */
}

void oncameralock_release(eventstrategy_t *event)
{
    ; /* empty */
}

int oncameralock_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    return level_is_camera_locked();
    /*player_t *player = enemy_get_observed_player(object);
    return player->in_locked_area;*/
}




/* onmusicplay_t strategy */
eventstrategy_t* onmusicplay_new()
{
    onmusicplay_t *x = mallocx(sizeof *x);
    eventstrategy_t *e = (eventstrategy_t*)x;

    e->init = onmusicplay_init;
    e->release = onmusicplay_release;
    e->should_trigger_event = onmusicplay_should_trigger_event;

    return e;
}

void onmusicplay_init(eventstrategy_t *event)
{
    ; /* empty */
}

void onmusicplay_release(eventstrategy_t *event)
{
    ; /* empty */
}

int onmusicplay_should_trigger_event(eventstrategy_t *event, object_t *object, player_t** team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    return music_is_playing();
}


