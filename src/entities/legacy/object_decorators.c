/*
 * Open Surge Engine
 * object_decorators.c - Legacy scripting API: commands
 * Copyright (C) 2010-2013, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <math.h>
#include <string.h>
#include "object_decorators.h"
#include "object_vm.h"
#include "nanocalc/nanocalc.h"
#include "nanocalc/nanocalc_addons.h"
#include "nanocalc/nanocalcext.h"
#include "../actor.h"
#include "../player.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/audio.h"
#include "../../core/soundfactory.h"
#include "../../core/timer.h"
#include "../../core/web.h"
#include "../../core/video.h"
#include "../../core/image.h"
#include "../../core/font.h"
#include "../../core/input.h"
#include "../../core/v2d.h"
#include "../../scenes/level.h"
#include "../../physics/obstacle.h"



/* ----- DECORATOR: definition ----- */

/* <<abstract>> object decorator class */
typedef struct objectdecorator_t objectdecorator_t;
struct objectdecorator_t {
    objectmachine_t base; /* objectdecorator_t implements the objectmachine_t interface */
    objectmachine_t *decorated_machine; /* what are we decorating? */
};

/* given an object machine, get its object instance */
object_t* objectdecorator_get_object_instance(objectmachine_t *obj)
{
    objectdecorator_t *me = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = me->decorated_machine;
    return decorated_machine->get_object_instance(decorated_machine);
}



/* ----- COMMANDS ----- */

/* objectdecorator_addcollectibles_t class */
typedef struct objectdecorator_addcollectibles_t objectdecorator_addcollectibles_t;
struct objectdecorator_addcollectibles_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *collectibles; /* how many collectibles will be added? */
};

/* private methods */
static void addcollectibles_init(objectmachine_t *obj);
static void addcollectibles_release(objectmachine_t *obj);
static void addcollectibles_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void addcollectibles_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_addcollectibles_new(objectmachine_t *decorated_machine, expression_t *collectibles)
{
    objectdecorator_addcollectibles_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = addcollectibles_init;
    obj->release = addcollectibles_release;
    obj->update = addcollectibles_update;
    obj->render = addcollectibles_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->collectibles = collectibles;

    return obj;
}




/* private methods */
void addcollectibles_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void addcollectibles_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_addcollectibles_t *me = (objectdecorator_addcollectibles_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    expression_destroy(me->collectibles);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void addcollectibles_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_addcollectibles_t *me = (objectdecorator_addcollectibles_t*)obj;

    player_set_collectibles( player_get_collectibles() + (int)expression_evaluate(me->collectibles) );

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void addcollectibles_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_addlives_t class */
typedef struct objectdecorator_addlives_t objectdecorator_addlives_t;
struct objectdecorator_addlives_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *lives; /* how many lives will be added? */
};

/* private methods */
static void addlives_init(objectmachine_t *obj);
static void addlives_release(objectmachine_t *obj);
static void addlives_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void addlives_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_addlives_new(objectmachine_t *decorated_machine, expression_t *lives)
{
    objectdecorator_addlives_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = addlives_init;
    obj->release = addlives_release;
    obj->update = addlives_update;
    obj->render = addlives_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->lives = lives;

    return obj;
}




/* private methods */
void addlives_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void addlives_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_addlives_t *me = (objectdecorator_addlives_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    expression_destroy(me->lives);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void addlives_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_addlives_t *me = (objectdecorator_addlives_t*)obj;

    player_set_lives( player_get_lives() + (int)expression_evaluate(me->lives) );

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void addlives_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_addtoscore_t class */
typedef struct objectdecorator_addtoscore_t objectdecorator_addtoscore_t;
struct objectdecorator_addtoscore_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *score; /* score to be added */
};

/* private methods */
static void addtoscore_init(objectmachine_t *obj);
static void addtoscore_release(objectmachine_t *obj);
static void addtoscore_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void addtoscore_render(objectmachine_t *obj, v2d_t camera_position);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_addtoscore_new(objectmachine_t *decorated_machine, expression_t *score)
{
    objectdecorator_addtoscore_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = addtoscore_init;
    obj->release = addtoscore_release;
    obj->update = addtoscore_update;
    obj->render = addtoscore_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->score = score;

    return obj;
}




/* private methods */
void addtoscore_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void addtoscore_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_addtoscore_t *me = (objectdecorator_addtoscore_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    expression_destroy(me->score);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void addtoscore_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_addtoscore_t *me = (objectdecorator_addtoscore_t*)obj;

    level_add_to_score( (int)expression_evaluate(me->score) );

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void addtoscore_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_asktoleave_t class */
typedef struct objectdecorator_asktoleave_t objectdecorator_asktoleave_t;
struct objectdecorator_asktoleave_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void asktoleave_init(objectmachine_t *obj);
static void asktoleave_release(objectmachine_t *obj);
static void asktoleave_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void asktoleave_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_asktoleave_new(objectmachine_t *decorated_machine)
{
    objectdecorator_asktoleave_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = asktoleave_init;
    obj->release = asktoleave_release;
    obj->update = asktoleave_update;
    obj->render = asktoleave_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void asktoleave_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void asktoleave_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void asktoleave_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    level_ask_to_leave();

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void asktoleave_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}


/* objectdecorator_attachtoplayer_t class */
typedef struct objectdecorator_attachtoplayer_t objectdecorator_attachtoplayer_t;
struct objectdecorator_attachtoplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *offset_x, *offset_y;
};

/* private methods */
static void attachtoplayer_init(objectmachine_t *obj);
static void attachtoplayer_release(objectmachine_t *obj);
static void attachtoplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void attachtoplayer_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_attachtoplayer_new(objectmachine_t *decorated_machine, expression_t *offset_x, expression_t *offset_y)
{
    objectdecorator_attachtoplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = attachtoplayer_init;
    obj->release = attachtoplayer_release;
    obj->update = attachtoplayer_update;
    obj->render = attachtoplayer_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->offset_x = offset_x;
    me->offset_y = offset_y;

    return obj;
}





/* private methods */
void attachtoplayer_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void attachtoplayer_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_attachtoplayer_t *me = (objectdecorator_attachtoplayer_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    expression_destroy(me->offset_x);
    expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void attachtoplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_attachtoplayer_t *me = (objectdecorator_attachtoplayer_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    float player_direction = (player->actor->mirror & IF_HFLIP) ? -1.0f : 1.0f;
    v2d_t offset = v2d_new(player_direction * expression_evaluate(me->offset_x), expression_evaluate(me->offset_y));

    object->attached_to_player = TRUE;
    object->attached_to_player_offset = v2d_rotate(offset, -player->actor->angle);
    object->actor->position = v2d_add(player->actor->position, object->attached_to_player_offset);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void attachtoplayer_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

typedef struct objectdecorator_audio_t objectdecorator_audio_t;
typedef struct audiostrategy_t audiostrategy_t;
typedef struct playsamplestrategy_t playsamplestrategy_t;
typedef struct playmusicstrategy_t playmusicstrategy_t;
typedef struct playlevelmusicstrategy_t playlevelmusicstrategy_t;
typedef struct setmusicvolumestrategy_t setmusicvolumestrategy_t;
typedef struct stopsamplestrategy_t stopsamplestrategy_t;

/* objectdecorator_audio_t class */
struct objectdecorator_audio_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    audiostrategy_t *strategy;
};

/* strategy pattern */
struct audiostrategy_t {
    void (*update)(audiostrategy_t*);
    void (*release)(audiostrategy_t*);
};

/* play_sample strategy */
struct playsamplestrategy_t {
    audiostrategy_t base;
    sound_t *sfx; /* sample to be played */
    expression_t *vol, *pan, *freq;
    expression_t *loop;
};

static audiostrategy_t* playsamplestrategy_new(const char *sample_name, expression_t *vol, expression_t *pan, expression_t *freq, expression_t *loop);
static void playsamplestrategy_update(audiostrategy_t *s);
static void playsamplestrategy_release(audiostrategy_t *s);

/* play_music strategy */
struct playmusicstrategy_t {
    audiostrategy_t base;
    music_t *mus; /* music to be played */
    expression_t *loop;
};

static audiostrategy_t* playmusicstrategy_new(const char *music_name, expression_t *loop);
static void playmusicstrategy_update(audiostrategy_t *s);
static void playmusicstrategy_release(audiostrategy_t *s);

/* set_music_volume strategy */
struct setmusicvolumestrategy_t {
    audiostrategy_t base;
    expression_t *vol;
};

static audiostrategy_t* setmusicvolumestrategy_new(expression_t *vol);
static void setmusicvolumestrategy_update(audiostrategy_t *s);
static void setmusicvolumestrategy_release(audiostrategy_t *s);

/* play_level_music strategy */
struct playlevelmusicstrategy_t {
    audiostrategy_t base;
};

static audiostrategy_t* playlevelmusicstrategy_new();
static void playlevelmusicstrategy_update(audiostrategy_t *s);
static void playlevelmusicstrategy_release(audiostrategy_t *s);

/* stop_sample strategy */
struct stopsamplestrategy_t {
    audiostrategy_t base;
    sound_t *sfx; /* sample to be stoped */
};

static audiostrategy_t* stopsamplestrategy_new(const char *sample_name);
static void stopsamplestrategy_update(audiostrategy_t *s);
static void stopsamplestrategy_release(audiostrategy_t *s);

/* private methods */
static void audiocommand_init(objectmachine_t *obj);
static void audiocommand_release(objectmachine_t *obj);
static void audiocommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void audiocommand_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* audiocommand_make_decorator(objectmachine_t *decorated_machine, audiostrategy_t *strategy);



/* public methods */

/* class constructors */
objectmachine_t* objectdecorator_playsample_new(objectmachine_t *decorated_machine, const char *sample_name, expression_t *vol, expression_t *pan, expression_t *freq, expression_t *loop)
{
    return audiocommand_make_decorator(decorated_machine, playsamplestrategy_new(sample_name, vol, pan, freq, loop));
}

objectmachine_t* objectdecorator_playmusic_new(objectmachine_t *decorated_machine, const char *music_name, expression_t *loop)
{
    return audiocommand_make_decorator(decorated_machine, playmusicstrategy_new(music_name, loop));
}

objectmachine_t* objectdecorator_playlevelmusic_new(objectmachine_t *decorated_machine)
{
    return audiocommand_make_decorator(decorated_machine, playlevelmusicstrategy_new());
}

objectmachine_t* objectdecorator_setmusicvolume_new(objectmachine_t *decorated_machine, expression_t *vol)
{
    return audiocommand_make_decorator(decorated_machine, setmusicvolumestrategy_new(vol));
}

objectmachine_t* objectdecorator_stopsample_new(objectmachine_t *decorated_machine, const char *sample_name)
{
    return audiocommand_make_decorator(decorated_machine, stopsamplestrategy_new(sample_name));
}



/* private methods */
void audiocommand_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void audiocommand_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_audio_t *me = (objectdecorator_audio_t*)obj;

    me->strategy->release(me->strategy);
    free(me->strategy);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void audiocommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_audio_t *me = (objectdecorator_audio_t*)obj;

    me->strategy->update(me->strategy);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void audiocommand_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}






/* private stuff */
objectmachine_t* audiocommand_make_decorator(objectmachine_t *decorated_machine, audiostrategy_t *strategy)
{
    objectdecorator_audio_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = audiocommand_init;
    obj->release = audiocommand_release;
    obj->update = audiocommand_update;
    obj->render = audiocommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->strategy = strategy;

    return obj;
}

/* ------------------------ */

audiostrategy_t* playsamplestrategy_new(const char *sample_name, expression_t *vol, expression_t *pan, expression_t *freq, expression_t *loop)
{
    playsamplestrategy_t *s = mallocx(sizeof *s);
    ((audiostrategy_t*)s)->update = playsamplestrategy_update;
    ((audiostrategy_t*)s)->release = playsamplestrategy_release;

    s->sfx = sound_load(sample_name);
    s->vol = vol;
    s->pan = pan;
    s->freq = freq;
    s->loop = loop;

    return (audiostrategy_t*)s;
}

void playsamplestrategy_update(audiostrategy_t *s)
{
    playsamplestrategy_t *me = (playsamplestrategy_t*)s;
    float vol, pan, freq;

    vol = clip(expression_evaluate(me->vol), 0.0f, 1.0f);
    pan = clip(expression_evaluate(me->pan), -1.0f, 1.0f);
    freq = expression_evaluate(me->freq);
    /*loop = expression_evaluate(me->loop);*/ /* deprecated */

    sound_play_ex(me->sfx, vol, pan, freq);
}

void playsamplestrategy_release(audiostrategy_t *s)
{
    playsamplestrategy_t *me = (playsamplestrategy_t*)s;

    expression_destroy(me->vol);
    expression_destroy(me->pan);
    expression_destroy(me->freq);
    expression_destroy(me->loop);
}

/* ------------------------ */

audiostrategy_t* playmusicstrategy_new(const char *music_name, expression_t *loop)
{
    playmusicstrategy_t *s = mallocx(sizeof *s);
    ((audiostrategy_t*)s)->update = playmusicstrategy_update;
    ((audiostrategy_t*)s)->release = playmusicstrategy_release;

    s->mus = music_load(music_name);
    s->loop = loop;

    return (audiostrategy_t*)s;
}

void playmusicstrategy_update(audiostrategy_t *s)
{
    playmusicstrategy_t *me = (playmusicstrategy_t*)s;
    int loop = expression_evaluate(me->loop);

    music_play(me->mus, (loop != 0));
}

void playmusicstrategy_release(audiostrategy_t *s)
{
    playmusicstrategy_t *me = (playmusicstrategy_t*)s;
    expression_destroy(me->loop);
    music_unref(me->mus);
}

/* ------------------------ */

audiostrategy_t* playlevelmusicstrategy_new()
{
    playmusicstrategy_t *s = mallocx(sizeof *s);
    ((audiostrategy_t*)s)->update = playlevelmusicstrategy_update;
    ((audiostrategy_t*)s)->release = playlevelmusicstrategy_release;

    return (audiostrategy_t*)s;
}

void playlevelmusicstrategy_update(audiostrategy_t *s)
{
    if(level_music() != NULL)
        music_play(level_music(), TRUE);
}

void playlevelmusicstrategy_release(audiostrategy_t *s)
{
    /* empty */
}

/* ------------------------ */

audiostrategy_t* setmusicvolumestrategy_new(expression_t *vol)
{
    setmusicvolumestrategy_t *s = mallocx(sizeof *s);
    ((audiostrategy_t*)s)->update = setmusicvolumestrategy_update;
    ((audiostrategy_t*)s)->release = setmusicvolumestrategy_release;

    s->vol = vol;

    return (audiostrategy_t*)s;
}

void setmusicvolumestrategy_update(audiostrategy_t *s)
{
    setmusicvolumestrategy_t *me = (setmusicvolumestrategy_t*)s;
    float vol;

    vol = clip(expression_evaluate(me->vol), 0.0f, 1.0f);
    music_set_volume(vol);
}

void setmusicvolumestrategy_release(audiostrategy_t *s)
{
    setmusicvolumestrategy_t *me = (setmusicvolumestrategy_t*)s;
    expression_destroy(me->vol);
}

/* ------------------------ */

audiostrategy_t* stopsamplestrategy_new(const char *sample_name)
{
    stopsamplestrategy_t *s = mallocx(sizeof *s);
    ((audiostrategy_t*)s)->update = stopsamplestrategy_update;
    ((audiostrategy_t*)s)->release = stopsamplestrategy_release;

    s->sfx = sound_load(sample_name);

    return (audiostrategy_t*)s;
}

void stopsamplestrategy_update(audiostrategy_t *s)
{
    stopsamplestrategy_t *me = (stopsamplestrategy_t*)s;
    sound_stop(me->sfx);
}

void stopsamplestrategy_release(audiostrategy_t *s)
{
    /* empty */
}

/* objectdecorator_bounceplayer_t class */
typedef struct objectdecorator_bounceplayer_t objectdecorator_bounceplayer_t;
struct objectdecorator_bounceplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void bounceplayer_init(objectmachine_t *obj);
static void bounceplayer_release(objectmachine_t *obj);
static void bounceplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void bounceplayer_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_bounceplayer_new(objectmachine_t *decorated_machine)
{
    objectdecorator_bounceplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = bounceplayer_init;
    obj->release = bounceplayer_release;
    obj->update = bounceplayer_update;
    obj->render = bounceplayer_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void bounceplayer_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void bounceplayer_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void bounceplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    player_bounce_ex(player, object->actor, FALSE);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void bounceplayer_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}


