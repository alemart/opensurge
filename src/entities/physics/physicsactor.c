/*
 * Open Surge Engine
 * physics/physicsactor.c - physics system: actor
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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
#include "physicsactor.h"
#include "sensor.h"
#include "obstaclemap.h"
#include "obstacle.h"
#include "../../core/util.h"
#include "../../core/image.h"
#include "../../core/video.h"
#include "../../core/input.h"
#include "../../core/timer.h"

/* physicsactor class */
struct physicsactor_t
{
    v2d_t position; /* center of the sprite */
    float xsp; /* x speed */
    float ysp; /* y speed */
    float gsp; /* ground speed */
    float acc; /* acceleration */
    float dec; /* deceleration */
    float frc; /* friction */
    float topspeed; /* top speed */
    float topyspeed; /* top y speed */
    float air; /* air acceleration */
    float airdragmultiplier; /* air drag multiplier */
    float airdragthreshold; /* air drag threshold */
    float airdragcondition; /* air drag condition */
    float jmp; /* initial jump velocity */
    float jmprel; /* release jump velocity */
    float grv; /* gravity */
    float slp; /* slope factor */
    float unrollthreshold; /* unroll threshold */
    float rollthreshold; /* roll threshold */
    float rollfrc; /* roll friction */
    float rolldec; /* roll deceleration */
    float rolluphillslp; /* roll uphill slope */
    float rolldownhillslp; /* roll downhill slope */
    float falloffthreshold; /* fall off threshold */
    float brakingthreshold; /* braking animation treshold */
    int angle; /* angle (0-255 clockwise) */
    int in_the_air; /* is the player in the air? */
    physicsactorstate_t state; /* state */
    float horizontal_control_lock_timer; /* lock timer, in seconds */
    int facing_right; /* is the player facing right? */
    movmode_t movmode; /* current movement mode, based on the angle */
    input_t *input; /* input device */
    float wait_timer; /* the time, in seconds, that the physics actor is stopped */
    int winning_pose; /* winning pose enabled? */
    float breathe_timer; /* if greater than zero, set animation to breathing */

    sensor_t *A_normal; /* sensors */
    sensor_t *B_normal;
    sensor_t *C_normal;
    sensor_t *D_normal;
    sensor_t *M_normal;
    sensor_t *U_normal;
    sensor_t *A_intheair;
    sensor_t *B_intheair;
    sensor_t *C_intheair;
    sensor_t *D_intheair;
    sensor_t *M_intheair;
    sensor_t *U_intheair;
    sensor_t *A_jumproll;
    sensor_t *B_jumproll;
    sensor_t *C_jumproll;
    sensor_t *D_jumproll;
    sensor_t *M_jumproll;
    sensor_t *U_jumproll;
};

/* private stuff ;-) */
#define DEBUG video_showmessage
static void run_simulation(physicsactor_t *pa, const obstaclemap_t *obstaclemap);
static char pick_the_best_ground(const physicsactor_t *pa, const obstacle_t *a, const obstacle_t *b, const sensor_t *a_sensor, const sensor_t *b_sensor);
static char pick_the_best_ceiling(const physicsactor_t *pa, const obstacle_t *c, const obstacle_t *d, const sensor_t *c_sensor, const sensor_t *d_sensor);


/*
   this is the character:   O
                            |
                            !

   the character has a few sensors:    U
   A (vertical; left bottom)          ---
   B (vertical; right bottom)       C |O| D
   C (vertical; left top)             -+- M
   D (vertical; right top)          A |!| B
   M (horizontal; middle)           ^^^^^^^
   U (horizontal; up)                    ground

   the position of the sensors may change
   according to the state of the player.
   Instead of modifying the coordinates of
   the sensors, which could complicate
   things, we have multiple, non-mutable
   copies of them, and we retrieve them
   appropriately.
*/
static const sensor_t* sensor_A(const physicsactor_t *pa);
static const sensor_t* sensor_B(const physicsactor_t *pa);
static const sensor_t* sensor_C(const physicsactor_t *pa);
static const sensor_t* sensor_D(const physicsactor_t *pa);
static const sensor_t* sensor_M(const physicsactor_t *pa);
static const sensor_t* sensor_U(const physicsactor_t *pa);


/*
 * sine/cosine table
 * In this subsystem, the angle ranges in 0-255 and increases clockwise
 * conversion formula:
 *     degrees = ((256 - angle) * 1.40625) % 360
 *     angle = (256 - degrees / 1.40625) % 256
 *
 * ps: 180/128 = 1.40625
 */

/* use these macros to get the values */
#define SIN(a) cos_table[((a) + 0x40) & 0xFF]
#define COS(a) cos_table[(a) & 0xFF]

