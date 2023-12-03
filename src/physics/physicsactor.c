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
#include "../core/engine.h"
#include "../core/global.h"
#include "../util/numeric.h"
#include "../util/util.h"

typedef struct physicsactorobserverlist_t physicsactorobserverlist_t;

/* physicsactor class */
struct physicsactor_t
{
    physicsactorstate_t state; /* state */

    double xpos; /* x position in world space */
    double ypos; /* y position in world space */

    double xsp; /* x speed */
    double ysp; /* y speed */
    double gsp; /* ground speed */

    movmode_t movmode; /* current movement mode, based on the angle */
    int angle; /* angle (0-255 clockwise) */
    int prev_angle; /* previous angle */

    bool facing_right; /* is the player facing right? */
    bool midair; /* is the player midair? */
    bool was_midair; /* was the player midair in the previous frame of the simulation? */
    bool touching_ceiling; /* is the player touching a ceiling? */

    double acc; /* acceleration */
    double dec; /* deceleration */
    double frc; /* friction */
    double capspeed; /* cap speed */
    double topspeed; /* top speed */
    double topyspeed; /* top y speed */
    double air; /* air acceleration */
    double airdrag; /* air drag (friction) */
    double jmp; /* initial jump velocity */
    double jmprel; /* release jump velocity */
    double diejmp; /* death jump velocity */
    double hitjmp; /* get hit jump velocity */
    double grv; /* gravity */
    double slp; /* slope factor */
    double chrg; /* charge-and-release max speed */
    double rollfrc; /* roll friction */
    double rolldec; /* roll deceleration */
    double rolluphillslp; /* roll uphill slope factor */
    double rolldownhillslp; /* roll downhill slope factor */
    double rollthreshold; /* roll threshold */
    double unrollthreshold; /* unroll threshold */
    double movethreshold; /* minimal movement threshold */
    double falloffthreshold; /* fall off threshold */
    double brakingthreshold; /* braking animation treshold */
    double airdragthreshold; /* air drag threshold */
    double airdragxthreshold; /* air drag x-threshold */
    double chrgthreshold; /* charge intensity threshold */
    double waittime; /* wait time in seconds */

    double charge_intensity; /* charge intensity */
    double airdrag_coefficient[2]; /* airdrag approx. coefficient */

    double hlock_timer; /* horizontal control lock timer, in seconds */
    double jump_lock_timer; /* jump lock timer, in seconds */
    double wait_timer; /* how long has the physics actor been stopped, in seconds */
    double midair_timer; /* how long has the physics actor been midair, in second */
    double breathe_timer; /* if greater than zero, set animation to breathing */

    bool winning_pose; /* winning pose enabled? */
    bool want_to_detach_from_ground; /* sticky physics helper */
    int unstable_angle_counter; /* a helper */

    obstaclelayer_t layer; /* current layer */
    input_t* input; /* input device */
    physicsactorobserverlist_t* observers; /* observers */

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

    double reference_time; /* used in fixed_update */
    double fixed_time;
    bool delayed_jump;

    int compatibility_version; /* used for compatibility with previous versions */
};

/* observer pattern */
struct physicsactorobserverlist_t
{
    void (*callback)(physicsactor_t*,physicsactorevent_t,void*);
    void* context;
    physicsactorobserverlist_t* next;
};

static physicsactorobserverlist_t* create_observer(void (*callback)(physicsactor_t*,physicsactorevent_t,void*), void* context, physicsactorobserverlist_t* next);
static physicsactorobserverlist_t* destroy_observers(physicsactorobserverlist_t* list);
static void notify_observers(physicsactor_t* pa, physicsactorevent_t event);

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
#define WANT_DEBUG              0
#define WANT_JUMP_ATTENUATION   0
#define AB_SENSOR_OFFSET        1 /* test with 0 and 1; with 0 it misbehaves a bit (unstable pa->midair) */
#define CLOUD_OFFSET            16
#define TARGET_FPS              60.0 /* target framerate of the simulation */
#define HARD_CAPSPEED           (24.0 * TARGET_FPS)
static void fixed_update(physicsactor_t *pa, const obstaclemap_t *obstaclemap, double dt);
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
 *
 * In this subsystem, the angle range is [0,255] (increases clockwise)
 * Conversion formula:
 *
 *     degrees = ((256 - angle) * 1.40625) % 360
 *     angle = (256 - degrees / 1.40625) % 256
 *
 * ps: 180/128 = 1.40625
 */