/* objectdecorator_bullettrajectory_t class */
typedef struct objectdecorator_bullettrajectory_t objectdecorator_bullettrajectory_t;
struct objectdecorator_bullettrajectory_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed_x, *speed_y; /* bullet speed */
};

/* private methods */
static void bullettrajectory_init(objectmachine_t *obj);
static void bullettrajectory_release(objectmachine_t *obj);
static void bullettrajectory_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void bullettrajectory_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_bullettrajectory_new(objectmachine_t *decorated_machine, expression_t *speed_x, expression_t *speed_y)
{
    objectdecorator_bullettrajectory_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = bullettrajectory_init;
    obj->release = bullettrajectory_release;
    obj->update = bullettrajectory_update;
    obj->render = bullettrajectory_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed_x = speed_x;
    me->speed_y = speed_y;

    return obj;
}





/* private methods */
void bullettrajectory_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void bullettrajectory_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_bullettrajectory_t *me = (objectdecorator_bullettrajectory_t*)obj;

    expression_destroy(me->speed_x);
    expression_destroy(me->speed_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void bullettrajectory_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_bullettrajectory_t *me = (objectdecorator_bullettrajectory_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float dt = timer_get_delta();
    v2d_t ds, speed;

    speed = v2d_new(expression_evaluate(me->speed_x), expression_evaluate(me->speed_y));
    ds = v2d_multiply(speed, dt);
    object->actor->position = v2d_add(object->actor->position, ds);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void bullettrajectory_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}


/* objectdecorator_camerafocus_t class */
typedef struct objectdecorator_camerafocus_t objectdecorator_camerafocus_t;
struct objectdecorator_camerafocus_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    void (*strategy)(objectmachine_t*); /* strategy pattern */
};

/* private methods */
static void camerafocus_init(objectmachine_t *obj);
static void camerafocus_release(objectmachine_t *obj);
static void camerafocus_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void camerafocus_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* camerafocus_make_decorator(objectmachine_t *decorated_machine, void (*strategy)(objectmachine_t*));

/* private strategies */
static void request_camera_focus(objectmachine_t *obj);
static void drop_camera_focus(objectmachine_t *obj);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_requestcamerafocus_new(objectmachine_t *decorated_machine)
{
    return camerafocus_make_decorator(decorated_machine, request_camera_focus);
}

objectmachine_t* objectdecorator_dropcamerafocus_new(objectmachine_t *decorated_machine)
{
    return camerafocus_make_decorator(decorated_machine, drop_camera_focus);
}

objectmachine_t* camerafocus_make_decorator(objectmachine_t *decorated_machine, void (*strategy)(objectmachine_t*))
{
    objectdecorator_camerafocus_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = camerafocus_init;
    obj->release = camerafocus_release;
    obj->update = camerafocus_update;
    obj->render = camerafocus_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->strategy = strategy;

    return obj;
}




/* private methods */
void camerafocus_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void camerafocus_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void camerafocus_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_camerafocus_t *me = (objectdecorator_camerafocus_t*)obj;

    me->strategy(obj);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void camerafocus_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}


/* private strategies */
void request_camera_focus(objectmachine_t *obj)
{
    level_set_camera_focus( obj->get_object_instance(obj)->actor );
}

void drop_camera_focus(objectmachine_t *obj)
{
    if(level_get_camera_focus() == obj->get_object_instance(obj)->actor)
        level_set_camera_focus( level_player()->actor );
}

/* objectdecorator_changeclosestobjectstate_t class */
typedef struct objectdecorator_changeclosestobjectstate_t objectdecorator_changeclosestobjectstate_t;
struct objectdecorator_changeclosestobjectstate_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *object_name; /* I'll find the closest object called object_name... */
    char *new_state_name; /* ...and change its state to new_state_name */
};

/* private methods */
static void changeclosestobjectstate_init(objectmachine_t *obj);
static void changeclosestobjectstate_release(objectmachine_t *obj);
static void changeclosestobjectstate_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void changeclosestobjectstate_render(objectmachine_t *obj, v2d_t camera_position);

static object_t *find_closest_object(object_t *me, object_list_t *list, const char* desired_name, float *distance);



/* public methods */
objectmachine_t* objectdecorator_changeclosestobjectstate_new(objectmachine_t *decorated_machine, const char *object_name, const char *new_state_name)
{
    objectdecorator_changeclosestobjectstate_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = changeclosestobjectstate_init;
    obj->release = changeclosestobjectstate_release;
    obj->update = changeclosestobjectstate_update;
    obj->render = changeclosestobjectstate_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->object_name = str_dup(object_name);
    me->new_state_name = str_dup(new_state_name);

    return obj;
}


/* private methods */
void changeclosestobjectstate_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void changeclosestobjectstate_release(objectmachine_t *obj)
{
    objectdecorator_changeclosestobjectstate_t *me = (objectdecorator_changeclosestobjectstate_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    free(me->object_name);
    free(me->new_state_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void changeclosestobjectstate_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_changeclosestobjectstate_t *me = (objectdecorator_changeclosestobjectstate_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    object_t *target = find_closest_object(object, object_list, me->object_name, NULL);

    if(target != NULL) {
        objectvm_set_current_state(target->vm, me->new_state_name);
        enemy_update(target, team, team_size, brick_list, item_list, object_list); /* important to exchange data between objects */
        nanocalcext_set_target_object(object, brick_list, item_list, object_list); /* restore nanocalc's target object */
    }

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void changeclosestobjectstate_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

object_t *find_closest_object(object_t *me, object_list_t *list, const char* desired_name, float *distance)
{
    float min_dist = INFINITY;
    object_list_t *it;
    object_t *ret = NULL;
    v2d_t v;

    for(it=list; it; it=it->next) { /* this list must be small enough */
        if(str_icmp(it->data->name, desired_name) == 0) {
            v = v2d_subtract(it->data->actor->position, me->actor->position);
            if(v2d_magnitude(v) < min_dist) {
                ret = it->data;
                min_dist = v2d_magnitude(v);
            }
        }
    }

    if(distance)
        *distance = min_dist;

    return ret;
}

typedef struct objectdecorator_children_t objectdecorator_children_t;
typedef void (*childrenstrategy_t)(objectdecorator_children_t*,player_t**,int,brick_list_t*,item_list_t*,object_list_t*);

/* objectdecorator_children_t class */
struct objectdecorator_children_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *object_name; /* I'll create an object called object_name... */
    expression_t *offset_x, *offset_y; /* ...at this offset */
    char *child_name; /* child name */
    char *new_state_name; /* new state name */
    childrenstrategy_t strategy; /* strategy pattern */
};

/* private strategies */
static void createchild_strategy(objectdecorator_children_t *me, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void changechildstate_strategy(objectdecorator_children_t *me, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void changeparentstate_strategy(objectdecorator_children_t *me, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);

/* private methods */
static void childrencommand_init(objectmachine_t *obj);
static void childrencommand_release(objectmachine_t *obj);
static void childrencommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void childrencommand_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t *childrencommand_make_decorator(objectmachine_t *decorated_machine, childrenstrategy_t strategy, expression_t *offset_x, expression_t *offset_y, const char *object_name, const char *child_name, const char *new_state_name);



/* public methods */
objectmachine_t* objectdecorator_createchild_new(objectmachine_t *decorated_machine, const char *object_name, expression_t *offset_x, expression_t *offset_y, const char *child_name)
{
    return childrencommand_make_decorator(decorated_machine, createchild_strategy, offset_x, offset_y, object_name, child_name, NULL);
}

objectmachine_t* objectdecorator_changechildstate_new(objectmachine_t *decorated_machine, const char *child_name, const char *new_state_name)
{
    return childrencommand_make_decorator(decorated_machine, changechildstate_strategy, NULL, NULL, NULL, child_name, new_state_name);
}

objectmachine_t* objectdecorator_changeparentstate_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return childrencommand_make_decorator(decorated_machine, changeparentstate_strategy, NULL, NULL, NULL, NULL, new_state_name);
}


/* make a decorator... */
objectmachine_t *childrencommand_make_decorator(objectmachine_t *decorated_machine, childrenstrategy_t strategy, expression_t *offset_x, expression_t *offset_y, const char *object_name, const char *child_name, const char *new_state_name)
{
    objectdecorator_children_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = childrencommand_init;
    obj->release = childrencommand_release;
    obj->update = childrencommand_update;
    obj->render = childrencommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->strategy = strategy;
    me->offset_x = offset_x;
    me->offset_y = offset_y;
    me->object_name = object_name != NULL ? str_dup(object_name) : NULL;
    me->child_name = child_name != NULL ? str_dup(child_name) : NULL;
    me->new_state_name = new_state_name != NULL ? str_dup(new_state_name) : NULL;

    return obj;
}


/* private methods */
void childrencommand_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void childrencommand_release(objectmachine_t *obj)
{
    objectdecorator_children_t *me = (objectdecorator_children_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    if(me->child_name != NULL)
        free(me->child_name);

    if(me->object_name != NULL)
        free(me->object_name);

    if(me->new_state_name != NULL)
        free(me->new_state_name);

    if(me->offset_x != NULL)
        expression_destroy(me->offset_x);

    if(me->offset_y != NULL)
        expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void childrencommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_children_t *me = (objectdecorator_children_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    me->strategy(me, team, team_size, brick_list, item_list, object_list);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void childrencommand_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}


/* private strategies */
void createchild_strategy(objectdecorator_children_t *me, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);
    object_t *child;
    v2d_t offset;

    offset.x = expression_evaluate(me->offset_x);
    offset.y = expression_evaluate(me->offset_y);
    child = level_create_enemy(me->object_name, v2d_add(object->actor->position, offset));
    if(child != NULL) {
        child->created_from_editor = FALSE;
        enemy_add_child(object, me->child_name, child);
        enemy_update(child, team, team_size, brick_list, item_list, object_list); /* important to exchange data between objects */
        nanocalcext_set_target_object(object, brick_list, item_list, object_list); /* restore nanocalc's target object */
    }
}

void changechildstate_strategy(objectdecorator_children_t *me, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);
    object_t *child;

    child = enemy_get_child(object, me->child_name);
    if(child != NULL) {
        objectvm_set_current_state(child->vm, me->new_state_name);
        enemy_update(child, team, team_size, brick_list, item_list, object_list); /* important to exchange data between objects */
        nanocalcext_set_target_object(object, brick_list, item_list, object_list); /* restore nanocalc's target object */
    }
}

void changeparentstate_strategy(objectdecorator_children_t *me, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);
    object_t *parent;

    parent = enemy_get_parent(object);
    if(parent != NULL) {
        objectvm_set_current_state(parent->vm, me->new_state_name);
        enemy_update(parent, team, team_size, brick_list, item_list, object_list); /* important to exchange data between objects */
        nanocalcext_set_target_object(object, brick_list, item_list, object_list); /* restore nanocalc's target object */
    }
}

/* objectdecorator_clearlevel_t class */
typedef struct objectdecorator_clearlevel_t objectdecorator_clearlevel_t;
struct objectdecorator_clearlevel_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void clearlevel_init(objectmachine_t *obj);
static void clearlevel_release(objectmachine_t *obj);
static void clearlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void clearlevel_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_clearlevel_new(objectmachine_t *decorated_machine)
{
    objectdecorator_clearlevel_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = clearlevel_init;
    obj->release = clearlevel_release;
    obj->update = clearlevel_update;
    obj->render = clearlevel_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void clearlevel_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void clearlevel_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void clearlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);

    level_clear(object->actor);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void clearlevel_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_createitem_t class */
typedef struct objectdecorator_createitem_t objectdecorator_createitem_t;
struct objectdecorator_createitem_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *item_id; /* I'll create an item which id is item_id... */
    expression_t *offset_x, *offset_y; /* ...at this offset */
};

/* private methods */
static void createitem_init(objectmachine_t *obj);
static void createitem_release(objectmachine_t *obj);
static void createitem_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void createitem_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */
objectmachine_t* objectdecorator_createitem_new(objectmachine_t *decorated_machine, expression_t *item_id, expression_t *offset_x, expression_t *offset_y)
{
    objectdecorator_createitem_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = createitem_init;
    obj->release = createitem_release;
    obj->update = createitem_update;
    obj->render = createitem_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->item_id = item_id;
    me->offset_x = offset_x;
    me->offset_y = offset_y;

    return obj;
}

/* private methods */
void createitem_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void createitem_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_createitem_t *me = (objectdecorator_createitem_t*)obj;

    expression_destroy(me->item_id);
    expression_destroy(me->offset_x);
    expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void createitem_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_createitem_t *me = (objectdecorator_createitem_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    int item_id;
    v2d_t offset;

    item_id = (int)expression_evaluate(me->item_id);
    offset.x = expression_evaluate(me->offset_x);
    offset.y = expression_evaluate(me->offset_y);

    level_create_item(item_id, v2d_add(object->actor->position, offset));

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void createitem_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_destroy_t class */
typedef struct objectdecorator_destroy_t objectdecorator_destroy_t;
struct objectdecorator_destroy_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void destroy_init(objectmachine_t *obj);
static void destroy_release(objectmachine_t *obj);
static void destroy_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void destroy_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_destroy_new(objectmachine_t *decorated_machine)
{
    objectdecorator_destroy_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = destroy_init;
    obj->release = destroy_release;
    obj->update = destroy_update;
    obj->render = destroy_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}





/* private methods */
void destroy_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void destroy_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void destroy_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    object_t *object = obj->get_object_instance(obj);
    object->state = ES_DEAD;

    /* suspend the execution */
    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void destroy_render(objectmachine_t *obj, v2d_t camera_position)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    ; /* empty */

    /* suspend the execution */
    /*decorated_machine->render(decorated_machine, camera_position);*/
}

/* objectdecorator_dialogbox_t class */
typedef struct objectdecorator_dialogbox_t objectdecorator_dialogbox_t;
struct objectdecorator_dialogbox_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *title; /* dialog box title */
    char *message; /* dialog box message */
    void (*strategy)(objectdecorator_dialogbox_t*);
};

/* private methods */
static void dialogbox_init(objectmachine_t *obj);
static void dialogbox_release(objectmachine_t *obj);
static void dialogbox_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void dialogbox_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* dialogbox_make_decorator(objectmachine_t *decorated_machine, const char *title, const char *message, void (*strategy)());

static void show_dialog_box(objectdecorator_dialogbox_t *me);
static void hide_dialog_box(objectdecorator_dialogbox_t *me);




/* public methods */
objectmachine_t* objectdecorator_showdialogbox_new(objectmachine_t *decorated_machine, const char *title, const char *message)
{
    return dialogbox_make_decorator(decorated_machine, title, message, show_dialog_box);
}

objectmachine_t* objectdecorator_hidedialogbox_new(objectmachine_t *decorated_machine)
{
    return dialogbox_make_decorator(decorated_machine, "", "", hide_dialog_box);
}


/* private methods */
objectmachine_t* dialogbox_make_decorator(objectmachine_t *decorated_machine, const char *title, const char *message, void (*strategy)())
{
    objectdecorator_dialogbox_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = dialogbox_init;
    obj->release = dialogbox_release;
    obj->update = dialogbox_update;
    obj->render = dialogbox_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->title = str_dup(title);
    me->message = str_dup(message);
    me->strategy = strategy;

    return obj;
}

void dialogbox_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void dialogbox_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_dialogbox_t *me = (objectdecorator_dialogbox_t*)obj;

    free(me->title);
    free(me->message);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void dialogbox_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_dialogbox_t *me = (objectdecorator_dialogbox_t*)obj;

    me->strategy(me);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void dialogbox_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

void show_dialog_box(objectdecorator_dialogbox_t *me)
{
    level_call_dialogbox(me->title, me->message);
}

void hide_dialog_box(objectdecorator_dialogbox_t *me)
{
    level_hide_dialogbox();
}

/* objectdecorator_ellipticaltrajectory_t class */
typedef struct objectdecorator_ellipticaltrajectory_t objectdecorator_ellipticaltrajectory_t;
struct objectdecorator_ellipticaltrajectory_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *amplitude_x, *amplitude_y; /* distance from the center of the ellipsis (actor's spawn point) */
    expression_t *angularspeed_x, *angularspeed_y; /* speed, in radians per second */
    expression_t *initialphase_x, *initialphase_y; /* initial phase in degrees */
    float elapsed_time;
};