static float cos_table[256] = {
     1.00000f,  0.99970f,  0.99880f,  0.99729f,  0.99518f,  0.99248f,  0.98918f,  0.98528f, 
     0.98079f,  0.97570f,  0.97003f,  0.96378f,  0.95694f,  0.94953f,  0.94154f,  0.93299f, 
     0.92388f,  0.91421f,  0.90399f,  0.89322f,  0.88192f,  0.87009f,  0.85773f,  0.84485f, 
     0.83147f,  0.81758f,  0.80321f,  0.78835f,  0.77301f,  0.75721f,  0.74095f,  0.72425f, 
     0.70711f,  0.68954f,  0.67156f,  0.65317f,  0.63439f,  0.61523f,  0.59570f,  0.57581f, 
     0.55557f,  0.53500f,  0.51410f,  0.49290f,  0.47140f,  0.44961f,  0.42755f,  0.40524f, 
     0.38268f,  0.35990f,  0.33689f,  0.31368f,  0.29028f,  0.26671f,  0.24298f,  0.21910f, 
     0.19509f,  0.17096f,  0.14673f,  0.12241f,  0.09802f,  0.07356f,  0.04907f,  0.02454f, 
     0.00000f, -0.02454f, -0.04907f, -0.07356f, -0.09802f, -0.12241f, -0.14673f, -0.17096f, 
    -0.19509f, -0.21910f, -0.24298f, -0.26671f, -0.29028f, -0.31368f, -0.33689f, -0.35990f, 
    -0.38268f, -0.40524f, -0.42755f, -0.44961f, -0.47140f, -0.49290f, -0.51410f, -0.53500f, 
    -0.55557f, -0.57581f, -0.59570f, -0.61523f, -0.63439f, -0.65317f, -0.67156f, -0.68954f, 
    -0.70711f, -0.72425f, -0.74095f, -0.75721f, -0.77301f, -0.78835f, -0.80321f, -0.81758f, 
    -0.83147f, -0.84485f, -0.85773f, -0.87009f, -0.88192f, -0.89322f, -0.90399f, -0.91421f, 
    -0.92388f, -0.93299f, -0.94154f, -0.94953f, -0.95694f, -0.96378f, -0.97003f, -0.97570f, 
    -0.98079f, -0.98528f, -0.98918f, -0.99248f, -0.99518f, -0.99729f, -0.99880f, -0.99970f, 
    -1.00000f, -0.99970f, -0.99880f, -0.99729f, -0.99518f, -0.99248f, -0.98918f, -0.98528f, 
    -0.98079f, -0.97570f, -0.97003f, -0.96378f, -0.95694f, -0.94953f, -0.94154f, -0.93299f, 
    -0.92388f, -0.91421f, -0.90399f, -0.89322f, -0.88192f, -0.87009f, -0.85773f, -0.84485f, 
    -0.83147f, -0.81758f, -0.80321f, -0.78835f, -0.77301f, -0.75721f, -0.74095f, -0.72425f, 
    -0.70711f, -0.68954f, -0.67156f, -0.65317f, -0.63439f, -0.61523f, -0.59570f, -0.57581f, 
    -0.55557f, -0.53500f, -0.51410f, -0.49290f, -0.47140f, -0.44961f, -0.42756f, -0.40524f, 
    -0.38268f, -0.35990f, -0.33689f, -0.31368f, -0.29028f, -0.26671f, -0.24298f, -0.21910f, 
    -0.19509f, -0.17096f, -0.14673f, -0.12241f, -0.09802f, -0.07356f, -0.04907f, -0.02454f, 
    -0.00000f,  0.02454f,  0.04907f,  0.07356f,  0.09802f,  0.12241f,  0.14673f,  0.17096f, 
     0.19509f,  0.21910f,  0.24298f,  0.26671f,  0.29028f,  0.31368f,  0.33689f,  0.35990f, 
     0.38268f,  0.40524f,  0.42756f,  0.44961f,  0.47140f,  0.49290f,  0.51410f,  0.53500f, 
     0.55557f,  0.57581f,  0.59570f,  0.61523f,  0.63439f,  0.65317f,  0.67156f,  0.68954f, 
     0.70711f,  0.72425f,  0.74095f,  0.75721f,  0.77301f,  0.78835f,  0.80321f,  0.81758f, 
     0.83147f,  0.84485f,  0.85773f,  0.87009f,  0.88192f,  0.89322f,  0.90399f,  0.91421f, 
     0.92388f,  0.93299f,  0.94154f,  0.94953f,  0.95694f,  0.96378f,  0.97003f,  0.97570f, 
     0.98079f,  0.98528f,  0.98918f,  0.99248f,  0.99518f,  0.99729f,  0.99880f,  0.99970f
};

/* public methods */
physicsactor_t* physicsactor_create(v2d_t position)
{
    physicsactor_t *pa = mallocx(sizeof *pa);
    float fpsmul = 60.0f;

    /* initializing... */
    pa->position = position;
    pa->xsp = 0.0f;
    pa->ysp = 0.0f;
    pa->gsp = 0.0f;
    pa->angle = 0x0;
    pa->movmode = MM_FLOOR;
    pa->in_the_air = TRUE;
    pa->state = PAS_STOPPED;
    pa->horizontal_control_lock_timer = 0.0f;
    pa->facing_right = TRUE;
    pa->input = input_create_computer();
    pa->wait_timer = 0.0f;
    pa->winning_pose = FALSE;
    pa->breathe_timer = 0.0f;

    /* initializing some constants ;-) */

    /*
      +----------------------+--------------+-----------------+
      | name                 | magic number | fps multitplier |
      +----------------------+--------------+-----------------+
     */
    pa->acc =                   0.046875f   * fpsmul * fpsmul ;
    pa->dec =                   0.5f        * fpsmul * fpsmul ;
    pa->frc =                   0.046875f   * fpsmul * fpsmul ;
    pa->topspeed =              6.0f        * fpsmul * 1.0f   ;
    pa->topyspeed =             12.0f       * fpsmul * 1.0f   ;
    pa->air =                   0.09375f    * fpsmul * fpsmul ;
    pa->airdragthreshold =      0.125f      * fpsmul * 1.0f   ;
    pa->airdragcondition =      -4.0f       * fpsmul * 1.0f   ;
    pa->jmp =                   -6.5f       * fpsmul * 1.0f   ;
    pa->jmprel =                -4.0f       * fpsmul * 1.0f   ;
    pa->grv =                   0.21875f    * fpsmul * fpsmul ;
    pa->slp =                   0.125f      * fpsmul * fpsmul ;
    pa->unrollthreshold =       0.5f        * fpsmul * 1.0f   ;
    pa->rollthreshold =         1.03125f    * fpsmul * 1.0f   ;
    pa->rollfrc =               0.0234375f  * fpsmul * fpsmul ;
    pa->rolldec =               0.125f      * fpsmul * fpsmul ;
    pa->rolluphillslp =         0.07812f    * fpsmul * fpsmul ;
    pa->rolldownhillslp =       0.3125f     * fpsmul * fpsmul ;
    pa->falloffthreshold =      2.5f        * fpsmul * 1.0f   ;
    pa->brakingthreshold =      4.5f        * fpsmul * 1.0f   ;
    pa->airdragmultiplier =     0.96875f    * 1.0f   * 1.0f   ;
    
    /* sensors */
    pa->A_normal = sensor_create_vertical(-9, 0, 20, image_rgb(0,255,0));
    pa->B_normal = sensor_create_vertical(9, 0, 20, image_rgb(255,255,0));
    pa->C_normal = sensor_create_vertical(-9, -20, 0, image_rgb(0,64,0));
    pa->D_normal = sensor_create_vertical(9, -20, 0, image_rgb(64,64,0));
    pa->M_normal = sensor_create_horizontal(4, -10, 10, image_rgb(255,0,0));
    pa->U_normal = sensor_create_horizontal(-25, -9, 9, image_rgb(255,255,255));

    pa->A_intheair = sensor_create_vertical(-9, 0, 20, image_rgb(0,255,0));
    pa->B_intheair = sensor_create_vertical(9, 0, 20, image_rgb(255,255,0));
    pa->C_intheair = sensor_create_vertical(-9, -20, 0, image_rgb(0,64,0));
    pa->D_intheair = sensor_create_vertical(9, -20, 0, image_rgb(64,64,0));
    pa->M_intheair = sensor_create_horizontal(0, -10, 10, image_rgb(255,0,0));
    pa->U_intheair = sensor_create_horizontal(-25, -9, 9, image_rgb(255,255,255));

    pa->A_jumproll = sensor_create_vertical(-7, 0, 20, image_rgb(0,255,0));
    pa->B_jumproll = sensor_create_vertical(7, 0, 20, image_rgb(255,255,0));
    pa->C_jumproll = sensor_create_vertical(-7, -20, 0, image_rgb(0,64,0));
    pa->D_jumproll = sensor_create_vertical(7, -20, 0, image_rgb(64,64,0));
    pa->M_jumproll = sensor_create_horizontal(0, -10, 10, image_rgb(255,0,0));
    pa->U_jumproll = sensor_create_horizontal(-25, -9, 9, image_rgb(255,255,255));

    /* success!!! ;-) */
    return pa;
}