/* use these macros to get the values */
#define SIN(a) cos_table[((a) + 0x40) & 0xFF]
#define COS(a) cos_table[(a) & 0xFF]
static const double cos_table[256] = {
     1.000000000000000,  0.999698818696204,  0.998795456205172,  0.997290456678690,
     0.995184726672197,  0.992479534598710,  0.989176509964781,  0.985277642388941,
     0.980785280403230,  0.975702130038529,  0.970031253194544,  0.963776065795440,
     0.956940335732209,  0.949528180593037,  0.941544065183021,  0.932992798834739,
     0.923879532511287,  0.914209755703531,  0.903989293123443,  0.893224301195515,
     0.881921264348355,  0.870086991108711,  0.857728610000272,  0.844853565249707,
     0.831469612302545,  0.817584813151584,  0.803207531480645,  0.788346427626606,
     0.773010453362737,  0.757208846506485,  0.740951125354959,  0.724247082951467,
     0.707106781186548,  0.689540544737067,  0.671558954847018,  0.653172842953777,
     0.634393284163645,  0.615231590580627,  0.595699304492433,  0.575808191417845,
     0.555570233019602,  0.534997619887097,  0.514102744193222,  0.492898192229784,
     0.471396736825998,  0.449611329654607,  0.427555093430282,  0.405241314004990,
     0.382683432365090,  0.359895036534988,  0.336889853392220,  0.313681740398892,
     0.290284677254462,  0.266712757474898,  0.242980179903264,  0.219101240156870,
     0.195090322016128,  0.170961888760301,  0.146730474455362,  0.122410675199216,
     0.098017140329561,  0.073564563599667,  0.049067674327418,  0.024541228522912,
     0.000000000000000, -0.024541228522912, -0.049067674327418, -0.073564563599667,
    -0.098017140329561, -0.122410675199216, -0.146730474455362, -0.170961888760301,
    -0.195090322016128, -0.219101240156870, -0.242980179903264, -0.266712757474898,
    -0.290284677254462, -0.313681740398891, -0.336889853392220, -0.359895036534988,
    -0.382683432365090, -0.405241314004990, -0.427555093430282, -0.449611329654607,
    -0.471396736825998, -0.492898192229784, -0.514102744193222, -0.534997619887097,
    -0.555570233019602, -0.575808191417845, -0.595699304492433, -0.615231590580627,
    -0.634393284163645, -0.653172842953777, -0.671558954847018, -0.689540544737067,
    -0.707106781186547, -0.724247082951467, -0.740951125354959, -0.757208846506485,
    -0.773010453362737, -0.788346427626606, -0.803207531480645, -0.817584813151584,
    -0.831469612302545, -0.844853565249707, -0.857728610000272, -0.870086991108711,
    -0.881921264348355, -0.893224301195515, -0.903989293123443, -0.914209755703531,
    -0.923879532511287, -0.932992798834739, -0.941544065183021, -0.949528180593037,
    -0.956940335732209, -0.963776065795440, -0.970031253194544, -0.975702130038528,
    -0.980785280403230, -0.985277642388941, -0.989176509964781, -0.992479534598710,
    -0.995184726672197, -0.997290456678690, -0.998795456205172, -0.999698818696204,
    -1.000000000000000, -0.999698818696204, -0.998795456205172, -0.997290456678690,
    -0.995184726672197, -0.992479534598710, -0.989176509964781, -0.985277642388941,
    -0.980785280403230, -0.975702130038529, -0.970031253194544, -0.963776065795440,
    -0.956940335732209, -0.949528180593037, -0.941544065183021, -0.932992798834739,
    -0.923879532511287, -0.914209755703531, -0.903989293123443, -0.893224301195515,
    -0.881921264348355, -0.870086991108711, -0.857728610000272, -0.844853565249707,
    -0.831469612302545, -0.817584813151584, -0.803207531480645, -0.788346427626606,
    -0.773010453362737, -0.757208846506485, -0.740951125354959, -0.724247082951467,
    -0.707106781186548, -0.689540544737067, -0.671558954847019, -0.653172842953777,
    -0.634393284163646, -0.615231590580627, -0.595699304492433, -0.575808191417845,
    -0.555570233019602, -0.534997619887097, -0.514102744193222, -0.492898192229784,
    -0.471396736825998, -0.449611329654607, -0.427555093430282, -0.405241314004990,
    -0.382683432365090, -0.359895036534988, -0.336889853392220, -0.313681740398891,
    -0.290284677254462, -0.266712757474899, -0.242980179903264, -0.219101240156870,
    -0.195090322016129, -0.170961888760302, -0.146730474455362, -0.122410675199216,
    -0.098017140329560, -0.073564563599667, -0.049067674327418, -0.024541228522912,
     0.000000000000000,  0.024541228522912,  0.049067674327418,  0.073564563599667,
     0.098017140329560,  0.122410675199216,  0.146730474455362,  0.170961888760301,
     0.195090322016128,  0.219101240156870,  0.242980179903264,  0.266712757474898,
     0.290284677254462,  0.313681740398891,  0.336889853392220,  0.359895036534988,
     0.382683432365090,  0.405241314004990,  0.427555093430282,  0.449611329654607,
     0.471396736825998,  0.492898192229784,  0.514102744193222,  0.534997619887097,
     0.555570233019602,  0.575808191417845,  0.595699304492433,  0.615231590580627,
     0.634393284163646,  0.653172842953777,  0.671558954847018,  0.689540544737067,
     0.707106781186547,  0.724247082951467,  0.740951125354959,  0.757208846506484,
     0.773010453362737,  0.788346427626606,  0.803207531480645,  0.817584813151584,
     0.831469612302545,  0.844853565249707,  0.857728610000272,  0.870086991108711,
     0.881921264348355,  0.893224301195515,  0.903989293123443,  0.914209755703530,
     0.923879532511287,  0.932992798834739,  0.941544065183021,  0.949528180593037,
     0.956940335732209,  0.963776065795440,  0.970031253194544,  0.975702130038528,
     0.980785280403230,  0.985277642388941,  0.989176509964781,  0.992479534598710,
     0.995184726672197,  0.997290456678690,  0.998795456205172,  0.999698818696204
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
    pa->xpos = position.x;
    pa->ypos = position.y;

    pa->xsp = 0.0;
    pa->ysp = 0.0;
    pa->gsp = 0.0;

    pa->angle = 0x0;
    pa->prev_angle = 0x0;
    pa->movmode = MM_FLOOR;
    pa->state = PAS_STOPPED;
    pa->layer = OL_DEFAULT;

    pa->input = input_create_computer();
    pa->observers = NULL;

    pa->midair = true;
    pa->was_midair = true;
    pa->midair_timer = 0.0;
    pa->hlock_timer = 0.0;
    pa->jump_lock_timer = 0.0;
    pa->facing_right = true;
    pa->touching_ceiling = false;
    pa->wait_timer = 0.0;
    pa->winning_pose = false;
    pa->breathe_timer = 0.0;
    pa->want_to_detach_from_ground = false;
    pa->charge_intensity = 0.0;
    pa->airdrag_coefficient[0] = 0.0;
    pa->airdrag_coefficient[1] = 1.0;
    pa->unstable_angle_counter = 0;
    pa->reference_time = 0.0;
    pa->fixed_time = 0.0;
    pa->delayed_jump = false;
    pa->compatibility_version = engine_compatibility_version_code();

    /* initialize the physics model */
    physicsactor_reset_model_parameters(pa);

    /* let's initialize the sensors with a simple box model */

    /* set box size (W,H) and half box size (w,h) relative to sensors A, B, C, D.
       These sensors are vertical and symmetric; hence,
       W = w - (-w) + 1 = 2*w + 1 => w = (W-1)/2; also, h = (H-1)/2 */
    const int default_width = 19, default_height = 39; /* pick odd numbers */
    const int roll_width = 15, roll_height = 29; /* this is expected to be smaller than the default box (about 75%) */
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

    destroy_observers(pa->observers);
    input_destroy(pa->input);
    free(pa);
    return NULL;
}

void physicsactor_reset_model_parameters(physicsactor_t* pa)
{
    const double fpsmul = 60.0;

    /*
      +--------------------+----------------+-----------------+
      | model parameter    |  magic number  | fps multitplier |
      +--------------------+----------------+-----------------+
     */
    pa->acc =                    (3.0/64.0) * fpsmul * fpsmul ;
    pa->dec =                     0.5       * fpsmul * fpsmul ;
    pa->frc =                    (3.0/64.0) * fpsmul * fpsmul ;
    pa->capspeed =                16.0      * fpsmul * 1.0    ; /* tiers: default 16; super 20; ultra 24 */
    pa->topspeed =                6.0       * fpsmul * 1.0    ;
    pa->topyspeed =               16.0      * fpsmul * 1.0    ;
    pa->air =                    (6.0/64.0) * fpsmul * fpsmul ;
    pa->airdrag =               (31.0/32.0) * 1.0    * 1.0    ;
    pa->jmp =                     -6.5      * fpsmul * 1.0    ;
    pa->jmprel =                  -4.0      * fpsmul * 1.0    ;
    pa->diejmp =                  -7.0      * fpsmul * 1.0    ;
    pa->hitjmp =                  -4.0      * fpsmul * 1.0    ;
    pa->grv =                   (14.0/64.0) * fpsmul * fpsmul ;
    pa->slp =                    (8.0/64.0) * fpsmul * fpsmul ;
    pa->chrg =                    12.0      * fpsmul * 1.0    ;
    pa->movethreshold =           0.125     * fpsmul * 1.0    ;
    pa->unrollthreshold =         0.5       * fpsmul * 1.0    ;
    pa->rollthreshold =           1.0       * fpsmul * 1.0    ;
    pa->rollfrc =               (3.0/128.0) * fpsmul * fpsmul ;
    pa->rolldec =                (8.0/64.0) * fpsmul * fpsmul ;
    pa->rolluphillslp =          (5.0/64.0) * fpsmul * fpsmul ;
    pa->rolldownhillslp =       (20.0/64.0) * fpsmul * fpsmul ;
    pa->falloffthreshold =        2.5       * fpsmul * 1.0    ;
    pa->brakingthreshold =        4.0       * fpsmul * 1.0    ;
    pa->airdragthreshold =        -4.0      * fpsmul * 1.0    ;
    pa->airdragxthreshold =      (8.0/64.0) * fpsmul * 1.0    ;
    pa->chrgthreshold =          (1.0/64.0) * 1.0    * 1.0    ;
    pa->waittime =                3.0       * 1.0    * 1.0    ;

    /* recompute airdrag coefficients */
    physicsactor_set_airdrag(pa, pa->airdrag);

    /* compatibility settings */
    if(pa->compatibility_version < VERSION_CODE(0,6,1)) {
        pa->topyspeed = 12.0f * fpsmul;
        /*pa->falloffthreshold = 0.625f * fpsmul;*/ /* maybe not a good idea... */
    }
}