/* private methods */
static void ellipticaltrajectory_init(objectmachine_t *obj);
static void ellipticaltrajectory_release(objectmachine_t *obj);
static void ellipticaltrajectory_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void ellipticaltrajectory_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_ellipticaltrajectory_new(objectmachine_t *decorated_machine, expression_t *amplitude_x, expression_t *amplitude_y, expression_t *angularspeed_x, expression_t *angularspeed_y, expression_t *initialphase_x, expression_t *initialphase_y)
{
    objectdecorator_ellipticaltrajectory_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = ellipticaltrajectory_init;
    obj->release = ellipticaltrajectory_release;
    obj->update = ellipticaltrajectory_update;
    obj->render = ellipticaltrajectory_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->amplitude_x = amplitude_x;
    me->amplitude_y = amplitude_y;
    me->angularspeed_x = angularspeed_x;
    me->angularspeed_y = angularspeed_y;
    me->initialphase_x = initialphase_x;
    me->initialphase_y = initialphase_y;
    me->elapsed_time = 0.0f;

    return obj;
}




/* private methods */
void ellipticaltrajectory_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void ellipticaltrajectory_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_ellipticaltrajectory_t *me = (objectdecorator_ellipticaltrajectory_t*)obj;

    expression_destroy(me->amplitude_x);
    expression_destroy(me->amplitude_y);
    expression_destroy(me->angularspeed_x);
    expression_destroy(me->angularspeed_y);
    expression_destroy(me->initialphase_x);
    expression_destroy(me->initialphase_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void ellipticaltrajectory_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_ellipticaltrajectory_t *me = (objectdecorator_ellipticaltrajectory_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    float dt = timer_get_delta();
    brick_t *up = NULL, *upright = NULL, *right = NULL, *downright = NULL;
    brick_t *down = NULL, *downleft = NULL, *left = NULL, *upleft = NULL;
    float elapsed_time = (me->elapsed_time += dt);
    v2d_t old_position = act->position;

    /* elliptical trajectory */
    /*
        let C: R -> R^2 be such that:
            C(t) = (
                Ax * cos( Ix + Sx*t ) + Px,
                Ay * sin( Iy + Sy*t ) + Py
            )

        where:
            t  = elapsed_time       (in seconds)
            Ax = amplitude_x        (in pixels)
            Ay = amplitude_y        (in pixels)
            Sx = angularspeed_x     (in radians per second)
            Sy = angularspeed_y     (in radians per second)
            Ix = initialphase_x     (in radians)
            Iy = initialphase_y     (in radians)
            Px = act->spawn_point.x (in pixels)
            Py = act->spawn_point.y (in pixels)

        then:
            C'(t) = dC(t) / dt = (
                -Ax * Sx * sin( Ix + Sx*t ),
                 Ay * Sy * cos( Iy + Sy*t )
            )
    */

    float amplitude_x = expression_evaluate(me->amplitude_x);
    float amplitude_y = expression_evaluate(me->amplitude_y);
    float angularspeed_x = expression_evaluate(me->angularspeed_x) * (2.0f * PI);
    float angularspeed_y = expression_evaluate(me->angularspeed_y) * (2.0f * PI);
    float initialphase_x = (expression_evaluate(me->initialphase_x) * PI) / 180.0f;
    float initialphase_y = (expression_evaluate(me->initialphase_y) * PI) / 180.0f;

    act->position.x += (-amplitude_x * angularspeed_x * sin( initialphase_x + angularspeed_x * elapsed_time)) * dt;
    act->position.y += ( amplitude_y * angularspeed_y * cos( initialphase_y + angularspeed_y * elapsed_time)) * dt;

    /* sensors */
    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    /* I don't want to get stuck into walls */
    if(right != NULL) {
        if(act->position.x > old_position.x)
            act->position.x = act->hot_spot.x - image_width(actor_image(act)) + brick_position(right).x;
    }

    if(left != NULL) {
        if(act->position.x < old_position.x)
            act->position.x = act->hot_spot.x + brick_position(left).x + brick_size(left).x;
    }

    if(down != NULL) {
        if(act->position.y > old_position.y)
            act->position.y = act->hot_spot.y - image_height(actor_image(act)) + brick_position(down).y;
    }

    if(up != NULL) {
        if(act->position.y < old_position.y)
            act->position.y = act->hot_spot.y + brick_position(up).y + brick_size(up).y;
    }

    /* decorator pattern */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void ellipticaltrajectory_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_enemy_t class */
typedef struct objectdecorator_enemy_t objectdecorator_enemy_t;
struct objectdecorator_enemy_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *score;
};

/* private methods */
static void enemydecorator_init(objectmachine_t *obj);
static void enemydecorator_release(objectmachine_t *obj);
static void enemydecorator_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void enemydecorator_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_enemy_new(objectmachine_t *decorated_machine, expression_t *score)
{
    objectdecorator_enemy_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = enemydecorator_init;
    obj->release = enemydecorator_release;
    obj->update = enemydecorator_update;
    obj->render = enemydecorator_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->score = score;

    return obj;
}




/* private methods */
void enemydecorator_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void enemydecorator_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_enemy_t *me = (objectdecorator_enemy_t*)obj;

    expression_destroy(me->score);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void enemydecorator_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_enemy_t *me = (objectdecorator_enemy_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    int i, score;

    score = (int)expression_evaluate(me->score);

    /* player x object collision */
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(player_collision(player, object->actor)) {
            if(player_is_attacking(player) || player_is_invincible(player)) {
                /* I've been defeated */
                player_bounce_ex(player, object->actor, FALSE);
                level_add_to_score(score);
                level_create_item(IT_EXPLOSION, v2d_add(object->actor->position, v2d_new(0,-15)));
                level_create_animal(object->actor->position);
                sound_play(SFX_DESTROY);
                object->state = ES_DEAD;
            }
            else {
                /* The player has been hit by me */
                player_hit_ex(player, object->actor);
            }
        }
    }

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void enemydecorator_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_executebase_t class */
typedef struct objectdecorator_executebase_t objectdecorator_executebase_t;
struct objectdecorator_executebase_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *state_name; /* state to be called */
    void (*update)(objectdecorator_executebase_t*,object_t*,player_t**,int,brick_list_t*,item_list_t*,object_list_t*); /* abstract method */
    void (*render)(objectdecorator_executebase_t*,object_t*,v2d_t);
    void (*destructor)(objectdecorator_executebase_t*); /* abstract method */
};

/* derived classes */
typedef struct objectdecorator_execute_t objectdecorator_execute_t;
struct objectdecorator_execute_t {
    objectdecorator_executebase_t base;
};
static void objectdecorator_execute_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void objectdecorator_execute_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position);
static void objectdecorator_execute_destructor(objectdecorator_executebase_t *ex);

typedef struct objectdecorator_executeif_t objectdecorator_executeif_t;
struct objectdecorator_executeif_t {
    objectdecorator_executebase_t base;
    expression_t *condition;
};
static void objectdecorator_executeif_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void objectdecorator_executeif_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position);
static void objectdecorator_executeif_destructor(objectdecorator_executebase_t *ex);

typedef struct objectdecorator_executeunless_t objectdecorator_executeunless_t;
struct objectdecorator_executeunless_t {
    objectdecorator_executebase_t base;
    expression_t *condition;
};
static void objectdecorator_executeunless_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void objectdecorator_executeunless_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position);
static void objectdecorator_executeunless_destructor(objectdecorator_executebase_t *ex);

typedef struct objectdecorator_executewhile_t objectdecorator_executewhile_t;
struct objectdecorator_executewhile_t {
    objectdecorator_executebase_t base;
    expression_t *condition;
};
static void objectdecorator_executewhile_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void objectdecorator_executewhile_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position);
static void objectdecorator_executewhile_destructor(objectdecorator_executebase_t *ex);

typedef struct objectdecorator_executefor_t objectdecorator_executefor_t;
struct objectdecorator_executefor_t {
    objectdecorator_executebase_t base;
    expression_t *initial, *condition, *iteration;
};
static void objectdecorator_executefor_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void objectdecorator_executefor_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position);
static void objectdecorator_executefor_destructor(objectdecorator_executebase_t *ex);

/* private methods */
static void executecommand_init(objectmachine_t *obj);
static void executecommand_release(objectmachine_t *obj);
static void executecommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void executecommand_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */
objectmachine_t* objectdecorator_execute_new(objectmachine_t *decorated_machine, const char *state_name)
{
    objectdecorator_execute_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;
    objectdecorator_executebase_t *_me = (objectdecorator_executebase_t*)me;

    obj->init = executecommand_init;
    obj->release = executecommand_release;
    obj->update = executecommand_update;
    obj->render = executecommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    _me->state_name = str_dup(state_name);
    _me->update = objectdecorator_execute_update;
    _me->render = objectdecorator_execute_render;
    _me->destructor = objectdecorator_execute_destructor;

    return obj;
}

objectmachine_t* objectdecorator_executeif_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* condition)
{
    objectdecorator_executeif_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;
    objectdecorator_executebase_t *_me = (objectdecorator_executebase_t*)me;

    obj->init = executecommand_init;
    obj->release = executecommand_release;
    obj->update = executecommand_update;
    obj->render = executecommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    _me->state_name = str_dup(state_name);
    _me->update = objectdecorator_executeif_update;
    _me->render = objectdecorator_executeif_render;
    _me->destructor = objectdecorator_executeif_destructor;
    me->condition = condition;

    return obj;
}

objectmachine_t* objectdecorator_executeunless_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* condition)
{
    objectdecorator_executeunless_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;
    objectdecorator_executebase_t *_me = (objectdecorator_executebase_t*)me;

    obj->init = executecommand_init;
    obj->release = executecommand_release;
    obj->update = executecommand_update;
    obj->render = executecommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    _me->state_name = str_dup(state_name);
    _me->update = objectdecorator_executeunless_update;
    _me->render = objectdecorator_executeunless_render;
    _me->destructor = objectdecorator_executeunless_destructor;
    me->condition = condition;

    return obj;
}

objectmachine_t* objectdecorator_executewhile_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* condition)
{
    objectdecorator_executewhile_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;
    objectdecorator_executebase_t *_me = (objectdecorator_executebase_t*)me;

    obj->init = executecommand_init;
    obj->release = executecommand_release;
    obj->update = executecommand_update;
    obj->render = executecommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    _me->state_name = str_dup(state_name);
    _me->update = objectdecorator_executewhile_update;
    _me->render = objectdecorator_executewhile_render;
    _me->destructor = objectdecorator_executewhile_destructor;
    me->condition = condition;

    return obj;
}

objectmachine_t* objectdecorator_executefor_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* executecommand_initial, expression_t* condition, expression_t* iteration)
{
    objectdecorator_executefor_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;
    objectdecorator_executebase_t *_me = (objectdecorator_executebase_t*)me;

    obj->init = executecommand_init;
    obj->release = executecommand_release;
    obj->update = executecommand_update;
    obj->render = executecommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    _me->state_name = str_dup(state_name);
    _me->update = objectdecorator_executefor_update;
    _me->render = objectdecorator_executefor_render;
    _me->destructor = objectdecorator_executefor_destructor;
    me->initial = executecommand_initial;
    me->condition = condition;
    me->iteration = iteration;

    return obj;
}


/* private methods */
void executecommand_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void executecommand_release(objectmachine_t *obj)
{
    objectdecorator_executebase_t *me = (objectdecorator_executebase_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    me->destructor(me);
    free(me->state_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void executecommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_executebase_t *me = (objectdecorator_executebase_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);

    me->update(me, object, team, team_size, brick_list, item_list, object_list);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void executecommand_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_executebase_t *me = (objectdecorator_executebase_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);

    me->render(me, object, camera_position);

    decorated_machine->render(decorated_machine, camera_position);
}

/* private */
void objectdecorator_execute_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectmachine_t *other_state = objectvm_get_state_by_name(obj->vm, ex->state_name);
    other_state->update(other_state, team, team_size, brick_list, item_list, object_list);
}

void objectdecorator_execute_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position)
{
    objectmachine_t *other_state = objectvm_get_state_by_name(obj->vm, ex->state_name);
    other_state->render(other_state, camera_position);
}

void objectdecorator_execute_destructor(objectdecorator_executebase_t *ex)
{
    ;
}

void objectdecorator_executeif_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_executeif_t *me = (objectdecorator_executeif_t*)ex;
    objectmachine_t *other_state = objectvm_get_state_by_name(obj->vm, ex->state_name);

    if(fabs(expression_evaluate(me->condition)) >= 1e-5)
        other_state->update(other_state, team, team_size, brick_list, item_list, object_list);
}

void objectdecorator_executeif_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position)
{
    /* I don't know. update / render are separated cycles, and the condition may no longer be true */
}

void objectdecorator_executeif_destructor(objectdecorator_executebase_t *ex)
{
    objectdecorator_executeif_t *me = (objectdecorator_executeif_t*)ex;
    expression_destroy(me->condition);
}

void objectdecorator_executeunless_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_executeunless_t *me = (objectdecorator_executeunless_t*)ex;
    objectmachine_t *other_state = objectvm_get_state_by_name(obj->vm, ex->state_name);

    if(!(fabs(expression_evaluate(me->condition)) >= 1e-5))
        other_state->update(other_state, team, team_size, brick_list, item_list, object_list);
}

void objectdecorator_executeunless_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position)
{
    /* I don't know. update / render are separated cycles, and the condition may no longer be true */
}

void objectdecorator_executeunless_destructor(objectdecorator_executebase_t *ex)
{
    objectdecorator_executeunless_t *me = (objectdecorator_executeunless_t*)ex;
    expression_destroy(me->condition);
}

void objectdecorator_executewhile_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_executewhile_t *me = (objectdecorator_executewhile_t*)ex;
    objectmachine_t *other_state = objectvm_get_state_by_name(obj->vm, ex->state_name);
    objectmachine_t *this_state = *(objectvm_get_reference_to_current_state(obj->vm));

    while(fabs(expression_evaluate(me->condition)) >= 1e-5) {
        other_state->update(other_state, team, team_size, brick_list, item_list, object_list);
        if(this_state != *(objectvm_get_reference_to_current_state(obj->vm)))
            break;
    }
}

void objectdecorator_executewhile_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position)
{
    /* I don't know. update / render are separated cycles, and the condition may no longer be true */
}

void objectdecorator_executewhile_destructor(objectdecorator_executebase_t *ex)
{
    objectdecorator_executewhile_t *me = (objectdecorator_executewhile_t*)ex;
    expression_destroy(me->condition);
}

void objectdecorator_executefor_update(objectdecorator_executebase_t *ex, object_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_executefor_t *me = (objectdecorator_executefor_t*)ex;
    objectmachine_t *other_state = objectvm_get_state_by_name(obj->vm, ex->state_name);
    objectmachine_t *this_state = *(objectvm_get_reference_to_current_state(obj->vm));

    expression_evaluate(me->initial);
    while(fabs(expression_evaluate(me->condition)) >= 1e-5) {
        other_state->update(other_state, team, team_size, brick_list, item_list, object_list);
        if(this_state != *(objectvm_get_reference_to_current_state(obj->vm)))
            break;
        expression_evaluate(me->iteration);
    }
}

void objectdecorator_executefor_render(objectdecorator_executebase_t *ex, object_t *obj, v2d_t camera_position)
{
    /* I don't know. update / render are separated cycles, and the condition may no longer be true */
}

void objectdecorator_executefor_destructor(objectdecorator_executebase_t *ex)
{
    objectdecorator_executefor_t *me = (objectdecorator_executefor_t*)ex;
    expression_destroy(me->initial);
    expression_destroy(me->condition);
    expression_destroy(me->iteration);
}

/* objectdecorator_gravity_t class */
typedef struct objectdecorator_gravity_t objectdecorator_gravity_t;
struct objectdecorator_gravity_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void gravity_init(objectmachine_t *obj);
static void gravity_release(objectmachine_t *obj);
static void gravity_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void gravity_render(objectmachine_t *obj, v2d_t camera_position);

