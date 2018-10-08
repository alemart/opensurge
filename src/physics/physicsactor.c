/*
 * Open Surge Engine
 * physicsactor.c - physics system: actor
 * Copyright (C) 2011, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "physicsactor.h"
#include "sensor.h"
#include "obstaclemap.h"
#include "obstacle.h"
#include "../core/util.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../core/input.h"
#include "../core/timer.h"

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
    float airdrag; /* air drag (friction) */
    float jmp; /* initial jump velocity */
    float jmprel; /* release jump velocity */
    float diejmp; /* death jump velocity */
    float hitjmp; /* get hit jump velocity */
    float grv; /* gravity */
    float slp; /* slope factor */
    float chrg; /* charge-and-release max speed */
    float rollfrc; /* roll friction */
    float rolldec; /* roll deceleration */
    float rolluphillslp; /* roll uphill slope */
    float rolldownhillslp; /* roll downhill slope */
    float rollthreshold; /* roll threshold */
    float unrollthreshold; /* unroll threshold */
    float walkthreshold; /* walk threshold */
    float falloffthreshold; /* fall off threshold */
    float brakingthreshold; /* braking animation treshold */
    float airdragthreshold; /* air drag threshold */
    int angle; /* angle (0-255 clockwise) */
    int in_the_air; /* is the player in the air? */
    float horizontal_control_lock_timer; /* lock timer, in seconds */
    int facing_right; /* is the player facing right? */
    float wait_timer; /* the time, in seconds, that the physics actor is stopped */
    int winning_pose; /* winning pose enabled? */
    float breathe_timer; /* if greater than zero, set animation to breathing */
    int sticky_lock; /* sticky physics lock */
    float charge_intensity; /* charge intensity */
    float airdrag_exp; /* airdrag modifier */
    physicsactorstate_t state; /* state */
    movmode_t movmode; /* current movement mode, based on the angle */
    input_t *input; /* input device */

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
static void run_simulation(physicsactor_t *pa, const obstaclemap_t *obstaclemap);
static inline char pick_the_best_ground(const physicsactor_t *pa, const obstacle_t *a, const obstacle_t *b, const sensor_t *a_sensor, const sensor_t *b_sensor);
static inline char pick_the_best_ceiling(const physicsactor_t *pa, const obstacle_t *c, const obstacle_t *d, const sensor_t *c_sensor, const sensor_t *d_sensor);
static inline int ang_diff(int alpha, int beta);


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
   things, we have multiple, immutable
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
static const float cos_table[256] = {
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

/* slope table: stored angles */
/* SLOPE(y,x) is the angle of the (y,x) slope, where -11 <= y,x <= 11 */
#define SLOPE(y,x) slp_table[11 + ((y) + ((y)<-11)*(-11-(y)) + ((y)>11)*(11-(y)))][11 + ((x) + ((x)<-11)*(-11-(x)) + ((x)>11)*(11-(x)))]
static const int slp_table[23][23] = {
    { 0xA0, 0xA2, 0xA4, 0xA6, 0xA9, 0xAC, 0xAF, 0xB2, 0xB5, 0xB9, 0xBC, 0xC0, 0xC4, 0xC7, 0xCB, 0xCE, 0xD1, 0xD4, 0xD7, 0xDA, 0xDC, 0xDE, 0xE0 },
    { 0x9E, 0xA0, 0xA2, 0xA5, 0xA7, 0xAA, 0xAD, 0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD3, 0xD6, 0xD9, 0xDB, 0xDE, 0xE0, 0xE2 },
    { 0x9C, 0x9E, 0xA0, 0xA2, 0xA5, 0xA8, 0xAB, 0xAF, 0xB3, 0xB7, 0xBB, 0xC0, 0xC5, 0xC9, 0xCD, 0xD1, 0xD5, 0xD8, 0xDB, 0xDE, 0xE0, 0xE2, 0xE4 },
    { 0x9A, 0x9B, 0x9E, 0xA0, 0xA3, 0xA6, 0xA9, 0xAD, 0xB1, 0xB6, 0xBB, 0xC0, 0xC5, 0xCA, 0xCF, 0xD3, 0xD7, 0xDA, 0xDD, 0xE0, 0xE2, 0xE5, 0xE6 },
    { 0x97, 0x99, 0x9B, 0x9D, 0xA0, 0xA3, 0xA7, 0xAB, 0xB0, 0xB5, 0xBA, 0xC0, 0xC6, 0xCB, 0xD0, 0xD5, 0xD9, 0xDD, 0xE0, 0xE3, 0xE5, 0xE7, 0xE9 },
    { 0x94, 0x96, 0x98, 0x9A, 0x9D, 0xA0, 0xA4, 0xA8, 0xAD, 0xB3, 0xB9, 0xC0, 0xC7, 0xCD, 0xD3, 0xD8, 0xDC, 0xE0, 0xE3, 0xE6, 0xE8, 0xEA, 0xEC },
    { 0x91, 0x93, 0x95, 0x97, 0x99, 0x9C, 0xA0, 0xA5, 0xAA, 0xB0, 0xB8, 0xC0, 0xC8, 0xD0, 0xD6, 0xDB, 0xE0, 0xE4, 0xE7, 0xE9, 0xEB, 0xED, 0xEF },
    { 0x8E, 0x90, 0x91, 0x93, 0x95, 0x98, 0x9B, 0xA0, 0xA6, 0xAD, 0xB6, 0xC0, 0xCA, 0xD3, 0xDA, 0xE0, 0xE5, 0xE8, 0xEB, 0xED, 0xEF, 0xF0, 0xF2 },
    { 0x8B, 0x8C, 0x8D, 0x8F, 0x90, 0x93, 0x96, 0x9A, 0xA0, 0xA8, 0xB3, 0xC0, 0xCD, 0xD8, 0xE0, 0xE6, 0xEA, 0xED, 0xF0, 0xF1, 0xF3, 0xF4, 0xF5 },
    { 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8D, 0x90, 0x93, 0x98, 0xA0, 0xAD, 0xC0, 0xD3, 0xE0, 0xE8, 0xED, 0xF0, 0xF3, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9 },
    { 0x84, 0x84, 0x85, 0x85, 0x86, 0x87, 0x88, 0x8A, 0x8D, 0x93, 0xA0, 0xC0, 0xE0, 0xED, 0xF3, 0xF6, 0xF8, 0xF9, 0xFA, 0xFB, 0xFB, 0xFC, 0xFC },
    { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x7C, 0x7C, 0x7B, 0x7B, 0x7A, 0x79, 0x78, 0x76, 0x73, 0x6D, 0x60, 0x40, 0x20, 0x13, 0x0D, 0x0A, 0x08, 0x07, 0x06, 0x05, 0x05, 0x04, 0x04 },
    { 0x79, 0x78, 0x77, 0x76, 0x75, 0x73, 0x70, 0x6D, 0x68, 0x60, 0x53, 0x40, 0x2D, 0x20, 0x18, 0x13, 0x10, 0x0D, 0x0B, 0x0A, 0x09, 0x08, 0x07 },
    { 0x75, 0x74, 0x73, 0x71, 0x70, 0x6D, 0x6A, 0x66, 0x60, 0x58, 0x4D, 0x40, 0x33, 0x28, 0x20, 0x1A, 0x16, 0x13, 0x10, 0x0F, 0x0D, 0x0C, 0x0B },
    { 0x72, 0x70, 0x6F, 0x6D, 0x6B, 0x68, 0x65, 0x60, 0x5A, 0x53, 0x4A, 0x40, 0x36, 0x2D, 0x26, 0x20, 0x1B, 0x18, 0x15, 0x13, 0x11, 0x10, 0x0E },
    { 0x6F, 0x6D, 0x6B, 0x69, 0x67, 0x64, 0x60, 0x5B, 0x56, 0x50, 0x48, 0x40, 0x38, 0x30, 0x2A, 0x25, 0x20, 0x1C, 0x19, 0x17, 0x15, 0x13, 0x11 },
    { 0x6C, 0x6A, 0x68, 0x66, 0x63, 0x60, 0x5C, 0x58, 0x53, 0x4D, 0x47, 0x40, 0x39, 0x33, 0x2D, 0x28, 0x24, 0x20, 0x1D, 0x1A, 0x18, 0x16, 0x14 },
    { 0x69, 0x67, 0x65, 0x63, 0x60, 0x5D, 0x59, 0x55, 0x50, 0x4B, 0x46, 0x40, 0x3A, 0x35, 0x30, 0x2B, 0x27, 0x23, 0x20, 0x1D, 0x1B, 0x19, 0x17 },
    { 0x66, 0x65, 0x62, 0x60, 0x5D, 0x5A, 0x57, 0x53, 0x4F, 0x4A, 0x45, 0x40, 0x3B, 0x36, 0x31, 0x2D, 0x29, 0x26, 0x23, 0x20, 0x1E, 0x1B, 0x1A },
    { 0x64, 0x62, 0x60, 0x5E, 0x5B, 0x58, 0x55, 0x51, 0x4D, 0x49, 0x45, 0x40, 0x3B, 0x37, 0x33, 0x2F, 0x2B, 0x28, 0x25, 0x22, 0x20, 0x1E, 0x1C },
    { 0x62, 0x60, 0x5E, 0x5B, 0x59, 0x56, 0x53, 0x50, 0x4C, 0x48, 0x44, 0x40, 0x3C, 0x38, 0x34, 0x30, 0x2D, 0x2A, 0x27, 0x25, 0x22, 0x20, 0x1E },
    { 0x60, 0x5E, 0x5C, 0x5A, 0x57, 0x54, 0x51, 0x4E, 0x4B, 0x47, 0x44, 0x40, 0x3C, 0x39, 0x35, 0x32, 0x2F, 0x2C, 0x29, 0x26, 0x24, 0x22, 0x20 }
};

/* helper macro */
#define ANG2DEG(ang) ((int)((256 - (ang)) * 1.40625f) % 360)

/* public methods */
physicsactor_t* physicsactor_create(v2d_t position)
{
    physicsactor_t *pa = mallocx(sizeof *pa);
    const float fpsmul = 60.0f;

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
    pa->sticky_lock = FALSE;
    pa->charge_intensity = 0.0f;
    pa->airdrag_exp = 0.0f;

    /* initializing some constants ;-) */

    /*
      +----------------------+--------------+-----------------+
      | name                 | magic number | fps multitplier |
      +----------------------+--------------+-----------------+
     */
    pa->acc =                   0.05f       * fpsmul * fpsmul ;
    pa->dec =                   0.5f        * fpsmul * fpsmul ;
    pa->frc =                   0.05f       * fpsmul * fpsmul ;
    pa->topspeed =              6.0f        * fpsmul * 1.0f   ;
    pa->topyspeed =             12.0f       * fpsmul * 1.0f   ;
    pa->air =                   0.1f        * fpsmul * fpsmul ;
    pa->airdrag =              (31.0f/32.0f)* 1.0f   * 1.0f   ;
    pa->jmp =                   -6.7f       * fpsmul * 1.0f   ;
    pa->jmprel =                -4.0f       * fpsmul * 1.0f   ;
    pa->diejmp =                -7.0f       * fpsmul * 1.0f   ;
    pa->hitjmp =                -4.0f       * fpsmul * 1.0f   ;
    pa->grv =                   0.23f       * fpsmul * fpsmul ;
    pa->slp =                   0.14f       * fpsmul * fpsmul ;
    pa->chrg =                  12.0f       * fpsmul * 1.0f   ;
    pa->walkthreshold =         0.5f        * fpsmul * 1.0f   ;
    pa->unrollthreshold =       0.5f        * fpsmul * 1.0f   ;
    pa->rollthreshold =         1.0f        * fpsmul * 1.0f   ;
    pa->rollfrc =               0.03f       * fpsmul * fpsmul ;
    pa->rolldec =               0.125f      * fpsmul * fpsmul ;
    pa->rolluphillslp =         0.078f      * fpsmul * fpsmul ;
    pa->rolldownhillslp =       0.312f      * fpsmul * fpsmul ;
    pa->falloffthreshold =      0.625f      * fpsmul * 1.0f   ;
    pa->brakingthreshold =      4.5f        * fpsmul * 1.0f   ;
    pa->airdragthreshold =      -4.0f       * fpsmul * 1.0f   ;
    
    /* sensors */
    pa->A_normal = sensor_create_vertical(-9, 0, 20, image_rgb(0,255,0));
    pa->B_normal = sensor_create_vertical(9, 0, 20, image_rgb(255,255,0));
    pa->C_normal = sensor_create_vertical(-9, -24, 0, image_rgb(0,128,0));
    pa->D_normal = sensor_create_vertical(9, -24, 0, image_rgb(128,128,0));
    pa->M_normal = sensor_create_horizontal(4, -10, 10, image_rgb(255,0,0)); /* use 9 (sensor A) + 1 */
    pa->U_normal = sensor_create_horizontal(-24, -9, 9, image_rgb(255,255,255));

    pa->A_intheair = sensor_create_vertical(-9, 0, 20, image_rgb(0,255,0));
    pa->B_intheair = sensor_create_vertical(9, 0, 20, image_rgb(255,255,0));
    pa->C_intheair = sensor_create_vertical(-9, -24, 0, image_rgb(0,128,0));
    pa->D_intheair = sensor_create_vertical(9, -24, 0, image_rgb(128,128,0));
    pa->M_intheair = sensor_create_horizontal(0, -11, 11, image_rgb(255,0,0)); /* use 10 (sensor M_normal) + 1 */
    pa->U_intheair = sensor_create_horizontal(-24, -9, 9, image_rgb(255,255,255));

    pa->A_jumproll = sensor_create_vertical(-4, 0, 18, image_rgb(0,255,0)); /* use 9 (sensor A) / 2 */
    pa->B_jumproll = sensor_create_vertical(4, 0, 18, image_rgb(255,255,0)); /* use 20 (sensor A) - 2 */
    pa->C_jumproll = sensor_create_vertical(-4, -10, 0, image_rgb(0,128,0));
    pa->D_jumproll = sensor_create_vertical(4, -10, 0, image_rgb(128,128,0));
    pa->M_jumproll = sensor_create_horizontal(0, -10, 10, image_rgb(255,0,0));
    pa->U_jumproll = sensor_create_horizontal(-24, -9, 9, image_rgb(255,255,255));

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
        pa->facing_right = (pa->xsp < 0.0f);
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
        pa->facing_right = (pa->gsp > 0.0f);
    }

    /* don't bother jumping */
    at_U = sensor_check(sensor_U(pa), pa->position, pa->movmode, obstaclemap);
    if(at_U != NULL && obstacle_is_solid(at_U))
        input_simulate_button_up(pa->input, IB_FIRE1);

    /* face left/right */
    if(pa->state != PAS_ROLLING) {
        if((pa->gsp > 0.0f || pa->in_the_air) && input_button_down(pa->input, IB_RIGHT))
            pa->facing_right = TRUE;
        else if((pa->gsp < 0.0f || pa->in_the_air) && input_button_down(pa->input, IB_LEFT))
            pa->facing_right = FALSE;
    }

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
    return (int)((256 - pa->angle) * 1.40625) % 360;
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

int physicsactor_roll_delta(const physicsactor_t* pa)
{
    /* the difference of the height of the (foot) sensors */
    return sensor_get_y2(pa->A_normal) - sensor_get_y2(pa->A_jumproll);
}

float physicsactor_charge_intensity(const physicsactor_t* pa)
{
    return pa->charge_intensity;
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
GENERATE_ACCESSOR_AND_MUTATOR_OF(airdragthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(jmp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(jmprel)
GENERATE_ACCESSOR_AND_MUTATOR_OF(diejmp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(hitjmp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(grv)
GENERATE_ACCESSOR_AND_MUTATOR_OF(slp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(chrg)
GENERATE_ACCESSOR_AND_MUTATOR_OF(walkthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(unrollthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rollthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rollfrc)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rolldec)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rolluphillslp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(rolldownhillslp)
GENERATE_ACCESSOR_AND_MUTATOR_OF(falloffthreshold)
GENERATE_ACCESSOR_AND_MUTATOR_OF(brakingthreshold)

float physicsactor_get_airdrag(const physicsactor_t *pa) { return pa->airdrag; }
void physicsactor_set_airdrag(physicsactor_t *pa, float value)
{
    pa->airdrag = max(0.0f, value);
    pa->airdrag_exp = (pa->airdrag >= 0.001f) ? -pa->airdrag * logf(pa->airdrag) : 0.0f;
}

/* private stuff ;-) */

/* sensors */
#define GENERATE_SENSOR_ACCESSOR(x) \
const sensor_t* sensor_##x(const physicsactor_t *pa) \
{ \
    if((pa->state == PAS_JUMPING && pa->ysp < 0.0f) || pa->state == PAS_ROLLING) \
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
#define UPDATE_SENSORS() \
    do { \
        at_A = sensor_check(sensor_A(pa), pa->position, pa->movmode, obstaclemap); \
        at_B = sensor_check(sensor_B(pa), pa->position, pa->movmode, obstaclemap); \
        at_C = sensor_check(sensor_C(pa), pa->position, pa->movmode, obstaclemap); \
        at_D = sensor_check(sensor_D(pa), pa->position, pa->movmode, obstaclemap); \
        at_M = sensor_check(sensor_M(pa), pa->position, pa->movmode, obstaclemap); \
        if(at_A != NULL && !obstacle_is_solid(at_A)) { \
            int x, y; \
            sensor_worldpos(sensor_A(pa), pa->position, pa->movmode, NULL, NULL, &x, &y); \
            if(obstacle_got_collision(at_A, x, y, x, y) && pa->in_the_air) { \
                int ygnd = obstacle_ground_position(at_A, x, y, GD_DOWN); \
                at_A = (pa->ysp >= 0.0f && y < ygnd + 12) ? at_A : NULL; \
            } \
            else if(pa->in_the_air || at_A == at_M) \
                at_A = NULL; \
        } \
        if(at_B != NULL && !obstacle_is_solid(at_B)) { \
            int x, y; \
            sensor_worldpos(sensor_B(pa), pa->position, pa->movmode, NULL, NULL, &x, &y); \
            if(obstacle_got_collision(at_B, x, y, x, y) && pa->in_the_air) { \
                int ygnd = obstacle_ground_position(at_B, x, y, GD_DOWN); \
                at_B = (pa->ysp >= 0.0f && y < ygnd + 12) ? at_B : NULL; \
            } \
            else if(pa->in_the_air || at_B == at_M) \
                at_B = NULL; \
        } \
        at_C = (at_C != NULL && obstacle_is_solid(at_C)) ? at_C : NULL; \
        at_D = (at_D != NULL && obstacle_is_solid(at_D)) ? at_D : NULL; \
        at_M = (at_M != NULL && obstacle_is_solid(at_M)) ? at_M : NULL; \
        pa->in_the_air = (at_A == NULL) && (at_B == NULL); \
    } while(0)

/* call UPDATE_MOVMODE whenever you update pa->angle */
#define UPDATE_MOVMODE() \
    do { \
        if(pa->angle < 0x20 || pa->angle > 0xE0) \
            pa->movmode = MM_FLOOR; \
        else if(pa->angle > 0x20 && pa->angle < 0x60) \
            pa->movmode = MM_LEFTWALL; \
        else if(pa->angle > 0x60 && pa->angle < 0xA0) \
            pa->movmode = MM_CEILING; \
        else if(pa->angle > 0xA0 && pa->angle < 0xE0) \
            pa->movmode = MM_RIGHTWALL; \
    } while(0)

/* compute the current pa->angle */
#define UPDATE_ANGLE() \
    do { \
        const int hoff = 5; \
        const obstacle_t* gnd = NULL; \
        int sensor_len = sensor_get_y2(sensor_A(pa)) - sensor_get_y1(sensor_A(pa)); \
        int found_a = FALSE, found_b = FALSE; \
        int h, x, y, xa, ya, xb, yb, ang; \
        for(int i = 0; i < sensor_len * 2 && !(found_a && found_b); i++) { \
            h = i + sensor_len / 2; \
            x = pa->position.x + h * SIN(pa->angle) + 0.5f; \
            y = pa->position.y + h * COS(pa->angle) + 0.5f; \
            if(!found_a) { \
                xa = x - hoff * COS(pa->angle); \
                ya = y + hoff * SIN(pa->angle); \
                gnd = obstaclemap_get_best_obstacle_at(obstaclemap, xa, ya, xa, ya, pa->movmode); \
                found_a = (gnd != NULL && (obstacle_is_solid(gnd) || ( \
                    (pa->movmode == MM_FLOOR && ya < obstacle_ground_position(gnd, xa, ya, GD_DOWN) + 12) || \
                    (pa->movmode == MM_CEILING && ya > obstacle_ground_position(gnd, xa, ya, GD_UP) - 12) || \
                    (pa->movmode == MM_LEFTWALL && xa > obstacle_ground_position(gnd, xa, ya, GD_LEFT) - 12) || \
                    (pa->movmode == MM_RIGHTWALL && xa < obstacle_ground_position(gnd, xa, ya, GD_RIGHT) + 12) \
                ))); \
            } \
            if(!found_b) { \
                xb = x + hoff * COS(pa->angle); \
                yb = y - hoff * SIN(pa->angle); \
                gnd = obstaclemap_get_best_obstacle_at(obstaclemap, xb, yb, xb, yb, pa->movmode); \
                found_b = (gnd != NULL && (obstacle_is_solid(gnd) || ( \
                    (pa->movmode == MM_FLOOR && yb < obstacle_ground_position(gnd, xb, yb, GD_DOWN) + 12) || \
                    (pa->movmode == MM_CEILING && yb > obstacle_ground_position(gnd, xb, yb, GD_UP) - 12) || \
                    (pa->movmode == MM_LEFTWALL && xb > obstacle_ground_position(gnd, xb, yb, GD_LEFT) - 12) || \
                    (pa->movmode == MM_RIGHTWALL && xb < obstacle_ground_position(gnd, xb, yb, GD_RIGHT) + 12) \
                ))); \
            } \
        } \
        if(found_a && found_b) { \
            const obstacle_t* ga = obstaclemap_get_best_obstacle_at(obstaclemap, xa, ya, xa, ya, pa->movmode); \
            const obstacle_t* gb = obstaclemap_get_best_obstacle_at(obstaclemap, xb, yb, xb, yb, pa->movmode); \
            if(ga && gb) { \
                switch(pa->movmode) { \
                    case MM_FLOOR: \
                        ya = obstacle_ground_position(ga, xa, ya, GD_DOWN); \
                        yb = obstacle_ground_position(gb, xb, yb, GD_DOWN); \
                        break; \
                    case MM_LEFTWALL: \
                        xa = obstacle_ground_position(ga, xa, ya, GD_LEFT); \
                        xb = obstacle_ground_position(gb, xb, yb, GD_LEFT); \
                        break; \
                    case MM_CEILING: \
                        ya = obstacle_ground_position(ga, xa, ya, GD_UP); \
                        yb = obstacle_ground_position(gb, xb, yb, GD_UP); \
                        break; \
                    case MM_RIGHTWALL: \
                        xa = obstacle_ground_position(ga, xa, ya, GD_RIGHT); \
                        xb = obstacle_ground_position(gb, xb, yb, GD_RIGHT); \
                        break; \
                } \
                x = xb - xa; y = yb - ya; \
                ang = SLOPE(y, x); \
                if(ga == gb || ang_diff(ang, pa->angle) <= 0x25) \
                    pa->angle = ang; \
            } \
        } \
    } while(0)

/* physics simulation */
void run_simulation(physicsactor_t *pa, const obstaclemap_t *obstaclemap)
{
    const obstacle_t *at_A, *at_B, *at_C, *at_D, *at_M;
    float dt = timer_get_delta();
    int was_in_the_air;

    UPDATE_SENSORS();

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

    /*
     *
     * walking
     *
     */

    if(!pa->in_the_air && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING) {

        /* acceleration */
        if(input_button_down(pa->input, IB_RIGHT) && !input_button_down(pa->input, IB_LEFT) && pa->gsp >= 0.0f) {
            if(pa->gsp < pa->topspeed) {
                pa->gsp += pa->acc * dt;
                if(pa->gsp >= pa->topspeed) {
                    pa->gsp = pa->topspeed;
                    pa->state = PAS_RUNNING;
                }
                else if(!(pa->state == PAS_PUSHING && pa->facing_right))
                    pa->state = PAS_WALKING;
            }
        }
        else if(input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT) && pa->gsp <= 0.0f) {
            if(pa->gsp > -pa->topspeed) {
                pa->gsp -= pa->acc * dt;
                if(pa->gsp <= -pa->topspeed) {
                    pa->gsp = -pa->topspeed;
                    pa->state = PAS_RUNNING;
                }
                else if(!(pa->state == PAS_PUSHING && !pa->facing_right))
                    pa->state = PAS_WALKING;
            }
        }

        /* deceleration / braking */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0f) {
            pa->gsp += pa->dec * dt;
            if(pa->movmode == MM_FLOOR && fabs(pa->gsp) >= pa->brakingthreshold)
                pa->state = PAS_BRAKING;
        }
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0f) {
            pa->gsp -= pa->dec * dt;
            if(pa->movmode == MM_FLOOR && fabs(pa->gsp) >= pa->brakingthreshold)
                pa->state = PAS_BRAKING;
        }

        /* friction */
        if(!input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)) {
            if(fabs(pa->gsp) <= pa->frc * dt) {
                pa->gsp = 0.0f;
                pa->state = PAS_STOPPED;
            }
            else
                pa->gsp -= pa->frc * sign(pa->gsp) * dt;
        }
        else if(input_button_down(pa->input, IB_LEFT) && input_button_down(pa->input, IB_RIGHT)) {
            if(fabs(pa->gsp) <= pa->frc * dt) {
                pa->gsp = 0.0f;
                pa->state = PAS_STOPPED;
            }
        }

        /* slope factor */
        if(fabs(pa->gsp) >= pa->walkthreshold || fabs(SIN(pa->angle)) >= 0.707f)
            pa->gsp += pa->slp * -SIN(pa->angle) * dt;

        /* animation issues */
        if(fabs(pa->gsp) < pa->walkthreshold) {
            if(pa->state != PAS_PUSHING && input_button_down(pa->input, IB_DOWN) && fabs(pa->gsp) < EPSILON)
                pa->state = PAS_DUCKING;
            else if(pa->state != PAS_PUSHING && input_button_down(pa->input, IB_UP) && fabs(pa->gsp) < EPSILON)
                pa->state = PAS_LOOKINGUP;
            else if(pa->state != PAS_PUSHING && (input_button_down(pa->input, IB_LEFT) || input_button_down(pa->input, IB_RIGHT)))
                pa->state = input_button_down(pa->input, IB_LEFT) && input_button_down(pa->input, IB_RIGHT) ? PAS_STOPPED : PAS_WALKING;
            else if((pa->state != PAS_PUSHING && pa->state != PAS_WAITING) || (pa->state == PAS_PUSHING && !input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)))
                pa->state = PAS_STOPPED;
            else if((pa->state == PAS_STOPPED || pa->state == PAS_WAITING) && fabs(pa->gsp) > EPSILON)
                pa->state = PAS_WALKING;
        }
        else {
            if(pa->state == PAS_STOPPED || pa->state == PAS_WAITING || pa->state == PAS_LEDGE || pa->state == PAS_WALKING || pa->state == PAS_RUNNING)
                pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
            else if(pa->state == PAS_PUSHING)
                pa->state = PAS_WALKING;
        }
    }

    /*
     *
     * rolling
     *
     */

    /* start rolling */
    if(!pa->in_the_air && (pa->state == PAS_WALKING || pa->state == PAS_RUNNING)) {
        if(fabs(pa->gsp) >= pa->rollthreshold && input_button_down(pa->input, IB_DOWN))
            pa->state = PAS_ROLLING;
    }

    /* roll */
    if(!pa->in_the_air && pa->state == PAS_ROLLING) {

        /* slope factor */
        if(pa->gsp * SIN(pa->angle) >= 0.0f)
            pa->gsp += pa->rolluphillslp * -SIN(pa->angle) * dt;
        else
            pa->gsp += pa->rolldownhillslp * -SIN(pa->angle) * dt;

        /* velocity & control */
        if(pa->angle % 0x40 == 0) {

            /* deceleration */
            if(pa->angle == 0x0) {
                if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0f)
                    pa->gsp = min(0.0f, pa->gsp + pa->rolldec * dt);
                else if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0f)
                    pa->gsp = max(0.0f, pa->gsp - pa->rolldec * dt);
            }

            /* friction */
            if(fabs(pa->gsp) > pa->rollfrc * dt)
                pa->gsp -= pa->rollfrc * sign(pa->gsp) * dt;
            else
                pa->gsp = 0.0f;

            /* unroll */
            if(fabs(pa->gsp) < pa->unrollthreshold)
                pa->state = PAS_WALKING;
        }

        /* face right? */
        pa->facing_right = (pa->gsp >= 0.0f);
    }

    /*
     * charge and release
     */

    /* begin to charge */
    if(pa->state == PAS_DUCKING) {
        if(input_button_down(pa->input, IB_DOWN) && input_button_pressed(pa->input, IB_FIRE1)) {
            if(fabs(pa->chrg) > EPSILON) /* check if the player has the ability to charge */
                pa->state = PAS_CHARGING;
        }
    }

    /* charging... */
    if(pa->state == PAS_CHARGING) {
        /* release */
        if(!input_button_down(pa->input, IB_DOWN)) {
            float s = pa->facing_right ? 1.0f : -1.0f;
            pa->gsp = (s * pa->chrg) * (0.67f + pa->charge_intensity * 0.33f);
            pa->state = PAS_ROLLING;
            pa->charge_intensity = 0.0f;
        }
        else
            pa->gsp = 0.0f;

        /* charging more...! */
        if(input_button_pressed(pa->input, IB_FIRE1))
            pa->charge_intensity = min(1.0f, pa->charge_intensity + 0.25f);
        else
            pa->charge_intensity *= 0.99955f - 1.84845f * dt; /* attenuate charge intensity */
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
        if(pa->movmode != MM_FLOOR) {
            if(fabs(pa->gsp) < pa->falloffthreshold) {
                pa->horizontal_control_lock_timer = 0.5f;
                if(pa->angle >= 0x40 && pa->angle <= 0xC0) {
                    pa->angle = 0x0;
                    UPDATE_MOVMODE();
                }
            }
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
        if(pa->ysp < 0.0f && pa->ysp > pa->airdragthreshold) {
            /*pa->xsp *= powf(31.0f / 32.0f, 60.0f * dt);*/
            if(pa->airdrag_exp > 0.0f)
                pa->xsp *= (pa->airdrag + pa->airdrag_exp) - 60.0f * pa->airdrag_exp * dt;
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
            pa->ysp = min(pa->ysp + (0.85f * pa->grv) * dt, pa->topyspeed);
    }
    else {

        /* jumping */
        if(input_button_down(pa->input, IB_FIRE1) && !input_button_down(pa->input, IB_DOWN) && !input_button_down(pa->input, IB_UP)) {
            float grv_attenuation = (pa->gsp * SIN(pa->angle) < 0.0f) ? 1.0f : 0.5f;
            pa->xsp = pa->jmp * SIN(pa->angle) + pa->gsp * COS(pa->angle);
            pa->ysp = pa->jmp * COS(pa->angle) - pa->gsp * SIN(pa->angle) * grv_attenuation;
            pa->gsp = 0.0f;
            pa->angle = 0x0;
            pa->state = PAS_JUMPING;
            UPDATE_MOVMODE();
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

    /* previous air state */
    UPDATE_SENSORS();
    was_in_the_air = pa->in_the_air;

    /* moving */
    pa->position.x += pa->xsp * dt;
    pa->position.y += pa->ysp * dt;
    UPDATE_SENSORS();

    /* pushing against the walls */
    if(at_M != NULL) {
        const sensor_t* sensor = sensor_M(pa);
        int sensor_offset = 1 + (sensor_get_x2(sensor) - sensor_get_x1(sensor)) / 2;
        int x1, y1, x2, y2;

        sensor_worldpos(sensor, pa->position, pa->movmode, &x1, &y1, &x2, &y2);
        if(pa->movmode == MM_FLOOR || pa->movmode == MM_CEILING) {
            /* floor and ceiling modes */
            if(obstacle_got_collision(at_M, x2, y2, x2, y2)) {
                /* adjust position */
                if(pa->movmode == MM_FLOOR)
                    pa->position.x = obstacle_ground_position(at_M, x2, y2, GD_RIGHT) - sensor_offset;
                else
                    pa->position.x = obstacle_ground_position(at_M, x2, y2, GD_LEFT) + sensor_offset;
                UPDATE_SENSORS(); /* at_M should be set to NULL */

                /* set speed */
                pa->gsp = 0.0f;
                if(!pa->in_the_air) {
                    pa->xsp = 0.0f;
                    if(pa->state != PAS_ROLLING) {
                        if(input_button_down(pa->input, IB_RIGHT)) {
                            pa->state = PAS_PUSHING;
                            pa->facing_right = TRUE;
                        }
                        else
                            pa->state = PAS_STOPPED;
                    }
                }
                else
                    pa->xsp = min(pa->xsp, 0.0f);
            }
            else if(obstacle_got_collision(at_M, x1, y1, x1, y1)) {
                /* adjust position */
                if(pa->movmode == MM_FLOOR)
                    pa->position.x = obstacle_ground_position(at_M, x1, y1, GD_LEFT) + sensor_offset;
                else
                    pa->position.x = obstacle_ground_position(at_M, x1, y1, GD_RIGHT) - sensor_offset;
                UPDATE_SENSORS(); /* at_M should be set to NULL */

                /* set speed */
                pa->gsp = 0.0f;
                if(!pa->in_the_air) {
                    pa->xsp = 0.0f;
                    if(pa->state != PAS_ROLLING) {
                        if(input_button_down(pa->input, IB_LEFT)) {
                            pa->state = PAS_PUSHING;
                            pa->facing_right = FALSE;
                        }
                        else
                            pa->state = PAS_STOPPED;
                    }
                }
                else
                    pa->xsp = max(pa->xsp, 0.0f);
            }
        }
        else {
            if(!(pa->angle >= 0x40 && pa->angle <= 0xC0)) {
                pa->angle = 0x0;
                UPDATE_MOVMODE();
            }
            else {
                /* right wall and left wall modes */
                if(obstacle_got_collision(at_M, x2, y2, x2, y2)) {
                    /* adjust position */
                    if(pa->movmode == MM_RIGHTWALL)
                        pa->position.y = obstacle_ground_position(at_M, x2, y2, GD_UP) + sensor_offset;
                    else
                        pa->position.y = obstacle_ground_position(at_M, x2, y2, GD_DOWN) - sensor_offset;
                    UPDATE_SENSORS();

                    /* set speed */
                    pa->gsp = 0.0f;
                    if(!pa->in_the_air) {
                        pa->xsp = 0.0f;
                        pa->state = PAS_STOPPED;
                    }
                    else
                        pa->ysp = min(pa->ysp, 0.0f);
                }
                else if(obstacle_got_collision(at_M, x1, y1, x1, y1)) {
                    /* adjust position */
                    if(pa->movmode == MM_RIGHTWALL)
                        pa->position.y = obstacle_ground_position(at_M, x1, y1, GD_DOWN) - sensor_offset;
                    else
                        pa->position.y = obstacle_ground_position(at_M, x1, y1, GD_UP) + sensor_offset;
                    UPDATE_SENSORS();

                    /* set speed */
                    pa->gsp = 0.0f;
                    if(!pa->in_the_air) {
                        pa->xsp = 0.0f;
                        pa->state = PAS_STOPPED;
                    }
                    else
                        pa->ysp = max(pa->ysp, 0.0f);
                }
            }
        }
    }

    /* sticky physics */
    if(pa->in_the_air && (
        (!was_in_the_air && pa->state != PAS_JUMPING && pa->state != PAS_GETTINGHIT) ||
        (pa->state == PAS_ROLLING && !pa->sticky_lock)
    )){
        v2d_t offset = v2d_new(0,0);
        int u = 4; /* FIXME: try to use a fraction of the sensor height as well */

        /* mystery */
        if(fabs(pa->xsp) > pa->topspeed || pa->state == PAS_ROLLING) {
            int x, y, h = 12; /* shouldn't be higher */
            const sensor_t* s = (pa->xsp > 0) ? sensor_B(pa) : sensor_A(pa);
            sensor_worldpos(s, pa->position, pa->movmode, NULL, NULL, &x, &y);
            /*
            // FIXME: rolling in a U
            if(pa->state == PAS_ROLLING) // rolling hack
                u *= 2;
            */
            for(; u < h; u++) {
                if(pa->movmode == MM_FLOOR) {
                    if(obstaclemap_obstacle_exists(obstaclemap, x, y + u))
                        break;
                }
                else if(pa->movmode == MM_RIGHTWALL) {
                    if(obstaclemap_obstacle_exists(obstaclemap, y + u, x))
                        break;
                }
                else if(pa->movmode == MM_CEILING) {
                    if(obstaclemap_obstacle_exists(obstaclemap, x, y - u))
                        break;
                }
                else if(pa->movmode == MM_LEFTWALL) {
                    if(obstaclemap_obstacle_exists(obstaclemap, y - u, x))
                        break;
                }
            }
        }

        /* computing the test offset */
        if(pa->movmode == MM_FLOOR)
            offset = v2d_new(0,u);
        else if(pa->movmode == MM_CEILING)
            offset = v2d_new(0,-u);
        else if(pa->movmode == MM_RIGHTWALL)
            offset = v2d_new(u,0);
        else if(pa->movmode == MM_LEFTWALL)
            offset = v2d_new(-u,0);

        /* offset the character */
        pa->position = v2d_add(pa->position, offset);
        pa->in_the_air = FALSE; /* cloud bugfix for UPDATE_SENSORS */
        UPDATE_ANGLE();
        UPDATE_MOVMODE();
        UPDATE_SENSORS();

        /* if the player is still in the air,
           undo the offset */
        if(pa->in_the_air) {
            pa->position = v2d_subtract(pa->position, offset);
            UPDATE_ANGLE();
            UPDATE_MOVMODE();
            UPDATE_SENSORS();

            /* sticky physics hack */
            if(pa->state == PAS_ROLLING)
                pa->sticky_lock = TRUE;
        }
    }
    else if(!pa->in_the_air && pa->state == PAS_ROLLING) {
        /* undo sticky physics hack */
        pa->sticky_lock = FALSE;
    }

    /* stick to the ground */
    if((!pa->in_the_air) && !((pa->state == PAS_JUMPING || pa->state == PAS_GETTINGHIT) && pa->ysp < 0.0f)) {
        const obstacle_t *ground = NULL;
        const sensor_t *ground_sensor = NULL;
        int offset = 0;

        /* picking the ground */
        if(pick_the_best_ground(pa, at_A, at_B, sensor_A(pa), sensor_B(pa)) == 'a') {
            ground = at_A;
            ground_sensor = sensor_A(pa);
        }
        else {
            ground = at_B;
            ground_sensor = sensor_B(pa);
        }

        /* computing the offset (note: if !pa->in_the_air, then ground_sensor != NULL) */
        offset = sensor_get_y2(ground_sensor) - sensor_get_y1(ground_sensor) - 1; /* need -1 */

        /* adjust position */
        if(pa->movmode == MM_LEFTWALL)
            pa->position.x = obstacle_ground_position(ground, (int)pa->position.x - sensor_get_y2(ground_sensor), (int)pa->position.y + sensor_get_x2(ground_sensor), GD_LEFT) + offset;
        else if(pa->movmode == MM_CEILING)
            pa->position.y = obstacle_ground_position(ground, (int)pa->position.x - sensor_get_x2(ground_sensor), (int)pa->position.y - sensor_get_y2(ground_sensor), GD_UP) + offset;
        else if(pa->movmode == MM_RIGHTWALL)
            pa->position.x = obstacle_ground_position(ground, (int)pa->position.x + sensor_get_y2(ground_sensor), (int)pa->position.y - sensor_get_x2(ground_sensor), GD_RIGHT) - offset;
        else if(pa->movmode == MM_FLOOR)
            pa->position.y = obstacle_ground_position(ground, (int)pa->position.x + sensor_get_x2(ground_sensor), (int)pa->position.y + sensor_get_y2(ground_sensor), GD_DOWN) - offset;

        /* update the angle */
        UPDATE_ANGLE();
        UPDATE_MOVMODE();
        UPDATE_SENSORS();
    }

    /* reacquisition of the ground */
    if(!pa->in_the_air && was_in_the_air) {
        if(pa->angle >= 0xF0 || pa->angle <= 0x0F)
            pa->gsp = pa->xsp;
        else if((pa->angle >= 0xE0 && pa->angle <= 0xEF) || (pa->angle >= 0x10 && pa->angle <= 0x1F))
            pa->gsp = fabs(pa->xsp) > pa->ysp ? pa->xsp : pa->ysp * 1.0f * -sign(SIN(pa->angle));
        else if((pa->angle >= 0xC0 && pa->angle <= 0xDF) || (pa->angle >= 0x20 && pa->angle <= 0x3F))
            pa->gsp = fabs(pa->xsp) > pa->ysp ? pa->xsp : pa->ysp * -sign(SIN(pa->angle));

        pa->xsp = pa->ysp = 0.0f;
        if(pa->state != PAS_ROLLING)
            pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
    }

    /* bump into ceilings */
    if(pa->in_the_air && (at_C != NULL || at_D != NULL) && pa->ysp < 0.0f) {
        const obstacle_t *ceiling = NULL;
        const sensor_t *ceiling_sensor = NULL;

        /* picking the ceiling */
        if(pick_the_best_ceiling(pa, at_C, at_D, sensor_C(pa), sensor_D(pa)) == 'c') {
            ceiling = at_C;
            ceiling_sensor = sensor_C(pa);
        }
        else {
            ceiling = at_D;
            ceiling_sensor = sensor_D(pa);
        }

        /* compute the angle */
        pa->angle = 0x80;
        UPDATE_MOVMODE();
        UPDATE_SENSORS();
        UPDATE_ANGLE();
        UPDATE_MOVMODE();
        UPDATE_SENSORS();

        /* reattach to the ceiling */
        if((pa->angle >= 0xA0 && pa->angle <= 0xC0) || (pa->angle >= 0x40 && pa->angle <= 0x60)) {
            pa->gsp = (fabs(pa->xsp) > -pa->ysp) ? -pa->xsp : pa->ysp * -sign(SIN(pa->angle));
            pa->xsp = pa->ysp = 0.0f;
            if(pa->state != PAS_ROLLING)
                pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
        }
        else {
            /* won't reattach */
            int offset = sensor_get_y2(ceiling_sensor) - sensor_get_y1(ceiling_sensor);

            /* adjust speed & angle */
            pa->ysp = 0.0f;
            pa->angle = 0x0;
            UPDATE_MOVMODE();
            UPDATE_SENSORS();

            /* adjust position */
            pa->position.y = obstacle_ground_position(ceiling, (int)pa->position.x + sensor_get_x1(ceiling_sensor), (int)pa->position.y + sensor_get_y1(ceiling_sensor), GD_UP) + offset;
            UPDATE_SENSORS();
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
        UPDATE_MOVMODE();
    }

    /* I'm on the edge */
    if(!pa->in_the_air && fabs(pa->gsp) < EPSILON && pa->state != PAS_PUSHING && pa->movmode == MM_FLOOR) {
        const sensor_t* s = at_A ? sensor_A(pa) : sensor_B(pa);
        int x = (int)pa->position.x;
        int y = (int)pa->position.y + sensor_get_y2(s) + (sensor_get_y2(s) - sensor_get_y1(s)) / 2;
        if(at_A != NULL && at_B == NULL && !obstacle_got_collision(at_A, x, y, x, y)) {
            pa->state = PAS_LEDGE;
            pa->facing_right = TRUE;
        }
        else if(at_A == NULL && at_B != NULL && !obstacle_got_collision(at_B, x, y, x, y)) {
            pa->state = PAS_LEDGE;
            pa->facing_right = FALSE;
        }
    }

    /* waiting... */
    if(pa->state == PAS_STOPPED) {
        if((pa->wait_timer += dt) >= 3.0f)
            pa->state = PAS_WAITING;
    }
    else
        pa->wait_timer = 0.0f;

    /* winning */
    if(pa->winning_pose && fabs(pa->gsp) < pa->walkthreshold && !pa->in_the_air)
        pa->state = PAS_WINNING;

    /* animation bugfix */
    if(pa->in_the_air && (pa->state == PAS_PUSHING || pa->state == PAS_STOPPED || pa->state == PAS_DUCKING || pa->state == PAS_LOOKINGUP))
        pa->state = (fabs(pa->gsp) >= pa->topspeed) ? PAS_RUNNING : PAS_WALKING;
}

/* which one is the tallest obstacle, a or b? */
char pick_the_best_ground(const physicsactor_t *pa, const obstacle_t *a, const obstacle_t *b, const sensor_t *a_sensor, const sensor_t *b_sensor)
{
    int xa, ya, xb, yb, ha, hb;

    if(a == NULL)
        return 'b';
    else if(b == NULL)
        return 'a';

    switch(pa->movmode) {
        case MM_FLOOR:
            xa = (int)pa->position.x + sensor_get_x2(a_sensor);
            ya = (int)pa->position.y + sensor_get_y2(a_sensor);
            xb = (int)pa->position.x + sensor_get_x2(b_sensor);
            yb = (int)pa->position.y + sensor_get_y2(b_sensor);
            ha = obstacle_ground_position(a, xa, ya, GD_DOWN);
            hb = obstacle_ground_position(b, xb, yb, GD_DOWN);
            return ha < hb ? 'a' : 'b';

        case MM_LEFTWALL:
            xa = (int)pa->position.x - sensor_get_y2(a_sensor);
            ya = (int)pa->position.y + sensor_get_x2(a_sensor);
            xb = (int)pa->position.x - sensor_get_y2(b_sensor);
            yb = (int)pa->position.y + sensor_get_x2(b_sensor);
            ha = obstacle_ground_position(a, xa, ya, GD_LEFT);
            hb = obstacle_ground_position(b, xb, yb, GD_LEFT);
            return ha >= hb ? 'a' : 'b';

        case MM_CEILING:
            xa = (int)pa->position.x - sensor_get_x2(a_sensor);
            ya = (int)pa->position.y - sensor_get_y2(a_sensor);
            xb = (int)pa->position.x - sensor_get_x2(b_sensor);
            yb = (int)pa->position.y - sensor_get_y2(b_sensor);
            ha = obstacle_ground_position(a, xa, ya, GD_UP);
            hb = obstacle_ground_position(b, xb, yb, GD_UP);
            return ha >= hb ? 'a' : 'b';

        case MM_RIGHTWALL:
            xa = (int)pa->position.x + sensor_get_y2(a_sensor);
            ya = (int)pa->position.y - sensor_get_x2(a_sensor);
            xb = (int)pa->position.x + sensor_get_y2(b_sensor);
            yb = (int)pa->position.y - sensor_get_x2(b_sensor);
            ha = obstacle_ground_position(a, xa, ya, GD_RIGHT);
            hb = obstacle_ground_position(b, xb, yb, GD_RIGHT);
            return ha < hb ? 'a' : 'b';
    }

    return 'a';
}

/* which one is the best ceiling, c or d? */
char pick_the_best_ceiling(const physicsactor_t *pa, const obstacle_t *c, const obstacle_t *d, const sensor_t *c_sensor, const sensor_t *d_sensor)
{
    int xc, yc, xd, yd, hc, hd;

    if(c == NULL)
        return 'd';
    else if(d == NULL)
        return 'c';

    switch(pa->movmode) {
        case MM_FLOOR:
            xc = (int)pa->position.x + sensor_get_x1(c_sensor);
            yc = (int)pa->position.y + sensor_get_y1(c_sensor);
            xd = (int)pa->position.x + sensor_get_x1(d_sensor);
            yd = (int)pa->position.y + sensor_get_y1(d_sensor);
            hc = obstacle_ground_position(c, xc, yc, GD_UP);
            hd = obstacle_ground_position(d, xd, yd, GD_UP);
            return hc >= hd ? 'c' : 'd';

        case MM_LEFTWALL:
            xc = (int)pa->position.x - sensor_get_y1(c_sensor);
            yc = (int)pa->position.y + sensor_get_x1(c_sensor);
            xd = (int)pa->position.x - sensor_get_y1(d_sensor);
            yd = (int)pa->position.y + sensor_get_x1(d_sensor);
            hc = obstacle_ground_position(c, xc, yc, GD_RIGHT);
            hd = obstacle_ground_position(d, xd, yd, GD_RIGHT);
            return hc < hd ? 'c' : 'd';

        case MM_CEILING:
            xc = (int)pa->position.x - sensor_get_x1(c_sensor);
            yc = (int)pa->position.y - sensor_get_y1(c_sensor);
            xd = (int)pa->position.x - sensor_get_x1(d_sensor);
            yd = (int)pa->position.y - sensor_get_y1(d_sensor);
            hc = obstacle_ground_position(c, xc, yc, GD_DOWN);
            hd = obstacle_ground_position(d, xd, yd, GD_DOWN);
            return hc < hd ? 'c' : 'd';

        case MM_RIGHTWALL:
            xc = (int)pa->position.x + sensor_get_y1(c_sensor);
            yc = (int)pa->position.y - sensor_get_x1(c_sensor);
            xd = (int)pa->position.x + sensor_get_y1(d_sensor);
            yd = (int)pa->position.y - sensor_get_x1(d_sensor);
            hc = obstacle_ground_position(c, xc, yc, GD_LEFT);
            hd = obstacle_ground_position(d, xd, yd, GD_LEFT);
            return hc >= hd ? 'c' : 'd';
    }

    return 'c';
}

/* the min angle between alpha and beta, assuming 0x0 <= alpha, beta <= 0xFF */
int ang_diff(int alpha, int beta)
{
    int diff = alpha > beta ? alpha - beta : beta - alpha;
    return diff > 0x80 ? 0xFF - diff + 1 : diff;
}