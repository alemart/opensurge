/*
 * Open Surge Engine
 * physicsactor.c - physics system: actor
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

#include <math.h>
#include "physicsactor.h"
#include "sensor.h"
#include "obstaclemap.h"
#include "obstacle.h"
#include "../core/image.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../util/numeric.h"
#include "../util/util.h"

/* physicsactor class */
struct physicsactor_t
{
    v2d_t position; /* position of the physics actor */
    float xsp; /* x speed */
    float ysp; /* y speed */
    float gsp; /* ground speed */
    float acc; /* acceleration */
    float dec; /* deceleration */
    float frc; /* friction */
    float capspeed; /* cap speed */
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
    float airdragxthreshold; /* air drag x-threshold */
    float chrgthreshold; /* charge intensity threshold */
    float waittime; /* wait time in seconds */
    int angle; /* angle (0-255 clockwise) */
    int prev_angle; /* previous angle */
    bool midair; /* is the player midair? */
    bool was_midair; /* was the player midair in the previous frame of the simulation? */
    bool facing_right; /* is the player facing right? */
    bool touching_ceiling; /* is the player touching a ceiling? */
    bool smashed; /* inside a solid brick, smashed */
    bool winning_pose; /* winning pose enabled? */
    float hlock_timer; /* horizontal control lock timer, in seconds */
    float jump_lock_timer; /* jump lock timer, in seconds */
    float wait_timer; /* how long has the physics actor been stopped, in seconds */
    float midair_timer; /* how long has the physics actor been midair, in second */
    float breathe_timer; /* if greater than zero, set animation to breathing */
    bool want_to_detach_from_ground; /* sticky physics helper */
    float charge_intensity; /* charge intensity */
    float airdrag_coefficient[2]; /* airdrag approx. coefficient */
    physicsactorstate_t state; /* state */
    movmode_t movmode; /* current movement mode, based on the angle */
    obstaclelayer_t layer; /* current layer */
    input_t* input; /* input device */

    sensor_t* A_normal; /* sensors */
    sensor_t* B_normal;
    sensor_t* C_normal;
    sensor_t* D_normal;
    sensor_t* M_normal;
    sensor_t* N_normal;
    sensor_t* U_normal;
    sensor_t* A_jumproll;
    sensor_t* B_jumproll;
    sensor_t* C_jumproll;
    sensor_t* D_jumproll;
    sensor_t* M_jumproll;
    sensor_t* N_jumproll;
    sensor_t* M_flatgnd;
    sensor_t* N_flatgnd;
    sensor_t* M_rollflatgnd;
    sensor_t* N_rollflatgnd;
    v2d_t angle_sensor[2];

    float reference_time; /* used in fixed_update */
    float fixed_time;
};

/* private stuff ;-) */
static void render_ball(v2d_t sensor_position, int radius, color_t color, v2d_t camera_position);
static char pick_the_best_floor(const physicsactor_t *pa, const obstacle_t *a, const obstacle_t *b, const sensor_t *a_sensor, const sensor_t *b_sensor);
static char pick_the_best_ceiling(const physicsactor_t *pa, const obstacle_t *c, const obstacle_t *d, const sensor_t *c_sensor, const sensor_t *d_sensor);
static const obstacle_t* find_ground_with_extended_sensor(const physicsactor_t* pa, const obstaclemap_t* obstaclemap, const sensor_t* sensor, int extended_sensor_length, int* out_ground_position);
static bool is_smashed(const physicsactor_t* pa, const obstaclemap_t* obstaclemap);
static inline int distance_between_angle_sensors(const physicsactor_t* pa);
static inline int delta_angle(int alpha, int beta);
static int interpolate_angle(int alpha, int beta, float t);
static int extrapolate_angle(int curr_angle, int prev_angle, float t);

#define MM_TO_GD(mm) _MM_TO_GD[(mm) & 3]
static const grounddir_t _MM_TO_GD[4] = {
    [MM_FLOOR] = GD_DOWN,
    [MM_RIGHTWALL] = GD_RIGHT,
    [MM_CEILING] = GD_UP,
    [MM_LEFTWALL] = GD_LEFT
};

/* physics simulation */
#define WANT_JUMP_ATTENUATION   0
#define WANT_FIXED_TIMESTEP     1
#define AB_SENSOR_OFFSET        1 /* test with 0 and 1; with 0 it misbehaves a bit (unstable pa->midair) */
#define CLOUD_OFFSET            12
#define TARGET_FPS              60.0f
#define FIXED_TIMESTEP          (1.0f / TARGET_FPS)
#define HARD_CAPSPEED           (24.0f * TARGET_FPS)
static void run_simulation(physicsactor_t *pa, const obstaclemap_t *obstaclemap, float dt);
static void update_sensors(physicsactor_t* pa, const obstaclemap_t* obstaclemap, obstacle_t const** const at_A, obstacle_t const** const at_B, obstacle_t const** const at_C, obstacle_t const** const at_D, obstacle_t const** const at_M, obstacle_t const** const at_N);
static void update_movmode(physicsactor_t* pa);


/*
 * The character has a few sensors:
 * the dot '.' represents the position of the character;
 * sensors specified relative to this dot
 *
 *                                     U
 * A (vertical; left bottom)          ---
 * B (vertical; right bottom)       C | | D
 * C (vertical; left top)           M -.- N
 * D (vertical; right top)          A | | B
 * M (horizontal; left middle)      ^^^^^^^
 * N (horizontal; right middle)      ground
 * U (horizontal; up)
 *
 * The position of the sensors change according to the state of the player.
 * Instead of modifying the coordinates of the sensor, we have multiple,
 * immutable copies of them, and we retrieve them appropriately.
 */
static sensor_t* sensor_A(const physicsactor_t *pa);
static sensor_t* sensor_B(const physicsactor_t *pa);
static sensor_t* sensor_C(const physicsactor_t *pa);
static sensor_t* sensor_D(const physicsactor_t *pa);
static sensor_t* sensor_M(const physicsactor_t *pa);
static sensor_t* sensor_N(const physicsactor_t *pa);
static sensor_t* sensor_U(const physicsactor_t *pa);


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
/* SLOPE(y,x) is the angle of the (y,x) slope, where -SLOPE_LIMIT <= y,x <= SLOPE_LIMIT */
#define SLOPE_LIMIT 11
#define SLOPE(y,x) slp_table[(SLOPE_LIMIT) + ((y) + ((y)<-(SLOPE_LIMIT))*(-(SLOPE_LIMIT)-(y)) + ((y)>(SLOPE_LIMIT))*((SLOPE_LIMIT)-(y)))][(SLOPE_LIMIT) + ((x) + ((x)<-(SLOPE_LIMIT))*(-(SLOPE_LIMIT)-(x)) + ((x)>(SLOPE_LIMIT))*((SLOPE_LIMIT)-(x)))]
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

/* public methods */
physicsactor_t* physicsactor_create(v2d_t position)
{
    physicsactor_t *pa = mallocx(sizeof *pa);

    /* initializing... */
    pa->position = position;
    pa->xsp = 0.0f;
    pa->ysp = 0.0f;
    pa->gsp = 0.0f;
    pa->angle = 0x0;
    pa->prev_angle = 0x0;
    pa->movmode = MM_FLOOR;
    pa->state = PAS_STOPPED;
    pa->layer = OL_DEFAULT;
    pa->midair = true;
    pa->midair_timer = 0.0f;
    pa->hlock_timer = 0.0f;
    pa->jump_lock_timer = 0.0f;
    pa->facing_right = true;
    pa->touching_ceiling = false;
    pa->smashed = false;
    pa->input = input_create_computer();
    pa->wait_timer = 0.0f;
    pa->winning_pose = false;
    pa->breathe_timer = 0.0f;
    pa->want_to_detach_from_ground = false;
    pa->charge_intensity = 0.0f;
    pa->airdrag_coefficient[0] = 0.0f;
    pa->airdrag_coefficient[1] = 1.0f;
    pa->reference_time = 0.0f;
    pa->fixed_time = 0.0f;

    /* initialize the physics model */
    physicsactor_reset_model_parameters(pa);

    /* let's initialize the sensors with a simple box model */

    /* set box size (W,H) and half box size (w,h) relative to sensors A, B, C, D.
       These sensors are vertical and symmetric; hence,
       W = w - (-w) + 1 = 2*w + 1 => w = (W-1)/2; also, h = (H-1)/2 */
    const int default_width = 19, default_height = 39; /* pick odd numbers */
    const int roll_width = 15, roll_height = 29; /* this is expected to be smaller than the default box */
    const int roll_y_offset = 5; /* offset from the sensor origin */

    int w = (default_width - 1) / 2, h = (default_height - 1) / 2;
    int rw = (roll_width - 1) / 2, rh = (roll_height - 1) / 2;
    int ry = roll_y_offset;

    h += AB_SENSOR_OFFSET; /* grow heights */
    rh += AB_SENSOR_OFFSET;

    /* set up the sensors */
    pa->A_normal = sensor_create_vertical(-w, 0, h, color_rgb(0,255,0));
    pa->B_normal = sensor_create_vertical(w, 0, h, color_rgb(255,255,0));
    pa->C_normal = sensor_create_vertical(-w, 0, -h, color_rgb(64,255,255));
    pa->D_normal = sensor_create_vertical(w, 0, -h, color_rgb(255,255,255));
    pa->M_normal = sensor_create_horizontal(0, 0, -(w+1), color_rgb(255,0,0)); /* use x(sensor A) + 1 */
    pa->N_normal = sensor_create_horizontal(0, 0, w+1, color_rgb(255,64,255));
    pa->U_normal = sensor_create_horizontal(-4, 0, 0, color_rgb(0,192,255));

    pa->A_jumproll = sensor_create_vertical(-rw, ry, ry+rh, sensor_color(pa->A_normal));
    pa->B_jumproll = sensor_create_vertical(rw, ry, ry+rh, sensor_color(pa->B_normal));
    pa->C_jumproll = sensor_create_vertical(-rw, ry, ry-rh, sensor_color(pa->C_normal));
    pa->D_jumproll = sensor_create_vertical(rw, ry, ry-rh, sensor_color(pa->D_normal));
    pa->M_jumproll = sensor_create_horizontal(ry, 0, -(w+1), sensor_color(pa->M_normal));
    pa->N_jumproll = sensor_create_horizontal(ry, 0, w+1, sensor_color(pa->N_normal));

    pa->M_flatgnd = sensor_create_horizontal(8, 0, -(w+1), sensor_color(pa->M_normal));
    pa->N_flatgnd = sensor_create_horizontal(8, 0, w+1, sensor_color(pa->N_normal));
    pa->M_rollflatgnd = sensor_create_horizontal(max(ry,8), ry, -(w+1), sensor_color(pa->M_normal));
    pa->N_rollflatgnd = sensor_create_horizontal(max(ry,8), ry, w+1, sensor_color(pa->N_normal));

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
    sensor_destroy(pa->N_normal);
    sensor_destroy(pa->U_normal);

    sensor_destroy(pa->A_jumproll);
    sensor_destroy(pa->B_jumproll);
    sensor_destroy(pa->C_jumproll);
    sensor_destroy(pa->D_jumproll);
    sensor_destroy(pa->M_jumproll);
    sensor_destroy(pa->N_jumproll);

    sensor_destroy(pa->M_flatgnd);
    sensor_destroy(pa->N_flatgnd);
    sensor_destroy(pa->M_rollflatgnd);
    sensor_destroy(pa->N_rollflatgnd);

    input_destroy(pa->input);
    free(pa);
    return NULL;
}