static int hit_test(const brick_t* brk, int x, int y);
static int sticky_test(const actor_t *act, const brick_list_t *brick_list);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_gravity_new(objectmachine_t *decorated_machine)
{
    objectdecorator_gravity_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = gravity_init;
    obj->release = gravity_release;
    obj->update = gravity_update;
    obj->render = gravity_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void gravity_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void gravity_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void gravity_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    float dt = timer_get_delta();

    /* --------------------------- */

    /* in order to avoid too much processor load,
       we adopt this simplified platform system */
    int rx, ry, rw, rh, bx, by, bw, bh;
    const image_t *ri;
    brick_list_t *it;
    enum { NONE, FLOOR, CEILING } collided = NONE;
    int i, j, sticky_max_offset = 3;
    const obstacle_t* bo;

    ri = actor_image(act);
    rx = (int)(act->position.x - act->hot_spot.x);
    ry = (int)(act->position.y - act->hot_spot.y);
    rw = image_width(ri);
    rh = image_height(ri);

    /* check for collisions */
    for(it = brick_list; it != NULL && collided == NONE; it = it->next) {
        bo = brick_obstacle(it->data);
        if(bo && brick_type(it->data) != BRK_PASSABLE) {
            bx = brick_position(it->data).x;
            by = brick_position(it->data).y;
            bw = brick_size(it->data).x;
            bh = brick_size(it->data).y;

            if(rx<bx+bw && rx+rw>bx && ry<by+bh && ry+rh>by) {
                if(obstacle_got_collision(bo, rx+rw/2, ry, rx+rw/2, ry)) {
                    /* ceiling */
                    collided = CEILING;
                    for(j=1; j<=bh; j++) {
                        if(!obstacle_got_collision(bo, rx, ry+j, rx, ry+j)) {
                            act->position.y += j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw/2, ry+rh-1, rx+rw/2, ry+rh-1)) {
                    /* floor */
                    collided = FLOOR;
                    for(j=1; j<=bh; j++) {
                        if(!obstacle_got_collision(bo, rx, ry-j, rx, ry-j)) {
                            act->position.y -= j-1;
                            break;
                        }
                    }
                }
            }
        }
    }

    /* collided & gravity */
    switch(collided) {
        case FLOOR:
            if(act->speed.y > 0.0f)
                act->speed.y = 0.0f;
            break;

        case CEILING:
            if(act->speed.y < 0.0f)
                act->speed.y = 0.0f;
            break;

        default:
            act->speed.y += (0.21875f * 60.0f * 60.0f) * dt;
            break;
    }

    /* move */
    act->position.y += act->speed.y * dt;

    /* sticky physics */
    if(!sticky_test(act, brick_list)) {
        for(i=sticky_max_offset; i>0; i--) {
            act->position.y += i;
            if(!sticky_test(act, brick_list)) {
                act->position.y += (i == sticky_max_offset) ? -i : 1;
                break;
            }
            else
                act->position.y -= i;
        }
    }

    /* --------------------------- */

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void gravity_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}




/* (x,y) collides with the brick */
int hit_test(const brick_t* brk, int x, int y)
{
    const obstacle_t* obstacle = brick_obstacle(brk);

    if(obstacle != NULL)
        return obstacle_got_collision(obstacle, x, y, x, y);
    else
        return FALSE;
}

/* act collides with some brick? */
int sticky_test(const actor_t *act, const brick_list_t *brick_list)
{
    const brick_list_t *it;
    const brick_t *b;
    const image_t *ri;
    int rx, ry, rw, rh;

    ri = actor_image(act);
    rx = (int)(act->position.x - act->hot_spot.x);
    ry = (int)(act->position.y - act->hot_spot.y);
    rw = image_width(ri);
    rh = image_height(ri);

    for(it = brick_list; it; it = it->next) {
        b = it->data;
        if(brick_type(b) != BRK_PASSABLE) {
            if(hit_test(b, rx+rw/2, ry+rh-1))
                return TRUE;
        }
    }

    return FALSE;
}

/* objectdecorator_hitplayer_t class */
typedef struct objectdecorator_hitplayer_t objectdecorator_hitplayer_t;
struct objectdecorator_hitplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int (*should_hit_the_player)(player_t*); /* strategy pattern */
};

/* private strategies */
static int hit_strategy(player_t *p);
static int burn_strategy(player_t *p);
static int shock_strategy(player_t *p);
static int acid_strategy(player_t *p);

/* private methods */
static void hitplayer_init(objectmachine_t *obj);
static void hitplayer_release(objectmachine_t *obj);
static void hitplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void hitplayer_render(objectmachine_t *obj, v2d_t camera_position);
static objectmachine_t *hitplayer_make_decorator(objectmachine_t *decorated_machine, int (*strategy)(player_t*));


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_hitplayer_new(objectmachine_t *decorated_machine)
{
    return hitplayer_make_decorator(decorated_machine, hit_strategy);
}

objectmachine_t* objectdecorator_burnplayer_new(objectmachine_t *decorated_machine)
{
    return hitplayer_make_decorator(decorated_machine, burn_strategy);
}

objectmachine_t* objectdecorator_shockplayer_new(objectmachine_t *decorated_machine)
{
    return hitplayer_make_decorator(decorated_machine, shock_strategy);
}

objectmachine_t* objectdecorator_acidplayer_new(objectmachine_t *decorated_machine)
{
    return hitplayer_make_decorator(decorated_machine, acid_strategy);
}


/* private methods */

/* builder */
objectmachine_t *hitplayer_make_decorator(objectmachine_t *decorated_machine, int (*strategy)(player_t*))
{
    objectdecorator_hitplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = hitplayer_init;
    obj->release = hitplayer_release;
    obj->update = hitplayer_update;
    obj->render = hitplayer_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->should_hit_the_player = strategy;

    return obj;
}

void hitplayer_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void hitplayer_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void hitplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_hitplayer_t *me = (objectdecorator_hitplayer_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    if(!player_is_invincible(player) && me->should_hit_the_player(player))
        player_hit_ex(player, object->actor);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void hitplayer_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* private strategies */
int hit_strategy(player_t *p)
{
    return TRUE;
}

int burn_strategy(player_t *p)
{
    return player_shield_type(p) != SH_FIRESHIELD && player_shield_type(p) != SH_WATERSHIELD;
}

int shock_strategy(player_t *p)
{
    return player_shield_type(p) != SH_THUNDERSHIELD;
}

int acid_strategy(player_t *p)
{
    return player_shield_type(p) != SH_ACIDSHIELD;
}

/* objectdecorator_jump_t class */
typedef struct objectdecorator_jump_t objectdecorator_jump_t;
struct objectdecorator_jump_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *jump_strength; /* jump strength */
};

/* private methods */
static void jump_init(objectmachine_t *obj);
static void jump_release(objectmachine_t *obj);
static void jump_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void jump_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_jump_new(objectmachine_t *decorated_machine, expression_t *jump_strength)
{
    objectdecorator_jump_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = jump_init;
    obj->release = jump_release;
    obj->update = jump_update;
    obj->render = jump_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->jump_strength = jump_strength;

    return obj;
}




/* private methods */
void jump_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void jump_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_jump_t *me = (objectdecorator_jump_t*)obj;

    expression_destroy(me->jump_strength);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void jump_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_jump_t *me = (objectdecorator_jump_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    brick_t *down = NULL;
    float jump_strength = expression_evaluate(me->jump_strength);

    /* sensors */
    actor_sensors(act, brick_list, NULL, NULL, NULL, NULL, &down, NULL, NULL, NULL);

    /* jump! */
    if(down != NULL)
        act->speed.y = -jump_strength;

    /* TODO */
    /*act->jump_strength = jump_strength;
    input_simulate_button_down(act->input, IB_FIRE1);*/

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void jump_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_killplayer_t class */
typedef struct objectdecorator_killplayer_t objectdecorator_killplayer_t;
struct objectdecorator_killplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void killplayer_init(objectmachine_t *obj);
static void killplayer_release(objectmachine_t *obj);
static void killplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void killplayer_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_killplayer_new(objectmachine_t *decorated_machine)
{
    objectdecorator_killplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = killplayer_init;
    obj->release = killplayer_release;
    obj->update = killplayer_update;
    obj->render = killplayer_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}





/* private methods */
void killplayer_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void killplayer_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void killplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    player_kill(player);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void killplayer_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_launchurl_t class */
typedef struct objectdecorator_launchurl_t objectdecorator_launchurl_t;
struct objectdecorator_launchurl_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *url;
};

/* private methods */
static void launchurl_init(objectmachine_t *obj);
static void launchurl_release(objectmachine_t *obj);
static void launchurl_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void launchurl_render(objectmachine_t *obj, v2d_t camera_position);



/* ------------------------------------------ */


/* public methods */

objectmachine_t* objectdecorator_launchurl_new(objectmachine_t *decorated_machine, const char *url)
{
    objectdecorator_launchurl_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = launchurl_init;
    obj->release = launchurl_release;
    obj->update = launchurl_update;
    obj->render = launchurl_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->url = str_dup(url);

    return obj;
}



/* private methods */
void launchurl_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void launchurl_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_launchurl_t *me = (objectdecorator_launchurl_t*)obj;

    free(me->url);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void launchurl_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_launchurl_t *me = (objectdecorator_launchurl_t*)obj;

    if(!launch_url(me->url))
        video_showmessage("Can't open URL.");

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void launchurl_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_loadlevel_t class */
typedef struct objectdecorator_loadlevel_t objectdecorator_loadlevel_t;
struct objectdecorator_loadlevel_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *level_path; /* level to be loaded */
};

/* private methods */
static void loadlevel_init(objectmachine_t *obj);
static void loadlevel_release(objectmachine_t *obj);
static void loadlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void loadlevel_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_loadlevel_new(objectmachine_t *decorated_machine, const char *level_path)
{
    objectdecorator_loadlevel_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = loadlevel_init;
    obj->release = loadlevel_release;
    obj->update = loadlevel_update;
    obj->render = loadlevel_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->level_path = str_dup(level_path);

    return obj;
}





/* private methods */
void loadlevel_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void loadlevel_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_loadlevel_t *me = (objectdecorator_loadlevel_t*)obj;

    free(me->level_path);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void loadlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/
    objectdecorator_loadlevel_t *me = (objectdecorator_loadlevel_t*)obj;

    level_change(me->level_path);

    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void loadlevel_render(objectmachine_t *obj, v2d_t camera_position)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    ; /* empty */

    /*decorated_machine->render(decorated_machine, camera_position);*/
}


/* objectdecorator_lockcamera_t class */
typedef struct objectdecorator_lockcamera_t objectdecorator_lockcamera_t;
struct objectdecorator_lockcamera_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *x1, *y1, *x2, *y2;
    int has_locked_somebody;
    int _x1, _y1, _x2, _y2;
};

/* private methods */
static void lockcamera_init(objectmachine_t *obj);
static void lockcamera_release(objectmachine_t *obj);
static void lockcamera_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void lockcamera_render(objectmachine_t *obj, v2d_t camera_position);

static void get_rectangle_coordinates(objectdecorator_lockcamera_t *me, int *x1, int *y1, int *x2, int *y2);
static void update_rectangle_coordinates(objectdecorator_lockcamera_t *me, int x1, int y1, int x2, int y2);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_lockcamera_new(objectmachine_t *decorated_machine, expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2)
{
    objectdecorator_lockcamera_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = lockcamera_init;
    obj->release = lockcamera_release;
    obj->update = lockcamera_update;
    obj->render = lockcamera_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->x1 = x1;
    me->y1 = y1;
    me->x2 = x2;
    me->y2 = y2;

    return obj;
}





/* private methods */
void lockcamera_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
    int x1, x2, y1, y2;

    me->has_locked_somebody = FALSE;
    get_rectangle_coordinates(me, &x1, &y1, &x2, &y2);
    update_rectangle_coordinates(me, x1, y1, x2, y2);

    decorated_machine->init(decorated_machine);
}

void lockcamera_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    if(me->has_locked_somebody) {
        player->in_locked_area = FALSE;
        level_unlock_camera();
    }

    expression_destroy(me->x1);
    expression_destroy(me->x2);
    expression_destroy(me->y1);
    expression_destroy(me->y2);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void lockcamera_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
    actor_t *act = object->actor, *ta;
    float rx, ry, rw, rh;
    int x1, x2, y1, y2;
    int i;

    /* configuring the rectangle */
    get_rectangle_coordinates(me, &x1, &y1, &x2, &y2);
    update_rectangle_coordinates(me, x1, y1, x2, y2);

    /* my rectangle, in world coordinates */
    rx = act->position.x + x1;
    ry = act->position.y + y1;
    rw = x2 - x1;
    rh = y2 - y1;

    /* only the observed player can enter this area */
    for(i=0; i<team_size; i++) {
        ta = team[i]->actor;

        if(team[i] != player) {
            /* hey, you can't enter here! */
            float border = 30.0f;
            if(ta->position.x > rx - border && ta->position.x < rx) {
                ta->position.x = rx - border;
                ta->speed.x = 0.0f;
            }
            if(ta->position.x > rx + rw && ta->position.x < rx + rw + border) {
                ta->position.x = rx + rw + border;
                ta->speed.x = 0.0f;
            }
        }
        else {
            /* test if the player has got inside my rectangle */
            float a[4], b[4];

            a[0] = ta->position.x;
            a[1] = ta->position.y;
            a[2] = ta->position.x + 1;
            a[3] = ta->position.y + 1;

            b[0] = rx;
            b[1] = ry;
            b[2] = rx + rw;
            b[3] = ry + rh;

            if(bounding_box(a, b)) {
                /* welcome, player! You have been locked. BWHAHAHA!!! */
                me->has_locked_somebody = TRUE;
                team[i]->in_locked_area = TRUE;
                level_lock_camera(rx, ry, rx+rw, ry+rh);
            }
        }
    }


    /* cage */
    if(me->has_locked_somebody) {
        ta = player->actor;
        if(ta->position.x < rx) {
            ta->position.x = rx;
            ta->speed.x = max(0.0f, ta->speed.x);
            player->at_some_border = TRUE;
        }
        if(ta->position.x > rx + rw) {
            ta->position.x = rx + rw;
            ta->speed.x = min(0.0f, ta->speed.x);
            player->at_some_border = TRUE;
        }
        ta->position.y = clip(ta->position.y, ry, ry + rh);
    }

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void lockcamera_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    if(level_editmode()) {
        objectdecorator_lockcamera_t *me = (objectdecorator_lockcamera_t*)obj;
        actor_t *act = obj->get_object_instance(obj)->actor;
        color_t color = color_rgb(255, 0, 0);
        int x1, y1, x2, y2;

        x1 = (act->position.x + me->_x1) - (camera_position.x - VIDEO_SCREEN_W/2);
        y1 = (act->position.y + me->_y1) - (camera_position.y - VIDEO_SCREEN_H/2);
        x2 = (act->position.x + me->_x2) - (camera_position.x - VIDEO_SCREEN_W/2);
        y2 = (act->position.y + me->_y2) - (camera_position.y - VIDEO_SCREEN_H/2);

        image_rect(x1, y1, x2, y2, color);
    }

    decorated_machine->render(decorated_machine, camera_position);
}


/* auxiliary functions */

void get_rectangle_coordinates(objectdecorator_lockcamera_t *me, int *x1, int *y1, int *x2, int *y2)
{
    int mi, ma;

    *x1 = (int)expression_evaluate(me->x1);
    *x2 = (int)expression_evaluate(me->x2);
    *y1 = (int)expression_evaluate(me->y1);
    *y2 = (int)expression_evaluate(me->y2);

    if(*x1 == *x2) (*x2)++;
    if(*y1 == *y2) (*y2)++;

    mi = min(*x1, *x2);
    ma = max(*x1, *x2);
    *x1 = mi;
    *x2 = ma;

    mi = min(*y1, *y2);
    ma = max(*y1, *y2);
    *y1 = mi;
    *y2 = ma;
}

void update_rectangle_coordinates(objectdecorator_lockcamera_t *me, int x1, int y1, int x2, int y2)
{
    me->_x1 = x1;
    me->_y1 = y1;
    me->_x2 = x2;
    me->_y2 = y2;
}

