/*
 * Open Surge Engine
 * keyframes.c - keyframe-based animations (a.k.a. programmatic animations)
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
#include <string.h>
#include "keyframes.h"
#include "nanoparser.h"
#include "logfile.h"
#include "../util/numeric.h"
#include "../util/transform.h"
#include "../util/stringutil.h"
#include "../util/util.h"

typedef struct proganim_keyframe_t proganim_keyframe_t;
typedef double (*proganim_easing_t)(double,const double*);


/* programmatic animation */
struct proganim_t {
    double duration; /* in seconds */
    proganim_easing_t easing; /* easing function */
    proganim_keyframe_t* keyframe; /* array of keyframes */
    int keyframe_count; /* length of keyframe[] */
};

/* keyframe struct */
struct proganim_keyframe_t {

    /* percentage */
    int percentage; /* 0 to 100 or UNDEFINED_PERCENTAGE */

    /* transform */
    v2d_t translation; /* in pixels */
    float rotation; /* in degrees */
    v2d_t scale; /* scale factors in the x and y axes */

    /* opacity */
    int opacity; /* 100: unmodified; 0: fully translucent */

};

/* easing functions */
static double easing_linear(double t, const double* p);
static double easing_in_quadratic(double t, const double* p);
static double easing_out_quadratic(double t, const double* p);
static double easing_inout_quadratic(double t, const double* p);

/* constants */
#define UNDEFINED_PERCENTAGE -1
static const proganim_keyframe_t DEFAULT_KEYFRAME = {
    .percentage = UNDEFINED_PERCENTAGE,
    .translation = { 0.0f, 0.0f },
    .rotation = 0.0f,
    .scale = { 1.0f, 1.0f },
    .opacity = 100
};

static const proganim_t DEFAULT_PROGANIM = {
    .duration = 0.0,
    .easing = easing_linear,
    .keyframe = NULL,
    .keyframe_count = 0
};


/* helpers */
static void proganim_add_keyframe(proganim_t* prog_anim, proganim_keyframe_t keyframe);
static void find_keyframes_suitable_for_interpolation(const proganim_t* prog_anim, double percentage, const proganim_keyframe_t** out_a, const proganim_keyframe_t** out_b);
static int compare_keyframes(const void* a, const void* b);
static float normalized_percentage(float percentage, const proganim_keyframe_t* a, const proganim_keyframe_t* b);
static int parse_percentage(const parsetree_parameter_t* param);
static proganim_easing_t parse_easing_function(const parsetree_parameter_t* param);
static int traverse_keyframe(const parsetree_statement_t *stmt, void *context);




/*
 *
 * public
 *
 */


/*
 * proganim_duration()
 * The duration, in seconds, of a keyframe-based animation
 */
double proganim_duration(const proganim_t* prog_anim)
{
    return prog_anim->duration;
}

/*
 * proganim_interpolated_transform()
 * The interpolated transform of a programmatic animation computed at the given time
 */