void physicsactor_reset_model_parameters(physicsactor_t* pa)
{
    const float fpsmul = TARGET_FPS;

    /*
      +--------------------+----------------+-----------------+
      | model parameter    |  magic number  | fps multitplier |
      +--------------------+----------------+-----------------+
     */
    pa->acc =                  (3.0f/64.0f) * fpsmul * fpsmul ;
    pa->dec =                   0.5f        * fpsmul * fpsmul ;
    pa->frc =                  (3.0f/64.0f) * fpsmul * fpsmul ;
    pa->capspeed =              16.0f       * fpsmul * 1.0f   ; /* tiers: default 16; super 20; ultra 24 */
    pa->topspeed =              6.0f        * fpsmul * 1.0f   ;
    pa->topyspeed =             16.0f       * fpsmul * 1.0f   ;
    pa->air =                  (6.0f/64.0f) * fpsmul * fpsmul ;
    pa->airdrag =             (31.0f/32.0f) * 1.0f   * 1.0f   ;
    pa->jmp =                   -6.5f       * fpsmul * 1.0f   ;
    pa->jmprel =                -4.0f       * fpsmul * 1.0f   ;
    pa->diejmp =                -7.0f       * fpsmul * 1.0f   ;
    pa->hitjmp =                -4.0f       * fpsmul * 1.0f   ;
    pa->grv =                 (14.0f/64.0f) * fpsmul * fpsmul ;
    pa->slp =                  (8.0f/64.0f) * fpsmul * fpsmul ;
    pa->chrg =                  12.0f       * fpsmul * 1.0f   ;
    pa->walkthreshold =         0.5f        * fpsmul * 1.0f   ;
    pa->unrollthreshold =       0.5f        * fpsmul * 1.0f   ;
    pa->rollthreshold =         1.0f        * fpsmul * 1.0f   ;
    pa->rollfrc =             (3.0f/128.0f) * fpsmul * fpsmul ;
    pa->rolldec =              (8.0f/64.0f) * fpsmul * fpsmul ;
    pa->rolluphillslp =        (5.0f/64.0f) * fpsmul * fpsmul ;
    pa->rolldownhillslp =     (20.0f/64.0f) * fpsmul * fpsmul ;
    pa->falloffthreshold =      2.5f        * fpsmul * 1.0f   ;
    pa->brakingthreshold =      4.0f        * fpsmul * 1.0f   ;
    pa->airdragthreshold =      -4.0f       * fpsmul * 1.0f   ;
    pa->airdragxthreshold =    (8.0f/64.0f) * fpsmul * 1.0f   ;
    pa->chrgthreshold =        (1.0f/64.0f) * 1.0f   * 1.0f   ;
    pa->waittime =              3.0f        * 1.0f   * 1.0f   ;

    /* recompute airdrag coefficients */
    physicsactor_set_airdrag(pa, pa->airdrag);
}

void physicsactor_update(physicsactor_t *pa, const obstaclemap_t *obstaclemap)
{
    float dt = timer_get_delta();

    /* run the physics simulation */
#if WANT_FIXED_TIMESTEP
    pa->reference_time += dt;
    if(pa->reference_time <= pa->fixed_time + FIXED_TIMESTEP) {
        /* will run with a fixed timestep, but only at TARGET_FPS */
        run_simulation(pa, obstaclemap, FIXED_TIMESTEP); /* improved precision */
        pa->fixed_time += FIXED_TIMESTEP;
    }
    else {
        /* prevent jittering at lower fps rates */
        run_simulation(pa, obstaclemap, dt); /* can't use a fixed timestep */
        pa->fixed_time = pa->reference_time;
    }
#else
    run_simulation(pa, obstaclemap, dt);
#endif
}