/* objectdecorator_look_t class */
typedef struct objectdecorator_look_t objectdecorator_look_t;
struct objectdecorator_look_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    float old_x;
    void (*look_strategy)(objectdecorator_look_t*);
};

/* private methods */
static objectmachine_t* objectdecorator_look_new(objectmachine_t *decorated_machine, void (*look_strategy)(objectdecorator_look_t*));
static void look_init(objectmachine_t *obj);
static void look_release(objectmachine_t *obj);
static void look_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void look_render(objectmachine_t *obj, v2d_t camera_position);

/* private strategies */
static void look_left(objectdecorator_look_t *me);
static void look_right(objectdecorator_look_t *me);
static void look_at_player(objectdecorator_look_t *me);
static void look_at_walking_direction(objectdecorator_look_t *me);





/* public methods */
objectmachine_t* objectdecorator_lookleft_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_left);
}

objectmachine_t* objectdecorator_lookright_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_right);
}

objectmachine_t* objectdecorator_lookatplayer_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_at_player);
}

objectmachine_t* objectdecorator_lookatwalkingdirection_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_at_walking_direction);
}






/* private methods */
objectmachine_t* objectdecorator_look_new(objectmachine_t *decorated_machine, void (*look_strategy)(objectdecorator_look_t*))
{
    objectdecorator_look_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = look_init;
    obj->release = look_release;
    obj->update = look_update;
    obj->render = look_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->look_strategy = look_strategy;

    return obj;
}

void look_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_look_t *me = (objectdecorator_look_t*)obj;

    me->old_x = 0.0f;

    decorated_machine->init(decorated_machine);
}

void look_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void look_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_look_t *me = (objectdecorator_look_t*)obj;

    me->look_strategy(me);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void look_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}



/* private strategies */
void look_left(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);

    object->actor->mirror |= IF_HFLIP;
}

void look_right(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);

    object->actor->mirror &= ~IF_HFLIP;
}

void look_at_player(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);

    if(object->actor->position.x < player->actor->position.x)
        object->actor->mirror &= ~IF_HFLIP;
    else
        object->actor->mirror |= IF_HFLIP;
}

void look_at_walking_direction(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);

    if(object->actor->position.x > me->old_x)
        object->actor->mirror &= ~IF_HFLIP;
    else
        object->actor->mirror |= IF_HFLIP;

    me->old_x = object->actor->position.x;
}

/* objectdecorator_mosquitomovement_t class */
typedef struct objectdecorator_mosquitomovement_t objectdecorator_mosquitomovement_t;
struct objectdecorator_mosquitomovement_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed; /* speed magnitude */
};

/* private methods */
static void mosquitomovement_init(objectmachine_t *obj);
static void mosquitomovement_release(objectmachine_t *obj);
static void mosquitomovement_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void mosquitomovement_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_mosquitomovement_new(objectmachine_t *decorated_machine, expression_t *speed)
{
    objectdecorator_mosquitomovement_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = mosquitomovement_init;
    obj->release = mosquitomovement_release;
    obj->update = mosquitomovement_update;
    obj->render = mosquitomovement_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed = speed;

    return obj;
}





/* private methods */
void mosquitomovement_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void mosquitomovement_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_mosquitomovement_t *me = (objectdecorator_mosquitomovement_t*)obj;

    expression_destroy(me->speed);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void mosquitomovement_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_mosquitomovement_t *me = (objectdecorator_mosquitomovement_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    v2d_t diff = v2d_subtract(player->actor->position, object->actor->position);
    float speed = expression_evaluate(me->speed);

    if(v2d_magnitude(diff) >= 5.0f) {
        float dt = timer_get_delta();
        v2d_t direction = v2d_normalize(diff);
        v2d_t ds = v2d_multiply(direction, speed * dt);
        object->actor->position = v2d_add(object->actor->position, ds);
    }

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void mosquitomovement_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_moveplayer_t class */
typedef struct objectdecorator_moveplayer_t objectdecorator_moveplayer_t;
struct objectdecorator_moveplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed_x, *speed_y; /* speed */
};

/* private methods */
static void moveplayer_init(objectmachine_t *obj);
static void moveplayer_release(objectmachine_t *obj);
static void moveplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void moveplayer_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_moveplayer_new(objectmachine_t *decorated_machine, expression_t *speed_x, expression_t *speed_y)
{
    objectdecorator_moveplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = moveplayer_init;
    obj->release = moveplayer_release;
    obj->update = moveplayer_update;
    obj->render = moveplayer_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed_x = speed_x;
    me->speed_y = speed_y;

    return obj;
}





/* private methods */
void moveplayer_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void moveplayer_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_moveplayer_t *me = (objectdecorator_moveplayer_t*)obj;

    expression_destroy(me->speed_x);
    expression_destroy(me->speed_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void moveplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_moveplayer_t *me = (objectdecorator_moveplayer_t*)obj;
    float dt = timer_get_delta();
    v2d_t speed = v2d_new(expression_evaluate(me->speed_x), expression_evaluate(me->speed_y));
    v2d_t ds = v2d_multiply(speed, dt);
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    player->actor->position = v2d_add(player->actor->position, ds);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void moveplayer_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_nextlevel_t class */
typedef struct objectdecorator_nextlevel_t objectdecorator_nextlevel_t;
struct objectdecorator_nextlevel_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void nextlevel_init(objectmachine_t *obj);
static void nextlevel_release(objectmachine_t *obj);
static void nextlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void nextlevel_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_nextlevel_new(objectmachine_t *decorated_machine)
{
    objectdecorator_nextlevel_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = nextlevel_init;
    obj->release = nextlevel_release;
    obj->update = nextlevel_update;
    obj->render = nextlevel_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void nextlevel_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void nextlevel_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void nextlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    level_jump_to_next_stage();

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void nextlevel_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

typedef struct objectdecorator_observeplayer_t objectdecorator_observeplayer_t;
typedef struct observeplayerstrategy_t observeplayerstrategy_t;

/* objectdecorator_observeplayer_t class */
struct objectdecorator_observeplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    observeplayerstrategy_t *strategy;
};

/* observeplayerstrategy_t class */
struct observeplayerstrategy_t {
    char *player_name; /* player name */
    object_t *object; /* pointer to the object instance */
    void (*run)(observeplayerstrategy_t*, player_t**, int);
};

/* private methods */
static void observeplayer_init(objectmachine_t *obj);
static void observeplayer_release(objectmachine_t *obj);
static void observeplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void observeplayer_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* observeplayer_make_decorator(objectmachine_t *decorated_machine, observeplayerstrategy_t *strategy);
static observeplayerstrategy_t* observeplayer_make_strategy(const char *player_name, object_t *object, void (*run_func)(observeplayerstrategy_t*,player_t**,int));

static void observe_player(observeplayerstrategy_t *strategy, player_t **team, int team_size);
static void observe_current_player(observeplayerstrategy_t *strategy, player_t **team, int team_size);
static void observe_active_player(observeplayerstrategy_t *strategy, player_t **team, int team_size);
static void observe_all_players(observeplayerstrategy_t *strategy, player_t **team, int team_size);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_observeplayer_new(objectmachine_t *decorated_machine, const char *player_name)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return observeplayer_make_decorator(decorated_machine, observeplayer_make_strategy(player_name, object, observe_player));
}

objectmachine_t* objectdecorator_observecurrentplayer_new(objectmachine_t *decorated_machine)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return observeplayer_make_decorator(decorated_machine, observeplayer_make_strategy("", object, observe_current_player));
}

objectmachine_t* objectdecorator_observeactiveplayer_new(objectmachine_t *decorated_machine)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return observeplayer_make_decorator(decorated_machine, observeplayer_make_strategy("", object, observe_active_player));
}

objectmachine_t* objectdecorator_observeallplayers_new(objectmachine_t *decorated_machine)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return observeplayer_make_decorator(decorated_machine, observeplayer_make_strategy("", object, observe_all_players));
}


/* private methods */
objectmachine_t* observeplayer_make_decorator(objectmachine_t *decorated_machine, observeplayerstrategy_t *strategy)
{
    objectdecorator_observeplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = observeplayer_init;
    obj->release = observeplayer_release;
    obj->update = observeplayer_update;
    obj->render = observeplayer_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->strategy = strategy;

    return obj;
}

observeplayerstrategy_t* observeplayer_make_strategy(const char *player_name, object_t *object, void (*run_func)(observeplayerstrategy_t*,player_t**,int))
{
    observeplayerstrategy_t *x = mallocx(sizeof *x);

    x->player_name = str_dup(player_name);
    x->object = object;
    x->run = run_func;

    return x;
}

void observeplayer_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void observeplayer_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_observeplayer_t *me = (objectdecorator_observeplayer_t*)obj;

    free(me->strategy->player_name);
    free(me->strategy);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void observeplayer_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_observeplayer_t *me = (objectdecorator_observeplayer_t*)obj;

    me->strategy->run(me->strategy, team, team_size);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void observeplayer_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

void observe_player(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    int i;
    player_t *player = NULL;

    for(i=0; i<team_size; i++) {
        if(str_icmp(team[i]->name, strategy->player_name) == 0)
            player = team[i];
    }

    if(player == NULL)
        fatal_error("Can't observe player \"%s\": player does not exist!", strategy->player_name);

    enemy_observe_player(strategy->object, player);
}

void observe_current_player(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    enemy_observe_current_player(strategy->object);
}

void observe_active_player(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    enemy_observe_active_player(strategy->object);
}

void observe_all_players(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    player_t *observed_player = enemy_get_observed_player(strategy->object);
    int i;

    for(i=0; i<team_size; i++) {
        if(team[i] == observed_player) {
            enemy_observe_player(strategy->object, team[(i+1)%team_size]);
            break;
        }
    }
}

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
static eventstrategy_t* onplayerlookup_new() { return onplayerevent_new(player_is_looking_up); }
static eventstrategy_t* onplayerwait_new() { return onplayerevent_new(player_is_waiting); }
static eventstrategy_t* onplayerwin_new() { return onplayerevent_new(player_is_winning); }
static eventstrategy_t* onplayerintheair_new() { return onplayerevent_new(player_is_in_the_air); }
static eventstrategy_t* onplayerunderwater_new() { return onplayerevent_new(player_is_underwater); }
static eventstrategy_t* onplayerspeedshoes_new() { return onplayerevent_new(player_is_ultrafast); }
static eventstrategy_t* onplayerinvincible_new() { return onplayerevent_new(player_is_invincible); }

/* onplayershield_t concrete strategy */
struct onplayershield_t {
    eventstrategy_t base; /* implements eventstrategy_t */
    playershield_t shield_type; /* a SH_* constant defined at player.h */
};
static eventstrategy_t* onplayershield_new(playershield_t shield_type);
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
static objectmachine_t *onevent_make_decorator(objectmachine_t *decorated_machine, const char *new_state_name, eventstrategy_t *strategy);
static void onevent_init(objectmachine_t *obj);
static void onevent_release(objectmachine_t *obj);
static void onevent_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void onevent_render(objectmachine_t *obj, v2d_t camera_position);



/* ---------------------------------- */

/* public methods */

objectmachine_t* objectdecorator_onalways_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onalways_new());
}

objectmachine_t* objectdecorator_ontimeout_new(objectmachine_t *decorated_machine, expression_t *timeout, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, ontimeout_new(timeout));
}

objectmachine_t* objectdecorator_oncollision_new(objectmachine_t *decorated_machine, const char *target_name, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, oncollision_new(target_name));
}

objectmachine_t* objectdecorator_onanimationfinished_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onanimationfinished_new());
}

objectmachine_t* objectdecorator_onrandomevent_new(objectmachine_t *decorated_machine, expression_t *probability, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onrandomevent_new(probability));
}

objectmachine_t* objectdecorator_onlevelcleared_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onlevelcleared_new());
}

objectmachine_t* objectdecorator_onplayercollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayercollision_new());
}

objectmachine_t* objectdecorator_onplayerattack_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerattack_new());
}

objectmachine_t* objectdecorator_onplayerrectcollision_new(objectmachine_t *decorated_machine, expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerrectcollision_new(x1,y1,x2,y2));
}

objectmachine_t* objectdecorator_onobservedplayer_new(objectmachine_t *decorated_machine, const char *player_name, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onobservedplayer_new(player_name));
}

objectmachine_t* objectdecorator_onplayerstop_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerstop_new());
}

objectmachine_t* objectdecorator_onplayerwalk_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerwalk_new());
}

objectmachine_t* objectdecorator_onplayerrun_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerrun_new());
}

objectmachine_t* objectdecorator_onplayerjump_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerjump_new());
}


objectmachine_t* objectdecorator_onplayerroll_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerroll_new());
}

objectmachine_t* objectdecorator_onplayerspring_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerspring_new());
}


objectmachine_t* objectdecorator_onplayerpush_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerpush_new());
}


objectmachine_t* objectdecorator_onplayergethit_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayergethit_new());
}


objectmachine_t* objectdecorator_onplayerdeath_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerdeath_new());
}


objectmachine_t* objectdecorator_onplayerbrake_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerbrake_new());
}


objectmachine_t* objectdecorator_onplayerledge_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerledge_new());
}


objectmachine_t* objectdecorator_onplayerdrown_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerdrown_new());
}


objectmachine_t* objectdecorator_onplayerbreathe_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerbreathe_new());
}


objectmachine_t* objectdecorator_onplayerduck_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerduck_new());
}

objectmachine_t* objectdecorator_onplayerlookup_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerlookup_new());
}

objectmachine_t* objectdecorator_onplayerwait_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerwait_new());
}


objectmachine_t* objectdecorator_onplayerwin_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerwin_new());
}

objectmachine_t* objectdecorator_onplayerintheair_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerintheair_new());
}

objectmachine_t* objectdecorator_onplayerunderwater_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerunderwater_new());
}

objectmachine_t* objectdecorator_onplayerspeedshoes_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerspeedshoes_new());
}

objectmachine_t* objectdecorator_onplayerinvincible_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayerinvincible_new());
}

objectmachine_t* objectdecorator_onnoshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_NONE));
}

objectmachine_t* objectdecorator_onshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_SHIELD));
}

objectmachine_t* objectdecorator_onfireshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_FIRESHIELD));
}

objectmachine_t* objectdecorator_onthundershield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_THUNDERSHIELD));
}

objectmachine_t* objectdecorator_onwatershield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_WATERSHIELD));
}

objectmachine_t* objectdecorator_onacidshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_ACIDSHIELD));
}

objectmachine_t* objectdecorator_onwindshield_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onplayershield_new(SH_WINDSHIELD));
}

objectmachine_t* objectdecorator_onbrickcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onbrickcollision_new());
}

objectmachine_t* objectdecorator_onfloorcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onfloorcollision_new());
}

objectmachine_t* objectdecorator_onceilingcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onceilingcollision_new());
}

objectmachine_t* objectdecorator_onleftwallcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onleftwallcollision_new());
}

objectmachine_t* objectdecorator_onrightwallcollision_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onrightwallcollision_new());
}

objectmachine_t* objectdecorator_onbuttondown_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onbutton_new(button_name, input_button_down));
}

objectmachine_t* objectdecorator_onbuttonpressed_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onbutton_new(button_name, input_button_pressed));
}

objectmachine_t* objectdecorator_onbuttonup_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onbutton_new(button_name, input_button_up));
}

objectmachine_t* objectdecorator_onmusicplay_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, onmusicplay_new());
}

objectmachine_t* objectdecorator_oncamerafocus_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, oncamerafocus_new());
}

objectmachine_t* objectdecorator_oncamerafocusplayer_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, oncamerafocusplayer_new());
}

objectmachine_t* objectdecorator_oncameralock_new(objectmachine_t *decorated_machine, const char *new_state_name)
{
    return onevent_make_decorator(decorated_machine, new_state_name, oncameralock_new());
}

/* ---------------------------------- */

/* private methods */

objectmachine_t *onevent_make_decorator(objectmachine_t *decorated_machine, const char *new_state_name, eventstrategy_t *strategy)
{
    objectdecorator_onevent_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = onevent_init;
    obj->release = onevent_release;
    obj->update = onevent_update;
    obj->render = onevent_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->new_state_name = str_dup(new_state_name);
    me->strategy = strategy;

    return obj;
}

void onevent_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_onevent_t *me = (objectdecorator_onevent_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    me->strategy->init(me->strategy);

    decorated_machine->init(decorated_machine);
}