physicsactor_t* physicsactor_destroy(physicsactor_t *pa)
{
    sensor_destroy(pa->A_normal);
    sensor_destroy(pa->B_normal);
    sensor_destroy(pa->C_normal);
    sensor_destroy(pa->D_normal);
    sensor_destroy(pa->M_normal);
    sensor_destroy(pa->U_normal);
    sensor_destroy(pa->A_intheair);
    sensor_destroy(pa->B_intheair);
    sensor_destroy(pa->C_intheair);
    sensor_destroy(pa->D_intheair);
    sensor_destroy(pa->M_intheair);
    sensor_destroy(pa->U_intheair);
    sensor_destroy(pa->A_jumproll);
    sensor_destroy(pa->B_jumproll);
    sensor_destroy(pa->C_jumproll);
    sensor_destroy(pa->D_jumproll);
    sensor_destroy(pa->M_jumproll);
    sensor_destroy(pa->U_jumproll);

    input_destroy(pa->input);
    free(pa);
    return NULL;
}

void physicsactor_update(physicsactor_t *pa, const obstaclemap_t *obstaclemap)
{
    int i;
    float dt = timer_get_delta();
    const obstacle_t *at_U;

    /* getting hit & winning pose */
    if(pa->state == PAS_GETTINGHIT) {
        input_ignore(pa->input);
        pa->facing_right = pa->xsp < 0.0f;
    }
    else if(pa->winning_pose) {
        /* brake on level clear */
        const float thr = 60.0f;

        for(i=0; i<IB_MAX; i++)
            input_simulate_button_up(pa->input, (inputbutton_t)i);

        pa->gsp = clip(pa->gsp, -1.8f * pa->topspeed, 1.8f * pa->topspeed);
        if(pa->state == PAS_ROLLING)
            pa->state = PAS_BRAKING;

        if(pa->gsp > thr)
            input_simulate_button_down(pa->input, IB_LEFT);
        else if(pa->gsp < -thr)
            input_simulate_button_down(pa->input, IB_RIGHT);
        else
            input_ignore(pa->input);
    }
    else
        input_restore(pa->input);

    /* horizontal control lock timer */
    if(pa->horizontal_control_lock_timer > 0.0f) {
        pa->horizontal_control_lock_timer = max(0.0f, pa->horizontal_control_lock_timer - dt);
        input_simulate_button_up(pa->input, IB_LEFT);
        input_simulate_button_up(pa->input, IB_RIGHT);
        pa->facing_right = pa->gsp > EPSILON;
    }

    /* don't bother jumping */
    at_U = sensor_check(sensor_U(pa), pa->position, pa->movmode, obstaclemap);
    if(at_U != NULL && obstacle_is_solid(at_U))
        input_simulate_button_up(pa->input, IB_FIRE1);

    /* face left/right */
    if((pa->gsp > EPSILON || pa->in_the_air) && input_button_down(pa->input, IB_RIGHT))
        pa->facing_right = TRUE;
    else if((pa->gsp < -EPSILON || pa->in_the_air) && input_button_down(pa->input, IB_LEFT))
        pa->facing_right = FALSE;

    /* get to the real physics... */
    run_simulation(pa, obstaclemap);

    /* reset input */
    for(i=0; i<IB_MAX; i++)
        input_simulate_button_up(pa->input, (inputbutton_t)i);
}