transform_t* proganim_interpolated_transform(const proganim_t* prog_anim, double seconds, bool repeat, transform_t* out_transform)
{
    transform_t* t = out_transform;

    /* no keyframes? */
    if(prog_anim->keyframe_count == 0) {
        /*transform_build(t, DEFAULT_KEYFRAME.translation, -DEFAULT_KEYFRAME.rotation * DEG2RAD, DEFAULT_KEYFRAME.scale, v2d_new(0.0f, 0.0f));*/
        transform_identity(t);
        return t;
    }

    /* only 1 keyframe? */
    if(prog_anim->keyframe_count == 1) {
        const proganim_keyframe_t* keyframe = &prog_anim->keyframe[0];
        transform_build(t, keyframe->translation, -keyframe->rotation * DEG2RAD, keyframe->scale, v2d_new(0.0f, 0.0f));
        return t;
    }

    /* we have at least 2 keyframes */

    /* is this a repeating animation? */
    if(repeat)
        seconds = fmod(seconds, prog_anim->duration);

    /* find the current percentage */
    double percentage = clip01(seconds / prog_anim->duration);

    /* apply easing function */
    percentage = prog_anim->easing(percentage, NULL);

    /* get two keyframes suitable for interpolation */
    const proganim_keyframe_t *keyframe_a, *keyframe_b;
    find_keyframes_suitable_for_interpolation(prog_anim, percentage, &keyframe_a, &keyframe_b);

    /* interpolate */
    float p = normalized_percentage(percentage, keyframe_a, keyframe_b);
    v2d_t interpolated_translation = v2d_lerp(keyframe_a->translation, keyframe_b->translation, p);
    float interpolated_rotation = lerp_angle(keyframe_a->rotation * DEG2RAD, keyframe_b->rotation * DEG2RAD, p);
    v2d_t interpolated_scale = v2d_lerp(keyframe_a->scale, keyframe_b->scale, p);

    /* set the transform */
    transform_build(t, interpolated_translation, -interpolated_rotation, interpolated_scale, v2d_new(0.0f, 0.0f));
    return t;
}

/*
 * proganim_interpolated_opacity()
 * The interpolated opacity of a programmatic animation computed at the given time
 */
float proganim_interpolated_opacity(const proganim_t* prog_anim, double seconds, bool repeat)
{
    /* no keyframes? */
    if(prog_anim->keyframe_count == 0)
        return (float)DEFAULT_KEYFRAME.opacity * 0.01f;

    /* only 1 keyframe? */
    if(prog_anim->keyframe_count == 1) {
        const proganim_keyframe_t* keyframe = &prog_anim->keyframe[0];
        return (float)keyframe->opacity * 0.01f;
    }

    /* we have at least 2 keyframes */

    /* is this a repeating animation? */
    if(repeat)
        seconds = fmod(seconds, prog_anim->duration);

    /* find the current percentage */
    double percentage = clip01(seconds / prog_anim->duration);

    /* apply easing function */
    percentage = prog_anim->easing(percentage, NULL);
    percentage = clip01(percentage); /* no opacity values outside [0,1] */

    /* get two keyframes suitable for interpolation */
    const proganim_keyframe_t *keyframe_a, *keyframe_b;
    find_keyframes_suitable_for_interpolation(prog_anim, percentage, &keyframe_a, &keyframe_b);

    /* interpolate */
    float p = normalized_percentage(percentage, keyframe_a, keyframe_b);
    float a = (float)keyframe_a->opacity * 0.01f;
    float b = (float)keyframe_b->opacity * 0.01f;
    float interpolated_opacity = lerp(a, b, p);
    return interpolated_opacity;
}

/*
 * find_keyframes_suitable_for_interpolation()
 * Find keyframes out_a and out_b suitable for interpolation at the given percentage
 */
void find_keyframes_suitable_for_interpolation(const proganim_t* prog_anim, double percentage, const proganim_keyframe_t** out_a, const proganim_keyframe_t** out_b)
{
    int p = (int)floor(100.0 * percentage);

    /* make sure that we have at least 2 keyframes */
    assertx(prog_anim->keyframe_count >= 2);

    /* keyframes are sorted by percentages */

    /* out of bounds check */
    if(p < prog_anim->keyframe[0].percentage) {
        *out_a = &prog_anim->keyframe[0];
        *out_b = &prog_anim->keyframe[0];
        return;
    }
    else if(p > prog_anim->keyframe[prog_anim->keyframe_count - 1].percentage) {
        *out_a = &prog_anim->keyframe[prog_anim->keyframe_count - 1];
        *out_b = &prog_anim->keyframe[prog_anim->keyframe_count - 1];
        return;
    }

    /* find a suitable interval */
    for(int k = 0; k+1 < prog_anim->keyframe_count; k++) {
        int start_percentage = prog_anim->keyframe[k].percentage;
        int end_percentage = prog_anim->keyframe[k+1].percentage;

        if(p >= start_percentage && p <= end_percentage) {
            *out_a = &prog_anim->keyframe[k];
            *out_b = &prog_anim->keyframe[k+1];
            return;
        }
    }

    /* can't find an interval (this shouldn't happen) */
    *out_a = &prog_anim->keyframe[prog_anim->keyframe_count - 1];
    *out_b = &prog_anim->keyframe[prog_anim->keyframe_count - 1];
}