void onevent_release(objectmachine_t *obj)
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

void onevent_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
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

void onevent_render(objectmachine_t *obj, v2d_t camera_position)
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
            if(actor_collision(it->data->actor, object->actor))
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
    return player_collision(player, object->actor);
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
    return player_is_attacking(player) && player_collision(player, object->actor);
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
eventstrategy_t* onplayershield_new(playershield_t shield_type)
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

    return player_shield_type(player) == me->shield_type;
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
        (up != NULL && brick_type(up) == BRK_OBSTACLE) ||
        (upright != NULL && brick_type(upright) == BRK_OBSTACLE) ||
        (right != NULL && brick_type(right) == BRK_OBSTACLE) ||
        (downright != NULL && brick_type(downright) != BRK_PASSABLE) ||
        (down != NULL && brick_type(down) != BRK_PASSABLE) ||
        (downleft != NULL && brick_type(downleft) != BRK_PASSABLE) ||
        (left != NULL && brick_type(left) == BRK_OBSTACLE) ||
        (upleft != NULL && brick_type(upleft) == BRK_OBSTACLE)
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
        (downright != NULL && brick_type(downright) != BRK_PASSABLE) ||
        (down != NULL && brick_type(down) != BRK_PASSABLE) ||
        (downleft != NULL && brick_type(downleft) != BRK_PASSABLE)
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
        (upleft != NULL && brick_type(upleft) == BRK_OBSTACLE) ||
        (up != NULL && brick_type(up) == BRK_OBSTACLE) ||
        (upright != NULL && brick_type(upright) == BRK_OBSTACLE)
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
        (left != NULL && brick_type(left) == BRK_OBSTACLE) ||
        (upleft != NULL && brick_type(upleft) == BRK_OBSTACLE)
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
        (right != NULL && brick_type(right) == BRK_OBSTACLE) ||
        (upright != NULL && brick_type(upright) == BRK_OBSTACLE)
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

/* objectdecorator_pause_t class */
typedef struct objectdecorator_pause_t objectdecorator_pause_t;
struct objectdecorator_pause_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void pause_init(objectmachine_t *obj);
static void pause_release(objectmachine_t *obj);
static void pause_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void pause_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_pause_new(objectmachine_t *decorated_machine)
{
    objectdecorator_pause_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = pause_init;
    obj->release = pause_release;
    obj->update = pause_update;
    obj->render = pause_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void pause_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void pause_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void pause_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    level_pause();

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void pause_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_playeraction_t class */
typedef struct objectdecorator_playeraction_t objectdecorator_playeraction_t;
struct objectdecorator_playeraction_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    void (*update)(player_t*); /* strategy */
};

/* private methods */
static void playeraction_init(objectmachine_t *obj);
static void playeraction_release(objectmachine_t *obj);
static void playeraction_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void playeraction_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t *playeraction_make_decorator(objectmachine_t *decorated_machine, void (*update_strategy)(player_t*));

/* private strategies */
static void springfy(player_t *player);
static void roll(player_t *player);
static void enable_roll(player_t *player);
static void disable_roll(player_t *player);
static void strong(player_t *player);
static void weak(player_t *player);
static void enterwater(player_t *player);
static void leavewater(player_t *player);
static void breathe(player_t *player);
static void drown(player_t *player);
static void resetunderwatertimer(player_t *player);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_springfyplayer_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, springfy);
}

objectmachine_t* objectdecorator_rollplayer_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, roll);
}

objectmachine_t* objectdecorator_enableplayerroll_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, enable_roll);
}

objectmachine_t* objectdecorator_disableplayerroll_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, disable_roll);
}

objectmachine_t* objectdecorator_strongplayer_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, strong);
}

objectmachine_t* objectdecorator_weakplayer_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, weak);
}

objectmachine_t* objectdecorator_playerenterwater_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, enterwater);
}

objectmachine_t* objectdecorator_playerleavewater_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, leavewater);
}

objectmachine_t* objectdecorator_playerbreathe_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, breathe);
}

objectmachine_t* objectdecorator_playerdrown_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, drown);
}

objectmachine_t* objectdecorator_playerresetunderwatertimer_new(objectmachine_t *decorated_machine)
{
    return playeraction_make_decorator(decorated_machine, resetunderwatertimer);
}





/* private methods */

objectmachine_t* playeraction_make_decorator(objectmachine_t *decorated_machine, void (*update_strategy)(player_t*))
{
    objectdecorator_playeraction_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = playeraction_init;
    obj->release = playeraction_release;
    obj->update = playeraction_update;
    obj->render = playeraction_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->update = update_strategy;

    return obj;
}


void playeraction_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void playeraction_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void playeraction_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_playeraction_t *me = (objectdecorator_playeraction_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    me->update(player);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void playeraction_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* private strategies */
void springfy(player_t *player)
{
    player_spring(player);
}

void roll(player_t *player)
{
    player_roll(player);
}

void enable_roll(player_t *player)
{
    player_enable_roll(player);
}

void disable_roll(player_t *player)
{
    player_disable_roll(player);
}

void strong(player_t *player)
{
    player->attacking = TRUE;
}

void weak(player_t *player)
{
    player->attacking = FALSE;
}

void enterwater(player_t *player)
{
    player_enter_water(player);
}

void leavewater(player_t *player)
{
    player_leave_water(player);
}

void breathe(player_t *player)
{
    player_breathe(player);
}

void drown(player_t *player)
{
    player_drown(player);
}

void resetunderwatertimer(player_t *player)
{
    player_reset_underwater_timer(player);
}


/* objectdecorator_playermovement_t class */
typedef struct objectdecorator_playermovement_t objectdecorator_playermovement_t;
struct objectdecorator_playermovement_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int enable; /* should the movement be enabled? */
};

/* private methods */
static void playermovement_init(objectmachine_t *obj);
static void playermovement_release(objectmachine_t *obj);
static void playermovement_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void playermovement_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t *playermovement_make_decorator(objectmachine_t *decorated_machine, int enable);




/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_enableplayermovement_new(objectmachine_t *decorated_machine)
{
    return playermovement_make_decorator(decorated_machine, TRUE);
}

objectmachine_t* objectdecorator_disableplayermovement_new(objectmachine_t *decorated_machine)
{
    return playermovement_make_decorator(decorated_machine, FALSE);
}





/* private methods */

objectmachine_t* playermovement_make_decorator(objectmachine_t *decorated_machine, int enable)
{
    objectdecorator_playermovement_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = playermovement_init;
    obj->release = playermovement_release;
    obj->update = playermovement_update;
    obj->render = playermovement_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->enable = enable;

    return obj;
}


void playermovement_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void playermovement_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void playermovement_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_playermovement_t *me = (objectdecorator_playermovement_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    player_set_frozen(player, !me->enable);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void playermovement_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_quest_t class */
typedef struct objectdecorator_quest_t objectdecorator_quest_t;
struct objectdecorator_quest_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    void (*update)(objectdecorator_quest_t*); /* questcommand_update function */
};

typedef struct objectdecorator_pushquest_t objectdecorator_pushquest_t;
struct objectdecorator_pushquest_t {
    objectdecorator_quest_t base;
    char filepath[1024];
};

typedef struct objectdecorator_popquest_t objectdecorator_popquest_t;
struct objectdecorator_popquest_t {
    objectdecorator_quest_t base;
};

/* private methods */
static void questcommand_init(objectmachine_t *obj);
static void questcommand_release(objectmachine_t *obj);
static void questcommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void questcommand_render(objectmachine_t *obj, v2d_t camera_position);


static objectmachine_t* setup_decorator(objectdecorator_quest_t *me, objectmachine_t *decorated_machine, void (*update_fun)(objectdecorator_quest_t*));


static void pushquest(objectdecorator_quest_t *q);
static void popquest(objectdecorator_quest_t *q);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_pushquest_new(objectmachine_t *decorated_machine, const char *path_to_qst_file)
{
    objectdecorator_pushquest_t *me = mallocx(sizeof *me);
    str_cpy(me->filepath,  path_to_qst_file, sizeof(me->filepath));
    return setup_decorator((objectdecorator_quest_t*)me, decorated_machine, pushquest);
}

objectmachine_t* objectdecorator_popquest_new(objectmachine_t *decorated_machine)
{
    objectdecorator_popquest_t *me = mallocx(sizeof *me);
    return setup_decorator((objectdecorator_quest_t*)me, decorated_machine, popquest);
}



/* private methods */
objectmachine_t* setup_decorator(objectdecorator_quest_t *me, objectmachine_t *decorated_machine, void (*update_fun)(objectdecorator_quest_t*))
{
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = questcommand_init;
    obj->release = questcommand_release;
    obj->update = questcommand_update;
    obj->render = questcommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->update = update_fun;

    return obj;
}

void questcommand_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void questcommand_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void questcommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_quest_t *me = (objectdecorator_quest_t*)obj;
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    me->update(me);

    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void questcommand_render(objectmachine_t *obj, v2d_t camera_position)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    /* empty */

    /*decorated_machine->render(decorated_machine, camera_position);*/
}





/* functions */
void pushquest(objectdecorator_quest_t *q)
{
    level_push_quest(((objectdecorator_pushquest_t*)q)->filepath);
}

void popquest(objectdecorator_quest_t *q)
{
    level_abort();
}