void physicsactor_render_sensors(const physicsactor_t *pa, v2d_t camera_position)
{
    render_ball(pa->position, 1, color_rgb(255, 255, 255), camera_position);

    if(!pa->midair) {
        render_ball(pa->angle_sensor[0], 1, sensor_color(sensor_A(pa)), camera_position);
        render_ball(pa->angle_sensor[1], 1, sensor_color(sensor_B(pa)), camera_position);
    }

    sensor_render(sensor_A(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_B(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_C(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_D(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_M(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_N(pa), pa->position, pa->movmode, camera_position);
    sensor_render(sensor_U(pa), pa->position, pa->movmode, camera_position);
}

physicsactorstate_t physicsactor_get_state(const physicsactor_t *pa)
{
    return pa->state;
}

int physicsactor_get_angle(const physicsactor_t *pa)
{
    return (((256 - pa->angle) * 180) / 128) % 360;
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
    seconds = max(seconds, 0.0f);
    if(seconds > pa->hlock_timer)
        pa->hlock_timer = seconds;
}

float physicsactor_hlock_timer(const physicsactor_t *pa)
{
    return pa->hlock_timer;
}

bool physicsactor_ressurrect(physicsactor_t *pa, v2d_t position)
{
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED) {
        pa->gsp = 0.0f;
        pa->xsp = 0.0f;
        pa->ysp = 0.0f;
        pa->facing_right = true;
        pa->state = PAS_STOPPED;
        physicsactor_set_position(pa, position);
        return true;
    }

    return false;
}

bool physicsactor_is_midair(const physicsactor_t *pa)
{
    return pa->midair;
}

bool physicsactor_is_touching_ceiling(const physicsactor_t *pa)
{
    return pa->touching_ceiling;
}

bool physicsactor_is_facing_right(const physicsactor_t *pa)
{
    return pa->facing_right;
}

bool physicsactor_is_smashed(const physicsactor_t *pa)
{
    return pa->smashed;
}

void physicsactor_enable_winning_pose(physicsactor_t *pa)
{
    pa->winning_pose = true;
}

void physicsactor_detach_from_ground(physicsactor_t *pa)
{
    pa->want_to_detach_from_ground = true;
}

movmode_t physicsactor_get_movmode(const physicsactor_t *pa)
{
    return pa->movmode;
}

obstaclelayer_t physicsactor_get_layer(const physicsactor_t *pa)
{
    return pa->layer;
}

void physicsactor_set_layer(physicsactor_t *pa, obstaclelayer_t layer)
{
    pa->layer = layer;
}

int physicsactor_roll_delta(const physicsactor_t* pa)
{
    /* the difference of the height of the ground sensors */
    return sensor_get_y2(pa->A_normal) - sensor_get_y2(pa->A_jumproll);
}

float physicsactor_charge_intensity(const physicsactor_t* pa)
{
    return pa->charge_intensity;
}

void physicsactor_capture_input(physicsactor_t *pa, const input_t *in)
{
    input_copy(pa->input, in);
}

void physicsactor_kill(physicsactor_t *pa)
{
    if(pa->state != PAS_DEAD && pa->state != PAS_DROWNED) {
        pa->xsp = 0.0f;
        pa->ysp = pa->diejmp;

        pa->state = PAS_DEAD;
    }
}

void physicsactor_hit(physicsactor_t *pa, float direction)
{
    /* direction: >0 right; <0 left */

    if(pa->state != PAS_GETTINGHIT) {
        float dir = (direction != 0.0f) ? sign(direction) : (pa->facing_right ? -1.0f : 1.0f);
        pa->xsp = (-pa->hitjmp * 0.5f) * dir;
        pa->ysp = pa->hitjmp;

        physicsactor_detach_from_ground(pa);
        pa->state = PAS_GETTINGHIT;
    }
}

bool physicsactor_bounce(physicsactor_t *pa, float direction)
{
    /* direction: >0 down; <0 up */

    /* do nothing if on the ground */
    if(!pa->midair || pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return false;

    /* bounce */
    if(direction < 0.0f && pa->ysp > 0.0f) /* the specified direction is just a hint */
        pa->ysp = -pa->ysp;
    else
        pa->ysp -= 60.0f * sign(pa->ysp);

    pa->state = PAS_JUMPING;
    return true;
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
    if(pa->state != PAS_DROWNED && pa->state != PAS_DEAD) {
        pa->xsp = 0.0f;
        pa->ysp = 0.0f;

        pa->state = PAS_DROWNED;
    }
}

void physicsactor_breathe(physicsactor_t *pa)
{
    if(pa->state != PAS_BREATHING) {
        pa->xsp = 0.0f;
        pa->ysp = 0.0f;

        pa->breathe_timer = 0.5f;
        pa->state = PAS_BREATHING;
    }
}

/* getters and setters */
#define GENERATE_GETTER_AND_SETTER_OF(x) \
float physicsactor_get_##x(const physicsactor_t *pa) \
{ \
    return pa->x; \
} \
\
void physicsactor_set_##x(physicsactor_t *pa, float value) \
{ \
    pa->x = value; \
}

GENERATE_GETTER_AND_SETTER_OF(xsp)
GENERATE_GETTER_AND_SETTER_OF(ysp)
GENERATE_GETTER_AND_SETTER_OF(gsp)
GENERATE_GETTER_AND_SETTER_OF(acc)
GENERATE_GETTER_AND_SETTER_OF(dec)
GENERATE_GETTER_AND_SETTER_OF(frc)
GENERATE_GETTER_AND_SETTER_OF(topspeed)
GENERATE_GETTER_AND_SETTER_OF(capspeed)
GENERATE_GETTER_AND_SETTER_OF(air)
GENERATE_GETTER_AND_SETTER_OF(jmp)
GENERATE_GETTER_AND_SETTER_OF(jmprel)
GENERATE_GETTER_AND_SETTER_OF(diejmp)
GENERATE_GETTER_AND_SETTER_OF(hitjmp)
GENERATE_GETTER_AND_SETTER_OF(grv)
GENERATE_GETTER_AND_SETTER_OF(slp)
GENERATE_GETTER_AND_SETTER_OF(chrg)
GENERATE_GETTER_AND_SETTER_OF(rollfrc)
GENERATE_GETTER_AND_SETTER_OF(rolldec)
GENERATE_GETTER_AND_SETTER_OF(rolluphillslp)
GENERATE_GETTER_AND_SETTER_OF(rolldownhillslp)
GENERATE_GETTER_AND_SETTER_OF(walkthreshold)
GENERATE_GETTER_AND_SETTER_OF(rollthreshold)
GENERATE_GETTER_AND_SETTER_OF(unrollthreshold)
GENERATE_GETTER_AND_SETTER_OF(falloffthreshold)
GENERATE_GETTER_AND_SETTER_OF(brakingthreshold)
GENERATE_GETTER_AND_SETTER_OF(airdragthreshold)
GENERATE_GETTER_AND_SETTER_OF(airdragxthreshold)
GENERATE_GETTER_AND_SETTER_OF(chrgthreshold)
GENERATE_GETTER_AND_SETTER_OF(waittime)

float physicsactor_get_airdrag(const physicsactor_t *pa) { return pa->airdrag; }
void physicsactor_set_airdrag(physicsactor_t *pa, float value)
{
    pa->airdrag = clip01(value);
    if(pa->airdrag > 0.0f && pa->airdrag < 1.0f) {
        /* recompute airdrag coefficients */
        pa->airdrag_coefficient[0] = 60.0f * pa->airdrag * logf(pa->airdrag);
        pa->airdrag_coefficient[1] = pa->airdrag * (1.0f - logf(pa->airdrag));
    }
    else if(pa->airdrag > 0.0f) {
        /* no airdrag */
        pa->airdrag_coefficient[0] = 0.0f;
        pa->airdrag_coefficient[1] = 1.0f;
    }
    else {
        pa->airdrag_coefficient[0] = 0.0f;
        pa->airdrag_coefficient[1] = 0.0f;
    }
}


/* sensors */
#define GENERATE_SENSOR_GETTER(x, standing, jumping, rolling, in_the_air, standing_on_flat_ground, rolling_on_flat_ground) \
sensor_t* sensor_##x(const physicsactor_t *pa) \
{ \
    if(pa->state == PAS_ROLLING || pa->state == PAS_CHARGING) { \
        if(!pa->midair && pa->angle % 0x40 == 0) \
            return pa->x##_##rolling_on_flat_ground; \
        else \
            return pa->x##_##rolling; \
    } \
    else if(pa->state == PAS_JUMPING) \
        return pa->x##_##jumping; \
    else if(pa->midair || pa->state == PAS_SPRINGING) \
        return pa->x##_##in_the_air; \
    else if(!pa->midair && pa->angle % 0x40 == 0) \
        return pa->x##_##standing_on_flat_ground; \
    else \
        return pa->x##_##standing; \
}

GENERATE_SENSOR_GETTER(A, normal, jumproll, jumproll, normal, normal,  jumproll)
GENERATE_SENSOR_GETTER(B, normal, jumproll, jumproll, normal, normal,  jumproll)
GENERATE_SENSOR_GETTER(C, normal, jumproll, jumproll, normal, normal,  jumproll)
GENERATE_SENSOR_GETTER(D, normal, jumproll, jumproll, normal, normal,  jumproll)
GENERATE_SENSOR_GETTER(M, normal, jumproll, jumproll, normal, flatgnd, rollflatgnd)
GENERATE_SENSOR_GETTER(N, normal, jumproll, jumproll, normal, flatgnd, rollflatgnd)
GENERATE_SENSOR_GETTER(U, normal, normal,   normal,   normal, normal,  normal)


/* get bounding box */
void physicsactor_bounding_box(const physicsactor_t *pa, int *width, int *height, v2d_t *center)
{
    const sensor_t* sensor_a = sensor_A(pa);
    const sensor_t* sensor_d = sensor_D(pa);

    /* find size */
    point2d_t a = sensor_local_tail(sensor_a);
    point2d_t d = sensor_local_tail(sensor_d);
    int w = d.x - a.x + 1;
    int h = a.y - d.y + 1;

    /* adjust size */
    h -= 2 * AB_SENSOR_OFFSET; /* subtract two offsets: one from A, another from D */
    h -= 6;
    w -= 2;

    /* find center */
    int x = (int)floorf(pa->position.x);
    int y = (int)floorf(pa->position.y);

    /* rotate and apply offset */
    point2d_t offset = sensor_local_head(sensor_d);
    int rw, rh;
    switch(pa->movmode) {
        case MM_FLOOR:
            rw = w;
            rh = h;
            y += offset.y;
            break;

        case MM_CEILING:
            rw = w;
            rh = h;
            y -= offset.y;
            break;

        case MM_RIGHTWALL:
            rw = h;
            rh = w;
            x += offset.y;
            break;

        case MM_LEFTWALL:
            rw = h;
            rh = w;
            x -= offset.y;
            break;
    }

    /* return values */
    if(width != NULL)
        *width = max(rw, 1);
    if(height != NULL)
        *height = max(rh, 1);
    if(center != NULL)
        *center = v2d_new(x, y);
}

/* check if the physicsactor is standing on a specific platform (obstacle) */
bool physicsactor_is_standing_on_platform(const physicsactor_t *pa, const obstacle_t *obstacle)
{
    int x1, y1, x2, y2;
    const sensor_t *a = sensor_A(pa);
    const sensor_t *b = sensor_B(pa);

    /* check sensor A */
    sensor_worldpos(a, pa->position, pa->movmode, &x1, &y1, &x2, &y2);
    if(obstacle_got_collision(obstacle, x1, y1, x2, y2))
        return true;

    /* check sensor B */
    sensor_worldpos(b, pa->position, pa->movmode, &x1, &y1, &x2, &y2);
    if(obstacle_got_collision(obstacle, x1, y1, x2, y2))
        return true;

    /* no collision */
    return false;
}



/*
 * ---------------------------------------
 *           PHYSICS ENGINE
 * ---------------------------------------
 */

/* utility macros */
#define WALKING_OR_RUNNING(pa)      ((fabs((pa)->gsp) >= (pa)->topspeed) ? PAS_RUNNING : PAS_WALKING)

/* call UPDATE_SENSORS whenever you update pa->position or pa->angle */
#define UPDATE_SENSORS()            update_sensors(pa, obstaclemap, &at_A, &at_B, &at_C, &at_D, &at_M, &at_N)

/* call UPDATE_MOVMODE whenever you update pa->angle */
#define UPDATE_MOVMODE()            update_movmode(pa)

/* compute the current pa->angle */
#define UPDATE_ANGLE() \
    do { \
        const sensor_t* sensor = sensor_A(pa); \
        int sensor_height = sensor_get_y2(sensor) - sensor_get_y1(sensor); \
        int search_base = sensor_get_y2(sensor) - 1; \
        int max_iterations = sensor_height * 3; \
        int half_dist = distance_between_angle_sensors(pa) / 2; \
        int hoff = half_dist + (1 - half_dist % 2); /* odd number */ \
        int min_hoff = pa->was_midair ? 3 : 1; \
        int max_delta = min(hoff * 2, SLOPE_LIMIT); \
        int angular_tolerance = 0x14; \
        int current_angle = pa->angle; \
        int dx = 0, dy = 0; \
        \
        float abs_gsp = fabsf(pa->gsp); \
        bool within_default_capspeed = (abs_gsp <= 16.0f * TARGET_FPS); \
        bool within_increased_capspeed = (abs_gsp <= 20.0f * TARGET_FPS); \
        \
        v2d_t vel = v2d_new(pa->xsp, pa->ysp); \
        v2d_t ds = v2d_multiply(vel, dt); \
        float linear_prediction_factor = within_default_capspeed ? 0.4f : (within_increased_capspeed ? 0.5f : 0.6f); \
        v2d_t predicted_offset = v2d_multiply(ds, linear_prediction_factor); \
        v2d_t predicted_position = v2d_add(pa->position, predicted_offset); \
        \
        float angular_prediction_factor = within_default_capspeed ? 0.0f : (within_increased_capspeed ? 0.0f : 0.2f); \
        int predicted_angle = extrapolate_angle(pa->angle, pa->prev_angle, angular_prediction_factor); \
        (void)interpolate_angle; \
        \
        do { \
            pa->angle = predicted_angle; /* assume continuity */ \
            UPDATE_ANGLE_STEP(hoff, search_base, predicted_angle, predicted_position, max_iterations, dx, dy); \
            hoff -= 2; /* increase precision */ \
        } while(hoff >= min_hoff && (at_M == NULL && at_N == NULL) && (dx < -max_delta || dx > max_delta || dy < -max_delta || dy > max_delta || delta_angle(pa->angle, current_angle) > angular_tolerance)); \
    } while(0)

#define UPDATE_ANGLE_STEP(hoff, search_base, guess_angle, curr_position, max_iterations, out_dx, out_dy) \
    do { \
        const obstacle_t* gnd = NULL; \
        bool found_a = false, found_b = false; \
        int xa, ya, xb, yb; \
        \
        for(int i = 0; i < (max_iterations) && !(found_a && found_b); i++) { \
            float h = (search_base) + i; \
            float x = floorf(curr_position.x) + h * SIN(guess_angle) + 0.5f; \
            float y = floorf(curr_position.y) + h * COS(guess_angle) + 0.5f; \
            \
            if(!found_a) { \
                xa = x - (hoff) * COS(guess_angle); \
                ya = y + (hoff) * SIN(guess_angle); \
                gnd = obstaclemap_get_best_obstacle_at(obstaclemap, xa, ya, xa, ya, pa->movmode, pa->layer); \
                found_a = (gnd != NULL && (obstacle_is_solid(gnd) || ( \
                    (pa->movmode == MM_FLOOR && ya < obstacle_ground_position(gnd, xa, ya, GD_DOWN) + CLOUD_OFFSET) || \
                    (pa->movmode == MM_CEILING && ya > obstacle_ground_position(gnd, xa, ya, GD_UP) - CLOUD_OFFSET) || \
                    (pa->movmode == MM_LEFTWALL && xa > obstacle_ground_position(gnd, xa, ya, GD_LEFT) - CLOUD_OFFSET) || \
                    (pa->movmode == MM_RIGHTWALL && xa < obstacle_ground_position(gnd, xa, ya, GD_RIGHT) + CLOUD_OFFSET) \
                ))); \
            } \
            \
            if(!found_b) { \
                xb = x + (hoff) * COS(guess_angle); \
                yb = y - (hoff) * SIN(guess_angle); \
                gnd = obstaclemap_get_best_obstacle_at(obstaclemap, xb, yb, xb, yb, pa->movmode, pa->layer); \
                found_b = (gnd != NULL && (obstacle_is_solid(gnd) || ( \
                    (pa->movmode == MM_FLOOR && yb < obstacle_ground_position(gnd, xb, yb, GD_DOWN) + CLOUD_OFFSET) || \
                    (pa->movmode == MM_CEILING && yb > obstacle_ground_position(gnd, xb, yb, GD_UP) - CLOUD_OFFSET) || \
                    (pa->movmode == MM_LEFTWALL && xb > obstacle_ground_position(gnd, xb, yb, GD_LEFT) - CLOUD_OFFSET) || \
                    (pa->movmode == MM_RIGHTWALL && xb < obstacle_ground_position(gnd, xb, yb, GD_RIGHT) + CLOUD_OFFSET) \
                ))); \
            } \
        } \
        \
        out_dx = out_dy = 0; \
        pa->angle_sensor[0] = pa->angle_sensor[1] = curr_position; \
        if(found_a && found_b) { \
            int x, y; \
            const obstacle_t* ga = obstaclemap_get_best_obstacle_at(obstaclemap, xa, ya, xa, ya, pa->movmode, pa->layer); \
            const obstacle_t* gb = obstaclemap_get_best_obstacle_at(obstaclemap, xb, yb, xb, yb, pa->movmode, pa->layer); \
            \
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
                \
                x = xb - xa; \
                y = yb - ya; \
                if(x != 0 || y != 0) { \
                    int ang = SLOPE(y, x); \
                    if(ga == gb || delta_angle(ang, guess_angle) <= 0x25) { \
                        pa->angle = ang; \
                        pa->angle_sensor[0] = v2d_new(xa, ya); \
                        pa->angle_sensor[1] = v2d_new(xb, yb); \
                        out_dx = x; out_dy = y; \
                    } \
                } \
            } \
        } \
    } while(0)

/* auxiliary macro to force the angle to a value */
#define FORCE_ANGLE(new_angle) \
    do { \
        pa->angle = (new_angle); \
        pa->prev_angle = pa->angle; \
        UPDATE_MOVMODE(); \
        UPDATE_SENSORS(); \
    } while(0)

/* auxiliary macro to compute the angle automatically */
#define SET_AUTO_ANGLE() \
    do { \
        UPDATE_ANGLE(); \
        UPDATE_MOVMODE(); \
        UPDATE_SENSORS(); \
    } while(0)

/* physics simulation */
void run_simulation(physicsactor_t *pa, const obstaclemap_t *obstaclemap, float dt)
{
    const obstacle_t *at_A, *at_B, *at_C, *at_D, *at_M, *at_N;

    /*
     *
     * initialization
     *
     */

    /* save previous state */
    UPDATE_SENSORS();
    pa->was_midair = pa->midair;
    pa->prev_angle = pa->angle;

    /* disable simultaneous left + right input */
    if(input_button_down(pa->input, IB_LEFT) && input_button_down(pa->input, IB_RIGHT)) {
        input_simulate_button_up(pa->input, IB_LEFT);
        input_simulate_button_up(pa->input, IB_RIGHT);
    }

    /*
     *
     * horizontal control lock
     *
     */

    if(pa->hlock_timer > 0.0f) {
        pa->hlock_timer -= dt;
        if(pa->hlock_timer < 0.0f)
            pa->hlock_timer = 0.0f;

        input_simulate_button_up(pa->input, IB_LEFT);
        input_simulate_button_up(pa->input, IB_RIGHT);

        if(!pa->midair && !nearly_zero(pa->gsp))
            pa->facing_right = (pa->gsp > 0.0f);
        else if(pa->midair && !nearly_zero(pa->xsp))
            pa->facing_right = (pa->xsp > 0.0f);
    }

    /*
     *
     * death
     *
     */

    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED) {
        pa->ysp = min(pa->ysp + pa->grv * dt, pa->topyspeed);
        pa->position.y += pa->ysp * dt;
        pa->facing_right = true;
        return;
    }

    /*
     *
     * getting hit
     *
     */

    if(pa->state == PAS_GETTINGHIT) {
        input_reset(pa->input);
    }

    /*
     *
     * waiting
     *
     */

    if(pa->state == PAS_STOPPED) {
        pa->wait_timer += dt;
        if(pa->wait_timer >= pa->waittime)
            pa->state = PAS_WAITING;
    }
    else
        pa->wait_timer = 0.0f;

    /*
     *
     * winning
     *
     */

    if(pa->winning_pose) {
        /* brake on level clear */
        const float threshold = 60.0f;
        input_reset(pa->input);

        pa->gsp = clip(pa->gsp, -0.67f * pa->capspeed, 0.67f * pa->capspeed);
        if(pa->state == PAS_ROLLING)
            pa->state = PAS_BRAKING;

        if(pa->gsp > threshold)
            input_simulate_button_down(pa->input, IB_LEFT);
        else if(pa->gsp < -threshold)
            input_simulate_button_down(pa->input, IB_RIGHT);
        else
            input_disable(pa->input);
    }

    /*
     *
     * facing left or right
     *
     */

    if(pa->state != PAS_ROLLING && (!nearly_zero(pa->gsp) || !nearly_zero(pa->xsp))) {
        if((pa->gsp > 0.0f || pa->midair) && input_button_down(pa->input, IB_RIGHT))
            pa->facing_right = true;
        else if((pa->gsp < 0.0f || pa->midair) && input_button_down(pa->input, IB_LEFT))
            pa->facing_right = false;
    }

    /*
     *
     * walking & running
     *
     */

    if(!pa->midair && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING) {

        /* slope factor */
        if(fabs(pa->gsp) >= pa->walkthreshold || fabs(SIN(pa->angle)) >= 0.707f)
            pa->gsp += pa->slp * -SIN(pa->angle) * dt;

        /* acceleration */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp >= 0.0f) {
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
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp <= 0.0f) {
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

        /* deceleration */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0f) {
            pa->gsp += pa->dec * dt;
            if(pa->gsp >= 0.0f) {
                pa->gsp = 0.0f;
                pa->state = PAS_STOPPED;
            }
            else if(fabs(pa->gsp) >= pa->brakingthreshold) {
                if(pa->movmode == MM_FLOOR)
                    pa->state = PAS_BRAKING;
            }
        }
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0f) {
            pa->gsp -= pa->dec * dt;
            if(pa->gsp <= 0.0f) {
                pa->gsp = 0.0f;
                pa->state = PAS_STOPPED;
            }
            else if(fabs(pa->gsp) >= pa->brakingthreshold) {
                if(pa->movmode == MM_FLOOR)
                    pa->state = PAS_BRAKING;
            }
        }

        /* braking & friction */
        if(pa->state == PAS_BRAKING) {
            /* braking */
            float brk = pa->frc * (1.5f + 3.0f * fabs(SIN(pa->angle)));
            if(fabs(pa->gsp) <= brk * dt) {
                pa->gsp = 0.0f;
                pa->state = PAS_STOPPED;
            }
            else
                pa->gsp -= brk * sign(pa->gsp) * dt;
        }
        else {
            /* friction */
            if(!input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)) {
                if(fabs(pa->gsp) <= pa->frc * dt) {
                    pa->gsp = 0.0f;
                    pa->state = PAS_STOPPED;
                }
                else
                    pa->gsp -= pa->frc * sign(pa->gsp) * dt;
            }
        }

        /* animation issues */
        if(fabs(pa->gsp) < pa->walkthreshold) {
            if(pa->state == PAS_PUSHING && !input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT))
                pa->state = PAS_STOPPED;
            else if(pa->state == PAS_PUSHING || pa->state == PAS_LOOKINGUP || pa->state == PAS_DUCKING)
                ;
            else if(input_button_down(pa->input, IB_LEFT) || input_button_down(pa->input, IB_RIGHT))
                pa->state = PAS_WALKING;
            else if(pa->state != PAS_WAITING)
                pa->state = PAS_STOPPED;
            else if((pa->state == PAS_STOPPED || pa->state == PAS_WAITING) && !nearly_zero(pa->gsp))
                pa->state = PAS_WALKING;
        }
        else {
            if(pa->state == PAS_STOPPED || pa->state == PAS_WAITING || pa->state == PAS_LEDGE || pa->state == PAS_WALKING || pa->state == PAS_RUNNING || pa->state == PAS_DUCKING || pa->state == PAS_LOOKINGUP)
                pa->state = WALKING_OR_RUNNING(pa);
            else if(pa->state == PAS_PUSHING)
                pa->state = PAS_WALKING;
        }
    }

    /*
     *
     * looking up & crouching down
     *
     */

    if(!pa->midair) {
        if(pa->state != PAS_PUSHING && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING) {
            if(nearly_zero(pa->gsp)) {
                if(input_button_down(pa->input, IB_DOWN))
                    pa->state = PAS_DUCKING;
                else if(input_button_down(pa->input, IB_UP))
                    pa->state = PAS_LOOKINGUP;
            }
        }
    }

    /*
     *
     * springing
     *
     */

    if(pa->state == PAS_SPRINGING) {
        if(pa->midair && pa->ysp > 0.0f)
            pa->state = PAS_WALKING;

        pa->want_to_detach_from_ground = pa->want_to_detach_from_ground || (
            (pa->movmode == MM_FLOOR && pa->ysp < 0.0f) ||
            (pa->movmode == MM_RIGHTWALL && pa->xsp < 0.0f) ||
            (pa->movmode == MM_CEILING && pa->ysp > 0.0f) ||
            (pa->movmode == MM_LEFTWALL && pa->xsp > 0.0f)
        );
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
    else if(pa->state == PAS_BREATHING && pa->midair) {
        pa->breathe_timer = 0.0f;
        pa->state = PAS_WALKING;
    }

    /*
     *
     * balancing on ledges
     *
     */

    if(
        !pa->midair && pa->movmode == MM_FLOOR &&
        !(pa->state == PAS_LEDGE || pa->state == PAS_PUSHING) &&
        ((at_A == NULL) ^ (at_B == NULL)) &&
        nearly_zero(pa->gsp)
    ) {
        const sensor_t* sensor = at_A ? sensor_A(pa) : sensor_B(pa);
        v2d_t position = v2d_new(floorf(pa->position.x), floorf(pa->position.y));
        point2d_t tail = sensor_tail(sensor, position, pa->movmode);

        int delta = (int)position.x - tail.x;
        int midpoint = (int)position.x + delta / 2;
        point2d_t sweet_spot = point2d_new(midpoint, tail.y + 8);

        if(NULL == obstaclemap_get_best_obstacle_at(obstaclemap, sweet_spot.x, sweet_spot.y, sweet_spot.x, sweet_spot.y, pa->movmode, pa->layer)) {
            pa->state = PAS_LEDGE;
            /*pa->facing_right = (at_B == NULL);*/ /* this may not be desirable */
        }
    }

    /*
     *
     * rolling
     *
     */

    /* start rolling */
    if(!pa->midair && (pa->state == PAS_WALKING || pa->state == PAS_RUNNING)) {
        if(fabs(pa->gsp) >= pa->rollthreshold && input_button_down(pa->input, IB_DOWN))
            pa->state = PAS_ROLLING;
    }

    /* roll */
    if(!pa->midair && pa->state == PAS_ROLLING) {

        /* slope factor */
        if(pa->gsp * SIN(pa->angle) >= 0.0f)
            pa->gsp += pa->rolluphillslp * -SIN(pa->angle) * dt;
        else
            pa->gsp += pa->rolldownhillslp * -SIN(pa->angle) * dt;

        /* deceleration */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0f)
            pa->gsp = min(0.0f, pa->gsp + pa->rolldec * dt);
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0f)
            pa->gsp = max(0.0f, pa->gsp - pa->rolldec * dt);

        /* friction */
        if(fabs(pa->gsp) > pa->rollfrc * dt)
            pa->gsp -= pa->rollfrc * sign(pa->gsp) * dt;
        else
            pa->gsp = 0.0f;

        /* unroll */
        if(fabs(pa->gsp) < pa->unrollthreshold)
            pa->state = PAS_STOPPED; /*PAS_WALKING;*/ /* anim transition: rolling -> stopped */

        /* facing right? */
        if(!nearly_zero(pa->gsp))
            pa->facing_right = (pa->gsp > 0.0f);
    }

    /*
     *
     * charge and release
     *
     */

    /* begin to charge */
    if(pa->state == PAS_DUCKING) {
        if(input_button_down(pa->input, IB_DOWN) && input_button_pressed(pa->input, IB_FIRE1)) {
            if(!nearly_zero(pa->chrg)) /* check if the player has the ability to charge */
                pa->state = PAS_CHARGING;
        }
    }

    /* charging... */
    if(pa->state == PAS_CHARGING) {
        /* charging more...! */
        if(input_button_pressed(pa->input, IB_FIRE1))
            pa->charge_intensity = min(1.0f, pa->charge_intensity + 0.25f);
        else if(fabs(pa->charge_intensity) >= pa->chrgthreshold)
            pa->charge_intensity *= 0.999506551f - 1.84539309f * dt; /* attenuate charge intensity */

        /* release */
        if(!input_button_down(pa->input, IB_DOWN)) {
            float s = pa->facing_right ? 1.0f : -1.0f;
            pa->gsp = (s * pa->chrg) * (0.67f + pa->charge_intensity * 0.33f);
            pa->state = PAS_ROLLING;
            pa->charge_intensity = 0.0f;
            pa->jump_lock_timer = 6.0f / TARGET_FPS;
        }
        else
            pa->gsp = 0.0f;
    }

    /*
     *
     * speed cap
     *
     */

    if(!pa->midair) {

        /* cap gsp; you're way too fast... */
        float gnd_capspeed = min(pa->capspeed, HARD_CAPSPEED);
        pa->gsp = clip(pa->gsp, -gnd_capspeed, gnd_capspeed);

        /* convert gsp to xsp and ysp */
        if(!pa->want_to_detach_from_ground) { /* if not springing, etc. */
            pa->xsp = pa->gsp * COS(pa->angle);
            pa->ysp = pa->gsp * -SIN(pa->angle);
        }
        else {
            /* xsp and/or ysp may have been changed externally */
            ;
        }

    }
    else {

        const float air_capspeed = HARD_CAPSPEED;

        /* cap xsp & ysp */
        /* Note: alternatively, this could be such that xsp^2 + ysp^2 <= air_capspeed^2 */
        pa->xsp = clip(pa->xsp, -air_capspeed, air_capspeed);
        pa->ysp = clip(pa->ysp, -air_capspeed, air_capspeed);

    }

    /*
     *
     * falling off
     *
     */

    if(pa->midair) {

        /* air acceleration */
        if(input_button_down(pa->input, IB_RIGHT)) {
            if(pa->xsp < pa->topspeed) {
                pa->xsp += pa->air * dt;
                if(pa->xsp > pa->topspeed)
                    pa->xsp = pa->topspeed;
            }
        }
        else if(input_button_down(pa->input, IB_LEFT)) {
            if(pa->xsp > -pa->topspeed) {
                pa->xsp -= pa->air * dt;
                if(pa->xsp < -pa->topspeed)
                    pa->xsp = -pa->topspeed;
            }
        }

        /* air drag */
        if(pa->ysp < 0.0f && pa->ysp > pa->airdragthreshold && pa->state != PAS_GETTINGHIT) {
            if(fabs(pa->xsp) >= pa->airdragxthreshold) {
                /*pa->xsp *= powf(pa->airdrag, 60.0f * dt);*/
                pa->xsp *= pa->airdrag_coefficient[0] * dt + pa->airdrag_coefficient[1];
            }
        }

        /* gravity */
        if(pa->ysp < pa->topyspeed) {
            float grv = (pa->state != PAS_GETTINGHIT) ? pa->grv : (pa->grv / 7.0f) * 6.0f;
            pa->ysp += grv * dt;
            if(pa->ysp > pa->topyspeed)
                pa->ysp = pa->topyspeed;
        }

    }

    /*
     *
     * jumping
     *
     */

    if(!pa->midair) {

        pa->jump_lock_timer -= dt;
        if(pa->jump_lock_timer <= 0.0f) {
            pa->jump_lock_timer = 0.0f;

            /* jump */
            if(
                input_button_pressed(pa->input, IB_FIRE1) && (
                    (!input_button_down(pa->input, IB_UP) && !input_button_down(pa->input, IB_DOWN)) ||
                    pa->state == PAS_ROLLING
                ) &&
                !pa->touching_ceiling /* don't bother jumping if near a ceiling */
            ) {
#if WANT_JUMP_ATTENUATION
                /* reduce the jump height when moving uphill */
                float grv_attenuation = (pa->gsp * SIN(pa->angle) < 0.0f) ? 1.0f : 0.5f;
                pa->xsp = pa->jmp * SIN(pa->angle) + pa->gsp * COS(pa->angle);
                pa->ysp = pa->jmp * COS(pa->angle) - pa->gsp * SIN(pa->angle) * grv_attenuation;
#else
                pa->xsp = pa->jmp * SIN(pa->angle) + pa->gsp * COS(pa->angle);
                pa->ysp = pa->jmp * COS(pa->angle) - pa->gsp * SIN(pa->angle);
#endif
                pa->state = PAS_JUMPING;
                pa->want_to_detach_from_ground = true;
                FORCE_ANGLE(0x0);
            }
        }
    }
    else {

        /* jump sensitivity */
        if(pa->state == PAS_JUMPING) {
            if(!input_button_down(pa->input, IB_FIRE1) && pa->ysp < pa->jmprel)
                pa->ysp = pa->jmprel;
        }

    }

    /*
     *
     * moving the player
     *
     */

    /* update the position */
    pa->position.x += pa->xsp * dt;
    pa->position.y += pa->ysp * dt;
    UPDATE_SENSORS();

    /* stop if we land after getting hit */
    if(pa->state == PAS_GETTINGHIT) {
        if(!pa->midair && pa->was_midair) {
            pa->gsp = pa->xsp = 0.0f;
            pa->state = PAS_STOPPED;
        }
    }

    /*
     *
     * wall collisions
     *
     */

    /* right wall */
    if(at_N != NULL) {
        const sensor_t* sensor = sensor_N(pa);
        v2d_t position = v2d_new(floorf(pa->position.x), floorf(pa->position.y));
        point2d_t tail = sensor_tail(sensor, position, pa->movmode);
        point2d_t local_tail = point2d_subtract(tail, point2d_from_v2d(position));
        bool reset_angle = false;
        int wall;

        /* reset gsp */
        if(pa->gsp > 0.0f)
            pa->gsp = 0.0f;

        /* reposition the player */
        switch(pa->movmode) {
            case MM_FLOOR:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_RIGHT);
                pa->position.x = wall - local_tail.x - 1;
                pa->xsp = min(pa->xsp, 0.0f);
                reset_angle = false;
                break;

            case MM_CEILING:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_LEFT);
                pa->position.x = wall - local_tail.x + 1;
                pa->xsp = max(pa->xsp, 0.0f);
                reset_angle = true;
                break;

            case MM_RIGHTWALL:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_UP);
                pa->position.y = wall - local_tail.y - 1;
                pa->ysp = max(pa->ysp, 0.0f);
                reset_angle = true;
                break;

            case MM_LEFTWALL:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_DOWN);
                pa->position.y = wall - local_tail.y + 1;
                pa->ysp = min(pa->ysp, 0.0f);
                reset_angle = true;
                break;
        }

        /* update sensors */
        if(!reset_angle) {
            UPDATE_SENSORS();
        }
        else {
            FORCE_ANGLE(0x0);
        }

        /* pushing a wall */
        if(!pa->midair && pa->movmode == MM_FLOOR && pa->state != PAS_ROLLING) {
            if(input_button_down(pa->input, IB_RIGHT)) {
                pa->state = PAS_PUSHING;
                pa->facing_right = true;
            }
            else
                pa->state = PAS_STOPPED;
        }
    }

    /* left wall */
    if(at_M != NULL) {
        const sensor_t* sensor = sensor_M(pa);
        v2d_t position = v2d_new(floorf(pa->position.x), floorf(pa->position.y));
        point2d_t tail = sensor_tail(sensor, position, pa->movmode);
        point2d_t local_tail = point2d_subtract(tail, point2d_from_v2d(position));
        bool reset_angle = false;
        int wall;

        /* reset gsp */
        if(pa->gsp < 0.0f)
            pa->gsp = 0.0f;

        /* reposition the player */
        switch(pa->movmode) {
            case MM_FLOOR:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_LEFT);
                pa->position.x = wall - local_tail.x + 1;
                pa->xsp = max(pa->xsp, 0.0f);
                reset_angle = false;
                break;

            case MM_CEILING:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_RIGHT);
                pa->position.x = wall - local_tail.x - 1;
                pa->xsp = min(pa->xsp, 0.0f);
                reset_angle = true;
                break;

            case MM_RIGHTWALL:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_DOWN);
                pa->position.y = wall - local_tail.y - 1;
                pa->ysp = min(pa->ysp, 0.0f);
                reset_angle = true;
                break;

            case MM_LEFTWALL:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_UP);
                pa->position.y = wall - local_tail.y + 1;
                pa->ysp = max(pa->ysp, 0.0f);
                reset_angle = true;
                break;
        }

        /* update sensors */
        if(!reset_angle) {
            UPDATE_SENSORS();
        }
        else {
            FORCE_ANGLE(0x0);
        }

        /* pushing a wall */
        if(!pa->midair && pa->movmode == MM_FLOOR && pa->state != PAS_ROLLING) {
            if(input_button_down(pa->input, IB_LEFT)) {
                pa->state = PAS_PUSHING;
                pa->facing_right = false;
            }
            else
                pa->state = PAS_STOPPED;
        }
    }

    /*
     *
     * getting smashed
     *
     */

    if(!pa->smashed)
        pa->smashed = is_smashed(pa, obstaclemap);

    /*
     *
     * ceiling collision
     *
     */

    if(pa->midair && pa->touching_ceiling) {
        bool must_reattach = false;

        /* get the ceiling sensors */
        const sensor_t* c = sensor_C(pa);
        const sensor_t* d = sensor_D(pa);

        /* picking the ceiling */
        char best_ceiling = pick_the_best_ceiling(pa, at_C, at_D, c, d);
        const obstacle_t* ceiling = (best_ceiling == 'c') ? at_C : at_D;
        const sensor_t* c_or_d = (best_ceiling == 'c') ? c : d;

        /* are we touching the ceiling for the first time? */
        if(pa->ysp < 0.0f) {
            /* compute the angle */
            FORCE_ANGLE(0x80);
            pa->midair = false; /* enable the ground sensors */
            SET_AUTO_ANGLE();

            /* reattach to the ceiling? */
            if((pa->angle >= 0xA0 && pa->angle <= 0xBF) || (pa->angle >= 0x40 && pa->angle <= 0x5F))
                must_reattach = !pa->midair;
        }

        /* reattach to the ceiling or bump the head */
        if(must_reattach) {
            /* adjust speeds */
            pa->gsp = (fabs(pa->xsp) > -pa->ysp) ? -pa->xsp : pa->ysp * -sign(SIN(pa->angle));
            pa->xsp = pa->ysp = 0.0f;

            /* adjust the state */
            if(pa->state != PAS_ROLLING)
                pa->state = WALKING_OR_RUNNING(pa);

            /* make sure we stick to the ground */
            pa->want_to_detach_from_ground = false;
        }
        else {
            /* adjust speed & angle */
            pa->ysp = max(pa->ysp, 0.0f);
            FORCE_ANGLE(0x0);

            /* find the position of the sensor after setting the angle to 0 */
            v2d_t position = v2d_new(floorf(pa->position.x), floorf(pa->position.y));
            point2d_t tail = sensor_tail(c_or_d, position, pa->movmode);
            point2d_t local_tail = point2d_subtract(tail, point2d_from_v2d(position));

            /* reposition the player */
            int ceiling_position = obstacle_ground_position(ceiling, tail.x, tail.y, GD_UP);
            pa->position.y = ceiling_position - local_tail.y + 1;

            /* update the sensors */
            pa->midair = true; /* enable the ceiling sensors */
            UPDATE_SENSORS();
        }
    }

    /*
     *
     * sticky physics
     *
     */

    /* skip this section if we intend to leave the ground */
    if(!pa->want_to_detach_from_ground) {
        int cnt = 0;
        movmode_t prev_movmode = pa->movmode;

        sticky_loop:

        /* if the player is on the ground or has just left the ground, stick to it! */
        if(!pa->midair || !pa->was_midair || cnt > 0) {
            int gnd_pos_a, gnd_pos_b, gnd_pos;
            const obstacle_t *gnd_a, *gnd_b;

            /* get the ground sensors */
            const sensor_t* a = sensor_A(pa);
            const sensor_t* b = sensor_B(pa);

            /* find the nearest ground that already collide with the sensors */
            gnd_a = at_A;
            gnd_b = at_B;
            if(gnd_a || gnd_b) {
                char best = pick_the_best_floor(pa, gnd_a, gnd_b, a, b);
                const obstacle_t* gnd = (best == 'a') ? gnd_a : gnd_b;
                const sensor_t* a_or_b = (best == 'a') ? a : b;
                point2d_t tail = sensor_tail(a_or_b, pa->position, pa->movmode);
                gnd_pos = obstacle_ground_position(gnd, tail.x, tail.y, MM_TO_GD(pa->movmode));
            }
            else {

                /* compute an extended length measured from the tail of the sensors */
                float abs_xsp = fabsf(pa->xsp), abs_ysp = fabsf(pa->ysp);
                float max_abs_speed = max(abs_xsp, abs_ysp); /* <= fabsf(pa->gsp) */
                int max_abs_ds = (int)ceilf(max_abs_speed * dt);
                const int min_length = 14, max_length = 32;
                const int tail_depth = AB_SENSOR_OFFSET + 1; /* the extension starts from the tail (inclusive), and the tail touches the ground */
                int extended_length = clip(max_abs_ds + 4, min_length, max_length) + (tail_depth - 1);

                /* find the nearest ground using both sensors */
                gnd_a = find_ground_with_extended_sensor(pa, obstaclemap, a, extended_length, &gnd_pos_a);
                gnd_b = find_ground_with_extended_sensor(pa, obstaclemap, b, extended_length, &gnd_pos_b);

                if(gnd_a && gnd_b) {
                    switch(pa->movmode) {
                        case MM_FLOOR:      gnd_pos = min(gnd_pos_a, gnd_pos_b); break;
                        case MM_RIGHTWALL:  gnd_pos = min(gnd_pos_a, gnd_pos_b); break;
                        case MM_CEILING:    gnd_pos = max(gnd_pos_a, gnd_pos_b); break;
                        case MM_LEFTWALL:   gnd_pos = max(gnd_pos_a, gnd_pos_b); break;
                    }
                }
                else if(gnd_a)
                    gnd_pos = gnd_pos_a;
                else if(gnd_b)
                    gnd_pos = gnd_pos_b;

            }

            /* reposition the player */
            if(gnd_a || gnd_b) {
                const sensor_t* a_or_b = (gnd_pos == gnd_pos_a) ? a : b;
                v2d_t position = v2d_new(floorf(pa->position.x), floorf(pa->position.y));
                point2d_t tail = sensor_tail(a_or_b, position, pa->movmode);

                /* put the tail of the sensor on the ground */
                const int offset = AB_SENSOR_OFFSET;
                switch(pa->movmode) {
                    case MM_FLOOR:
                        pa->position.y = (int)position.y + (gnd_pos - tail.y) + offset;
                        break;

                    case MM_CEILING:
                        pa->position.y = (int)position.y + (gnd_pos - tail.y) - offset;
                        break;

                    case MM_RIGHTWALL:
                        pa->position.x = (int)position.x + (gnd_pos - tail.x) + offset;
                        break;

                    case MM_LEFTWALL:
                        pa->position.x = (int)position.x + (gnd_pos - tail.x) - offset;
                        break;
                }

                /* update the sensors */
                pa->midair = false; /* get the correct sensors on this call to UPDATE_SENSORS() */
                UPDATE_SENSORS();
            }
            else {
                /* the distance is too great; we won't stick to the ground */
                ;
            }

        }

        /* if the player is still on the ground, update the angle */
        if(!pa->midair)
            SET_AUTO_ANGLE();

        /* repeat once if convenient; maybe we've changed the movmode */
        if(pa->movmode != prev_movmode && 0 == cnt++)
            goto sticky_loop;
    }

    /* reset flag */
    pa->want_to_detach_from_ground = false;

    /*
     *
     * reacquisition of the ground
     *
     */

    if(!pa->midair && pa->was_midair) {
        if(pa->angle >= 0xF0 || pa->angle <= 0x0F)
            pa->gsp = pa->xsp;
        else if((pa->angle >= 0xE0 && pa->angle <= 0xEF) || (pa->angle >= 0x10 && pa->angle <= 0x1F))
            pa->gsp = fabs(pa->xsp) > pa->ysp ? pa->xsp : pa->ysp * 0.5f * -sign(SIN(pa->angle));
        else if((pa->angle >= 0xC0 && pa->angle <= 0xDF) || (pa->angle >= 0x20 && pa->angle <= 0x3F))
            pa->gsp = fabs(pa->xsp) > pa->ysp ? pa->xsp : pa->ysp * -sign(SIN(pa->angle));

        /* reset speeds */
        pa->xsp = pa->ysp = 0.0f;

        /* animation fix */
        if(pa->state == PAS_ROLLING) {
            /* unroll when landing on the ground */
            if(pa->midair_timer >= 0.2f && !input_button_down(pa->input, IB_DOWN)) {
                pa->state = WALKING_OR_RUNNING(pa);
                if(!nearly_zero(pa->gsp))
                    pa->facing_right = (pa->gsp > 0.0f);
            }
        }
        else {
            /* walk / run */
            pa->state = WALKING_OR_RUNNING(pa);
        }
    }

    /*
     *
     * falling off walls and ceilings
     *
     */

    if(!pa->midair) {
        if(pa->movmode != MM_FLOOR && pa->hlock_timer == 0.0f) {
            if(fabs(pa->gsp) < pa->falloffthreshold) {
                pa->hlock_timer = 0.5f;
                if(pa->angle >= 0x40 && pa->angle <= 0xC0) {
                    pa->gsp = 0.0f;
                    FORCE_ANGLE(0x0);
                }
            }
        }
    }

    /*
     *
     * misc
     *
     */

    /* reset the angle & update the midair_timer */
    if(pa->midair) {
        pa->midair_timer += dt;
        FORCE_ANGLE(0x0);
    }
    else
        pa->midair_timer = 0.0f;

    /* remain on the winning state */
    if(pa->winning_pose && !pa->midair) {
        if(fabs(pa->gsp) < pa->walkthreshold)
            pa->state = PAS_WINNING;
    }

    /* fix invalid states */
    if(pa->midair) {
        if(pa->state == PAS_PUSHING || pa->state == PAS_STOPPED || pa->state == PAS_WAITING || pa->state == PAS_DUCKING || pa->state == PAS_LOOKINGUP)
            pa->state = WALKING_OR_RUNNING(pa);
    }
    else {
        if(pa->state == PAS_WALKING && nearly_zero(pa->gsp))
            pa->state = PAS_STOPPED;
    }
}