/*
 *
 * friend class spriteinfo_t
 *
 */


/*
 * proganim_create()
 * Create a programmatic animation
 */
proganim_t* proganim_create()
{
    proganim_t* prog_anim = mallocx(sizeof *prog_anim);

    *prog_anim = DEFAULT_PROGANIM;

    return prog_anim;
}

/*
 * proganim_destroy()
 * Destroy a programmatic animation
 */
proganim_t* proganim_destroy(proganim_t* prog_anim)
{
    if(prog_anim->keyframe != NULL)
        free(prog_anim->keyframe);

    free(prog_anim);
    return NULL;
}

/*
 * proganim_validate()
 * Validate and preprocess a programmatic animation
 */
void proganim_validate(proganim_t* prog_anim)
{
    /* validate duration */
    if(prog_anim->duration <= 0.0) {
        logfile_message("Programmatic animation warning: 'duration' should be a positive number, but it has been set to %f", prog_anim->duration);
        prog_anim->duration = 0.0; /* non-negative */
    }

    /* validate the number of keyframes */
    if(prog_anim->keyframe_count == 0) {
        /* this is acceptable if a duration is defined...
           the animation is not considered to be over until the "duration" of
           the programmatic animation is over, despite having no keyframes */
        logfile_message("Programmatic animation warning: no keyframes have been defined");
    }

    /* validate keyframes & set percentages */
    for(int i = 0; i < prog_anim->keyframe_count; i++) {
        proganim_keyframe_t* keyframe = &(prog_anim->keyframe[i]);

        if(keyframe->opacity < 0 || keyframe->opacity > 100) {
            logfile_message("Programmatic animation warning: not a valid opacity value for keyframe #%d: %d%%", i+1, keyframe->opacity);
            keyframe->opacity = clip(keyframe->opacity, 0, 100);
        }

        if(keyframe->percentage == UNDEFINED_PERCENTAGE) {
            if(prog_anim->keyframe_count > 1)
                keyframe->percentage = (100 * i) / (prog_anim->keyframe_count - 1);
            else
                keyframe->percentage = 0;
        }
        else if(keyframe->percentage < 0 || keyframe->percentage > 100) {
            logfile_message("Programmatic animation warning: not a valid percentage for keyframe #%d: %d%%", i+1, keyframe->percentage);
            keyframe->percentage = clip(keyframe->percentage, 0, 100);
        }
    }

#if 0
    /* stable sort keyframes by percentage */
    merge_sort(prog_anim->keyframe, prog_anim->keyframe_count, sizeof(proganim_keyframe_t), compare_keyframes);
#else
    /* keyframes are already declared in a sorted way */
    (void)compare_keyframes;
#endif
}

/*
 * traverse_keyframes()
 * Traverse the attributes of a programmatic animation
 */