/* objectdecorator_resetglobals_t class */
typedef struct objectdecorator_resetglobals_t objectdecorator_resetglobals_t;
struct objectdecorator_resetglobals_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void resetglobals_init(objectmachine_t *obj);
static void resetglobals_release(objectmachine_t *obj);
static void resetglobals_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void resetglobals_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_resetglobals_new(objectmachine_t *decorated_machine)
{
    objectdecorator_resetglobals_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = resetglobals_init;
    obj->release = resetglobals_release;
    obj->update = resetglobals_update;
    obj->render = resetglobals_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void resetglobals_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void resetglobals_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void resetglobals_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    /* reset globals */
    symboltable_clear(symboltable_get_global_table());

    /* reset arrays */
    nanocalc_addons_resetarrays();

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void resetglobals_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_restartlevel_t class */
typedef struct objectdecorator_restartlevel_t objectdecorator_restartlevel_t;
struct objectdecorator_restartlevel_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void restartlevel_init(objectmachine_t *obj);
static void restartlevel_release(objectmachine_t *obj);
static void restartlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void restartlevel_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_restartlevel_new(objectmachine_t *decorated_machine)
{
    objectdecorator_restartlevel_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = restartlevel_init;
    obj->release = restartlevel_release;
    obj->update = restartlevel_update;
    obj->render = restartlevel_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}





/* private methods */
void restartlevel_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void restartlevel_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void restartlevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    level_restart();

    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void restartlevel_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_returntopreviousstate_t class */
typedef struct objectdecorator_returntopreviousstate_t objectdecorator_returntopreviousstate_t;
struct objectdecorator_returntopreviousstate_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void returntopreviousstate_init(objectmachine_t *obj);
static void returntopreviousstate_release(objectmachine_t *obj);
static void returntopreviousstate_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void returntopreviousstate_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_returntopreviousstate_new(objectmachine_t *decorated_machine)
{
    objectdecorator_returntopreviousstate_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = returntopreviousstate_init;
    obj->release = returntopreviousstate_release;
    obj->update = returntopreviousstate_update;
    obj->render = returntopreviousstate_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void returntopreviousstate_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void returntopreviousstate_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void returntopreviousstate_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    object_t *object = obj->get_object_instance(obj);
    objectvm_return_to_previous_state(object->vm);

    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void returntopreviousstate_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_savelevel_t class */
typedef struct objectdecorator_savelevel_t objectdecorator_savelevel_t;
struct objectdecorator_savelevel_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void savelevel_init(objectmachine_t *obj);
static void savelevel_release(objectmachine_t *obj);
static void savelevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void savelevel_render(objectmachine_t *obj, v2d_t camera_position);
static void fix_objects(object_t *obj, void *any_data); /* will fix obj and its children (ie, set ptr->created_from_editor to TRUE) */
static void unfix_objects(object_t *obj, void *any_data); /* will undo whatever fix_objects() did */





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_savelevel_new(objectmachine_t *decorated_machine)
{
    objectdecorator_savelevel_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = savelevel_init;
    obj->release = savelevel_release;
    obj->update = savelevel_update;
    obj->render = savelevel_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}





/* private methods */
void savelevel_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void savelevel_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void savelevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *o = obj->get_object_instance(obj);

    fix_objects(o, NULL);
    level_persist();
    unfix_objects(o, NULL);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void savelevel_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* ---------------------------- */

void fix_objects(object_t *obj, void *any_data)
{
    obj->created_from_editor ^= 0x10;
    enemy_visit_children(obj, any_data, (void(*)(enemy_t*,void*))fix_objects);
}

void unfix_objects(object_t *obj, void *any_data)
{
    obj->created_from_editor ^= 0x10;
    enemy_visit_children(obj, any_data, (void(*)(enemy_t*,void*))fix_objects);
}

/* objectdecorator_setabsoluteposition_t class */
typedef struct objectdecorator_setabsoluteposition_t objectdecorator_setabsoluteposition_t;
struct objectdecorator_setabsoluteposition_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *pos_x, *pos_y;
};

/* private methods */
static void setabsoluteposition_init(objectmachine_t *obj);
static void setabsoluteposition_release(objectmachine_t *obj);
static void setabsoluteposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setabsoluteposition_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setabsoluteposition_new(objectmachine_t *decorated_machine, expression_t *xpos, expression_t *ypos)
{
    objectdecorator_setabsoluteposition_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setabsoluteposition_init;
    obj->release = setabsoluteposition_release;
    obj->update = setabsoluteposition_update;
    obj->render = setabsoluteposition_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->pos_x = xpos;
    me->pos_y = ypos;

    return obj;
}





/* private methods */
void setabsoluteposition_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setabsoluteposition_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setabsoluteposition_t *me = (objectdecorator_setabsoluteposition_t*)obj;

    expression_destroy(me->pos_x);
    expression_destroy(me->pos_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setabsoluteposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setabsoluteposition_t *me = (objectdecorator_setabsoluteposition_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    v2d_t pos = v2d_new(expression_evaluate(me->pos_x), expression_evaluate(me->pos_y));

    object->actor->position = pos;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setabsoluteposition_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setalpha_t class */
typedef struct objectdecorator_setalpha_t objectdecorator_setalpha_t;
struct objectdecorator_setalpha_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *alpha; /* 0.0f (invisible) <= alpha <= 1.0f (opaque) */
};

/* private methods */
static void setalpha_init(objectmachine_t *obj);
static void setalpha_release(objectmachine_t *obj);
static void setalpha_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setalpha_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setalpha_new(objectmachine_t *decorated_machine, expression_t *alpha)
{
    objectdecorator_setalpha_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setalpha_init;
    obj->release = setalpha_release;
    obj->update = setalpha_update;
    obj->render = setalpha_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->alpha = alpha;

    return obj;
}





/* private methods */
void setalpha_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setalpha_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setalpha_t *me = (objectdecorator_setalpha_t*)obj;

    expression_destroy(me->alpha);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setalpha_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setalpha_t *me = (objectdecorator_setalpha_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float alpha = clip(expression_evaluate(me->alpha), 0.0f, 1.0f);

    object->actor->alpha = alpha;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setalpha_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setangle_t class */
typedef struct objectdecorator_setangle_t objectdecorator_setangle_t;
struct objectdecorator_setangle_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *angle;
};

/* private methods */
static void setangle_init(objectmachine_t *obj);
static void setangle_release(objectmachine_t *obj);
static void setangle_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setangle_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setangle_new(objectmachine_t *decorated_machine, expression_t *angle)
{
    objectdecorator_setangle_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setangle_init;
    obj->release = setangle_release;
    obj->update = setangle_update;
    obj->render = setangle_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->angle = angle;

    return obj;
}





/* private methods */
void setangle_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setangle_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setangle_t *me = (objectdecorator_setangle_t*)obj;

    expression_destroy(me->angle);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setangle_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setangle_t *me = (objectdecorator_setangle_t*)obj;
    object_t *object = obj->get_object_instance(obj);

    float angle = expression_evaluate(me->angle);
    object->actor->angle = angle * PI / 180.0f;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setangle_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* strategy pattern */
typedef struct objectdecorator_setanimationstrategy_t objectdecorator_setanimationstrategy_t;
struct objectdecorator_setanimationstrategy_t { /* <<interface>> */
    void (*init)(objectmachine_t *o);
    void (*release)(objectmachine_t *o);
    void (*update)(objectmachine_t *o);
};

/* command: set_animation */
typedef struct objectdecorator_setanimationstrategy_anim_t objectdecorator_setanimationstrategy_anim_t;
struct objectdecorator_setanimationstrategy_anim_t { /* implements objectdecorator_setanimationstrategy_t */
    objectdecorator_setanimationstrategy_t base;
    char *sprite_name;
    expression_t *animation_id;
};
static objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_anim_new(const char *sprite_name, expression_t *animation_id);
static void objectdecorator_setanimationstrategy_anim_init(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_anim_release(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_anim_update(objectmachine_t *o);

/* command: set_animation_frame */
typedef struct objectdecorator_setanimationstrategy_frame_t objectdecorator_setanimationstrategy_frame_t;
struct objectdecorator_setanimationstrategy_frame_t { /* implements objectdecorator_setanimationstrategy_t */
    objectdecorator_setanimationstrategy_t base;
    expression_t *animation_frame;
};
static objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_frame_new(expression_t *animation_frame);
static void objectdecorator_setanimationstrategy_frame_init(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_frame_release(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_frame_update(objectmachine_t *o);

/* command: set_animation_speed_factor */
typedef struct objectdecorator_setanimationstrategy_speed_t objectdecorator_setanimationstrategy_speed_t;
struct objectdecorator_setanimationstrategy_speed_t { /* implements objectdecorator_setanimationstrategy_t */
    objectdecorator_setanimationstrategy_t base;
    expression_t *animation_speed;
};
static objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_speed_new(expression_t *animation_speed);
static void objectdecorator_setanimationstrategy_speed_init(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_speed_release(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_speed_update(objectmachine_t *o);



/* ------------------------------------------ */


/* objectdecorator_setanimation_t class */
typedef struct objectdecorator_setanimation_t objectdecorator_setanimation_t;
struct objectdecorator_setanimation_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    objectdecorator_setanimationstrategy_t *strategy;
};

/* private methods */
static void setanimation_init(objectmachine_t *obj);
static void setanimation_release(objectmachine_t *obj);
static void setanimation_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setanimation_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* setanimation_make_decorator(objectdecorator_setanimationstrategy_t *strategy, objectmachine_t *decorated_machine);
static void change_the_animation(objectmachine_t *obj);


/* ------------------------------------------ */


/* public methods */

objectmachine_t* objectdecorator_setanimation_new(objectmachine_t *decorated_machine, const char *sprite_name, expression_t *animation_id)
{
    return setanimation_make_decorator( objectdecorator_setanimationstrategy_anim_new(sprite_name, animation_id), decorated_machine );
}

objectmachine_t* objectdecorator_setanimationframe_new(objectmachine_t *decorated_machine, expression_t *animation_frame)
{
    return setanimation_make_decorator( objectdecorator_setanimationstrategy_frame_new(animation_frame), decorated_machine );
}

objectmachine_t* objectdecorator_setanimationspeedfactor_new(objectmachine_t *decorated_machine, expression_t *animation_speed_factor)
{
    return setanimation_make_decorator( objectdecorator_setanimationstrategy_speed_new(animation_speed_factor), decorated_machine );
}





/* private methods */
void setanimation_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;

    me->strategy->init(obj);

    decorated_machine->init(decorated_machine);
}

void setanimation_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;

    me->strategy->release(obj);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setanimation_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;

    me->strategy->update(obj);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setanimation_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}



/* ------------------------------------------ */


/* command: set_animation */
objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_anim_new(const char *sprite_name, expression_t *animation_id)
{
    objectdecorator_setanimationstrategy_anim_t *s = mallocx(sizeof *s); /* strategy */
    objectdecorator_setanimationstrategy_t *p = (objectdecorator_setanimationstrategy_t*)s; /* parent class */

    p->init = objectdecorator_setanimationstrategy_anim_init;
    p->release = objectdecorator_setanimationstrategy_anim_release;
    p->update = objectdecorator_setanimationstrategy_anim_update;
    s->sprite_name = str_dup(sprite_name);
    s->animation_id = animation_id;

    return p;
}

void objectdecorator_setanimationstrategy_anim_init(objectmachine_t *o)
{
    change_the_animation(o);
}

void objectdecorator_setanimationstrategy_anim_release(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_anim_t *s = (objectdecorator_setanimationstrategy_anim_t*)(me->strategy);

    expression_destroy(s->animation_id);
    free(s->sprite_name);
    free(s);
}

void objectdecorator_setanimationstrategy_anim_update(objectmachine_t *o)
{
    change_the_animation(o);
}



/* command: set_animation_frame */
objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_frame_new(expression_t *animation_frame)
{
    objectdecorator_setanimationstrategy_frame_t *s = mallocx(sizeof *s); /* strategy */
    objectdecorator_setanimationstrategy_t *p = (objectdecorator_setanimationstrategy_t*)s; /* parent class */

    p->init = objectdecorator_setanimationstrategy_frame_init;
    p->release = objectdecorator_setanimationstrategy_frame_release;
    p->update = objectdecorator_setanimationstrategy_frame_update;
    s->animation_frame = animation_frame;

    return p;
}

void objectdecorator_setanimationstrategy_frame_init(objectmachine_t *o)
{
    ; /* empty */
}

void objectdecorator_setanimationstrategy_frame_release(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_frame_t *s = (objectdecorator_setanimationstrategy_frame_t*)(me->strategy);

    expression_destroy(s->animation_frame);
    free(s);
}

void objectdecorator_setanimationstrategy_frame_update(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_frame_t *s = (objectdecorator_setanimationstrategy_frame_t*)(me->strategy);
    object_t *object = o->get_object_instance(o);

    actor_change_animation_frame(object->actor, (int)expression_evaluate(s->animation_frame));
}


/* command: set_animation_speed_factor */
objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_speed_new(expression_t *animation_speed)
{
    objectdecorator_setanimationstrategy_speed_t *s = mallocx(sizeof *s); /* strategy */
    objectdecorator_setanimationstrategy_t *p = (objectdecorator_setanimationstrategy_t*)s; /* parent class */

    p->init = objectdecorator_setanimationstrategy_speed_init;
    p->release = objectdecorator_setanimationstrategy_speed_release;
    p->update = objectdecorator_setanimationstrategy_speed_update;
    s->animation_speed = animation_speed;

    return p;
}

void objectdecorator_setanimationstrategy_speed_init(objectmachine_t *o)
{
    ; /* empty */
}

void objectdecorator_setanimationstrategy_speed_release(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_speed_t *s = (objectdecorator_setanimationstrategy_speed_t*)(me->strategy);

    expression_destroy(s->animation_speed);
    free(s);
}

void objectdecorator_setanimationstrategy_speed_update(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_speed_t *s = (objectdecorator_setanimationstrategy_speed_t*)(me->strategy);
    object_t *object = o->get_object_instance(o);

    actor_change_animation_speed_factor(object->actor, expression_evaluate(s->animation_speed));
}



/* ------------------------------------------ */



/* this method effectively changes the animation of the object */
void change_the_animation(objectmachine_t *obj)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    objectdecorator_setanimationstrategy_anim_t *s = (objectdecorator_setanimationstrategy_anim_t*)(me->strategy);
    int animation_id = (int)expression_evaluate(s->animation_id);
    animation_t *anim = sprite_get_animation(s->sprite_name, animation_id);

    actor_change_animation(object->actor, anim);
}

/* instantiates a decorator */
objectmachine_t* setanimation_make_decorator(objectdecorator_setanimationstrategy_t *strategy, objectmachine_t *decorated_machine)
{
    objectdecorator_setanimation_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setanimation_init;
    obj->release = setanimation_release;
    obj->update = setanimation_update;
    obj->render = setanimation_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->strategy = strategy;

    return obj;
}

/* objectdecorator_setobstacle_t class */
typedef struct objectdecorator_setobstacle_t objectdecorator_setobstacle_t;
struct objectdecorator_setobstacle_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int is_obstacle; /* should the object be an obstacle brick or not? */
    expression_t *angle; /* if this is an obstacle, what is my angle a, 0 <= a < 360? */
};

/* private methods */
static void setobstacle_init(objectmachine_t *obj);
static void setobstacle_release(objectmachine_t *obj);
static void setobstacle_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setobstacle_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setobstacle_new(objectmachine_t *decorated_machine, int is_obstacle, expression_t *angle)
{
    objectdecorator_setobstacle_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setobstacle_init;
    obj->release = setobstacle_release;
    obj->update = setobstacle_update;
    obj->render = setobstacle_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->is_obstacle = is_obstacle;
    me->angle = angle;

    return obj;
}





/* private methods */
void setobstacle_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setobstacle_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setobstacle_t *me = (objectdecorator_setobstacle_t*)obj;

    expression_destroy(me->angle);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setobstacle_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setobstacle_t *me = (objectdecorator_setobstacle_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    /*float angle = expression_evaluate(me->angle);*/ /* deprecated */

    /* update collision mask */
    if(object->obstacle != me->is_obstacle) {
        if(object->mask != NULL)
            object->mask = collisionmask_destroy(object->mask);
        if(me->is_obstacle)
            object->mask = collisionmask_create_box(
                image_width(actor_image(object->actor)),
                image_height(actor_image(object->actor))
            );
    }

    /* update attributes */
    object->obstacle = me->is_obstacle;

    /* done */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setobstacle_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setplayeranimation_t class */
typedef struct objectdecorator_setplayeranimation_t objectdecorator_setplayeranimation_t;
struct objectdecorator_setplayeranimation_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *sprite_name;
    expression_t *animation_id;
};

/* private methods */
static void setplayeranimation_init(objectmachine_t *obj);
static void setplayeranimation_release(objectmachine_t *obj);
static void setplayeranimation_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setplayeranimation_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setplayeranimation_new(objectmachine_t *decorated_machine, const char *sprite_name, expression_t *animation_id)
{
    objectdecorator_setplayeranimation_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setplayeranimation_init;
    obj->release = setplayeranimation_release;
    obj->update = setplayeranimation_update;
    obj->render = setplayeranimation_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->sprite_name = str_dup(sprite_name);
    me->animation_id = animation_id;

    return obj;
}





/* private methods */
void setplayeranimation_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setplayeranimation_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayeranimation_t *me = (objectdecorator_setplayeranimation_t*)obj;

    free(me->sprite_name);
    expression_destroy(me->animation_id);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setplayeranimation_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayeranimation_t *me = (objectdecorator_setplayeranimation_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));
    int animation_id = (int)expression_evaluate(me->animation_id);

    player_override_animation(player, sprite_get_animation(me->sprite_name, animation_id));

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setplayeranimation_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setplayerinputmap_t class */
typedef struct objectdecorator_setplayerinputmap_t objectdecorator_setplayerinputmap_t;
struct objectdecorator_setplayerinputmap_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char* inputmap_name;
};

/* private methods */
static void setplayerinputmap_init(objectmachine_t *obj);
static void setplayerinputmap_release(objectmachine_t *obj);
static void setplayerinputmap_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setplayerinputmap_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setplayerinputmap_new(objectmachine_t *decorated_machine, const char *inputmap_name)
{
    objectdecorator_setplayerinputmap_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setplayerinputmap_init;
    obj->release = setplayerinputmap_release;
    obj->update = setplayerinputmap_update;
    obj->render = setplayerinputmap_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->inputmap_name = str_dup(inputmap_name);

    return obj;
}




/* private methods */
void setplayerinputmap_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setplayerinputmap_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerinputmap_t *me = (objectdecorator_setplayerinputmap_t*)obj;

    free(me->inputmap_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setplayerinputmap_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerinputmap_t *me = (objectdecorator_setplayerinputmap_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    input_t *in = player->actor->input;

    /* I'm sure 'in' is an inputuserdefined_t* */
    input_change_mapping((inputuserdefined_t*)in, me->inputmap_name);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setplayerinputmap_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setplayerposition_t class */
typedef struct objectdecorator_setplayerposition_t objectdecorator_setplayerposition_t;
struct objectdecorator_setplayerposition_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *offset_x, *offset_y;
};

/* private methods */
static void setplayerposition_init(objectmachine_t *obj);
static void setplayerposition_release(objectmachine_t *obj);
static void setplayerposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setplayerposition_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setplayerposition_new(objectmachine_t *decorated_machine, expression_t *xpos, expression_t *ypos)
{
    objectdecorator_setplayerposition_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setplayerposition_init;
    obj->release = setplayerposition_release;
    obj->update = setplayerposition_update;
    obj->render = setplayerposition_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->offset_x = xpos;
    me->offset_y = ypos;

    return obj;
}





/* private methods */
void setplayerposition_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setplayerposition_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerposition_t *me = (objectdecorator_setplayerposition_t*)obj;

    expression_destroy(me->offset_x);
    expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setplayerposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerposition_t *me = (objectdecorator_setplayerposition_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    v2d_t offset = v2d_new(expression_evaluate(me->offset_x), expression_evaluate(me->offset_y));

    player->actor->position = v2d_add(object->actor->position, offset);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setplayerposition_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setplayerspeed_t class */
typedef struct objectdecorator_setplayerspeed_t objectdecorator_setplayerspeed_t;
struct objectdecorator_setplayerspeed_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed;
    void (*strategy)(player_t*,expression_t*);
};

/* private methods */
static void setplayerspeed_init(objectmachine_t *obj);
static void setplayerspeed_release(objectmachine_t *obj);
static void setplayerspeed_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setplayerspeed_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* setplayerspeed_make_decorator(objectmachine_t *decorated_machine, expression_t *speed, void (*strategy)(player_t*,expression_t*));

/* private strategies */
static void set_xspeed(player_t *player, expression_t *speed);
static void set_yspeed(player_t *player, expression_t *speed);




/* public methods */
objectmachine_t* objectdecorator_setplayerxspeed_new(objectmachine_t *decorated_machine, expression_t *speed)
{
    return setplayerspeed_make_decorator(decorated_machine, speed, set_xspeed);
}

objectmachine_t* objectdecorator_setplayeryspeed_new(objectmachine_t *decorated_machine, expression_t *speed)
{
    return setplayerspeed_make_decorator(decorated_machine, speed, set_yspeed);
}





/* private methods */
objectmachine_t* setplayerspeed_make_decorator(objectmachine_t *decorated_machine, expression_t *speed, void (*strategy)(player_t*,expression_t*))
{
    objectdecorator_setplayerspeed_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setplayerspeed_init;
    obj->release = setplayerspeed_release;
    obj->update = setplayerspeed_update;
    obj->render = setplayerspeed_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed = speed;
    me->strategy = strategy;

    return obj;
}


void setplayerspeed_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setplayerspeed_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerspeed_t *me = (objectdecorator_setplayerspeed_t*)obj;

    expression_destroy(me->speed);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setplayerspeed_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerspeed_t *me = (objectdecorator_setplayerspeed_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    me->strategy(player, me->speed);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setplayerspeed_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* private strategies */
void set_xspeed(player_t *player, expression_t *speed)
{
    player->actor->speed.x = expression_evaluate(speed);
}

void set_yspeed(player_t *player, expression_t *speed)
{
    player->actor->speed.y = expression_evaluate(speed);
}

/* objectdecorator_setscale_t class */
typedef struct objectdecorator_setscale_t objectdecorator_setscale_t;
struct objectdecorator_setscale_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *scale_x, *scale_y;
};

/* private methods */
static void setscale_init(objectmachine_t *obj);
static void setscale_release(objectmachine_t *obj);
static void setscale_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setscale_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setscale_new(objectmachine_t *decorated_machine, expression_t *scale_x, expression_t *scale_y)
{
    objectdecorator_setscale_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setscale_init;
    obj->release = setscale_release;
    obj->update = setscale_update;
    obj->render = setscale_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->scale_x = scale_x;
    me->scale_y = scale_y;

    return obj;
}





/* private methods */
void setscale_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setscale_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setscale_t *me = (objectdecorator_setscale_t*)obj;

    expression_destroy(me->scale_x);
    expression_destroy(me->scale_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setscale_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setscale_t *me = (objectdecorator_setscale_t*)obj;
    object_t *object = obj->get_object_instance(obj);

    float scale_x = max(0.0f, expression_evaluate(me->scale_x));
    float scale_y = max(0.0f, expression_evaluate(me->scale_y));
    object->actor->scale = v2d_new(scale_x, scale_y);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setscale_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_setzindex_t class */
typedef struct objectdecorator_setzindex_t objectdecorator_setzindex_t;
struct objectdecorator_setzindex_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *zindex;
};

/* private methods */
static void setzindex_init(objectmachine_t *obj);
static void setzindex_release(objectmachine_t *obj);
static void setzindex_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setzindex_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setzindex_new(objectmachine_t *decorated_machine, expression_t *zindex)
{
    objectdecorator_setzindex_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setzindex_init;
    obj->release = setzindex_release;
    obj->update = setzindex_update;
    obj->render = setzindex_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->zindex = zindex;

    return obj;
}




/* private methods */
void setzindex_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setzindex_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setzindex_t *me = (objectdecorator_setzindex_t*)obj;

    expression_destroy(me->zindex);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setzindex_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setzindex_t *me = (objectdecorator_setzindex_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float zindex = expression_evaluate(me->zindex); /* no clip() */

    /* update */
    object->zindex = zindex;

    /* decorator pattern */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setzindex_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_showhide_t class */
typedef struct objectdecorator_showhide_t objectdecorator_showhide_t;
struct objectdecorator_showhide_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int show; /* TRUE or FALSE */
};

/* private methods */
static void showhide_init(objectmachine_t *obj);
static void showhide_release(objectmachine_t *obj);
static void showhide_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void showhide_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* showhide_make_decorator(objectmachine_t *decorated_machine, int show);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_show_new(objectmachine_t *decorated_machine)
{
    return showhide_make_decorator(decorated_machine, TRUE);
}

objectmachine_t* objectdecorator_hide_new(objectmachine_t *decorated_machine)
{
    return showhide_make_decorator(decorated_machine, FALSE);
}


/* make decorator */
static objectmachine_t* showhide_make_decorator(objectmachine_t *decorated_machine, int show)
{
    objectdecorator_showhide_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = showhide_init;
    obj->release = showhide_release;
    obj->update = showhide_update;
    obj->render = showhide_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->show = show;

    return obj;
}




/* private methods */
void showhide_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void showhide_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void showhide_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    object_t *object = obj->get_object_instance(obj);
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_showhide_t *me = (objectdecorator_showhide_t*)dec;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    object->actor->visible = me->show;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void showhide_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_simulatebutton_t class */
typedef struct objectdecorator_simulatebutton_t objectdecorator_simulatebutton_t;
struct objectdecorator_simulatebutton_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    inputbutton_t button; /* button to be simulated */
    void (*callback)(input_t*,inputbutton_t); /* strategy */
};

/* private methods */
static void simulatebutton_init(objectmachine_t *obj);
static void simulatebutton_release(objectmachine_t *obj);
static void simulatebutton_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void simulatebutton_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* simulatebutton_make_decorator(objectmachine_t *decorated_machine, const char *button_name, void (*callback)(input_t*,inputbutton_t));



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_simulatebuttondown_new(objectmachine_t *decorated_machine, const char *button_name)
{
    return simulatebutton_make_decorator(decorated_machine, button_name, input_simulate_button_down);
}

objectmachine_t* objectdecorator_simulatebuttonup_new(objectmachine_t *decorated_machine, const char *button_name)
{
    return simulatebutton_make_decorator(decorated_machine, button_name, input_simulate_button_up);
}

objectmachine_t* simulatebutton_make_decorator(objectmachine_t *decorated_machine, const char *button_name, void (*callback)(input_t*,inputbutton_t))
{
    objectdecorator_simulatebutton_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = simulatebutton_init;
    obj->release = simulatebutton_release;
    obj->update = simulatebutton_update;
    obj->render = simulatebutton_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->callback = callback;
    me->button = IB_UP;

    if(str_icmp(button_name, "up") == 0)
        me->button = IB_UP;
    else if(str_icmp(button_name, "right") == 0)
        me->button = IB_RIGHT;
    else if(str_icmp(button_name, "down") == 0)
        me->button = IB_DOWN;
    else if(str_icmp(button_name, "left") == 0)
        me->button = IB_LEFT;
    else if(str_icmp(button_name, "fire1") == 0)
        me->button = IB_FIRE1;
    else if(str_icmp(button_name, "fire2") == 0)
        me->button = IB_FIRE2;
    else if(str_icmp(button_name, "fire3") == 0)
        me->button = IB_FIRE3;
    else if(str_icmp(button_name, "fire4") == 0)
        me->button = IB_FIRE4;
    else if(str_icmp(button_name, "fire5") == 0)
        me->button = IB_FIRE5;
    else if(str_icmp(button_name, "fire6") == 0)
        me->button = IB_FIRE6;
    else if(str_icmp(button_name, "fire7") == 0)
        me->button = IB_FIRE7;
    else if(str_icmp(button_name, "fire8") == 0)
        me->button = IB_FIRE8;
    else
        fatal_error("Invalid button '%s' in simulate_button", button_name);

    return obj;
}




/* private methods */
void simulatebutton_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void simulatebutton_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void simulatebutton_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_simulatebutton_t *me = (objectdecorator_simulatebutton_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);

    input_restore(player->actor->input); /* so that non-active players will respond to this command */
    me->callback(player->actor->input, me->button);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void simulatebutton_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_switchcharacter_t class */
typedef struct objectdecorator_switchcharacter_t objectdecorator_switchcharacter_t;
struct objectdecorator_switchcharacter_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *name; /* character name */
    int force_switch; /* forces the character switch, even if the engine does not want it */
};