void physicsactor_render_sensors(const physicsactor_t *pa, v2d_t camera_position)
{
    sensor_render(sensor_A(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_B(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_C(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_D(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_M(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_U(pa), pa->position, pa->movmode, camera_position);
}

int physicsactor_is_facing_right(const physicsactor_t *pa)
{
    return pa->facing_right;
}

physicsactorstate_t physicsactor_get_state(const physicsactor_t *pa)
{
    return pa->state;
}

int physicsactor_get_angle(const physicsactor_t *pa)
{
    return pa->angle;
}

v2d_t physicsactor_get_position(const physicsactor_t *pa)
{
    return pa->position;
}

void physicsactor_set_position(physicsactor_t *pa, v2d_t position)
{
    pa->position = position;
}

void physicsactor_lock_horizontally_for(physicsactor_t *pa, float seconds)
{
    pa->horizontal_control_lock_timer = max(seconds, 0.0f);
}

int physicsactor_is_in_the_air(const physicsactor_t *pa)
{
    return pa->in_the_air;
}

void physicsactor_enable_winning_pose(physicsactor_t *pa)
{
    pa->winning_pose = TRUE;
}

movmode_t physicsactor_get_movmode(physicsactor_t *pa)
{
    return pa->movmode;
}

void physicsactor_walk_right(physicsactor_t *pa)
{
    input_simulate_button_down(pa->input, IB_RIGHT);
}

void physicsactor_walk_left(physicsactor_t *pa)
{
    input_simulate_button_down(pa->input, IB_LEFT);
}

void physicsactor_duck(physicsactor_t *pa)
{
    input_simulate_button_down(pa->input, IB_DOWN);
}

void physicsactor_look_up(physicsactor_t *pa)
{
    input_simulate_button_down(pa->input, IB_UP);
}

void physicsactor_jump(physicsactor_t *pa)
{
    input_simulate_button_down(pa->input, IB_FIRE1);
}

void physicsactor_kill(physicsactor_t *pa)
{
    pa->state = PAS_DEAD;
}

void physicsactor_hit(physicsactor_t *pa)
{
    pa->state = PAS_GETTINGHIT;
}

void physicsactor_bounce(physicsactor_t *pa)
{
    pa->state = PAS_JUMPING;
}

void physicsactor_spring(physicsactor_t *pa)
{
    pa->state = PAS_SPRINGING;
}

void physicsactor_roll(physicsactor_t *pa)
{
    pa->state = PAS_ROLLING;
}

void physicsactor_drown(physicsactor_t *pa)
{
    pa->state = PAS_DROWNED;
}

void physicsactor_breathe(physicsactor_t *pa)
{
    pa->state = PAS_BREATHING;
    pa->breathe_timer = 0.5f;
}

/* getters and setters */
#define GENERATE_ACCESSOR_AND_MUTATOR_OF(x) \
float physicsactor_get_##x(const physicsactor_t *pa) \
{ \
    return pa->x; \
} \
\
void physicsactor_set_##x(physicsactor_t *pa, float value) \
{ \
    pa->x = value; \
}

GENERATE_ACCESSOR_AND_MUTATOR_OF(xsp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(ysp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(gsp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(acc)
GENERATE_ACCESSOR_AND_MUTATOR_OF(dec)
GENERATE_ACCESSOR_AND_MUTATOR_OF(frc)
GENERATE_ACCESSOR_AND_MUTATOR_OF(topspeed)
GENERATE_ACCESSOR_AND_MUTATOR_OF(topyspeed)
GENERATE_ACCESSOR_AND_MUTATOR_OF(air)
GENERATE_ACCESSOR_AND_MUTATOR_OF(airdragmultiplier)
GENERATE_ACCESSOR_AND_MUTATOR_OF(airdragthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(airdragcondition)
GENERATE_ACCESSOR_AND_MUTATOR_OF(jmp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(jmprel)
GENERATE_ACCESSOR_AND_MUTATOR_OF(grv)
GENERATE_ACCESSOR_AND_MUTATOR_OF(slp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(unrollthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rollthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rollfrc)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rolldec)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rolluphillslp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rolldownhillslp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(falloffthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(brakingthreshold)

/* private stuff ;-) */

/* sensors */
#define GENERATE_SENSOR_ACCESSOR(x) \
const sensor_t* sensor_##x(const physicsactor_t *pa) \
{ \
    if(pa->state == PAS_JUMPING || pa->state == PAS_ROLLING) \
        return pa->x##_jumproll; \
    else if(pa->in_the_air || pa->state == PAS_SPRINGING) \
        return pa->x##_intheair; \
    else \
        return pa->x##_normal; \
}

GENERATE_SENSOR_ACCESSOR(A)
GENERATE_SENSOR_ACCESSOR(B)
GENERATE_SENSOR_ACCESSOR(C)
GENERATE_SENSOR_ACCESSOR(D)
GENERATE_SENSOR_ACCESSOR(M)
GENERATE_SENSOR_ACCESSOR(U)




/*
 * ---------------------------------------
 *           PHYSICS ENGINE
 * ---------------------------------------
 */


/* call UPDATE_SENSORS whenever you update pa->position */
#define UPDATE_SENSORS \
    at_A = sensor_check(sensor_A(pa), pa->position, pa->movmode, obstaclemap); \
    at_B = sensor_check(sensor_B(pa), pa->position, pa->movmode, obstaclemap); \
    at_C = sensor_check(sensor_C(pa), pa->position, pa->movmode, obstaclemap); \
    at_D = sensor_check(sensor_D(pa), pa->position, pa->movmode, obstaclemap); \
    at_M = sensor_check(sensor_M(pa), pa->position, pa->movmode, obstaclemap); \
    at_A = (at_A != NULL && !obstacle_is_solid(at_A)) ? (pa->ysp >= 0.0f && pa->position.y + sensor_get_y2(sensor_A(pa)) - min(15, obstacle_get_height_at(at_A, (int)(pa->position.x + sensor_get_x1(sensor_A(pa)) - obstacle_get_position(at_A).x), FROM_BOTTOM)/3) <= obstacle_get_position(at_A).y + (obstacle_get_height(at_A) - 1) - obstacle_get_height_at(at_A, (int)(pa->position.x + sensor_get_x1(sensor_A(pa)) - obstacle_get_position(at_A).x), FROM_BOTTOM) ? at_A : NULL) : at_A; \
    at_B = (at_B != NULL && !obstacle_is_solid(at_B)) ? (pa->ysp >= 0.0f && pa->position.y + sensor_get_y2(sensor_B(pa)) - min(15, obstacle_get_height_at(at_B, (int)(pa->position.x + sensor_get_x1(sensor_B(pa)) - obstacle_get_position(at_B).x), FROM_BOTTOM)/3) <= obstacle_get_position(at_B).y + (obstacle_get_height(at_B) - 1) - obstacle_get_height_at(at_B, (int)(pa->position.x + sensor_get_x1(sensor_B(pa)) - obstacle_get_position(at_B).x), FROM_BOTTOM) ? at_B : NULL) : at_B; \
    at_C = (at_C != NULL && !obstacle_is_solid(at_C)) ? NULL : at_C; \
    at_D = (at_D != NULL && !obstacle_is_solid(at_D)) ? NULL : at_D; \
    at_M = (at_M != NULL && !obstacle_is_solid(at_M)) ? NULL : at_M; \
    pa->in_the_air = (at_A == NULL) && (at_B == NULL); \
    ;
    /*DEBUG("A: %8X B: %8X M: %8X $%02Xh", at_A ? obstacle_get_angle(at_A) : -1, at_B ? obstacle_get_angle(at_B) : -1, at_M ? obstacle_get_angle(at_M) : -1, pa->angle);*/

/* call UPDATE_MOVMODE whenever you update pa->angle */
#define UPDATE_MOVMODE \
    if(pa->angle < 0x20 || pa->angle > 0xE0) \
        pa->movmode = MM_FLOOR; \
    else if(pa->angle > 0x20 && pa->angle < 0x60) \
        pa->movmode = MM_LEFTWALL; \
    else if(pa->angle > 0x60 && pa->angle < 0xA0) \
        pa->movmode = MM_CEILING; \
    else if(pa->angle > 0xA0 && pa->angle < 0xE0) \
        pa->movmode = MM_RIGHTWALL;

/* height of the physics actor */
#define HEIGHT \
    20 /*abs(sensor_get_y2(sensor_A(pa)) - sensor_get_y1(sensor_A(pa)))*/

/* wall detection */
#define THERE_IS_A_WALL_ON_THE_RIGHT \
    (at_M != NULL && ( \
        (pa->movmode == MM_FLOOR && pa->position.x < obstacle_get_position(at_M).x) || \
        (pa->movmode == MM_RIGHTWALL && pa->position.y > obstacle_get_position(at_M).y + obstacle_get_height(at_M)) || \
        (pa->movmode == MM_CEILING && pa->position.x > obstacle_get_position(at_M).x + obstacle_get_width(at_M)) || \
        (pa->movmode == MM_LEFTWALL && pa->position.y < obstacle_get_position(at_M).y) \
    ))

#define THERE_IS_A_WALL_ON_THE_LEFT \
    (at_M != NULL && ( \
        (pa->movmode == MM_FLOOR && pa->position.x > obstacle_get_position(at_M).x + obstacle_get_width(at_M)) || \
        (pa->movmode == MM_RIGHTWALL && pa->position.y < obstacle_get_position(at_M).y) || \
        (pa->movmode == MM_CEILING && pa->position.x < obstacle_get_position(at_M).x) || \
        (pa->movmode == MM_LEFTWALL && pa->position.y > obstacle_get_position(at_M).y + obstacle_get_height(at_M)) \
    ))

/* physics simulation */
void run_simulation(physicsactor_t *pa, const obstaclemap_t *obstaclemap)
{
    const float walk_threshold = 30.0f;
    const float wait_threshold = 5.0f;
    float dt = timer_get_delta();
    const obstacle_t *at_A, *at_B, *at_C, *at_D, *at_M;
    int was_in_the_air;

    /*
     *
     * death
     *
     */

    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED) {
        pa->ysp = min(pa->ysp + pa->grv * dt, pa->topyspeed);
        pa->position.y += pa->ysp * dt;
        pa->facing_right = TRUE;
        return;
    }

    UPDATE_SENSORS

    /*
     *
     * walking
     *
     */

    if(!pa->in_the_air && pa->state != PAS_ROLLING) {

        /* acceleration */
        if(input_button_down(pa->input, IB_RIGHT) && !input_button_down(pa->input, IB_LEFT) && pa->gsp >= 0.0f) {
            if(pa->gsp < pa->topspeed) {
                pa->gsp = min(pa->gsp + pa->acc * dt, pa->topspeed);
                if(!(pa->state == PAS_PUSHING && pa->facing_right))
                    pa->state = PAS_WALKING;
            }
            else
                pa->state = PAS_RUNNING;
        }

        if(input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT) && pa->gsp <= 0.0f) {
            if(pa->gsp > -pa->topspeed) {
                pa->gsp = max(pa->gsp - pa->acc * dt, -pa->topspeed);
                if(!(pa->state == PAS_PUSHING && !pa->facing_right))
                    pa->state = PAS_WALKING;
            }
            else
                pa->state = PAS_RUNNING;
        }

        /* deceleration / braking */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0f && (pa->angle % 0x40 == 0x0 || !pa->facing_right)) {
            pa->gsp += pa->dec * dt;
            if(fabs(pa->gsp) >= pa->brakingthreshold)
                pa->state = PAS_BRAKING;
        }

        if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0f && (pa->angle % 0x40 == 0x0 || pa->facing_right)) {
            pa->gsp -= pa->dec * dt;
            if(fabs(pa->gsp) >= pa->brakingthreshold)
                pa->state = PAS_BRAKING;
        }

        /* friction */
        if(!input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)) {
            if(!(fabs(pa->gsp) < walk_threshold && pa->angle == 0x0))
                pa->gsp -= min(fabs(pa->gsp), pa->frc) * sign(pa->gsp) * dt;
            else
                pa->gsp = 0.0f;
        }

        if(input_button_down(pa->input, IB_LEFT) && input_button_down(pa->input, IB_RIGHT) && fabs(pa->gsp) < pa->frc)
            pa->gsp = 0.0f;

        /* slope factor */
        pa->gsp += pa->slp * -SIN(pa->angle) * dt;

        /* animation issues */
        if(fabs(pa->gsp) < walk_threshold && pa->angle == 0x0) {
            if(pa->state != PAS_PUSHING && input_button_down(pa->input, IB_DOWN))
                pa->state = PAS_DUCKING;
            else if(pa->state != PAS_PUSHING && input_button_down(pa->input, IB_UP))
                pa->state = PAS_LOOKINGUP;
            else if(pa->state != PAS_PUSHING && (input_button_down(pa->input, IB_LEFT) || input_button_down(pa->input, IB_RIGHT)))
                pa->state = input_button_down(pa->input, IB_LEFT) && input_button_down(pa->input, IB_RIGHT) ? PAS_STOPPED : PAS_WALKING;
            else if((pa->state != PAS_PUSHING && pa->state != PAS_WAITING) || (pa->state == PAS_PUSHING && !input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)))
                pa->state = PAS_STOPPED;
        }
        else if(pa->state == PAS_STOPPED || pa->state == PAS_WAITING || pa->state == PAS_LEDGE || pa->state == PAS_WALKING || pa->state == PAS_RUNNING)
            pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
        else if(pa->state == PAS_PUSHING && fabs(pa->gsp) >= 3.0)
            pa->state = PAS_WALKING;
    }

    /*
     *
     * rolling
     *
     */

    if(!pa->in_the_air && (pa->state == PAS_WALKING || pa->state == PAS_RUNNING)) {
        if(fabs(pa->gsp) > pa->rollthreshold && input_button_down(pa->input, IB_DOWN))
            pa->state = PAS_ROLLING;
    }

    if(!pa->in_the_air && pa->state == PAS_ROLLING) {

        /* deceleration */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0f)
            pa->gsp = min(0.0f, pa->gsp + pa->rolldec * dt);

        if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0f)
            pa->gsp = max(0.0f, pa->gsp - pa->rolldec * dt);

        /* friction */
        pa->gsp -= min(fabs(pa->gsp), pa->rollfrc) * sign(pa->gsp) * dt;

        /* slope factor */
        if(pa->gsp * SIN(pa->angle) >= 0.0f)
            pa->gsp += pa->rolluphillslp * -SIN(pa->angle) * dt;
        else
            pa->gsp += pa->rolldownhillslp * -SIN(pa->angle) * dt;

        /* unroll */
        if(fabs(pa->gsp) < pa->unrollthreshold && pa->angle % 0x40 == 0x0)
            pa->state = PAS_WALKING;
    }

    /*
     *
     * moving horizontally
     *
     */

    if(!pa->in_the_air) {

        /* you're way too fast... */
        pa->gsp = clip(pa->gsp, -2.5f * pa->topspeed, 2.5f * pa->topspeed);

        /* speed */
        pa->xsp = pa->gsp * COS(pa->angle);
        pa->ysp = pa->gsp * -SIN(pa->angle);

        /* BUGGED? falling off walls and ceilings */
        if(fabs(pa->gsp) < pa->falloffthreshold*0.25f && pa->angle >= 0x40 && pa->angle <= 0xC0) {
            if(pa->movmode == MM_RIGHTWALL) pa->position.x += 5;
            else if(pa->movmode == MM_LEFTWALL) pa->position.x -= 4;
            /*pa->gsp = 0.0f;*/
            pa->angle = 0x0;
            UPDATE_MOVMODE
            pa->horizontal_control_lock_timer = 0.5f;
        }
    }

    /*
     *
     * jumping and falling off
     *
     */

    /* falling off */
    if(pa->in_the_air) {

        /* air acceleration */
        if(input_button_down(pa->input, IB_RIGHT) && !input_button_down(pa->input, IB_LEFT)) {
            if(pa->xsp < pa->topspeed)
                pa->xsp = min(pa->xsp + pa->air * dt, pa->topspeed);
        }

        if(input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)) {
            if(pa->xsp > -pa->topspeed)
                pa->xsp = max(pa->xsp - pa->air * dt, -pa->topspeed);
        }

        /* air drag */
        if(pa->ysp < 0.0f && pa->ysp > pa->airdragcondition) {
            if(fabs(pa->xsp) >= pa->airdragthreshold) {
                float correction = pow(pa->airdragmultiplier, 60.0f * dt - 1.0f);
                pa->xsp *= pa->airdragmultiplier * correction;
            }
        }

        /* jump sensitivity */
        if(pa->state == PAS_JUMPING) {
            if(!input_button_down(pa->input, IB_FIRE1) && pa->ysp < pa->jmprel)
                pa->ysp = pa->jmprel;
        }

        /* gravity */
        if(pa->state != PAS_GETTINGHIT)
            pa->ysp = min(pa->ysp + pa->grv * dt, pa->topyspeed);
        else
            pa->ysp = min(pa->ysp + 0.1875f * (pa->grv / 0.21875f) * dt, pa->topyspeed);
    }
    else {

        /* jumping */
        if(input_button_down(pa->input, IB_FIRE1) && !input_button_down(pa->input, IB_DOWN) && !input_button_down(pa->input, IB_UP)) {
            float grv_attenuation = (sign(pa->gsp * SIN(pa->angle)) < 0.0f) ? 1.0f : 0.5f;
            pa->xsp = pa->jmp * SIN(pa->angle) + pa->gsp * COS(pa->angle);
            pa->ysp = pa->jmp * COS(pa->angle) - pa->gsp * SIN(pa->angle) * grv_attenuation;
            pa->gsp = 0.0f;
            pa->angle = 0x0;
            UPDATE_MOVMODE
            pa->state = PAS_JUMPING;
        }

    }

    /*
     *
     * springing
     *
     */

    if(pa->state == PAS_SPRINGING) {
        if(pa->in_the_air && pa->ysp > 0.0f)
            pa->state = PAS_WALKING;
    }

    /*
     *
     * breathing
     *
     */
    if(pa->breathe_timer > 0.0f) {
        pa->breathe_timer -= dt;
        pa->state = PAS_BREATHING;
    }
    else if(pa->state == PAS_BREATHING && pa->in_the_air)
        pa->state = PAS_WALKING;

    /*
     *
     * moving and detecting collisions
     *
     */

    /* moving */
    was_in_the_air = pa->in_the_air;
    pa->position.x += pa->xsp * dt;
    pa->position.y += pa->ysp * dt;
    UPDATE_SENSORS

    /* pushing against the walls */
    if(at_M != NULL) {
        if(pa->movmode == MM_FLOOR || pa->movmode == MM_CEILING) {
            /* floor and ceiling modes */
            if(obstacle_get_position(at_M).x + obstacle_get_width(at_M)/2 > pa->position.x) {
                pa->position.x = obstacle_get_position(at_M).x - 11;
                pa->gsp = 0.0f;
                if(!pa->in_the_air) {
                    pa->xsp = 0.0f;
                    if(input_button_down(pa->input, IB_RIGHT)) {
                        pa->state = PAS_PUSHING;
                        pa->facing_right = TRUE;
                    }
                    else
                        pa->state = PAS_STOPPED;
                }
                else
                    pa->xsp = min(pa->xsp, 0.0f);
                UPDATE_SENSORS
            }
            else if(obstacle_get_position(at_M).x + obstacle_get_width(at_M)/2 < pa->position.x) {
                pa->position.x = obstacle_get_position(at_M).x + (obstacle_get_width(at_M) - 1) + 11;
                pa->gsp = 0.0f;
                if(!pa->in_the_air) {
                    pa->xsp = 0.0f;
                    if(input_button_down(pa->input, IB_LEFT)) {
                        pa->state = PAS_PUSHING;
                        pa->facing_right = FALSE;
                    }
                    else
                        pa->state = PAS_STOPPED;
                }
                else
                    pa->xsp = max(pa->xsp, 0.0f);
                UPDATE_SENSORS
            }
        }
        else {
            if(!(pa->angle >= 0x40 && pa->angle <= 0xC0)) {
                pa->angle = 0x0;
                UPDATE_MOVMODE
            }
            else {
                /* right wall and ceiling modes */
                if(obstacle_get_position(at_M).y + obstacle_get_height(at_M)/2 > pa->position.y) {
                    pa->position.y = obstacle_get_position(at_M).y - 11;
                    pa->gsp = 0.0f;
                    if(!pa->in_the_air) {
                        pa->xsp = 0.0f;
                        pa->state = PAS_STOPPED;
                    }
                    else
                        pa->ysp = min(pa->ysp, 0.0f);
                    UPDATE_SENSORS
                }
                else if(obstacle_get_position(at_M).y + obstacle_get_height(at_M)/2 < pa->position.y) {
                    pa->position.y = obstacle_get_position(at_M).y + (obstacle_get_height(at_M) - 1) + 11;
                    pa->gsp = 0.0f;
                    if(!pa->in_the_air) {
                        pa->xsp = 0.0f;
                        pa->state = PAS_STOPPED;
                    }
                    else
                        pa->ysp = max(pa->ysp, 0.0f);
                    UPDATE_SENSORS
                }
            }
        }
    }

    /* sticky physics */
    if(pa->in_the_air && !was_in_the_air) {
        int u = fabs(pa->gsp) > 360.0f && pa->state != PAS_JUMPING ? 5 : 2;
        v2d_t offset = v2d_new(0,0);

        if(pa->state != PAS_JUMPING && ((pa->facing_right && pa->angle < 0x40) || (!pa->facing_right && pa->angle > 0xC0)))
            u += 1;

        if(pa->movmode == MM_FLOOR)
            offset = v2d_new(0,u);
        else if(pa->movmode == MM_LEFTWALL)
            offset = v2d_new(-u,0);
        else if(pa->movmode == MM_CEILING)
            offset = v2d_new(0,-u);
        else if(pa->movmode == MM_RIGHTWALL)
            offset = v2d_new(u,0);

        pa->position = v2d_add(pa->position, offset);
        UPDATE_SENSORS
        if(pa->in_the_air) {
            pa->position = v2d_subtract(pa->position, offset);
            UPDATE_SENSORS
        }
    }

    /* stick to the ground */
    if(!pa->in_the_air && !((pa->state == PAS_JUMPING || pa->state == PAS_GETTINGHIT) && pa->ysp < 0.0f)) {
        /* picking the ground */
        int u = 2;
        const obstacle_t *ground = NULL;
        const sensor_t *ground_sensor = NULL;

        if(pick_the_best_ground(pa, at_A, at_B, sensor_A(pa), sensor_B(pa)) == 'a') {
            ground = at_A;
            ground_sensor = sensor_A(pa);
        }
        else {
            ground = at_B;
            ground_sensor = sensor_B(pa);
        }

        /* adjust position */
        if(pa->movmode == MM_LEFTWALL)
            pa->position.x = obstacle_get_position(ground).x + obstacle_get_height_at(ground, (int)(pa->position.y + sensor_get_x1(ground_sensor) - obstacle_get_position(ground).y), FROM_LEFT) + (HEIGHT - u);
        else if(pa->movmode == MM_CEILING)
            pa->position.y = obstacle_get_position(ground).y + obstacle_get_height_at(ground, (int)(pa->position.x - sensor_get_x1(ground_sensor) - obstacle_get_position(ground).x), FROM_TOP) + (HEIGHT - u);
        else if(pa->movmode == MM_RIGHTWALL)
            pa->position.x = obstacle_get_position(ground).x + (obstacle_get_width(ground) - 1) - obstacle_get_height_at(ground, (int)(pa->position.y - sensor_get_x1(ground_sensor) - obstacle_get_position(ground).y), FROM_RIGHT) - (HEIGHT - u);
        else if(pa->movmode == MM_FLOOR)
            pa->position.y = obstacle_get_position(ground).y + (obstacle_get_height(ground) - 1) - obstacle_get_height_at(ground, (int)(pa->position.x + sensor_get_x1(ground_sensor) - obstacle_get_position(ground).x), FROM_BOTTOM) - (HEIGHT - u);

        /* updating the angle */
        pa->angle = obstacle_get_angle(ground);
        UPDATE_MOVMODE

        /* update the sensors */
        UPDATE_SENSORS
    }

    /* reacquisition of the ground */
    if(!pa->in_the_air && was_in_the_air) {
        if(pa->angle >= 0xF0 || pa->angle <= 0x0F)
            pa->gsp = pa->xsp;
        else if((pa->angle >= 0xE0 && pa->angle <= 0xEF) || (pa->angle >= 0x10 && pa->angle <= 0x1F))
            pa->gsp = fabs(pa->xsp) > pa->ysp ? pa->xsp : pa->ysp * 0.5f * -sign(SIN(pa->angle));
        else if((pa->angle >= 0xC0 && pa->angle <= 0xDF) || (pa->angle >= 0x20 && pa->angle <= 0x3F))
            pa->gsp = fabs(pa->xsp) > pa->ysp ? pa->xsp : pa->ysp * -sign(SIN(pa->angle));

        pa->xsp = 0.0f;
        pa->ysp = 0.0f;

        if(pa->state != PAS_ROLLING)
            pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
    }

    /* bump into ceilings */
    if(pa->in_the_air && (at_C != NULL || at_D != NULL)) {
        /* picking the ceiling */
        int u = 0;
        const obstacle_t *ceiling = NULL;
        const sensor_t *ceiling_sensor = NULL;

        if(pick_the_best_ceiling(pa, at_C, at_D, sensor_C(pa), sensor_D(pa)) == 'c') {
            ceiling = at_C;
            ceiling_sensor = sensor_C(pa);
        }
        else {
            ceiling = at_D;
            ceiling_sensor = sensor_D(pa);
        }

        /* reattach to the ceiling */
        if((obstacle_get_angle(ceiling) > 0xA0 && obstacle_get_angle(ceiling) <= 0xBF) || (obstacle_get_angle(ceiling) > 0x40 && obstacle_get_angle(ceiling) <= 0x5F)) {
            pa->gsp = pa->ysp * -sign(SIN(obstacle_get_angle(ceiling)));
            pa->xsp = 0.0f;
            pa->ysp = 0.0f;
            pa->angle = obstacle_get_angle(ceiling);
            pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
            UPDATE_MOVMODE
            UPDATE_SENSORS
        }
        else {

            /* adjust position */
            if(pa->movmode == MM_RIGHTWALL)
                pa->position.x = obstacle_get_position(ceiling).x + obstacle_get_height_at(ceiling, (int)(pa->position.y - sensor_get_x1(ceiling_sensor) - obstacle_get_position(ceiling).y), FROM_LEFT) + (HEIGHT - u);
            else if(pa->movmode == MM_FLOOR)
                pa->position.y = obstacle_get_position(ceiling).y + obstacle_get_height_at(ceiling, (int)(pa->position.x + sensor_get_x1(ceiling_sensor) - obstacle_get_position(ceiling).x), FROM_TOP) + (HEIGHT - u);
            else if(pa->movmode == MM_LEFTWALL)
                pa->position.x = obstacle_get_position(ceiling).x + (obstacle_get_width(ceiling) - 1) - obstacle_get_height_at(ceiling, (int)(pa->position.y + sensor_get_x1(ceiling_sensor) - obstacle_get_position(ceiling).y), FROM_RIGHT) - (HEIGHT - u);
            else if(pa->movmode == MM_CEILING)
                pa->position.y = obstacle_get_position(ceiling).y + (obstacle_get_height(ceiling) - 1) - obstacle_get_height_at(ceiling, (int)(pa->position.x - sensor_get_x1(ceiling_sensor) - obstacle_get_position(ceiling).x), FROM_BOTTOM) - (HEIGHT - u);

            /* adjust speed */
            pa->ysp = max(pa->ysp, 0.0f);

            /* update the sensors */
            UPDATE_SENSORS

        }
    }

    /*
     *
     * misc
     *
     */

    /* reset the angle */
    if(pa->in_the_air) {
        pa->angle = 0x0;
        UPDATE_MOVMODE
    }

    /* balancing on edges */
    if(!pa->in_the_air && fabs(pa->gsp) < EPSILON && pa->state != PAS_PUSHING) {
        if(at_A != NULL && at_B == NULL && pa->position.x >= obstacle_get_position(at_A).x + obstacle_get_width(at_A)) {
            pa->state = PAS_LEDGE;
            pa->facing_right = TRUE;
        }
        else if(at_A == NULL && at_B != NULL && pa->position.x < obstacle_get_position(at_B).x) {
            pa->state = PAS_LEDGE;
            pa->facing_right = FALSE;
        }
    }

    /* waiting... */
    if(pa->state == PAS_STOPPED) {
        if((pa->wait_timer += dt) >= wait_threshold)
            pa->state = PAS_WAITING;
    }
    else
        pa->wait_timer = 0.0f;

    /* winning */
    if(pa->winning_pose && fabs(pa->gsp) < walk_threshold && !pa->in_the_air)
        pa->state = PAS_WINNING;

    /* animation bugfix */
    if(pa->in_the_air && (pa->state == PAS_PUSHING || pa->state == PAS_STOPPED || pa->state == PAS_DUCKING || pa->state == PAS_LOOKINGUP))
        pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
}

/* which one is the tallest obstacle, a or b? */
char pick_the_best_ground(const physicsactor_t *pa, const obstacle_t *a, const obstacle_t *b, const sensor_t *a_sensor, const sensor_t *b_sensor)
{
    int ha, hb, xa, xb, ya, yb, x, y;
    int (*w)(const obstacle_t*) = obstacle_get_width;
    int (*h)(const obstacle_t*) = obstacle_get_height;

    if(a == NULL)
        return 'b';
    else if(b == NULL)
        return 'a';

    xa = (int)obstacle_get_position(a).x;
    xb = (int)obstacle_get_position(b).x;
    ya = (int)obstacle_get_position(a).y;
    yb = (int)obstacle_get_position(b).y;
    x = (int)pa->position.x;
    y = (int)pa->position.y;

    if(pa->movmode == MM_FLOOR) {
        ha = obstacle_get_height_at(a, x + sensor_get_x1(a_sensor) - xa, FROM_BOTTOM);
        hb = obstacle_get_height_at(b, x + sensor_get_x1(b_sensor) - xb, FROM_BOTTOM);
        return (ya + (h(a)-1) - ha < yb + (h(b)-1) - hb) ? 'a' : 'b';
    }
    else if(pa->movmode == MM_LEFTWALL) {
        ha = obstacle_get_height_at(a, y + sensor_get_x1(a_sensor) - ya, FROM_LEFT);
        hb = obstacle_get_height_at(b, y + sensor_get_x1(b_sensor) - yb, FROM_LEFT);
        return (xa + ha > xb + hb) ? 'a' : 'b';
    }
    else if(pa->movmode == MM_CEILING) {
        ha = obstacle_get_height_at(a, x - sensor_get_x1(a_sensor) - xa, FROM_TOP);
        hb = obstacle_get_height_at(b, x - sensor_get_x1(b_sensor) - xb, FROM_TOP);
        return (ya + ha > yb + hb) ? 'a' : 'b';
    }
    else if(pa->movmode == MM_RIGHTWALL) {
        ha = obstacle_get_height_at(a, y - sensor_get_x1(a_sensor) - ya, FROM_RIGHT);
        hb = obstacle_get_height_at(b, y - sensor_get_x1(b_sensor) - yb, FROM_RIGHT);
        return (xa + (w(a)-1) - ha < xb + (w(b)-1) - hb) ? 'a' : 'b';
    }

    return 'a';
}


/* which one is the best ceiling, c or d? */
char pick_the_best_ceiling(const physicsactor_t *pa, const obstacle_t *c, const obstacle_t *d, const sensor_t *c_sensor, const sensor_t *d_sensor)
{
    int hc, hd, xc, xd, yc, yd, x, y;
    int (*w)(const obstacle_t*) = obstacle_get_width;
    int (*h)(const obstacle_t*) = obstacle_get_height;

    if(c == NULL)
        return 'd';
    else if(d == NULL)
        return 'c';

    xc = (int)obstacle_get_position(c).x;
    xd = (int)obstacle_get_position(d).x;
    yc = (int)obstacle_get_position(c).y;
    yd = (int)obstacle_get_position(d).y;
    x = (int)pa->position.x;
    y = (int)pa->position.y;

    if(pa->movmode == MM_CEILING) {
        hc = obstacle_get_height_at(c, x + sensor_get_x1(c_sensor) - xc, FROM_BOTTOM);
        hd = obstacle_get_height_at(d, x + sensor_get_x1(d_sensor) - xd, FROM_BOTTOM);
        return (yc + (h(c)-1) - hc < yd + (h(d)-1) - hd) ? 'c' : 'd';
    }
    else if(pa->movmode == MM_RIGHTWALL) {
        hc = obstacle_get_height_at(c, y + sensor_get_x1(c_sensor) - yc, FROM_LEFT);
        hd = obstacle_get_height_at(d, y + sensor_get_x1(d_sensor) - yd, FROM_LEFT);
        return (xc + hc > xd + hd) ? 'c' : 'd';
    }
    else if(pa->movmode == MM_FLOOR) {
        hc = obstacle_get_height_at(c, x - sensor_get_x1(c_sensor) - xc, FROM_TOP);
        hd = obstacle_get_height_at(d, x - sensor_get_x1(d_sensor) - xd, FROM_TOP);
        return (yc + hc > yd + hd) ? 'c' : 'd';
    }
    else if(pa->movmode == MM_LEFTWALL) {
        hc = obstacle_get_height_at(c, y - sensor_get_x1(c_sensor) - yc, FROM_RIGHT);
        hd = obstacle_get_height_at(d, y - sensor_get_x1(d_sensor) - yd, FROM_RIGHT);
        return (xc + (w(c)-1) - hc < xd + (w(d)-1) - hd) ? 'c' : 'd';
    }

    return 'c';
}