/* call update_sensors() whenever you update pa->position or pa->angle */
void update_sensors(physicsactor_t* pa, const obstaclemap_t* obstaclemap, obstacle_t const** const at_A, obstacle_t const** const at_B, obstacle_t const** const at_C, obstacle_t const** const at_D, obstacle_t const** const at_M, obstacle_t const** const at_N)
{
    /* get sensors */
    sensor_t* a = sensor_A(pa);
    sensor_t* b = sensor_B(pa);
    sensor_t* c = sensor_C(pa);
    sensor_t* d = sensor_D(pa);
    sensor_t* m = sensor_M(pa);
    sensor_t* n = sensor_N(pa);

    /* disable sensors for efficiency */
    if(!pa->midair) {
        bool wanna_jump = input_button_pressed(pa->input, IB_FIRE1) && (pa->state != PAS_CHARGING); /* maybe; may be doing some other special move... */
        bool wanna_middle = (pa->angle <= 0x40 || pa->angle >= 0xC0 || pa->angle == 0x80);
        sensor_set_enabled(a, true);
        sensor_set_enabled(b, true);
        sensor_set_enabled(c, wanna_jump);
        sensor_set_enabled(d, wanna_jump);
        sensor_set_enabled(m, wanna_middle && (pa->gsp <= pa->walkthreshold)); /* gsp <= 0.0f */ /* regular movement & moving platforms */
        sensor_set_enabled(n, wanna_middle && (pa->gsp >= -pa->walkthreshold)); /* gsp >= 0.0f */
    }
    else {
        float abs_xsp = fabsf(pa->xsp), abs_ysp = fabsf(pa->ysp);
        sensor_set_enabled(a, pa->ysp >= -abs_xsp); /* ysp >= 0.0f */
        sensor_set_enabled(b, pa->ysp >= -abs_xsp); /* ysp >= 0.0f */
        sensor_set_enabled(c, pa->ysp < abs_xsp);   /* ysp < 0.0f */
        sensor_set_enabled(d, pa->ysp < abs_xsp);   /* ysp < 0.0f */
        sensor_set_enabled(m, pa->xsp < abs_ysp);   /* xsp < 0.0f */
        sensor_set_enabled(n, pa->xsp > -abs_ysp);  /* xsp > 0.0f */
    }

    /* read sensors */
    *at_A = sensor_check(a, pa->position, pa->movmode, pa->layer, obstaclemap);
    *at_B = sensor_check(b, pa->position, pa->movmode, pa->layer, obstaclemap);
    *at_C = sensor_check(c, pa->position, pa->movmode, pa->layer, obstaclemap);
    *at_D = sensor_check(d, pa->position, pa->movmode, pa->layer, obstaclemap);
    *at_M = sensor_check(m, pa->position, pa->movmode, pa->layer, obstaclemap);
    *at_N = sensor_check(n, pa->position, pa->movmode, pa->layer, obstaclemap);

    /* C, D, M, N: ignore clouds */
    *at_C = (*at_C != NULL && obstacle_is_solid(*at_C)) ? *at_C : NULL;
    *at_D = (*at_D != NULL && obstacle_is_solid(*at_D)) ? *at_D : NULL;
    *at_M = (*at_M != NULL && obstacle_is_solid(*at_M)) ? *at_M : NULL;
    *at_N = (*at_N != NULL && obstacle_is_solid(*at_N)) ? *at_N : NULL;

    /* A, B: ignore clouds when moving upwards */
    if(pa->ysp < 0.0f) {
        if(-pa->ysp > fabsf(pa->xsp)) {
            *at_A = (*at_A != NULL && obstacle_is_solid(*at_A)) ? *at_A : NULL;
            *at_B = (*at_B != NULL && obstacle_is_solid(*at_B)) ? *at_B : NULL;
        }
    }

    /* A: ignore if it's a cloud and if the tail of the sensor is not touching it */
    if(*at_A != NULL && !obstacle_is_solid(*at_A)) {
        point2d_t tail = sensor_tail(a, pa->position, pa->movmode);
        if(!obstacle_point_collision(*at_A, tail))
            *at_A = NULL;
        else if(pa->midair && pa->movmode == MM_FLOOR && pa->angle == 0x0) {
            int ygnd = obstacle_ground_position(*at_A, tail.x, tail.y, GD_DOWN);
            *at_A = (tail.y < ygnd + CLOUD_OFFSET || pa->ysp > 0.0f) ? *at_A : NULL;
        }
    }

    /* B: ignore if it's a cloud and if the tail of the sensor is not touching it */
    if(*at_B != NULL && !obstacle_is_solid(*at_B)) {
        point2d_t tail = sensor_tail(b, pa->position, pa->movmode);
        if(!obstacle_point_collision(*at_B, tail))
            *at_B = NULL;
        else if(pa->midair && pa->movmode == MM_FLOOR && pa->angle == 0x0) {
            int ygnd = obstacle_ground_position(*at_B, tail.x, tail.y, GD_DOWN);
            *at_B = (tail.y < ygnd + CLOUD_OFFSET || pa->ysp > 0.0f) ? *at_B : NULL;
        }
    }

    /* A, B: special logic when both are clouds and A != B */
    if(*at_A != NULL && *at_B != NULL && *at_A != *at_B && !obstacle_is_solid(*at_A) && !obstacle_is_solid(*at_B)) {
        if(pa->movmode == MM_FLOOR) {
            point2d_t tail_a = sensor_tail(a, pa->position, pa->movmode);
            point2d_t tail_b = sensor_tail(b, pa->position, pa->movmode);
            int gnd_a = obstacle_ground_position(*at_A, tail_a.x, tail_a.y, GD_DOWN);
            int gnd_b = obstacle_ground_position(*at_B, tail_b.x, tail_b.y, GD_DOWN);
            if(abs(gnd_a - gnd_b) > 8) {
                if(gnd_a < gnd_b)
                    *at_A = NULL;
                else
                    *at_B = NULL;
            }
        }
    }

    /* set flags */
    pa->midair = (*at_A == NULL) && (*at_B == NULL);
    pa->touching_ceiling = (*at_C != NULL) || (*at_D != NULL);
}