void physicsactor_update(physicsactor_t *pa, const obstaclemap_t *obstaclemap)
{
    /* we run the simulation with a fixed timestep for better accuracy and consistency */
    const double FIXED_TIMESTEP = 1.0 / TARGET_FPS;
#if 1
    /* advance the reference time */
    double dt = timer_get_delta();
    pa->reference_time += dt;

    /* frame skipping */
    bool skip_frame = (pa->fixed_time > pa->reference_time);
    if(skip_frame) {
#if 1
        /* Don't skip a frame, even though the engine is running faster than
           required by the simulation. Frame skipping generates jittering.

           Suppose that the player is running at a very high speed, say 1200
           px/s. That's 20 pixels per frame at 60 fps. If we skip a frame, the
           player will not move by that distance in that frame, but will move
           in the "adjacent" frames in time. A camera script, unaware of the
           frame skipping, will catch up with the player and cause jitter.

           If the target framerate of the physics simulation is equal to the
           target framerate of the engine, then frame skipping will seldom
           happen. It is acceptable to process an additional step of the
           simulation instead of skipping a frame in this case. The result is
           visually smooth. The differences in distances/speeds, compared to
           what would be if the target framerate of the simulation was strictly
           enforced, are negligible: proportional to a small FIXED_TIMESTEP and
           happening only occasionally compared to the total number of frames. */
        pa->reference_time = pa->fixed_time + FIXED_TIMESTEP * 0.5;
#else
        /* skip a frame if the engine is rendering more frames per second than
           required by the simulation. If jump is first pressed, we save that
           information and restore it when we decide not to skip a frame. */
        pa->delayed_jump = pa->delayed_jump || input_button_pressed(pa->input, IB_FIRE1);
    }
    else if(pa->delayed_jump) {
        /* simulate that the jump button is first pressed. We won't skip a frame now. */
        input_simulate_button_press(pa->input, IB_FIRE1);
        pa->delayed_jump = false;
#endif
    }

    /* run the simulation */
    int counter = 0;
    while(pa->fixed_time <= pa->reference_time) {
        if(0 == counter++) {
            /* run at most once per framestep in order to avoid jittering when
               the engine framerate drops below TARGET_FPS. The simulation will
               seem slower in this case. */
            fixed_update(pa, obstaclemap, FIXED_TIMESTEP);
        }

        /* advance the fixed time */
        pa->fixed_time += FIXED_TIMESTEP;
    }

#if WANT_DEBUG
    /* testing */
    if(skip_frame)
        video_showmessage("frame skip %lf", pa->reference_time);
    else if(counter > 1)
        video_showmessage("going slow (%d) %lf", counter, pa->reference_time);
#endif

#else
    /* running the simulation at most once per framestep and with no frame
       skipping is equivalent to running the simulation once per framestep */
    fixed_update(pa, obstaclemap, FIXED_TIMESTEP);
#endif
}

void physicsactor_render_sensors(const physicsactor_t *pa, v2d_t camera_position)
{
    v2d_t position = physicsactor_get_position(pa);

    render_ball(position, 1, color_rgb(255, 255, 255), camera_position);

    if(!pa->midair) {
        render_ball(pa->angle_sensor[0], 1, sensor_color(sensor_A(pa)), camera_position);
        render_ball(pa->angle_sensor[1], 1, sensor_color(sensor_B(pa)), camera_position);
    }

    sensor_render(sensor_A(pa), position, pa->movmode, camera_position);
    sensor_render(sensor_B(pa), position, pa->movmode, camera_position);
    sensor_render(sensor_C(pa), position, pa->movmode, camera_position);
    sensor_render(sensor_D(pa), position, pa->movmode, camera_position);
    sensor_render(sensor_M(pa), position, pa->movmode, camera_position);
    sensor_render(sensor_N(pa), position, pa->movmode, camera_position);
    sensor_render(sensor_U(pa), position, pa->movmode, camera_position);
}

void physicsactor_subscribe(physicsactor_t* pa, void (*callback)(physicsactor_t*,physicsactorevent_t,void*), void* context)
{
    pa->observers = create_observer(
        callback,
        context,
        pa->observers
    );
}

physicsactorstate_t physicsactor_get_state(const physicsactor_t *pa)
{
    return pa->state;
}

int physicsactor_get_angle(const physicsactor_t *pa)
{
    /* return angle in degrees: [0,360) counterclockwise */
    return (((256 - pa->angle) * 180) / 128) % 360;
}

v2d_t physicsactor_get_position(const physicsactor_t *pa)
{
    /* converts from double to float */
    return v2d_new(pa->xpos, pa->ypos);
}

void physicsactor_set_position(physicsactor_t *pa, v2d_t position)
{
    /* converts from float to double */
    pa->xpos = position.x;
    pa->ypos = position.y;
}

void physicsactor_lock_horizontally_for(physicsactor_t *pa, double seconds)
{
    seconds = max(seconds, 0.0);
    if(seconds > pa->hlock_timer)
        pa->hlock_timer = seconds;
}

double physicsactor_hlock_timer(const physicsactor_t *pa)
{
    return pa->hlock_timer;
}

bool physicsactor_resurrect(physicsactor_t *pa)
{
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED) {
        pa->gsp = 0.0;
        pa->xsp = 0.0;
        pa->ysp = 0.0;

        pa->angle = 0x0;
        pa->movmode = MM_FLOOR;
        pa->facing_right = true;

        pa->state = PAS_STOPPED;
        notify_observers(pa, PAE_RESURRECT);
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

double physicsactor_charge_intensity(const physicsactor_t* pa)
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
        pa->xsp = 0.0;
        pa->ysp = pa->diejmp;

        pa->angle = 0x0;
        pa->movmode = MM_FLOOR;
        pa->facing_right = true;

        pa->state = PAS_DEAD;
        notify_observers(pa, PAE_KILL);
    }
}

void physicsactor_hit(physicsactor_t *pa, double direction)
{
    /* direction: >0 right; <0 left */

    /* do nothing if dead or drowned */
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return;

    /* get hit if not already hit */
    if(pa->state != PAS_GETTINGHIT) {
        double dir = (direction != 0.0) ? sign(direction) : (pa->facing_right ? -1.0 : 1.0);
        pa->xsp = pa->hitjmp * 0.5 * -dir;
        pa->ysp = pa->hitjmp;

        physicsactor_detach_from_ground(pa);
        pa->state = PAS_GETTINGHIT;
        notify_observers(pa, PAE_HIT);
    }
}

bool physicsactor_bounce(physicsactor_t *pa, double direction)
{
    /* direction: >0 down; <0 up */

    /* do nothing if dead or drowned */
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return false;

    /* do nothing if on the ground */
    if(!pa->midair || pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return false;

    /* bounce */
    if(direction < 0.0 && pa->ysp > 0.0) /* the specified direction is just a hint */
        pa->ysp = -pa->ysp;
    else
        pa->ysp -= 60.0 * sign(pa->ysp);

    pa->state = PAS_JUMPING;
    return true;
}

void physicsactor_restore_state(physicsactor_t* pa)
{
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return;

    /* the player will be restored to a state vulnerable to attack
       (unless invincible, blinking, etc.) */
    if(fabs(pa->gsp) >= pa->topspeed)
        pa->state = PAS_RUNNING;
    else if(pa->midair || !nearly_zero(pa->gsp))
        pa->state = PAS_WALKING;
    else if(pa->state != PAS_WAITING && pa->state != PAS_PUSHING && pa->state != PAS_LEDGE && pa->state != PAS_LOOKINGUP && pa->state != PAS_DUCKING && pa->state != PAS_WINNING)
        pa->state = PAS_STOPPED;
}

void physicsactor_springify(physicsactor_t *pa)
{
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return;

    if(pa->state != PAS_SPRINGING) {
        pa->want_to_detach_from_ground = pa->want_to_detach_from_ground || (
            (pa->movmode == MM_FLOOR && pa->ysp < 0.0) ||
            (pa->movmode == MM_RIGHTWALL && pa->xsp < 0.0) ||
            (pa->movmode == MM_CEILING && pa->ysp > 0.0) ||
            (pa->movmode == MM_LEFTWALL && pa->xsp > 0.0)
        );
    }

    pa->state = PAS_SPRINGING;
}

void physicsactor_roll(physicsactor_t *pa)
{
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return;

    pa->state = PAS_ROLLING;
}

void physicsactor_drown(physicsactor_t *pa)
{
    if(pa->state != PAS_DROWNED && pa->state != PAS_DEAD) {
        pa->xsp = 0.0;
        pa->ysp = 0.0;

        pa->angle = 0x0;
        pa->movmode = MM_FLOOR;
        pa->facing_right = true;

        pa->state = PAS_DROWNED;
        notify_observers(pa, PAE_DROWN);
    }
}

void physicsactor_breathe(physicsactor_t *pa)
{
    /* do nothing if dead or drowned */
    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED)
        return;

    /* breathe if not already breathing */
    if(pa->state != PAS_BREATHING) {
        pa->xsp = 0.0;
        pa->ysp = 0.0;

        pa->breathe_timer = 0.5;
        pa->state = PAS_BREATHING;
        notify_observers(pa, PAE_BREATHE);
    }
}