int traverse_keyframes(const parsetree_statement_t* stmt, void* context)
{
    const char* identifier;
    const parsetree_parameter_t* param_list;
    proganim_t* prog_anim = (proganim_t*)context;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "duration") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "duration receives a positive number");
        prog_anim->duration = atof(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "easing") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "easing receives an easing function");
        prog_anim->easing = parse_easing_function(p1);
    }
    else if(str_icmp(identifier, "keyframe") == 0) {
        const parsetree_parameter_t* block = NULL;
        const parsetree_parameter_t* percentage_string = NULL;

        /* syntax: keyframe { ... } or keyframe <percentage> { ... } */
        switch(nanoparser_get_number_of_parameters(param_list)) {
            case 1:
                block = nanoparser_get_nth_parameter(param_list, 1);
                nanoparser_expect_program(block, "Must provide keyframe attributes");
                break;

            case 2:
                percentage_string = nanoparser_get_nth_parameter(param_list, 1);
                block = nanoparser_get_nth_parameter(param_list, 2);
                nanoparser_expect_string(percentage_string, "Must provide keyframe percentage");
                nanoparser_expect_program(block, "Must provide keyframe attributes");
                break;

            default:
                nanoparser_crash(stmt, "Syntax error");
                break;
        }

        /* read keyframe percentage */
        int percentage = UNDEFINED_PERCENTAGE;
        if(percentage_string != NULL)
            percentage = parse_percentage(percentage_string);
#if 0
        /* create a new keyframe. Repeat the last one (if any) */
        if(prog_anim->keyframe_count == 0)
            proganim_add_keyframe(prog_anim, DEFAULT_KEYFRAME);
        else
            proganim_add_keyframe(prog_anim, prog_anim->keyframe[prog_anim->keyframe_count - 1]);
#else
        /* create a new keyframe */
        proganim_add_keyframe(prog_anim, DEFAULT_KEYFRAME);
#endif

        /* set keyframe percentage, if defined */
        if(percentage != UNDEFINED_PERCENTAGE)
            prog_anim->keyframe[prog_anim->keyframe_count - 1].percentage = percentage;

        /* do not mix manually defined percentages with automatically defined percentages */
        if(prog_anim->keyframe_count >= 2) {
            int p1 = prog_anim->keyframe[prog_anim->keyframe_count - 1].percentage;
            int p2 = prog_anim->keyframe[prog_anim->keyframe_count - 2].percentage;

            if((p1 == UNDEFINED_PERCENTAGE) ^ (p2 == UNDEFINED_PERCENTAGE))
                nanoparser_crash(stmt, "Specify all keyframe percentages or specify none. Do not mix manually defined percentages with automatically defined percentages.");
        }

        /* declare keyframes in increasing order of percentages */
        if(prog_anim->keyframe_count >= 2) {
            int p1 = prog_anim->keyframe[prog_anim->keyframe_count - 1].percentage;
            int p2 = prog_anim->keyframe[prog_anim->keyframe_count - 2].percentage;

            if(p1 != UNDEFINED_PERCENTAGE && p2 != UNDEFINED_PERCENTAGE) {
                if(p1 < p2)
                    nanoparser_crash(stmt, "Keyframes must be specified in increasing order of percentages.");
            }
        }

        /* traverse the keyframe block */
        nanoparser_traverse_program_ex(
            nanoparser_get_program(block),
            &(prog_anim->keyframe[prog_anim->keyframe_count - 1]),
            traverse_keyframe
        );
    }
    else
        nanoparser_crash(stmt, "Unknown identifier \"%s\"", identifier);

    return 0;
}






/*
 *
 * private
 *
 */


/*
 * proganim_add_keyframe()
 * Add a keyframe to a programmatic animation
 */
void proganim_add_keyframe(proganim_t* prog_anim, proganim_keyframe_t keyframe)
{
    int last = prog_anim->keyframe_count++;
    prog_anim->keyframe = reallocx(prog_anim->keyframe, sizeof(proganim_keyframe_t) * prog_anim->keyframe_count);
    prog_anim->keyframe[last] = keyframe;
}


/*
 * normalized_percentage()
 * Input: a->percentage <= percentage <= b->percentage
 * Output: input percentage normalized to [0,1]
 */
float normalized_percentage(float percentage, const proganim_keyframe_t* a, const proganim_keyframe_t* b)
{
    /* we assume that a->percentage <= b->percentage */
    if(a->percentage == b->percentage) {
        return 1.0f; /* prioritize b. If a and b have the same percentage, this is probably intended by the user. */
        /*return 0.5f;*/
    }

    percentage = (percentage - (float)a->percentage * 0.01f) / ((float)(b->percentage - a->percentage) * 0.01f);
    return clip01(percentage);
}


/*
 * compare_keyframes()
 * Sorting function
 */