/* call update_movmode() whenever you update pa->angle */
void update_movmode(physicsactor_t* pa)
{
    /* angles 0x20, 0x60, 0xA0, 0xE0
       do not change the movmode */
    if(pa->angle < 0x20 || pa->angle > 0xE0) {
        if(pa->movmode == MM_CEILING)
            pa->gsp = -pa->gsp;
        pa->movmode = MM_FLOOR;
    }
    else if(pa->angle > 0x20 && pa->angle < 0x60)
        pa->movmode = MM_LEFTWALL;
    else if(pa->angle > 0x60 && pa->angle < 0xA0)
        pa->movmode = MM_CEILING;
    else if(pa->angle > 0xA0 && pa->angle < 0xE0)
        pa->movmode = MM_RIGHTWALL;
}

/* which one is the best floor, a or b? we evaluate the sensors also */
char pick_the_best_floor(const physicsactor_t *pa, const obstacle_t *a, const obstacle_t *b, const sensor_t *a_sensor, const sensor_t *b_sensor)
{
    point2d_t sa, sb;
    int ha, hb;

    if(a == NULL)
        return 'b';
    else if(b == NULL)
        return 'a';

    sa = sensor_head(a_sensor, pa->position, pa->movmode);
    sb = sensor_head(b_sensor, pa->position, pa->movmode);

    switch(pa->movmode) {
        case MM_FLOOR:
            ha = obstacle_ground_position(a, sa.x, sa.y, GD_DOWN);
            hb = obstacle_ground_position(b, sb.x, sb.y, GD_DOWN);
            return ha <= hb ? 'a' : 'b';

        case MM_LEFTWALL:
            ha = obstacle_ground_position(a, sa.x, sa.y, GD_LEFT);
            hb = obstacle_ground_position(b, sb.x, sb.y, GD_LEFT);
            return ha >= hb ? 'a' : 'b';

        case MM_CEILING:
            ha = obstacle_ground_position(a, sa.x, sa.y, GD_UP);
            hb = obstacle_ground_position(b, sb.x, sb.y, GD_UP);
            return ha >= hb ? 'a' : 'b';

        case MM_RIGHTWALL:
            ha = obstacle_ground_position(a, sa.x, sa.y, GD_RIGHT);
            hb = obstacle_ground_position(b, sb.x, sb.y, GD_RIGHT);
            return ha <= hb ? 'a' : 'b';
    }

    return 'a';
}