/* getters and setters */
#define GENERATE_GETTER_AND_SETTER_OF(x) \
double physicsactor_get_##x(const physicsactor_t *pa) \
{ \
    return pa->x; \
} \
\
void physicsactor_set_##x(physicsactor_t *pa, double value) \
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
GENERATE_GETTER_AND_SETTER_OF(rollthreshold)
GENERATE_GETTER_AND_SETTER_OF(unrollthreshold)
GENERATE_GETTER_AND_SETTER_OF(falloffthreshold)
GENERATE_GETTER_AND_SETTER_OF(brakingthreshold)
GENERATE_GETTER_AND_SETTER_OF(airdragthreshold)
GENERATE_GETTER_AND_SETTER_OF(waittime)

double physicsactor_get_airdrag(const physicsactor_t *pa) { return pa->airdrag; }
void physicsactor_set_airdrag(physicsactor_t *pa, double value)
{
    pa->airdrag = clip(value, 0.0, 1.0);

    if(pa->airdrag > 0.0 && pa->airdrag < 1.0) {
        /* recompute airdrag coefficients */
        pa->airdrag_coefficient[0] = 60.0 * pa->airdrag * log(pa->airdrag);
        pa->airdrag_coefficient[1] = pa->airdrag * (1.0 - log(pa->airdrag));
    }
    else if(pa->airdrag > 0.0) {
        /* no airdrag */
        pa->airdrag_coefficient[0] = 0.0;
        pa->airdrag_coefficient[1] = 1.0;
    }
    else {
        pa->airdrag_coefficient[0] = 0.0;
        pa->airdrag_coefficient[1] = 0.0;
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

    /* compatibility settings */
    if(pa->compatibility_version < VERSION_CODE(0,6,1)) {
        /* older versions had a larger hit box:
           21x45 normal; 23x31 jumproll; 23x45 springing / midair */
        if(pa->state == PAS_JUMPING || pa->state == PAS_ROLLING) {
            w = 23;
            h = 31;
        }
        else if(pa->midair || pa->state == PAS_SPRINGING) {
            w = 23;
            h = 45;
        }
        else {
            w = 21;
            h = 45;
        }
    }

    /* find center */
    int x = (int)floor(pa->xpos);
    int y = (int)floor(pa->ypos);

    /* rotate and apply offset */
    point2d_t offset = sensor_local_head(sensor_d);
    int rw = 0, rh = 0;
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
    v2d_t position = physicsactor_get_position(pa);
    const sensor_t *a = sensor_A(pa);
    const sensor_t *b = sensor_B(pa);
    int x1, y1, x2, y2;

    /* check sensor A */
    sensor_worldpos(a, position, pa->movmode, &x1, &y1, &x2, &y2);
    if(obstacle_got_collision(obstacle, x1, y1, x2, y2))
        return true;

    /* check sensor B */
    sensor_worldpos(b, position, pa->movmode, &x1, &y1, &x2, &y2);
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
        double abs_gsp = fabs(pa->gsp); \
        bool within_default_capspeed = (abs_gsp <= 16.0 * TARGET_FPS); \
        bool within_increased_capspeed = (abs_gsp <= 20.0 * TARGET_FPS); \
        \
        v2d_t position = physicsactor_get_position(pa); \
        v2d_t velocity = v2d_new(pa->xsp, pa->ysp); \
        v2d_t ds = v2d_multiply(velocity, dt); \
        float linear_prediction_factor = within_default_capspeed ? 0.4f : (within_increased_capspeed ? 0.5f : 0.67f); \
        v2d_t predicted_offset = v2d_multiply(ds, linear_prediction_factor); \
        v2d_t predicted_position = v2d_add(position, predicted_offset); \
        \
        /*float angular_prediction_factor = within_default_capspeed ? 0.0f : (within_increased_capspeed ? 0.0f : 0.2f);*/ \
        int predicted_angle = current_angle; /*extrapolate_angle(pa->angle, pa->prev_angle, angular_prediction_factor);*/ \
        (void)interpolate_angle; \
        (void)extrapolate_angle; \
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
void fixed_update(physicsactor_t *pa, const obstaclemap_t *obstaclemap, double dt)
{
    const obstacle_t *at_A, *at_B, *at_C, *at_D, *at_M, *at_N;

    /*
     *
     * initialization
     *
     */

    /* save previous state */
    UPDATE_SENSORS();
    pa->prev_angle = pa->angle;
    pa->was_midair = pa->midair; /* set after UPDATE_SENSORS() */

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

    if(pa->hlock_timer > 0.0) {
        if(1) { /*if(!pa->midair) {*/
            pa->hlock_timer -= dt;
            if(pa->hlock_timer < 0.0)
                pa->hlock_timer = 0.0;
        }

        if(!pa->midair) {
            input_simulate_button_up(pa->input, IB_LEFT);
            input_simulate_button_up(pa->input, IB_RIGHT);
        }

        if(!pa->midair && !nearly_zero(pa->gsp))
            pa->facing_right = (pa->gsp > 0.0);
        else if(pa->midair && !nearly_zero(pa->xsp))
            pa->facing_right = (pa->xsp > 0.0);
    }

    /*
     *
     * death
     *
     */

    if(pa->state == PAS_DEAD || pa->state == PAS_DROWNED) {
        pa->ysp = min(pa->ysp + pa->grv * dt, pa->topyspeed);
        pa->ypos += pa->ysp * dt;
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

        /* just to make sure that we don't get locked in this state
           maybe include a timer instead? */
        if(!pa->midair && !pa->was_midair && pa->ysp >= 0.0)
            pa->state = PAS_STOPPED;
    }

    /*
     *
     * winning
     *
     */

    if(pa->winning_pose) {
        /* brake on level clear */
        const double threshold = 60.0;
        input_reset(pa->input);

        pa->gsp = clip(pa->gsp, -0.625 * pa->capspeed, 0.625 * pa->capspeed);
        if(pa->state == PAS_ROLLING) {
            notify_observers(pa, PAE_BRAKE);
            pa->state = PAS_BRAKING;
        }

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

    if(pa->state != PAS_ROLLING && pa->state != PAS_CHARGING && (!nearly_zero(pa->gsp) || !nearly_zero(pa->xsp))) {
        if((pa->gsp > 0.0 || pa->midair) && input_button_down(pa->input, IB_RIGHT))
            pa->facing_right = true;
        else if((pa->gsp < 0.0 || pa->midair) && input_button_down(pa->input, IB_LEFT))
            pa->facing_right = false;
    }

    /*
     *
     * charge and release
     *
     */

    /* charging... */
    if(pa->state == PAS_CHARGING) {

        /* attenuate charge intensity */
        if(fabs(pa->charge_intensity) >= pa->chrgthreshold)
            pa->charge_intensity *= 0.999506551 - 1.84539309 * dt;
            /*pa->charge_intensity *= pow(31.0 / 32.0, 60.0 * dt);*/ /* 31 / 32 == airdrag */

        /* charging more...! */
        if(input_button_pressed(pa->input, IB_FIRE1)) {
            pa->charge_intensity = min(1.0, pa->charge_intensity + 0.25);
            notify_observers(pa, PAE_RECHARGE);
        }

        /* release */
        pa->gsp = 0.0;
        if(!input_button_down(pa->input, IB_DOWN)) {
            double direction = pa->facing_right ? 1.0 : -1.0;
            double multiplier = direction * (pa->chrg / 3.0);

            pa->gsp = multiplier * (2.0 + pa->charge_intensity);
            pa->charge_intensity = 0.0;
            pa->jump_lock_timer = 2.0 / TARGET_FPS;
            pa->state = PAS_ROLLING;

            notify_observers(pa, PAE_RELEASE);
        }

    }

    /* begin to charge */
    if(pa->state == PAS_DUCKING) {
        if(input_button_down(pa->input, IB_DOWN) && input_button_pressed(pa->input, IB_FIRE1)) {
            if(!nearly_zero(pa->chrg)) { /* check if the player has the ability to charge */
                pa->state = PAS_CHARGING;
                pa->charge_intensity = 0.0;
                notify_observers(pa, PAE_CHARGE);
            }
        }
    }

    /*
     *
     * slope factors
     *
     */

    if(!pa->midair && pa->movmode != MM_CEILING) {

        /* rolling */
        if(pa->state == PAS_ROLLING) {

            if(pa->gsp * SIN(pa->angle) >= 0.0) {
                /* rolling uphill */
                pa->gsp += pa->rolluphillslp * -SIN(pa->angle) * dt;
            }
            else if(fabs(pa->gsp) < pa->capspeed) {
                /* rolling downhill */
                pa->gsp += pa->rolldownhillslp * -SIN(pa->angle) * dt;
                if(fabs(pa->gsp) > pa->capspeed)
                    pa->gsp = pa->capspeed * sign(pa->gsp);
            }

        }

        /* not rolling */
        else if(pa->state != PAS_CHARGING && pa->state != PAS_GETTINGHIT) {

            /* apply if moving or if on a steep slope */
            if(fabs(pa->gsp) >= pa->movethreshold || fabs(SIN(pa->angle)) >= 0.707) {
                if(fabs(pa->gsp) < pa->capspeed) {
                    /* |slp * -sin(angle)| may be less than (2 * default_frc)
                       (friction when turbocharged), meaning: the friction
                       may nullify the slope factor when turbocharged.
                       Example: take angle = 45 degrees. In addition,
                       hlock_timer may be set, thus locking the player. */
                    pa->gsp += pa->slp * -SIN(pa->angle) * dt;
                    if(fabs(pa->gsp) > pa->capspeed)
                        pa->gsp = pa->capspeed * sign(pa->gsp);
                }
            }

        }

    }

    /*
     *
     * walking & running
     *
     */

    if(!pa->midair && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING && pa->state != PAS_GETTINGHIT) {

        /* acceleration */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp >= 0.0) {
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
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp <= 0.0) {
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
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0) {
            pa->gsp += pa->dec * dt;
            if(pa->gsp >= 0.0) {
                pa->gsp = 0.0;
                pa->state = PAS_STOPPED;
            }
            else if(fabs(pa->gsp) >= pa->brakingthreshold) {
                if(pa->movmode == MM_FLOOR && pa->state != PAS_BRAKING) {
                    pa->state = PAS_BRAKING;
                    notify_observers(pa, PAE_BRAKE);
                }
            }
        }
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0) {
            pa->gsp -= pa->dec * dt;
            if(pa->gsp <= 0.0) {
                pa->gsp = 0.0;
                pa->state = PAS_STOPPED;
            }
            else if(fabs(pa->gsp) >= pa->brakingthreshold) {
                if(pa->movmode == MM_FLOOR && pa->state != PAS_BRAKING) {
                    pa->state = PAS_BRAKING;
                    notify_observers(pa, PAE_BRAKE);
                }
            }
        }

        /* braking & friction */
        if(pa->state == PAS_BRAKING) {
            /* braking */
            double brk = pa->frc * (1.5 + 3.0 * fabs(SIN(pa->angle)));
            if(fabs(pa->gsp) <= brk * dt) {
                pa->gsp = 0.0;
                pa->state = PAS_STOPPED;
            }
            else
                pa->gsp -= brk * sign(pa->gsp) * dt;
        }
        else if(!input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT)) {
            /* friction */
            if(fabs(pa->gsp) <= pa->frc * dt) {
                pa->gsp = 0.0;
                pa->state = PAS_STOPPED;
            }
            else
                pa->gsp -= pa->frc * sign(pa->gsp) * dt;
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
        if(pa->midair && pa->ysp > 0.0)
            pa->state = PAS_WALKING;
    }

    /*
     *
     * breathing
     *
     */

    if(pa->breathe_timer > 0.0) {
        pa->breathe_timer -= dt;
        pa->state = PAS_BREATHING;
    }
    else if(pa->state == PAS_BREATHING && pa->midair) {
        pa->breathe_timer = 0.0;
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
        v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
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
        if(fabs(pa->gsp) >= pa->rollthreshold && input_button_down(pa->input, IB_DOWN)) {
            pa->state = PAS_ROLLING;
            notify_observers(pa, PAE_ROLL);
        }
    }

    /* roll */
    if(!pa->midair && pa->state == PAS_ROLLING) {

        /* deceleration */
        if(input_button_down(pa->input, IB_RIGHT) && pa->gsp < 0.0)
            pa->gsp = min(0.0, pa->gsp + pa->rolldec * dt);
        else if(input_button_down(pa->input, IB_LEFT) && pa->gsp > 0.0)
            pa->gsp = max(0.0, pa->gsp - pa->rolldec * dt);

        /* friction */
        if(fabs(pa->gsp) > pa->rollfrc * dt)
            pa->gsp -= pa->rollfrc * sign(pa->gsp) * dt;
        else
            pa->gsp = 0.0;

        /* unroll */
        if(fabs(pa->gsp) < pa->unrollthreshold)
            pa->state = PAS_STOPPED; /*PAS_WALKING;*/ /* anim transition: rolling -> stopped */

        /* facing right? */
        if(!nearly_zero(pa->gsp))
            pa->facing_right = (pa->gsp > 0.0);
    }

    /*
     *
     * speed cap & conversions
     *
     */

    if(!pa->midair) {

        /* cap gsp; you're way too fast... */
        pa->gsp = clip(pa->gsp, -HARD_CAPSPEED, HARD_CAPSPEED);

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

        /* cap xsp & ysp */
        pa->xsp = clip(pa->xsp, -HARD_CAPSPEED, HARD_CAPSPEED);
        pa->ysp = clip(pa->ysp, -HARD_CAPSPEED, HARD_CAPSPEED);

        /* alternatively, this cap could be such that xsp^2 + ysp^2 <= capspeed^2 */

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
        if(pa->ysp < 0.0 && pa->ysp > pa->airdragthreshold && pa->state != PAS_GETTINGHIT) {
            if(fabs(pa->xsp) >= pa->airdragxthreshold) {
                /*pa->xsp *= pow(pa->airdrag, 60.0 * dt);*/
                pa->xsp *= pa->airdrag_coefficient[0] * dt + pa->airdrag_coefficient[1];
            }
        }

        /* gravity */
        if(pa->ysp < pa->topyspeed) {
            double grv = (pa->state != PAS_GETTINGHIT) ? pa->grv : (pa->grv / 7.0) * 6.0;
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
        if(pa->jump_lock_timer <= 0.0) {
            pa->jump_lock_timer = 0.0;

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
                double grv_attenuation = (pa->gsp * SIN(pa->angle) < 0.0) ? 1.0 : 0.5;
                pa->xsp = pa->jmp * SIN(pa->angle) + pa->gsp * COS(pa->angle);
                pa->ysp = pa->jmp * COS(pa->angle) - pa->gsp * SIN(pa->angle) * grv_attenuation;
#else
                pa->xsp = pa->jmp * SIN(pa->angle) + pa->gsp * COS(pa->angle);
                pa->ysp = pa->jmp * COS(pa->angle) - pa->gsp * SIN(pa->angle);
#endif
                pa->state = PAS_JUMPING;
                pa->want_to_detach_from_ground = true;
                FORCE_ANGLE(0x0);

                notify_observers(pa, PAE_JUMP);
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
    pa->xpos += pa->xsp * dt;
    pa->ypos += pa->ysp * dt;
    UPDATE_SENSORS();

    /*
     *
     * getting smashed
     *
     */

    if(is_smashed(pa, obstaclemap)) {
        notify_observers(pa, PAE_SMASH);
        physicsactor_kill(pa);
        return;
    }

    /*
     *
     * wall collisions x ground & ceiling collisions
     *
     */

    /* we generally test for wall collisions first. However, this may not be
       appropriate when |ysp| is too large because the player may be spuriously
       repositioned when hitting the ground or the ceiling.

       delaying wall collision may cause wall bugs. Restrict this a lot. */
    bool delayed_wall_collisions = (
        fabs(pa->ysp) >= 900.0 && /* note: default topyspeed is 960 px/s */
        fabs(pa->xsp) <= 30.0 /* almost a vertical movement */
    );

    /* splitting these into function calls would be much nicer... :\
       TODO reformulate macros UPDATE_SENSORS(), FORCE_ANGLE(), etc. */
    if(delayed_wall_collisions)
        goto ceiling_collisions;

    /*
     *
     * wall collisions
     *
     */

    wall_collisions:

    /* right wall */
    if(at_N != NULL) {
        const sensor_t* sensor = sensor_N(pa);
        v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
        point2d_t tail = sensor_tail(sensor, position, pa->movmode);
        point2d_t local_tail = point2d_subtract(tail, point2d_from_v2d(position));
        bool reset_angle = false;
        int wall;

        /* reset gsp */
        if(pa->gsp > 0.0)
            pa->gsp = 0.0;

        /* reposition the player */
        switch(pa->movmode) {
            case MM_FLOOR:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_RIGHT);
                pa->xpos = wall - local_tail.x - 1;
                pa->xsp = min(pa->xsp, 0.0);
                reset_angle = false;
                break;

            case MM_CEILING:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_LEFT);
                pa->xpos = wall - local_tail.x + 1;
                pa->xsp = max(pa->xsp, 0.0);
                reset_angle = true;
                break;

            case MM_RIGHTWALL:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_UP);
                pa->ypos = wall - local_tail.y - 1;
                pa->ysp = max(pa->ysp, 0.0);
                reset_angle = true;
                break;

            case MM_LEFTWALL:
                wall = obstacle_ground_position(at_N, tail.x, tail.y, GD_DOWN);
                pa->ypos = wall - local_tail.y + 1;
                pa->ysp = min(pa->ysp, 0.0);
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
        if(!pa->midair && pa->movmode == MM_FLOOR && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING && pa->state != PAS_GETTINGHIT) {
            if(input_button_down(pa->input, IB_RIGHT)) {
                pa->state = PAS_PUSHING;
                pa->facing_right = true;
            }
        }
    }

    /* left wall */
    if(at_M != NULL) {
        const sensor_t* sensor = sensor_M(pa);
        v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
        point2d_t tail = sensor_tail(sensor, position, pa->movmode);
        point2d_t local_tail = point2d_subtract(tail, point2d_from_v2d(position));
        bool reset_angle = false;
        int wall;

        /* reset gsp */
        if(pa->gsp < 0.0)
            pa->gsp = 0.0;

        /* reposition the player */
        switch(pa->movmode) {
            case MM_FLOOR:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_LEFT);
                pa->xpos = wall - local_tail.x + 1;
                pa->xsp = max(pa->xsp, 0.0);
                reset_angle = false;
                break;

            case MM_CEILING:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_RIGHT);
                pa->xpos = wall - local_tail.x - 1;
                pa->xsp = min(pa->xsp, 0.0);
                reset_angle = true;
                break;

            case MM_RIGHTWALL:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_DOWN);
                pa->ypos = wall - local_tail.y - 1;
                pa->ysp = min(pa->ysp, 0.0);
                reset_angle = true;
                break;

            case MM_LEFTWALL:
                wall = obstacle_ground_position(at_M, tail.x, tail.y, GD_UP);
                pa->ypos = wall - local_tail.y + 1;
                pa->ysp = max(pa->ysp, 0.0);
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
        if(!pa->midair && pa->movmode == MM_FLOOR && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING && pa->state != PAS_GETTINGHIT) {
            if(input_button_down(pa->input, IB_LEFT)) {
                pa->state = PAS_PUSHING;
                pa->facing_right = false;
            }
        }
    }

    if(delayed_wall_collisions)
        goto end_of_collisions;

    /*
     *
     * ceiling collisions
     *
     */

    ceiling_collisions:

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
        if(pa->ysp < 0.0) {
            /* compute the angle */
            FORCE_ANGLE(0x80);
            pa->midair = false; /* enable the ground sensors */
            SET_AUTO_ANGLE();

            /* reattach to the ceiling if steep angle and moving upwards */
            if((pa->angle >= 0xA0 && pa->angle <= 0xBF) || (pa->angle >= 0x40 && pa->angle <= 0x5F)) {
                if(-pa->ysp >= fabs(pa->xsp))
                    must_reattach = !pa->midair;
            }
        }

        /* reattach to the ceiling or bump the head */
        if(must_reattach) {
            /* adjust speeds */
            pa->gsp = pa->ysp * -sign(SIN(pa->angle));
            pa->xsp = pa->ysp = 0.0;

            /* adjust the state */
            if(pa->state != PAS_ROLLING)
                pa->state = WALKING_OR_RUNNING(pa);

            /* make sure we stick to the ground */
            pa->want_to_detach_from_ground = false;
        }
        else {
            /* adjust speed & angle */
            pa->ysp = max(pa->ysp, 0.0);
            FORCE_ANGLE(0x0);

            /* find the position of the sensor after setting the angle to 0 */
            v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
            point2d_t tail = sensor_tail(c_or_d, position, pa->movmode);
            point2d_t local_tail = point2d_subtract(tail, point2d_from_v2d(position));

            /* reposition the player */
            int ceiling_position = obstacle_ground_position(ceiling, tail.x, tail.y, GD_UP);
            pa->ypos = ceiling_position - local_tail.y + 1;

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
        movmode_t prev_movmode = pa->movmode;

        sticky_loop:

        /* if the player is on the ground or has just left the ground, stick to it! */
        if(!pa->midair || !pa->was_midair || pa->unstable_angle_counter > 0) {
            int gnd_pos_a, gnd_pos_b, gnd_pos = 0;
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
                v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
                point2d_t tail = sensor_tail(a_or_b, position, pa->movmode);
                gnd_pos = obstacle_ground_position(gnd, tail.x, tail.y, MM_TO_GD(pa->movmode));
            }
            else {

                /* compute an extended length measured from the tail of the sensors */
                double abs_xsp = fabs(pa->xsp), abs_ysp = fabs(pa->ysp);
                double max_abs_speed = max(abs_xsp, abs_ysp); /* <= fabs(pa->gsp) */
                int max_abs_ds = (int)ceil(max_abs_speed * dt);
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
                v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
                point2d_t tail = sensor_tail(a_or_b, position, pa->movmode);

                /* put the tail of the sensor on the ground */
                const int offset = AB_SENSOR_OFFSET;
                switch(pa->movmode) {
                    case MM_FLOOR:
                        pa->ypos = (int)position.y + (gnd_pos - tail.y) + offset;
                        break;

                    case MM_CEILING:
                        pa->ypos = (int)position.y + (gnd_pos - tail.y) - offset;
                        break;

                    case MM_RIGHTWALL:
                        pa->xpos = (int)position.x + (gnd_pos - tail.x) + offset;
                        break;

                    case MM_LEFTWALL:
                        pa->xpos = (int)position.x + (gnd_pos - tail.x) - offset;
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
        if(pa->movmode != prev_movmode && 0 == pa->unstable_angle_counter) {
            /* unstable_angle_counter:
               avoid locking the player when moving slowly and getting unstable
               movmodes in a transition. Unstable angle measurements provoke
               unstable movmodes, as in: 0x5e, 0x62, 0x5e, 0x62, 0x5e...
               (left wall, ceiling...) */
            const double speed_threshold = 300.0; /* not moving slowly */

            if(fabs(pa->gsp) < speed_threshold) {
                /* we're moving slowly and MAY be getting unstable angles
                   (probably not... maybe if turbocharged...) */
                pa->unstable_angle_counter = 2;
            }
            else {
                /* we have enough speed and intend to run this sticky physics
                   routine on the next frame */
                pa->unstable_angle_counter = 1;
            }

            /* repeat */
            goto sticky_loop;
        }
    }

    /* reset the angle if midair */
    if(pa->midair) {
        /* if we're balancing on a ledge (of short height), we may be getting a
          spurious angle, and hence a spurious movmode. pa->midair may be set to
          true, even though we're on a ledge. That's due to the wall modes. */
        FORCE_ANGLE(0x0); /* pa->midair may be set to false here */
    }

    /* reset flag */
    pa->want_to_detach_from_ground = false;

    /* reset counter */
    if(pa->unstable_angle_counter > 0)
        --pa->unstable_angle_counter;

    /* end of collisions */
    if(delayed_wall_collisions)
        goto wall_collisions;

    end_of_collisions:

    /*
     *
     * reacquisition of the ground
     *
     */

    if(!pa->midair && pa->was_midair) {

        /* if the player is moving mostly horizontally, set gsp to xsp */
        if(fabs(pa->xsp) > pa->ysp)
            pa->gsp = pa->xsp;

        /* if not, set gsp based on the angle:

           [0x00, 0x0F] U [0xF0, 0xFF]: flat ground
           [0x10, 0x1F] U [0xE0, 0xEF]: slope
           [0x20, 0x3F] U [0xC0, 0xDF]: steep slope

           0x40, 0xC0 is +- ninety degrees... */
        else if(pa->angle >= 0xF0 || pa->angle <= 0x0F)
            pa->gsp = pa->xsp;
        else if((pa->angle >= 0xE0 && pa->angle <= 0xEF) || (pa->angle >= 0x10 && pa->angle <= 0x1F))
            pa->gsp = pa->ysp * 0.5 * -sign(SIN(pa->angle));
        else if((pa->angle >= 0xC0 && pa->angle <= 0xDF) || (pa->angle >= 0x20 && pa->angle <= 0x3F))
            pa->gsp = pa->ysp * -sign(SIN(pa->angle));

        /* reset speeds */
        pa->xsp = pa->ysp = 0.0;

    }

    /*
     *
     * falling off walls and ceilings
     *
     */

    if(!pa->midair && pa->hlock_timer == 0.0) {
        if(pa->movmode != MM_FLOOR) {
            if(fabs(pa->gsp) < pa->falloffthreshold) {
                pa->hlock_timer = 0.5;
                if(pa->angle >= 0x40 && pa->angle <= 0xC0) {
                    pa->gsp = 0.0;
                    FORCE_ANGLE(0x0);
                }
            }
        }
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
        pa->wait_timer = 0.0;

    /*
     *
     * misc
     *
     */

    /* corrections when landing on the ground */
    if(!pa->midair && pa->was_midair) {
        if(pa->state == PAS_GETTINGHIT) {
            /* stop when landing after getting hit */
            pa->gsp = pa->xsp = 0.0;
            pa->state = PAS_STOPPED;
            notify_observers(pa, PAE_BLINK);
        }
        else if(pa->state == PAS_ROLLING) {
            /* unroll when landing on the ground */
            if(pa->midair_timer >= 0.2) {
                /* ...unless the player wants to (and can) keep rolling */
                bool wanna_roll = input_button_down(pa->input, IB_DOWN);
                bool can_roll = fabs(pa->gsp) >= pa->rollthreshold;

                if(!(wanna_roll && can_roll)) {
                    pa->state = WALKING_OR_RUNNING(pa);
                    if(!nearly_zero(pa->gsp))
                        pa->facing_right = (pa->gsp > 0.0);
                }
            }
        }
        else {
            /* walk / run */
            pa->state = WALKING_OR_RUNNING(pa);
        }
    }

    /* animation corrections while on the ground */
    if(!pa->midair && pa->state != PAS_ROLLING && pa->state != PAS_CHARGING && pa->state != PAS_GETTINGHIT && pa->state != PAS_WINNING) {
        if(fabs(pa->gsp) < pa->movethreshold) {
            if(pa->state == PAS_PUSHING && !input_button_down(pa->input, IB_LEFT) && !input_button_down(pa->input, IB_RIGHT))
                pa->state = PAS_STOPPED;
            else if(pa->state == PAS_PUSHING || pa->state == PAS_LOOKINGUP || pa->state == PAS_DUCKING || pa->state == PAS_LEDGE)
                ;
            else if(input_button_down(pa->input, IB_LEFT) || input_button_down(pa->input, IB_RIGHT))
                pa->state = PAS_WALKING;
            else if(pa->state != PAS_WAITING)
                pa->state = PAS_STOPPED;
            else if(!nearly_zero(pa->gsp))
                pa->state = PAS_WALKING;
        }
        else {
            if(pa->state == PAS_STOPPED || pa->state == PAS_WAITING || pa->state == PAS_LEDGE || pa->state == PAS_WALKING || pa->state == PAS_RUNNING || pa->state == PAS_DUCKING || pa->state == PAS_LOOKINGUP)
                pa->state = WALKING_OR_RUNNING(pa);
            else if(pa->state == PAS_PUSHING && fabs(pa->gsp) >= 30.0)
                pa->state = PAS_WALKING;
        }
    }

    /* fix invalid states */
    if(pa->midair) {
        if(pa->state == PAS_PUSHING || pa->state == PAS_LEDGE || pa->state == PAS_STOPPED || pa->state == PAS_WAITING || pa->state == PAS_BRAKING || pa->state == PAS_DUCKING || pa->state == PAS_LOOKINGUP)
            pa->state = WALKING_OR_RUNNING(pa);
    }
    else {
        if(pa->state == PAS_WALKING && nearly_zero(pa->gsp))
            pa->state = PAS_STOPPED;
    }

    /* remain on the winning state */
    if(pa->winning_pose && !pa->midair) {
        if(fabs(pa->gsp) < pa->movethreshold)
            pa->state = PAS_WINNING;
    }

    /* update the midair_timer */
    if(pa->midair)
        pa->midair_timer += dt;
    else
        pa->midair_timer = 0.0;
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
        sensor_set_enabled(m, wanna_middle && (pa->gsp <= pa->movethreshold));  /* gsp <= 0.0 */ /* regular movement & moving platforms */
        sensor_set_enabled(n, wanna_middle && (pa->gsp >= -pa->movethreshold)); /* gsp >= 0.0 */
    }
    else {
#if 0
        double abs_xsp = fabs(pa->xsp), abs_ysp = fabs(pa->ysp);
        sensor_set_enabled(a, pa->ysp >= -abs_xsp); /* ysp >= 0.0 */
        sensor_set_enabled(b, pa->ysp >= -abs_xsp); /* ysp >= 0.0 */
        sensor_set_enabled(c, pa->ysp < abs_xsp);   /* ysp < 0.0 */
        sensor_set_enabled(d, pa->ysp < abs_xsp);   /* ysp < 0.0 */
        sensor_set_enabled(m, pa->xsp < abs_ysp);   /* xsp < 0.0 */
        sensor_set_enabled(n, pa->xsp > -abs_ysp);  /* xsp > 0.0 */
#else
        sensor_set_enabled(a, true);
        sensor_set_enabled(b, true);
        sensor_set_enabled(c, true);
        sensor_set_enabled(d, true);
        sensor_set_enabled(m, true);
        sensor_set_enabled(n, true);
#endif
    }

    /* read sensors */
    v2d_t position = physicsactor_get_position(pa);
    *at_A = sensor_check(a, position, pa->movmode, pa->layer, obstaclemap);
    *at_B = sensor_check(b, position, pa->movmode, pa->layer, obstaclemap);
    *at_C = sensor_check(c, position, pa->movmode, pa->layer, obstaclemap);
    *at_D = sensor_check(d, position, pa->movmode, pa->layer, obstaclemap);
    *at_M = sensor_check(m, position, pa->movmode, pa->layer, obstaclemap);
    *at_N = sensor_check(n, position, pa->movmode, pa->layer, obstaclemap);

    /* C, D, M, N: ignore clouds */
    *at_C = (*at_C != NULL && obstacle_is_solid(*at_C)) ? *at_C : NULL;
    *at_D = (*at_D != NULL && obstacle_is_solid(*at_D)) ? *at_D : NULL;
    *at_M = (*at_M != NULL && obstacle_is_solid(*at_M)) ? *at_M : NULL;
    *at_N = (*at_N != NULL && obstacle_is_solid(*at_N)) ? *at_N : NULL;

    /* A, B: ignore clouds when moving upwards */
    if(pa->ysp < 0.0) {
        if(pa->ysp < -fabs(pa->xsp) || (pa->was_midair && pa->state != PAS_JUMPING)) {
            *at_A = (*at_A != NULL && obstacle_is_solid(*at_A)) ? *at_A : NULL;
            *at_B = (*at_B != NULL && obstacle_is_solid(*at_B)) ? *at_B : NULL;
        }
    }

    /* A, B: cloud height */
    if(pa->midair && pa->angle == 0x0 && pa->ysp / 60.0f < CLOUD_OFFSET * 0.5f) {

        /* A: ignore if it's a cloud and if the tail of the sensor is too far away from the ground level */
        if(*at_A != NULL && !obstacle_is_solid(*at_A)) {
            point2d_t tail = sensor_tail(a, position, pa->movmode);
            int ygnd = obstacle_ground_position(*at_A, tail.x, tail.y, GD_DOWN);
            if(tail.y >= ygnd + CLOUD_OFFSET)
                *at_A = NULL;
        }

        /* B: ignore if it's a cloud and if the tail of the sensor is too far away from the ground level */
        if(*at_B != NULL && !obstacle_is_solid(*at_B)) {
            point2d_t tail = sensor_tail(b, position, pa->movmode);
            int ygnd = obstacle_ground_position(*at_B, tail.x, tail.y, GD_DOWN);
            if(tail.y >= ygnd + CLOUD_OFFSET)
                *at_B = NULL;
        }

    }

    /* A, B: conflict resolution when A != B */
    if(*at_A != NULL && *at_B != NULL && *at_A != *at_B) {

        /* A: if B is a solid and A is a cloud, ignore A */
        if(!obstacle_is_solid(*at_A) && obstacle_is_solid(*at_B))
            *at_A = NULL;

        /* B: if A is a solid and B is a cloud, ignore B */
        else if(obstacle_is_solid(*at_A) && !obstacle_is_solid(*at_B))
            *at_B = NULL;

        /* A, B: special logic when both are clouds and one is much taller than the other */
        else if(!obstacle_is_solid(*at_A) && !obstacle_is_solid(*at_B)) {
            if(pa->movmode == MM_FLOOR) {
                point2d_t tail_a = sensor_tail(a, position, pa->movmode);
                point2d_t tail_b = sensor_tail(b, position, pa->movmode);
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
#if 0
        if(pa->movmode == MM_CEILING)
            pa->gsp = -pa->gsp;
#endif
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
    int ha, hb;

    if(a == NULL)
        return 'b';
    else if(b == NULL)
        return 'a';

    v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
    point2d_t sa = sensor_head(a_sensor, position, pa->movmode);
    point2d_t sb = sensor_head(b_sensor, position, pa->movmode);

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
    int hc, hd;

    if(c == NULL)
        return 'd';
    else if(d == NULL)
        return 'c';

    v2d_t position = v2d_new(floor(pa->xpos), floor(pa->ypos));
    point2d_t sc = sensor_tail(c_sensor, position, pa->movmode);
    point2d_t sd = sensor_tail(d_sensor, position, pa->movmode);

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

    v2d_t position = physicsactor_get_position(pa);
    sensor_extend(sensor, position, pa->movmode, extended_sensor_length, &head, &tail);

    x1 = min(head.x, tail.x);
    y1 = min(head.y, tail.y);
    x2 = max(head.x, tail.x);
    y2 = max(head.y, tail.y);

    return obstaclemap_find_ground(obstaclemap, x1, y1, x2, y2, pa->layer, MM_TO_GD(pa->movmode), out_ground_position);
}

/* check if the physics actor is smashed / crushed / squashed */
bool is_smashed(const physicsactor_t* pa, const obstaclemap_t* obstaclemap)
{
    v2d_t position = physicsactor_get_position(pa);

    /* quit if midair */
    if(pa->midair)
        return false;

#if 1
    /* check if sensor U is overlapping a solid obstacle */
    const obstacle_t* at_U = sensor_check(sensor_U(pa), position, pa->movmode, pa->layer, obstaclemap);
    if(!(at_U != NULL && obstacle_is_solid(at_U))) /* sensor U is assumed to be enabled */
        return false; /* quit if no collision */
#endif

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

    const obstacle_t* at_A = sensor_check(a, position, pa->movmode, pa->layer, obstaclemap);
    const obstacle_t* at_B = sensor_check(b, position, pa->movmode, pa->layer, obstaclemap);
    const obstacle_t* at_C = sensor_check(c, position, pa->movmode, pa->layer, obstaclemap);
    const obstacle_t* at_D = sensor_check(d, position, pa->movmode, pa->layer, obstaclemap);

    sensor_set_enabled(d, d_enabled);
    sensor_set_enabled(c, c_enabled);
    sensor_set_enabled(b, b_enabled);
    sensor_set_enabled(a, a_enabled);

    /* possibly_smashed may be true when the player is being repositioned */
    bool possibly_smashed = (
        (at_A != NULL && obstacle_is_solid(at_A)) &&
        (at_B != NULL && obstacle_is_solid(at_B)) &&
        (at_C != NULL && obstacle_is_solid(at_C)) &&
        (at_D != NULL && obstacle_is_solid(at_D))
    );

    /* check also if the player is touching a moving obstacle */
    bool is_smashed = possibly_smashed && (
        (at_D != NULL && !obstacle_is_static(at_D)) ||
        (at_C != NULL && !obstacle_is_static(at_C)) ||
        (at_B != NULL && !obstacle_is_static(at_B)) ||
        (at_A != NULL && !obstacle_is_static(at_A))
    );

    return is_smashed;
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
    const double DEFAULT_CAPSPEED = 16.0 * TARGET_FPS;

    if(fabs(pa->gsp) <= DEFAULT_CAPSPEED)
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

/* notify observers */
void notify_observers(physicsactor_t* pa, physicsactorevent_t event)
{
    physicsactorobserverlist_t* observer = pa->observers;

    while(observer != NULL) {
        observer->callback(pa, event, observer->context);
        observer = observer->next;
    }
}

/* create an observer */
physicsactorobserverlist_t* create_observer(void (*callback)(physicsactor_t*,physicsactorevent_t,void*), void* context, physicsactorobserverlist_t* next)
{
    physicsactorobserverlist_t* observer = mallocx(sizeof *observer);

    observer->callback = callback;
    observer->context = context;
    observer->next = next;

    return observer;
}

/* destroy observers */
physicsactorobserverlist_t* destroy_observers(physicsactorobserverlist_t* list)
{
    while(list != NULL) {
        physicsactorobserverlist_t* next = list->next;
        free(list);
        list = next;
    }

    return NULL;
}