int compare_keyframes(const void* a, const void* b)
{
    const proganim_keyframe_t* keyframe_a = (const proganim_keyframe_t*)a;
    const proganim_keyframe_t* keyframe_b = (const proganim_keyframe_t*)b;

    return keyframe_a->percentage - keyframe_b->percentage;
}


/*
 * traverse_keyframe()
 * Traverse the attributes of a keyframe of a programmatic animation
 */
int traverse_keyframe(const parsetree_statement_t* stmt, void* context)
{
    const char* identifier;
    const parsetree_parameter_t* param_list;
    const parsetree_parameter_t* p1;
    const parsetree_parameter_t* p2;
    proganim_keyframe_t* keyframe = (proganim_keyframe_t*)context;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "translation") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "translation receives two numbers: xpos, ypos");
        nanoparser_expect_string(p2, "translation receives two numbers: xpos, ypos");
        keyframe->translation.x = (float)atof(nanoparser_get_string(p1));
        keyframe->translation.y = (float)atof(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "rotation") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "rotation receives a number: degrees");
        keyframe->rotation = (float)atof(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "scale") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "scale receives two numbers: xscale, yscale");
        nanoparser_expect_string(p2, "scale receives two numbers: xscale, yscale");
        keyframe->scale.x = (float)atof(nanoparser_get_string(p1));
        keyframe->scale.y = (float)atof(nanoparser_get_string(p2));
    }
    else if(str_icmp(identifier, "opacity") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "opacity receives a number: percentage");
        keyframe->opacity = parse_percentage(p1);
    }
    else
        nanoparser_crash(stmt, "Unknown identifier \"%s\"", identifier);

    return 0;
}


/*
 * parse_percentage()
 * Parse a percentage string
 */
int parse_percentage(const parsetree_parameter_t* param)
{
    const parsetree_statement_t* stmt = nanoparser_get_statement(param);
    const char* str = nanoparser_get_string(param);
    int len = strlen(str);

    /* match percentage: /^\d\d?\d?%$/ */
    if(len < 2 || len > 4 || !str_endswith(str, "%") || strspn(str, "0123456789") != len - 1)
        nanoparser_crash(stmt, "Invalid keyframe percentage \"%s\"", str);

    /* copy to buf[] and remove '%' */
    char buf[5] = { 0 };
    str_cpy(buf, str, sizeof(buf));
    buf[len-1] = '\0'; /* surely 2 <= len <= 4 */

    /* convert to integer */
    return atoi(buf);
}


/*
 * parse_easing_function()
 * Parse an easing function
 */
proganim_easing_t parse_easing_function(const parsetree_parameter_t* param)
{
    const parsetree_statement_t* stmt = nanoparser_get_statement(param);
    const char* str = nanoparser_get_string(param);

    /* compute a hash instead? */
    if(str_icmp(str, "ease_in_out") == 0)
        return easing_inout_quadratic;
    else if(str_icmp(str, "ease_in") == 0)
        return easing_in_quadratic;
    else if(str_icmp(str, "ease_out") == 0)
        return easing_out_quadratic;
    else if(str_icmp(str, "linear") == 0)
        return easing_linear;

    /* error */
    nanoparser_crash(stmt, "Invalid easing function \"%s\"", str);
    return easing_linear;
}




/*
 *
 * easing functions
 *
 * these functions receive t in [0,1] as input (as well as an optional vector p of parameters)
 * and return y = f(t, p) in [-m,1+w] for some small m, w >= 0 as output (currently m = w = 0)
 *
 */

double easing_linear(double t, const double* p)
{
    return t;
}

double easing_in_quadratic(double t, const double* p)
{
    return t * t;
}

double easing_out_quadratic(double t, const double* p)
{
    double x = 1.0 - t;
    return 1.0 - x * x;
}

double easing_inout_quadratic(double t, const double* p)
{
    if(t <= 0.5)
        return 2.0 * t * t;

    double x = 2.0 - 2.0 * t;
    return 1.0 - 0.5 * x * x;
}