/* which one is the best ceiling, c or d? we evaluate the sensors also */
char pick_the_best_ceiling(const physicsactor_t *pa, const obstacle_t *c, const obstacle_t *d, const sensor_t *c_sensor, const sensor_t *d_sensor)
{
    point2d_t sc, sd;
    int hc, hd;

    if(c == NULL)
        return 'd';
    else if(d == NULL)
        return 'c';

    sc = sensor_tail(c_sensor, pa->position, pa->movmode);
    sd = sensor_tail(d_sensor, pa->position, pa->movmode);

    switch(pa->movmode) {
        case MM_FLOOR:
            hc = obstacle_ground_position(c, sc.x, sc.y, GD_UP);
            hd = obstacle_ground_position(d, sd.x, sd.y, GD_UP);
            return hc >= hd ? 'c' : 'd';

        case MM_LEFTWALL:
            hc = obstacle_ground_position(c, sc.x, sc.y, GD_RIGHT);
            hd = obstacle_ground_position(d, sd.x, sd.y, GD_RIGHT);
            return hc <= hd ? 'c' : 'd';

        case MM_CEILING:
            hc = obstacle_ground_position(c, sc.x, sc.y, GD_DOWN);
            hd = obstacle_ground_position(d, sd.x, sd.y, GD_DOWN);
            return hc <= hd ? 'c' : 'd';

        case MM_RIGHTWALL:
            hc = obstacle_ground_position(c, sc.x, sc.y, GD_LEFT);
            hd = obstacle_ground_position(d, sd.x, sd.y, GD_LEFT);
            return hc >= hd ? 'c' : 'd';
    }

    return 'c';
}