/* private methods */
static void switchcharacter_init(objectmachine_t *obj);
static void switchcharacter_release(objectmachine_t *obj);
static void switchcharacter_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void switchcharacter_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_switchcharacter_new(objectmachine_t *decorated_machine, const char *name, int force_switch)
{
    objectdecorator_switchcharacter_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = switchcharacter_init;
    obj->release = switchcharacter_release;
    obj->update = switchcharacter_update;
    obj->render = switchcharacter_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->name = (name != NULL && *name != 0) ? str_dup(name) : NULL;
    me->force_switch = force_switch;

    return obj;
}




/* private methods */
void switchcharacter_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void switchcharacter_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_switchcharacter_t *me = (objectdecorator_switchcharacter_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    if(me->name != NULL)
        free(me->name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void switchcharacter_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_switchcharacter_t *me = (objectdecorator_switchcharacter_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = level_player(); /* active player */
    player_t *new_player = NULL;
    int i;

    if(me->name != NULL) {
        for(i=0; i<team_size && new_player == NULL; i++) {
            if(str_icmp(team[i]->name, me->name) == 0)
                new_player = team[i];
        }
    }
    else
        new_player = enemy_get_observed_player(object);

    if(new_player != NULL) {
        int allow_switching, got_dying_player = FALSE;

        for(i=0; i<team_size && !got_dying_player; i++)
            got_dying_player = player_is_dying(team[i]);

        allow_switching = !got_dying_player && !level_has_been_cleared() && !player_is_in_the_air(player) && !player->on_movable_platform && !player_is_frozen(player) && !player->in_locked_area;

        if(allow_switching || me->force_switch)
            level_change_player(new_player);
        else
            sound_play(SFX_DENY);
    }
    else
        fatal_error("Can't switch character: player '%s' does not exist!", me->name);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void switchcharacter_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

typedef enum { TEXTOUT_LEFT, TEXTOUT_CENTRE, TEXTOUT_RIGHT } textoutstyle_t;


/* objectdecorator_textout_t class */
typedef struct objectdecorator_textout_t objectdecorator_textout_t;
struct objectdecorator_textout_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    textoutstyle_t style;
    font_t *fnt;
    char *text;
    expression_t *xpos, *ypos;
    expression_t *max_width;
    expression_t *index_of_first_char, *length;
};

/* private methods */
static void textout_init(objectmachine_t *obj);
static void textout_release(objectmachine_t *obj);
static void textout_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void textout_render(objectmachine_t *obj, v2d_t camera_position);

objectmachine_t* textout_make_decorator(objectmachine_t *decorated_machine, textoutstyle_t style, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length);

static int tagged_strlen(const char *s);

/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_textout_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    return textout_make_decorator(decorated_machine, TEXTOUT_LEFT, font_name, xpos, ypos, text, max_width, index_of_first_char, length);
}

objectmachine_t* objectdecorator_textoutcentre_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    return textout_make_decorator(decorated_machine, TEXTOUT_CENTRE, font_name, xpos, ypos, text, max_width, index_of_first_char, length);
}

objectmachine_t* objectdecorator_textoutright_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    return textout_make_decorator(decorated_machine, TEXTOUT_RIGHT, font_name, xpos, ypos, text, max_width, index_of_first_char, length);
}




/* private methods */
void textout_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void textout_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_textout_t *me = (objectdecorator_textout_t*)obj;

    expression_destroy(me->xpos);
    expression_destroy(me->ypos);
    expression_destroy(me->max_width);
    expression_destroy(me->index_of_first_char);
    expression_destroy(me->length);
    font_destroy(me->fnt);
    free(me->text);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void textout_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_textout_t *me = (objectdecorator_textout_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    symboltable_t *st = objectvm_get_symbol_table(object->vm);
    char *processed_text;
    int start, length;
    v2d_t pos;

    /* calculate the range of the string (no need to clip() it) */
    start = (int)expression_evaluate(me->index_of_first_char);
    length = (int)expression_evaluate(me->length);

    /* configuring the font */
    font_use_substring(me->fnt, start, length);
    font_set_width(me->fnt, (int)expression_evaluate(me->max_width));

    /* font text */
    processed_text = nanocalc_interpolate_string(me->text, st);
    font_set_text(me->fnt, "%s", processed_text);
    free(processed_text);

    /* symbol table tricks */
    symboltable_set(st, "$_STRLEN", tagged_strlen(font_get_text(me->fnt))); /* store the length of the text in $_STRLEN */

    /* font position */
    pos = v2d_new(expression_evaluate(me->xpos), expression_evaluate(me->ypos));
    /*switch(me->style) {
    case TEXTOUT_LEFT: break;
    case TEXTOUT_CENTRE: pos.x -= font_get_textsize(me->fnt).x/2; break;
    case TEXTOUT_RIGHT: pos.x -= font_get_textsize(me->fnt).x; break;
    }*/
    switch(me->style) {
    case TEXTOUT_LEFT: font_set_align(me->fnt, FONTALIGN_LEFT); break;
    case TEXTOUT_CENTRE: font_set_align(me->fnt, FONTALIGN_CENTER); break;
    case TEXTOUT_RIGHT: font_set_align(me->fnt, FONTALIGN_RIGHT); break;
    }
    font_set_position(me->fnt, v2d_add(object->actor->position, pos));

    /* done! */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void textout_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_textout_t *me = (objectdecorator_textout_t*)obj;

    /* textout_render */
    font_render(me->fnt, camera_position);

    /* done! */
    decorated_machine->render(decorated_machine, camera_position);
}


objectmachine_t* textout_make_decorator(objectmachine_t *decorated_machine, textoutstyle_t style, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    objectdecorator_textout_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = textout_init;
    obj->release = textout_release;
    obj->update = textout_update;
    obj->render = textout_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->style = style;
    me->fnt = font_create(font_name);
    me->xpos = xpos;
    me->ypos = ypos;
    me->text = str_dup(text);
    me->max_width = max_width;
    me->index_of_first_char = index_of_first_char;
    me->length = length;

    return obj;
}


/* stuff */
static int tagged_strlen(const char *s)
{
    const char *p;
    int k = 0, in_tag = FALSE;

    for(p=s; *p; p++) {
        if(*p == '<') { in_tag = TRUE; continue; }
        else if(*p == '>') { in_tag = FALSE; continue; }
        k += in_tag ? 0 : 1;
    }

    return k;
}

/* objectdecorator_variables_t class */
typedef struct objectdecorator_variables_t objectdecorator_variables_t;
struct objectdecorator_variables_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *expr;
    char *new_state_name;
    int (*must_change_state)(float); /* strategy */
};

/* private methods */
static void varcommand_init(objectmachine_t *obj);
static void varcommand_release(objectmachine_t *obj);
static void varcommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void varcommand_render(objectmachine_t *obj, v2d_t camera_position);

/* private strategies */
static int let_strategy(float expr_result) { return FALSE; }
static int if_strategy(float expr_result) { return fabs(expr_result) >= 1e-5; }
static int unless_strategy(float expr_result) { return fabs(expr_result) < 1e-5; }




/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_let_new(objectmachine_t *decorated_machine, expression_t *expr)
{
    objectdecorator_variables_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = varcommand_init;
    obj->release = varcommand_release;
    obj->update = varcommand_update;
    obj->render = varcommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->expr = expr;
    me->new_state_name = NULL;
    me->must_change_state = let_strategy;

    return obj;
}

objectmachine_t* objectdecorator_if_new(objectmachine_t *decorated_machine, expression_t *expr, const char *new_state_name)
{
    objectdecorator_variables_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = varcommand_init;
    obj->release = varcommand_release;
    obj->update = varcommand_update;
    obj->render = varcommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->expr = expr;
    me->new_state_name = str_dup(new_state_name);
    me->must_change_state = if_strategy;

    return obj;
}

objectmachine_t* objectdecorator_unless_new(objectmachine_t *decorated_machine, expression_t *expr, const char *new_state_name)
{
    objectdecorator_variables_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = varcommand_init;
    obj->release = varcommand_release;
    obj->update = varcommand_update;
    obj->render = varcommand_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->expr = expr;
    me->new_state_name = str_dup(new_state_name);
    me->must_change_state = unless_strategy;

    return obj;
}



/* private methods */
void varcommand_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void varcommand_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_variables_t *me = (objectdecorator_variables_t*)obj;

    expression_destroy(me->expr);
    if(me->new_state_name != NULL)
        free(me->new_state_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void varcommand_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_variables_t *me = (objectdecorator_variables_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float result = expression_evaluate(me->expr);

    if(me->must_change_state(result))
        objectvm_set_current_state(object->vm, me->new_state_name);
    else
        decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void varcommand_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* objectdecorator_walk_t class */
typedef struct objectdecorator_walk_t objectdecorator_walk_t;
struct objectdecorator_walk_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed; /* movement speed */
    float direction; /* -1.0f or 1.0f */
};

/* private methods */
static void walk_init(objectmachine_t *obj);
static void walk_release(objectmachine_t *obj);
static void walk_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void walk_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_walk_new(objectmachine_t *decorated_machine, expression_t *speed)
{
    objectdecorator_walk_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = walk_init;
    obj->release = walk_release;
    obj->update = walk_update;
    obj->render = walk_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed = speed;

    return obj;
}




/* private methods */
void walk_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_walk_t *me = (objectdecorator_walk_t*)obj;

    me->direction = (random(2) == 0) ? -1.0f : 1.0f;

    decorated_machine->init(decorated_machine);
}

void walk_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_walk_t *me = (objectdecorator_walk_t*)obj;

    expression_destroy(me->speed);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void walk_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_walk_t *me = (objectdecorator_walk_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    float dt = timer_get_delta();
    brick_t *up = NULL, *upright = NULL, *right = NULL, *downright = NULL;
    brick_t *down = NULL, *downleft = NULL, *left = NULL, *upleft = NULL;
    float speed = expression_evaluate(me->speed);

    /* move! */
    act->position.x += (me->direction * speed) * dt;

    /* sensors */
    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    /* swap direction when a wall is touched */
    if(right != NULL) {
        if(me->direction > 0.0f) {
            act->position.x = act->hot_spot.x - image_width(actor_image(act)) + brick_position(right).x;
            me->direction = -1.0f;
        }
    }

    if(left != NULL) {
        if(me->direction < 0.0f) {
            act->position.x = act->hot_spot.x + brick_position(left).x + brick_size(left).x;
            me->direction = 1.0f;
        }
    }

    /* I don't want to fall from the platforms! */
    if(down != NULL) {
        if(downright == NULL && downleft != NULL && me->direction > 0.0f)
            me->direction = -1.0f;
        else if(downleft == NULL && downright != NULL && me->direction < 0.0f)
            me->direction = 1.0f;
    }

    /* decorator pattern */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void walk_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