/* extend a sensor and find the ground; returns NULL if no ground is found within the range of the extended sensor */
const obstacle_t* find_ground_with_extended_sensor(const physicsactor_t* pa, const obstaclemap_t* obstaclemap, const sensor_t* sensor, int extended_sensor_length, int* out_ground_position)
{
    point2d_t head, tail;
    int x1, y1, x2, y2;

    sensor_extend(sensor, pa->position, pa->movmode, extended_sensor_length, &head, &tail);
    x1 = min(head.x, tail.x);
    y1 = min(head.y, tail.y);
    x2 = max(head.x, tail.x);
    y2 = max(head.y, tail.y);

    return obstaclemap_find_ground(obstaclemap, x1, y1, x2, y2, pa->layer, MM_TO_GD(pa->movmode), out_ground_position);
}

/* check if the physics actor is smashed */
bool is_smashed(const physicsactor_t* pa, const obstaclemap_t* obstaclemap)
{
    /* first, we check if sensor U is overlapping a solid obstacle */
    const obstacle_t* at_U = sensor_check(sensor_U(pa), pa->position, pa->movmode, pa->layer, obstaclemap);
    if(!(at_U != NULL && obstacle_is_solid(at_U))) /* sensor U is assumed to be enabled */
        return false; /* quit if no collision */

    /* next, we check other sensors to make sure */
    sensor_t* a = sensor_A(pa);
    sensor_t* b = sensor_B(pa);
    sensor_t* c = sensor_C(pa);
    sensor_t* d = sensor_D(pa);

    bool a_enabled = sensor_is_enabled(a);
    bool b_enabled = sensor_is_enabled(b);
    bool c_enabled = sensor_is_enabled(c);
    bool d_enabled = sensor_is_enabled(d);

    sensor_set_enabled(a, true);
    sensor_set_enabled(b, true);
    sensor_set_enabled(c, true);
    sensor_set_enabled(d, true);

    const obstacle_t* at_A = sensor_check(a, pa->position, pa->movmode, pa->layer, obstaclemap);
    const obstacle_t* at_B = sensor_check(b, pa->position, pa->movmode, pa->layer, obstaclemap);
    const obstacle_t* at_C = sensor_check(c, pa->position, pa->movmode, pa->layer, obstaclemap);
    const obstacle_t* at_D = sensor_check(d, pa->position, pa->movmode, pa->layer, obstaclemap);

    bool smashed =  ((
        (at_A != NULL && obstacle_is_solid(at_A)) &&
        (at_B != NULL && obstacle_is_solid(at_B))
    ) && (
        (at_C != NULL && obstacle_is_solid(at_C)) && /* '||' is too sensitive */
        (at_D != NULL && obstacle_is_solid(at_D))
    ));

    sensor_set_enabled(d, d_enabled);
    sensor_set_enabled(c, c_enabled);
    sensor_set_enabled(b, b_enabled);
    sensor_set_enabled(a, a_enabled);

    return smashed;
}

/* renders an angle sensor */
void render_ball(v2d_t sensor_position, int radius, color_t color, v2d_t camera_position)
{
    v2d_t half_screen_size = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, half_screen_size);
    v2d_t position = v2d_subtract(sensor_position, topleft);
    color_t border_color = color_rgb(0, 0, 0);

    image_ellipse(position.x, position.y, radius + 1, radius + 1, border_color);
    image_ellipse(position.x, position.y, radius, radius, color);
}

/* distance between the angle sensors */
int distance_between_angle_sensors(const physicsactor_t* pa)
{
    const float DEFAULT_CAPSPEED = 16.0f * TARGET_FPS;

    if(fabsf(pa->gsp) <= DEFAULT_CAPSPEED)
        return 13;
    else
        return 11; /* very high speeds */
}

/* the min angle between alpha and beta */
int delta_angle(int alpha, int beta)
{
    alpha &= 0xFF; beta &= 0xFF;
    int diff = alpha > beta ? alpha - beta : beta - alpha;
    return diff > 0x80 ? 0xFF - diff + 1 : diff;
}

/* linear interpolation between angles; t in [0,1] */
int interpolate_angle(int alpha, int beta, float t)
{
    int mul = roundf(t * 256.0f);
    int delta = (delta_angle(alpha, beta) * mul) / 256;
    return (alpha + delta) & 0xFF;
}

/* angle extrapolation; t in [0,1] */
int extrapolate_angle(int curr_angle, int prev_angle, float t)
{
    int mul = roundf(256.0f * t);
    int delta = (delta_angle(curr_angle, prev_angle) * mul) / 256;
    int theta = (curr_angle < prev_angle) ? 0xFF - delta + 1 : delta;
    return (curr_angle + theta) & 0xFF;
}