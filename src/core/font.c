/*
 * Open Surge Engine
 * font.c - font module
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

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "font.h"
#include "global.h"
#include "video.h"
#include "image.h"
#include "color.h"
#include "asset.h"
#include "lang.h"
#include "logfile.h"
#include "nanoparser.h"
#include "input.h"
#include "../util/stringutil.h"
#include "../util/hashtable.h"
#include "../util/darray.h"
#include "../util/point2d.h"
#include "../util/rect.h"
#include "../entities/player.h"
#include "../scenes/level.h"
#include "../third_party/utf8.h"

/* private stuff */
#define FONT_STACKCAPACITY          8        /* color stack capacity */
#define FONT_TEXTMAXSIZE            65536    /* maximum size for texts */
#define FONT_PATHMAX                1024     /* buffer size for multilingual paths */
#define FONT_BLANKSMAXSIZE          8192     /* max buffer size for find_blanks() */
#define FONT_COLORBREAKPOINT     ((char)0x2) /* a control character that delimits a change of color */

/* macros */
#define IS_VAR_ANYCHAR(c)           ((isalnum((unsigned char)(c))) || ((c) == '_'))
#define IS_VAR_1STCHAR(c)           ((isalpha((unsigned char)(c))) || ((c) == '_'))
#define IS_TAG_1STCHAR(c)           ((isalpha((unsigned char)(c))) || ((c) == '/'))

/* ------------------------------- */

/* callback table: used for variable/text interpolation */
typedef const char* (*fontcallback_t)();
static void callbacktable_init();
static void callbacktable_release();
static void callbacktable_add(const char* variable_name, fontcallback_t callback);
static fontcallback_t callbacktable_find(const char* variable_name);
HASHTABLE_GENERATE_CODE(fontcallback_t, NULL);
static HASHTABLE(fontcallback_t, callback_table);

/* ------------------------------- */

/* font scripts (nanoparser) */
static int traverse(const parsetree_statement_t* stmt);
static int traverse_block(const parsetree_statement_t* stmt, void* data);
static int traverse_bmp(const parsetree_statement_t* stmt, void* data);
static int traverse_bmp_char(const parsetree_statement_t* stmt, void* data);
static int traverse_ttf(const parsetree_statement_t* stmt, void* data);
static int dirfill(const char* vpath, void* param);

typedef struct charproperties_t charproperties_t;
struct charproperties_t {
    bool valid; /* whether this character is valid (exists) */
    struct { /* spritesheet info */
        int x, y, width, height;
    } source_rect;
    int index; /* index is such that keymap[index] == this_character (if this_character is not in keymap, index is -1) */
};

typedef struct fontscript_t fontscript_t;
struct fontscript_t {
    enum { FONTSCRIPTTYPE_TTF, FONTSCRIPTTYPE_BMP } type;
    union {
        /* bitmap */
        struct {
            char source_file[FONT_PATHMAX]; /* source file (relative file path) */
            int source_rect[4]; /* spritesheet rect: x, y, width, height */
            int spacing[2]; /* character spacing: x, y */
            charproperties_t chr[256]; /* properties of char x (0..255) */
        } bmp;

        /* true-type */
        struct {
            char source_file[FONT_PATHMAX]; /* source file (relative file path) */
            int size; /* font size */
            bool antialias; /* enable antialiasing? */
            bool shadow; /* enable shadow? */
        } ttf;
    } data;
};

/* ------------------------------- */

/* fontdrv_t: a font driver stores the attributes the font class (bmp, ttf) */
typedef struct fontdrv_t fontdrv_t;
struct fontdrv_t { /* abstract font: base class */
    void (*textout)(const fontdrv_t*,const char*,int,int,color_t); /* prints an unformatted line of text */
    int (*line_width)(const fontdrv_t*,const char*); /* width in pixels of an unformatted line of text */
    int (*line_height)(const fontdrv_t*); /* height in pixels of any line of text */
    const char* (*filepath)(const fontdrv_t*); /* relative path of the font */
    const image_t* (*image)(const fontdrv_t*); /* image atlas (if any) */
    void (*release)(fontdrv_t*); /* release the fontdrv_t */
};
static fontdrv_t* fontdrv_bmp_new(const char* source_file, charproperties_t chr[], int spacing[2]);
static fontdrv_t* fontdrv_ttf_new(const char* source_file, int size, bool antialias, bool shadow);

typedef struct fontdrv_bmp_t fontdrv_bmp_t;
struct fontdrv_bmp_t { /* bitmap font */
    fontdrv_t base;
    const image_t* atlas; /* image atlas */
    image_t* glyph[256]; /* glyph indexed by codepoint */
    v2d_t spacing; /* character spacing */
    int line_height; /* max({ image_height(glyph[j]) | j >= 0 }) */
    char* filepath; /* relative path */
};
static void fontdrv_bmp_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color);
static int fontdrv_bmp_linewidth(const fontdrv_t* fnt, const char* text);
static int fontdrv_bmp_lineheight(const fontdrv_t* fnt);
static const char* fontdrv_bmp_filepath(const fontdrv_t* fnt);
static const image_t* fontdrv_bmp_image(const fontdrv_t* fnt);
static void fontdrv_bmp_release(fontdrv_t* fnt);
static inline const image_t* find_bmp_glyph(const fontdrv_bmp_t* f, uint32_t codepoint);

typedef struct fontdrv_ttf_t fontdrv_ttf_t;
struct fontdrv_ttf_t { /* truetype font */
    fontdrv_t base;
    ALLEGRO_FONT* font; /* TrueType font */
    int size; /* font size */
    bool antialias; /* enable antialiasing? */
    bool shadow; /* enable shadow? */
    int line_height; /* line height */
    char* filepath; /* relative path */
};
static void fontdrv_ttf_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color);
static int fontdrv_ttf_linewidth(const fontdrv_t* fnt, const char* text);
static int fontdrv_ttf_lineheight(const fontdrv_t* fnt);
static const char* fontdrv_ttf_filepath(const fontdrv_t* fnt);
static const image_t* fontdrv_ttf_image(const fontdrv_t* fnt);
static void fontdrv_ttf_release(fontdrv_t* fnt);

/* ------------------------------- */

/* list of fontdrv_t */
typedef struct fontdrv_list_t fontdrv_list_t;
struct fontdrv_list_t {
    char* name;
    fontdrv_t* data;
    fontdrv_list_t* next;
};

static fontdrv_list_t* fontdrv_list = NULL;
static void fontdrv_list_init();
static void fontdrv_list_release();
static void fontdrv_list_add(const char* name, fontdrv_t* drv);
static fontdrv_t* fontdrv_list_find(const char* name);
static fontdrv_t* fontdrv_list_find_ex(const char* name, const char* lang_id);

/* ------------------------------- */

/* font arguments: for a safe printf-like alternative (when dealing with user-provided format strings) */
#define FONTARGS_MAX 8 /* can go up to 9 in the current expand algorithm */
typedef char* fontargs_t[FONTARGS_MAX];

/* ------------------------------- */

/* preprocessed font text */
typedef struct fonttext_t fonttext_t;
struct fonttext_t
{
    /* the text is split into single-line, single-color segments */
    DARRAY(const char*, text_segment); /* preprocessed text segment */
    DARRAY(color_t, color); /* color of each segment */
    DARRAY(point2d_t, offset); /* (x,y) offset to be applied before each segment is rendered */
    DARRAY(v2d_t, size); /* the size in pixels of each segment */

    /* helpers */
    DARRAY(color_t, color_sequence); /* auxiliary array */
    DARRAY(int, line_width); /* the width in pixels of each line */
    DARRAY(char, buffer); /* string buffer */

    /* misc */
    bool is_dirty; /* do we need to preprocess the text? */
    v2d_t total_size; /* total size of the text, in pixels */
};

static void preprocess_expand(char* dest, char* tmp, size_t dest_size, fontargs_t args);
static char* preprocess_substring(char* text, int index_of_first_char, int max_length);
static void preprocess_colors(fonttext_t* out, const char* text);
static void preprocess_wordwrap(fonttext_t* out, const fontdrv_t* drv, int max_width);
static void preprocess_split(fonttext_t* out, const fontdrv_t* drv, fontalign_t align);
static void preprocess_text(fonttext_t* out, const fontdrv_t* drv, const char* text, int max_width, fontalign_t align, fontargs_t argument, int index_of_first_char, int max_length);
static void preprocess(font_t* f);

/* ------------------------------- */

/* font struct: this struct is used by the external world */
struct font_t {
    fontdrv_t* drv; /* font driver */
    char* text; /* unprocessed text */
    v2d_t position; /* position */
    int max_width; /* width (in pixels) for wordwrap */
    bool visible; /* is this font visible? */
    int index_of_first_char, max_length; /* substring (deprecated) */
    fontargs_t argument; /* text arguments: $1, $2 ... ${FONTARGS_MAX} */
    fontalign_t align; /* alignment */
    fonttext_t preprocessed_text; /* preprocessed text */
    char* lang_id; /* current language ID (multilingual support) */
    char* name; /* font name (not language specific) */
};

/* misc */
static void register_predefined_vars();
static const char* read_variable(const char* key, void* data);
static int expand_vars(char* dest, const char* src, size_t dest_size, const char* (*callback)(const char*,void*), void* data);
static inline bool has_vars_to_expand(const char* str);
static char* convert_to_ascii(char* str);
static char* find_wordwrap(const fontdrv_t* drv, char* text, int max_width);
static int find_blanks(const char* text, int blank[], size_t size);
static char* tagged_text_offset(char* text, int charnum);
static char* join_names(const char* name, const char* lang_id);
static bool must_refresh_driver(const font_t* fnt);
static void refresh_driver(font_t* fnt);
static inline bool has_loaded_ttf(const fontdrv_ttf_t* f);
static void load_ttf(fontdrv_ttf_t* f);
static void unload_ttf(fontdrv_ttf_t* f);

/*
 * font_init()
 * Initializes the font module
 */
void font_init()
{
    /* initializing Allegro's TTF addon */
    if(!al_is_ttf_addon_initialized()) {
        if(!al_init_ttf_addon())
            fatal_error("Can't initialize Allegro's TTF addon");
    }

    /* basic initialization */
    fontdrv_list_init();

    /* reading the font scripts */
    logfile_message("Loading fonts...");
    asset_foreach_file("fonts", ".fnt", dirfill, NULL, true);
    logfile_message("All fonts have been loaded.");

    /* initializing the font callback table */
    callbacktable_init();

    /* register predefined vars */
    register_predefined_vars();
}


/*
 * font_release()
 * Releases the font module
 */
void font_release()
{
    logfile_message("Unloading font callback table...");
    callbacktable_release();

    logfile_message("Unloading font scripts...");
    fontdrv_list_release();
}


/*
 * font_register_variable()
 * Variable/text interpolation.
 *
 * Example: font_register_variable("LEVEL_NAME", level_name)
 * will replace every occurrence of $LEVEL_NAME when
 * rendering any font by the result of level_name()
 *
 * Call this AFTER font_init()
 */
void font_register_variable(const char* variable_name, const char* (*callback)())
{
    if(callback_table != NULL)
        callbacktable_add(variable_name, (fontcallback_t)callback);
}


/*
 * font_exists()
 * Checks if a font script (.fnt) of the given name exists
 */
bool font_exists(const char* font_name)
{
    return fontdrv_list_find(font_name) != NULL;
}


/*
 * font_create()
 * Creates a new font object
 */
font_t* font_create(const char* font_name)
{
    int i;
    font_t* f = mallocx(sizeof *f);

    f->text = str_dup("");
    f->max_width = 0;
    f->visible = true;
    f->position = v2d_new(0, 0);
    f->index_of_first_char = -1;
    f->max_length = -1;
    f->align = FONTALIGN_LEFT;
    f->name = str_dup(font_name);
    f->lang_id = str_dup(lang_getid());

    f->drv = fontdrv_list_find_ex(f->name, f->lang_id);
    if(f->drv == NULL)
        fatal_error("Can't find font \"%s\"", f->name);

    for(i=0; i<FONTARGS_MAX; i++)
        f->argument[i] = NULL;

    darray_init_ex(f->preprocessed_text.text_segment, 16);
    darray_init_ex(f->preprocessed_text.color, 16);
    darray_init_ex(f->preprocessed_text.offset, 16);
    darray_init_ex(f->preprocessed_text.size, 16);
    darray_init_ex(f->preprocessed_text.color_sequence, 16);
    darray_init_ex(f->preprocessed_text.line_width, 4);
    darray_init_ex(f->preprocessed_text.buffer, 64);
    f->preprocessed_text.is_dirty = true;
    f->preprocessed_text.total_size = v2d_new(0, 0);

    return f;
}




/*
 * font_destroy()
 * Destroys an existing font object
 */
void font_destroy(font_t* f)
{
    darray_release(f->preprocessed_text.buffer);
    darray_release(f->preprocessed_text.line_width);
    darray_release(f->preprocessed_text.color_sequence);
    darray_release(f->preprocessed_text.size);
    darray_release(f->preprocessed_text.offset);
    darray_release(f->preprocessed_text.color);
    darray_release(f->preprocessed_text.text_segment);

    for(int i = 0; i < FONTARGS_MAX; i++) {
        if(f->argument[i] != NULL)
            free(f->argument[i]);
    }

    free(f->lang_id);
    free(f->name);
    free(f->text);
    free(f);
}




/*
 * font_set_text()
 * Sets the text... printf style. Be careful with unsanitized format strings.
 */
void font_set_text(font_t* f, const char* fmt, ...)
{
    static char buf[FONT_TEXTMAXSIZE];
    va_list args;

    /* printf BEFORE expanding any $VARIABLES */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* update text */
    if(f->text != NULL) {
        /* no change? */
        if(0 == strcmp(f->text, buf))
            return;

        /* release old text */
        free(f->text);
    }
    f->text = str_dup(buf);

    /* preprocess text */
    f->preprocessed_text.is_dirty = true;
}


/*
 * font_set_textarguments()
 * Kinda like a safe, sanitazed printf-format stuff.
 * pass <amount> of const char*'s; they'll be stored
 * in $1, $2, ... up to ${FONTARGS_MAX}
 */
void font_set_textarguments(font_t* f, int amount, ...)
{
    int i, m = min(FONTARGS_MAX, amount);
    va_list ap;

    /* update arguments */
    va_start(ap, amount);
    for(i = 0; i < m; i++) {
        if(f->argument[i] != NULL)
            free(f->argument[i]);
        f->argument[i] = str_dup(va_arg(ap, const char*));
    }
    va_end(ap);

    /* preprocess text */
    f->preprocessed_text.is_dirty = true;
}


/*
 * font_set_textargumentsv()
 * An alternative to font_set_textarguments().
 * Pass an array of strings instead of a variadic argument
 */
void font_set_textargumentsv(font_t* f, int argc, const char** argv)
{
    int m = min(FONTARGS_MAX, argc);

    /* update arguments */
    for(int i = 0; i < m; i++) {
        if(f->argument[i] != NULL)
            free(f->argument[i]);
        f->argument[i] = str_dup(argv[i]);
    }

    /* preprocess text */
    f->preprocessed_text.is_dirty = true;
}



/*
 * font_get_text()
 * Returns the text
 */
const char* font_get_text(const font_t* f)
{
    return f->text != NULL ? f->text : "";
}



/*
 * font_set_width()
 * Sets the width of the font (useful if
 * you want wordwrap). If w == 0, then
 * there's no wordwrap.
 */
void font_set_width(font_t* f, int w)
{
    bool is_dirty = false;

    /* validate */
    w = max(0, w);
    is_dirty = (f->max_width != w);

    /* update width */
    f->max_width = w;

    /* preprocess text */
    if(!f->preprocessed_text.is_dirty)
        f->preprocessed_text.is_dirty = is_dirty;
}


/*
 * font_render()
 * Renders the text
 */
void font_render(font_t* f, v2d_t camera_position)
{
    /* not visible? */
    if(!f->visible)
        return;

    /* multilingual support */
    if(must_refresh_driver(f))
        refresh_driver(f);

    /* need to preprocess the text? */
    if(f->preprocessed_text.is_dirty)
        preprocess(f);

    /* compute the position of the text in screen space */
    v2d_t half_screen_size = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, half_screen_size);
    v2d_t position = v2d_subtract(f->position, topleft);

    /* render the text */
    image_hold_drawing(true);
    do {
        /* boundaries of the drawing target */
        const image_t* target = image_drawing_target();
        int target_width = image_width(target);
        int target_height = image_height(target);
        rect_t target_rect = rect_new(0, 0, target_width, target_height);

        /* clip out the entire text if possible */
        point2d_t initial_position = point2d_new(floorf(position.x + 0.5f), floorf(position.y + 0.5f));
        v2d_t total_size = f->preprocessed_text.total_size;
        rect_t bounding_box = rect_new(initial_position.x, initial_position.y, total_size.x, total_size.y);
        if(!rect_overlaps(target_rect, bounding_box))
            break;

        /* for each preprocessed text segment */
        for(int i = 0; i < darray_length(f->preprocessed_text.text_segment); i++) {
            const char* text_segment = f->preprocessed_text.text_segment[i];
            color_t color = f->preprocessed_text.color[i];
            point2d_t offset = f->preprocessed_text.offset[i];
            v2d_t size = f->preprocessed_text.size[i];

            /* skip empty segments, as in "</color>[__empty__]\n" */
            if(*text_segment == '\0')
                continue;

            /* find the position of the segment in screen space */
            point2d_t segment_position = point2d_add(initial_position, offset);
            rect_t segment_rect = rect_new(segment_position.x, segment_position.y, size.x, size.y);

            /* clip out the segment if possible */
            if(segment_rect.y >= target_rect.height) /* exit early */
                break;
            if(!rect_overlaps(target_rect, segment_rect))
                continue;

            /* render the segment */
            f->drv->textout(f->drv, text_segment, segment_position.x, segment_position.y, color);
        }
    } while(0);
    image_hold_drawing(false);
}


/*
 * font_get_textsize()
 * Returns the size (in pixels) of the rendered text
 * (considering wordwrap)
 */
v2d_t font_get_textsize(const font_t* f)
{
    /* preprocess the text */
    if(f->preprocessed_text.is_dirty)
        preprocess((font_t*)f);

    /* return the total size */
    return f->preprocessed_text.total_size;
}


/*
 * font_set_position()
 * Sets the position of the font
 */
void font_set_position(font_t* f, v2d_t position)
{
    f->position = position;
}


/*
 * font_get_position()
 * Gets the position of the font
 */
v2d_t font_get_position(const font_t* f)
{
    return f->position;
}


/*
 * font_is_visible()
 * Is the font visible?
 */
bool font_is_visible(const font_t* f)
{
    return f->visible;
}


/*
 * font_set_visible()
 * Sets the visibility of the font
 */
void font_set_visible(font_t* f, bool is_visible)
{
    f->visible = is_visible;
}


/*
 * font_use_substring()
 * Since fonts may have color tags, variables, etc. , use this
 * to display a substring of the font (not the whole text)
 */
void font_use_substring(font_t* f, int index_of_first_char, int max_length)
{
    /*
     *
     * THIS FUNCTION IS OBSOLETE
     * TODO: remove?
     *
     */
    bool is_dirty = false;

    /* validate */
    index_of_first_char = max(-1, index_of_first_char);
    max_length = max(-1, max_length);
    is_dirty = (f->index_of_first_char != index_of_first_char) || (f->max_length != max_length);

    /* update substring */
    f->index_of_first_char = index_of_first_char;
    f->max_length = max_length;

    /* preprocess text */
    if(!f->preprocessed_text.is_dirty)
        f->preprocessed_text.is_dirty = is_dirty;
}


/*
 * font_get_align()
 * Get the current alignment of the text
 */
fontalign_t font_get_align(const font_t* f)
{
    return f->align;
}

/*
 * font_set_align()
 * Set the alignment of the font
 */
void font_set_align(font_t* f, fontalign_t align)
{
    bool is_dirty = (f->align != align);

    /* adjust alignment */
    f->align = align;

    /* preprocess text */
    if(!f->preprocessed_text.is_dirty)
        f->preprocessed_text.is_dirty = is_dirty;
}

/*
 * font_get_maxlength()
 * Get the maximum number of characters that can be printed, ignoring color tags and blanks
 */
int font_get_maxlength(const font_t* f)
{
    return f->max_length;
}

/*
 * font_set_maxlength()
 * Set the maximum number of characters that can be printed, ignoring color tags and blanks
 */
void font_set_maxlength(font_t* f, int max_length)
{
    bool is_dirty = false;

    /* validate */
    max_length = max(-1, max_length);
    is_dirty = (f->max_length != max_length);

    /* adjust length */
    f->max_length = max_length;

    /* preprocess text */
    if(!f->preprocessed_text.is_dirty)
        f->preprocessed_text.is_dirty = is_dirty;
}

/*
 * font_get_filepath()
 * Get the relative path of the file (image, truetype font...) that originates this font
 */
const char* font_get_filepath(const font_t* f)
{
    return f->drv->filepath(f->drv);
}

/*
 * font_get_image()
 * Get the image atlas if it's a bitmap font;
 * otherwise NULL is returned
 */
const image_t* font_get_image(const font_t* f)
{
    return f->drv->image(f->drv);
}



/* ------------------------------------------------- */
/* private utilities */
/* ------------------------------------------------- */

/* returns a static char* (case insensitive search) */
const char* read_variable(const char* key, void* data)
{
    const char* value = NULL;

    if(key[0] >= '1' && key[0] <= '9') {
        /* read $1, $2 ... $9 */
        const char** args = (const char**)data;
        int index = key[0] - '1';

        if(index >= 0 && index < FONTARGS_MAX)
            value = args[index]; /* may be NULL */
    }
    else {
        /* read $IDENTIFIER */
        fontcallback_t fun = callbacktable_find(key);
        static char buf[1024];

        if(fun != NULL)
            value = fun();
        else
            value = lang_getstring(key, buf, sizeof(buf));
    }

    return value != NULL ? value : "null";
}

/*

expands the variables, e.g.,

   "Welcome to $LEVEL_NAME"
            ... becomes ...
   "Welcome to Sunshine Paradise"

*/
int expand_vars(char* dest, const char* src, size_t dest_size, const char* (*callback)(const char*,void*), void* data)
{
    char acc[256];
    const int accsize = sizeof(acc) - 1;
    enum { COPYING, ACCUMULATING_DIGIT, ACCUMULATING_IDENTIFIER, ACCUMULATING_EXPRESSION } state = COPYING;
    int curly_counter = 0, number_of_substitutions = 0;
    int m = (int)dest_size - 1;
    int a, i, j;

    #define APPEND(str) do { \
        const char* p = (str); \
        while(*p && j < m) \
            dest[j++] = *(p++); \
        number_of_substitutions++; \
    } while(0);

    for(i = j = 0; '\0' != src[i] && j < m; i++) {
        char curr_char = src[i], next_char = src[i+1];

        switch(state) {
            /* copy char */
            case COPYING: {
                if(curr_char == '$') {
                    a = 0; /* initialize the accumulator */

                    if(next_char == '{') {
                        state = ACCUMULATING_EXPRESSION;
                        curly_counter = 0; /* reset curly counter */
                        i++; /* skip this curly brace */
                        break;
                    }
                    else if(next_char >= '1' && next_char <= '9') {
                        state = ACCUMULATING_DIGIT;
                        break;
                    }
                    else if(isalpha(next_char) || next_char == '_') {
                        state = ACCUMULATING_IDENTIFIER;
                        break;
                    }
                }

                /* copy 'til the end */
                dest[j++] = curr_char;
                break;
            }



            /* match $1, $2 ... $9 */
            case ACCUMULATING_DIGIT: {
                bool match = (curr_char >= '1' && curr_char <= '9'); /* true, redundant */

                if(match) {
                    acc[0] = curr_char;
                    acc[1] = '\0';
                }

                APPEND(callback(acc, data));
                state = COPYING;
                break;
            }



            /* match $IDENTIFIER */
            case ACCUMULATING_IDENTIFIER: {
                bool match = (isalnum(curr_char) || curr_char == '_');
                bool last_char = ('\0' == next_char);

                if(match) {
                    if(a < accsize)
                        acc[a++] = curr_char;
                }

                if(last_char || !match) {
                    acc[a] = '\0';
                    if(!match)
                        i--; /* put back */

                    APPEND(callback(acc, data));
                    state = COPYING;
                }

                break;
            }



            /* match ${EXPRESSION} */
            case ACCUMULATING_EXPRESSION: {
                bool match = ((curr_char != '}' || curly_counter > 0) && curr_char != '\0');
                bool last_char = ('\0' == next_char);

                if(match) {
                    if(a < accsize)
                        acc[a++] = curr_char;

                    if(src[i] == '{')
                        ++curly_counter;
                    else if(src[i] == '}')
                        --curly_counter;
                }

                if(last_char || !match) {
                    char expr[256];
                    acc[a] = '\0';

                    number_of_substitutions += expand_vars(expr, acc, sizeof(expr), callback, data);
                    APPEND(callback(expr, data));
                    state = COPYING;
                }

                break;
            }
        }
    }

    dest[j] = '\0';
    return number_of_substitutions;
}

/* does the string have variables or patterns ($1, $2...) to expand? */
bool has_vars_to_expand(const char* str)
{
    #define MATCH1(c) (isalnum(c) || (c) == '_' || (c) == '{')

    for(const char *p = str; *p; p++) {
        if(*p == '$' && MATCH1(p[1]))
            return true;
    }

    return false;
}

/* convert to ascii */
char* convert_to_ascii(char* str)
{
    char *p, *q;

    for(q = p = str; *p != '\0'; p++) {
        if(!(*p & 0x80))
            *(q++) = *p;
    }

    *q = '\0';
    return str;
}

/* find the next point where a wordwrap should be placed */
char* find_wordwrap(const fontdrv_t* drv, char* text, int max_width)
{
    if(max_width > 0) {
        static int blank[FONT_BLANKSMAXSIZE]; /* WARNING: using a fixed-length array. We just process single-line of text, though. */
        int blanks = find_blanks(text, blank, sizeof(blank) / sizeof(int));
        int m, l = 0, r = blanks - 1;
        int best_m = 0, width;
        char *wordwrap, chr;

        /* check if there is no wordwrap */
        if(blanks == 0 || drv->line_width(drv, text) <= max_width)
            return NULL;

        /*
            the wordwrap problem:

            given a text (string), a max_width (int),
            a vector of blank indexes (int[]) and
            a width function (string -> int) ...

            find max j such that
            width(text[0 .. blank[j]-1]) <= max_width
       */
        while(l <= r) {
            m = (l + r) / 2;

            /* compute the width of text[0 .. blank[m]-1] */
            chr = text[blank[m]]; text[blank[m]] = 0;
            width = drv->line_width(drv, text);
            text[blank[m]] = chr;

            if(width > max_width) {
                /* the text is too large */
                r = m-1;
            }
            else if(width <= max_width) {
                /* the text fits the space */
                best_m = m;
                l = m+1;
            }
        }

        /* skip spaces */
        /*for(wordwrap = text + blank[best_m] + 1; *wordwrap && isspace(*wordwrap); wordwrap++);*/

        /* skip spaces, except the last one (which we'll return) */
        for(wordwrap = text + blank[best_m]; *wordwrap && isspace(wordwrap[1]); wordwrap++);
        if(*wordwrap == '\0')
            return NULL;

        /* done */
        return wordwrap;
    }
    else
        return NULL; /* no wordwrap */
}

/* find all indexes of the text containing blank spaces */
int find_blanks(const char* text, int blank[], size_t size)
{
    const char* p;
    int n = 0;

    for(p = text; *p; p++) {
        if(isspace(*p)) {
            if(n < size)
                blank[n++] = p - text;
            else
                break;
        }
    }

    return n; /* returns the number of blank spaces */
}

/* character number to byte offset in a tagged text
   returns NULL if there is no such character */
char* tagged_text_offset(char* text, int charnum)
{
    uint32_t chr;
    size_t i, prev_i;
    bool tag = false;

    /* validate */
    if(charnum < 0)
        charnum = 0;

    /* search */
    for(prev_i = i = 0; (chr = u8_nextchar(text, &i)) != 0; prev_i = i) {
        if(tag)
            tag = (chr != '>');
        else if(chr == '<' && IS_TAG_1STCHAR(text[i]))
            tag = true;
        else if(!isspace(chr) && chr != FONT_COLORBREAKPOINT && charnum-- == 0)
            return text + prev_i;
    }

    /* character not found */
    return NULL;
}

/* true if we must refresh the font driver
   due to a language change */
bool must_refresh_driver(const font_t* fnt)
{
    return (0 != strcmp(fnt->lang_id, lang_getid()));
}

/* refresh the font driver to match
   the correct language */
void refresh_driver(font_t* fnt)
{
    /* update fnt->lang_id to be the current language ID */
    free(fnt->lang_id);
    fnt->lang_id = str_dup(lang_getid());

    /* update the driver */
    fnt->drv = fontdrv_list_find_ex(fnt->name, fnt->lang_id);
    if(fnt->drv == NULL)
        fatal_error("Can't find font \"%s\"", fnt->name);

    /* preprocess the text */
    fnt->preprocessed_text.is_dirty = true;
}

/* join_names(): joins name and lang_id into a single string
   you must free() the returned pointer after usage */
char* join_names(const char* name, const char* lang_id)
{
    size_t size = (2 + strlen(name) + strlen(lang_id));
    char* str = mallocx(size * sizeof(*str));
    snprintf(str, size, "%s:%s", name, lang_id);
    return str;
}




/* ------------------------------------------------- */
/* text preprocessing */
/* ------------------------------------------------- */

/* expand variables */
void preprocess_expand(char* dest, char* tmp, size_t dest_size, fontargs_t args)
{
    const int MAX_PASSES = 3;

    /* expand variables */
    for(int k = 0; k < MAX_PASSES && has_vars_to_expand(dest); k++) {
        int len = strlen(dest);
        memcpy(tmp, dest, len+1);
        expand_vars(dest, tmp, dest_size, read_variable, (void*)args);
    }

    /* utf8 check */
    if(!u8_isvalid(dest, strlen(dest)))
        convert_to_ascii(dest);
}

/* extract substring (aware of blanks & tags); this may change the input text */
char* preprocess_substring(char* text, int index_of_first_char, int max_length)
{
    char* substring = text;

    if(index_of_first_char >= 0) {
        substring = tagged_text_offset(substring, index_of_first_char);
        if(substring == NULL)
            return text + strlen(text); /* empty string */
    }

    if(max_length >= 0) {
        char* p = tagged_text_offset(substring, max_length);
        if(p != NULL)
            *p = '\0';
    }

    return substring;
}

/* preprocess colors */
void preprocess_colors(fonttext_t* out, const char* text)
{
    const color_t DEFAULT_COLOR = color_rgb(255, 255, 255); /* we use multiplicative blending */
    color_t stack[FONT_STACKCAPACITY];
    int stack_top = 0;
    bool tag = false;

    /* initialize the string buffer */
    darray_clear(out->buffer);

    /* initialize the color stack */
    stack[0] = DEFAULT_COLOR;
    stack_top = 0;

    /* register the first color */
    darray_clear(out->color_sequence);
    darray_push(out->color_sequence, stack[0]);

    /* scan the string */
    for(const char* p = text; *p != '\0'; p++) {
        if(tag) {
            /* skip characters until we close the tag */
            tag = (*p != '>');
        }
        else if(*p == '<' && IS_TAG_1STCHAR(p[1])) {
            /* read tag */
            //const char* tag_name = text + i;
            const char* tag_name = p + 1;
            tag = true; /* we must skip the characters of the tag */

            /* is this a closing tag? */
            if(*tag_name == '/') {

                /* close a <color> tag - as in </color> */
                if(0 == strncmp(tag_name + 1, "color>", 6)) {
                    /* pop the stack */
                    color_t prev_color = stack[stack_top];
                    color_t next_color = stack_top > 0 ? stack[--stack_top] : stack[0];

                    /* register the color */
                    if(!color_equals(next_color, prev_color)) {
                        darray_push(out->color_sequence, next_color);
                        darray_push(out->buffer, FONT_COLORBREAKPOINT); /* not matched by isspace() */
                    }
                }

            }
            else {

                /* open a <color> tag - as in <color=ffffff> */
                if(0 == strncmp(tag_name, "color=", 6)) {
                    const char* color_code = tag_name + 6;
                    char hex_code[9] = { 0 };

                    /* skip a preceding '#', if it exists */
                    if(*color_code == '#')
                        color_code++;

                    /* read the color_code and store it into the hex_code */
                    for(int h = 0; h < 8 && isalnum(*color_code); h++)
                        hex_code[h] = *color_code++; /* up to 8 characters (RGBA in hex notation) */

                    /* push the color onto the stack */
                    color_t prev_color = stack[stack_top];
                    color_t next_color = color_hex(hex_code);
                    if(stack_top + 1 < FONT_STACKCAPACITY)
                        stack[++stack_top] = next_color;
                    else
                        next_color = prev_color; /* stack is full; ignore color */

                    /* register the color */
                    if(!color_equals(next_color, prev_color)) {
                        darray_push(out->color_sequence, next_color);
                        darray_push(out->buffer, FONT_COLORBREAKPOINT);
                    }
                }
            }
        }
        else {
            /* add character to the buffer */
            darray_push(out->buffer, *p);
        }
    }

    /* rtrim the string buffer */
    for(char* p = out->buffer + (darray_length(out->buffer) - 1); p > out->buffer && isspace(*p); p--)
        *p = '\0';

    /* complete the string buffer */
    darray_push(out->buffer, '\0');
}

/* preprocess wordwrap */
void preprocess_wordwrap(fonttext_t* out, const fontdrv_t* drv, int max_width)
{
    char* r;
    int line_width = 0;
    darray_clear(out->line_width);

    for(char* p = out->buffer, *q, *w ;;) {
        /* we must scan one line at a time */
        if(NULL != (q = strchr(p, '\n')))
            *q = '\0';

        /* now p is a single line of text */
        while(NULL != (w = find_wordwrap(drv, p, max_width))) {
            *w = '\0';

            for(r = w-1; r > p && isspace(*r); r--) *r = '\0'; /* rtrim(p) to correctly calculate the width of the line */
            line_width = drv->line_width(drv, p);
            darray_push(out->line_width, line_width);
            for(r = w-1; r > p && *r == '\0'; r--) *r = ' '; /* "undo" rtrim(p) */

            *w = '\n'; /* replace a space by a newline */
            p = w + 1;
        }

        /* compute the width of the remaining text of the current line */
        line_width = drv->line_width(drv, p);
        darray_push(out->line_width, line_width);

        /* this was the last line; no more line breaks nor wordwraps */
        if(q == NULL)
            break;

        /* put the newline back and continue */
        *q = '\n';
        p = q + 1;
    }
}

/* split the buffer into segments */
void preprocess_split(fonttext_t* out, const fontdrv_t* drv, fontalign_t align)
{
    const color_t DEFAULT_COLOR = color_rgb(255, 255, 255); /* we use multiplicative blending */
    const char* current_segment = out->buffer;
    color_t color = DEFAULT_COLOR;
    point2d_t offset = point2d_new(0, 0);
    int color_cursor = 0;
    int line_cursor = 0;
    int line_width = 0;
    int line_height = drv->line_height(drv);
    int accum_segment_width = 0;
    float alignment_multiplier = (
        (float)(align == FONTALIGN_CENTER) * 0.5f + (float)(align == FONTALIGN_RIGHT)
    );

    assertx(darray_length(out->line_width) > 0);
    assertx(darray_length(out->color_sequence) > 0);

    darray_clear(out->text_segment);
    darray_clear(out->color);
    darray_clear(out->offset);
    out->total_size = v2d_new(0, 0);

    color = out->color_sequence[0];
    line_width = out->line_width[0];
    offset.x = -line_width * alignment_multiplier;
    offset.y = 0;
    accum_segment_width = 0;

    for(char* p = out->buffer; p < out->buffer + darray_length(out->buffer); p++) {
        if(*p == FONT_COLORBREAKPOINT) {
            /* close segment */
            *p = '\0';

            /* compute the size of the segment */
            int segment_width = drv->line_width(drv, current_segment);
            int segment_height = line_height;
            v2d_t segment_size = v2d_new(segment_width, segment_height);

            /* add segment */
            darray_push(out->text_segment, current_segment);
            darray_push(out->color, color);
            darray_push(out->offset, offset);
            darray_push(out->size, segment_size);

            /* next offset */
            offset.x += segment_width;
            /*offset.y += 0;*/

            /* total size */
            accum_segment_width += segment_width;
            out->total_size.x = max(out->total_size.x, accum_segment_width);

            /* color change */
            if(color_cursor + 1 < darray_length(out->color_sequence)) /* we have to check; what if the control character is manually placed by the user? */
                color = out->color_sequence[++color_cursor];

            /* next segment */
            current_segment = p + 1;
        }
        else if(*p == '\n') {
            /* line break */
            if(line_cursor + 1 < darray_length(out->line_width))
                line_width = out->line_width[++line_cursor];

            /* close segment */
            *p = '\0';

            /* compute the size of the segment */
            int segment_width = drv->line_width(drv, current_segment);
            int segment_height = line_height;
            v2d_t segment_size = v2d_new(segment_width, segment_height);

            /* add segment */
            darray_push(out->text_segment, current_segment);
            darray_push(out->color, color);
            darray_push(out->offset, offset);
            darray_push(out->size, segment_size);

            /* next offset */
            offset.x = -line_width * alignment_multiplier;
            offset.y += line_height;

            /* total size */
            accum_segment_width = 0;
            out->total_size.y += line_height;

            /* next segment */
            current_segment = p + 1;
        }
        else if(*p == '\0') {
            /* end of string */

            /* close segment */
            /* *p = '\0'; */

            /* compute the size of the segment */
            int segment_width = drv->line_width(drv, current_segment);
            int segment_height = line_height;
            v2d_t segment_size = v2d_new(segment_width, segment_height);

            /* add segment */
            darray_push(out->text_segment, current_segment);
            darray_push(out->color, color);
            darray_push(out->offset, offset);
            darray_push(out->size, segment_size);

            /* total size */
            accum_segment_width += segment_width;
            out->total_size.x = max(out->total_size.x, accum_segment_width);
            out->total_size.y += line_height;

            /* exit */
            break;
        }
    }
}

/* preprocess a text for rendering */
void preprocess_text(fonttext_t* out, const fontdrv_t* drv, const char* text, int max_width, fontalign_t align, fontargs_t args, int index_of_first_char, int max_length)
{
    static char buf[FONT_TEXTMAXSIZE], tmp[FONT_TEXTMAXSIZE];
    char* substr;

    /* reset arrays */
    darray_clear(out->text_segment);
    darray_clear(out->color);
    darray_clear(out->offset);
    darray_clear(out->line_width);
    darray_clear(out->color_sequence);
    darray_clear(out->buffer);

    /* copy text to a temporary buffer */
    str_cpy(buf, text, sizeof(buf));

    /* expand variables */
    preprocess_expand(buf, tmp, sizeof(buf), args);

    /* preprocess substring */
    substr = preprocess_substring(buf, index_of_first_char, max_length);

    /* preprocess colors */
    preprocess_colors(out, substr);

    /* set wordwrap points */
    preprocess_wordwrap(out, drv, max_width);

    /* split the text into segments */
    preprocess_split(out, drv, align);

#if 0
    /* test */
    for(int i = 0; i < darray_length(out->text_segment); i++) {
        static char hex[32];
        const char* text = out->text_segment[i];
        int x = out->offset[i].x;
        int y = out->offset[i].y;
        color_to_hex(out->color[i], hex, sizeof hex);
        printf("i=% 3d, color=%6s, offset=% 3d,% 3d\t\t: \"%s\"\n", i, hex, x, y, text);
    }
#endif
}

/* preprocess the font before the rendering takes place */
void preprocess(font_t* f)
{
    /* no need to do anything */
    if(!f->preprocessed_text.is_dirty)
        return;

    /* preprocess the font and clear up the is_dirty flag */
    preprocess_text(&f->preprocessed_text, f->drv, f->text, f->max_width, f->align, f->argument, f->index_of_first_char, f->max_length);
    f->preprocessed_text.is_dirty = false;
}

/* ------------------------------------------------- */
/* read the font scripts */
/* ------------------------------------------------- */

int traverse(const parsetree_statement_t* stmt)
{
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "font") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        fontscript_t header;
        fontdrv_t* drv;
        char* name;

        /* read the data */
        nanoparser_expect_string(p1, "Font script error: font name is expected");
        name = str_dup(nanoparser_get_string(p1));
        logfile_message("Loading font script \"%s\" defined in \"%s\" near line %d", name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        /* is this a language-specific font? */
        if(nanoparser_get_number_of_parameters(param_list) > 2) {
            const parsetree_parameter_t* opt_p2 = nanoparser_get_nth_parameter(param_list, 2);
            const parsetree_parameter_t* opt_p3 = nanoparser_get_nth_parameter(param_list, 3);
            const char* lang_id = NULL;
            char* lang_specific_name = NULL;

            /* read the language ID */
            nanoparser_expect_string(opt_p2, "Font script error: expected \"for\"");
            if(str_icmp(nanoparser_get_string(opt_p2), "for") != 0)
                fatal_error("Font script error: invalid format at \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

            nanoparser_expect_string(opt_p3, "Font script error: language ID is expected");
            lang_id = nanoparser_get_string(opt_p3);

            /* update variables */
            lang_specific_name = join_names(name, lang_id);
            free(name);
            name = lang_specific_name;
            p2 = nanoparser_get_nth_parameter(param_list, 4); /* block */
        }

        /* duplicate font? */
        if(NULL != fontdrv_list_find(name)) {
            /* fail silently */
            logfile_message("WARNING: can't redefine font \"%s\" in \"%s\" near line %d", name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
            return 0;
        }

        /* read the block */
        nanoparser_expect_program(p2, "Font script error: font block is expected after the font name");
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)(&header), traverse_block);

        /* create the fontdrv_t */
        switch(header.type) {
        case FONTSCRIPTTYPE_TTF:
            drv = fontdrv_ttf_new(
                header.data.ttf.source_file,
                header.data.ttf.size,
                header.data.ttf.antialias,
                header.data.ttf.shadow
            );
            break;

        case FONTSCRIPTTYPE_BMP:
            drv = fontdrv_bmp_new(
                header.data.bmp.source_file,
                header.data.bmp.chr,
                header.data.bmp.spacing
            );
            break;

        default:
            drv = NULL;
            fatal_error("Font script error: unknown font type");
            break;
        }

        /* register the driver */
        if(drv != NULL)
            fontdrv_list_add(name, drv);

        /* done */
        free(name);
    }
    else
        fatal_error("Font script error: unknown identifier \"%s\"\nin \"%s\" near line %d", id, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int traverse_block(const parsetree_statement_t* stmt, void* data)
{
    fontscript_t* header = (fontscript_t*)data;
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);

    nanoparser_expect_program(p1, "Font script error: block data is expected after the type of the font");

    if(str_icmp(id, "truetype") == 0) {
        /* default configuration */
        header->type = FONTSCRIPTTYPE_TTF;
        strcpy(header->data.ttf.source_file, "");
        header->data.ttf.size = 12;
        header->data.ttf.antialias = false;
        header->data.ttf.shadow = false;

        nanoparser_traverse_program_ex(nanoparser_get_program(p1), data, traverse_ttf);
    }
    else if(str_icmp(id, "bitmap") == 0) {
        /* default configuration */
        int i, n = sizeof(header->data.bmp.chr) / sizeof(charproperties_t);

        header->type = FONTSCRIPTTYPE_BMP;
        strcpy(header->data.bmp.source_file, "");
        header->data.bmp.spacing[0] = 1; /* default spacing */
        header->data.bmp.spacing[1] = 1;
        for(i = 0; i < n; i++) {
            /* initialize all characters to: unspecified */
            header->data.bmp.chr[i].valid = false;
            header->data.bmp.chr[i].index = -1;
            header->data.bmp.chr[i].source_rect.x = 0;
            header->data.bmp.chr[i].source_rect.y = 0;
            header->data.bmp.chr[i].source_rect.width = 0;
            header->data.bmp.chr[i].source_rect.height = 0;
        }

        nanoparser_traverse_program_ex(nanoparser_get_program(p1), data, traverse_bmp);

        for(i = 0; i < n; i++) {
            /* has the user declared a keymap? (monospaced bitmap font) */
            if(header->data.bmp.chr[i].index >= 0 && header->data.bmp.chr[i].source_rect.width > 0) {
                /* find the source_rect of individual characters (keymap) */
                int index = header->data.bmp.chr[i].index;
                int char_width = header->data.bmp.chr[i].source_rect.width;
                int char_height = header->data.bmp.chr[i].source_rect.height;
                int chars_in_row = header->data.bmp.source_rect[2] / char_width;
                int src_x = header->data.bmp.source_rect[0], src_y = header->data.bmp.source_rect[1];
                header->data.bmp.chr[i].source_rect.x = src_x + (index % chars_in_row) * char_width;
                header->data.bmp.chr[i].source_rect.y = src_y + (index / chars_in_row) * char_height;
            }
        }
    }
    else
        fatal_error("Font script error: unknown font type '%s'", id);

    return 0;
}

int traverse_bmp(const parsetree_statement_t* stmt, void* data)
{
    fontscript_t* header = (fontscript_t*)data;
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "source_file") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_string(p1, "Font script error: a relative filepath is expected in source_file");

        str_cpy(header->data.bmp.source_file, nanoparser_get_string(p1), sizeof(header->data.bmp.source_file));
    }
    else if(str_icmp(id, "source_rect") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        const parsetree_parameter_t* p3 = nanoparser_get_nth_parameter(param_list, 3);
        const parsetree_parameter_t* p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p2, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p3, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p4, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");

        header->data.bmp.source_rect[0] = max(0, atoi(nanoparser_get_string(p1)));
        header->data.bmp.source_rect[1] = max(0, atoi(nanoparser_get_string(p2)));
        header->data.bmp.source_rect[2] = max(1, atoi(nanoparser_get_string(p3)));
        header->data.bmp.source_rect[3] = max(1, atoi(nanoparser_get_string(p4)));
    }
    else if(str_icmp(id, "frame_size") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        int i, n = sizeof(header->data.bmp.chr) / sizeof(charproperties_t);
        int width, height;

        nanoparser_expect_string(p1, "Font script error: frame_size expects two parameters: char_width, char_height");
        nanoparser_expect_string(p2, "Font script error: frame_size expects two parameters: char_width, char_height");

        width = max(0, atoi(nanoparser_get_string(p1)));
        height = max(0, atoi(nanoparser_get_string(p2)));
        for(i = 0; i < n; i++) {
            if(header->data.bmp.chr[i].source_rect.width <= 0) {
                header->data.bmp.chr[i].source_rect.width = width;
                header->data.bmp.chr[i].source_rect.height = height;
            }
        }
    }
    else if(str_icmp(id, "keymap") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const char* keymap;
        size_t i, prev_i;
        uint32_t chr;

        nanoparser_expect_string(p1, "Font script error: a sequence of characters is expected in keymap");

        keymap = nanoparser_get_string(p1);
        for(prev_i = i = 0; (chr = u8_nextchar(keymap, &i)) != 0; prev_i = i) {
            if(chr <= 0xFF) {
                if(!header->data.bmp.chr[chr].valid) {
                    header->data.bmp.chr[chr].index = prev_i;
                    header->data.bmp.chr[chr].valid = true;
                }
            }
        }
    }
    else if(str_icmp(id, "spacing") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "Font script error: spacing expects two parameters: x, y");
        nanoparser_expect_string(p2, "Font script error: spacing expects two parameters: x, y");

        header->data.bmp.spacing[0] = atoi(nanoparser_get_string(p1));
        header->data.bmp.spacing[1] = atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(id, "char") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        int c;

        nanoparser_expect_string(p1, "Font script error: a character is expected in char");
        c = *(nanoparser_get_string(p1));

        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)(header->data.bmp.chr + c), traverse_bmp_char);
    }
    else
        fatal_error("Font script error: unknown keyword '%s' in bitmap font", id);

    return 0;
}

int traverse_bmp_char(const parsetree_statement_t* stmt, void* data)
{
    charproperties_t* chr = (charproperties_t*)data;
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "source_rect") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        const parsetree_parameter_t* p3 = nanoparser_get_nth_parameter(param_list, 3);
        const parsetree_parameter_t* p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p2, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p3, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p4, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");

        chr->source_rect.x = max(0, atoi(nanoparser_get_string(p1)));
        chr->source_rect.y = max(0, atoi(nanoparser_get_string(p2)));
        chr->source_rect.width = max(0, atoi(nanoparser_get_string(p3)));
        chr->source_rect.height = max(0, atoi(nanoparser_get_string(p4)));
        chr->index = -1;
        chr->valid = true;
    }
    else
        fatal_error("Font script error: unknown keyword '%s' in bitmap font", id);

    return 0;
}

int traverse_ttf(const parsetree_statement_t* stmt, void* data)
{
    fontscript_t* header = (fontscript_t*)data;
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "source_file") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_string(p1, "Font script error: a relative filepath is expected in source_file");

        str_cpy(header->data.ttf.source_file, nanoparser_get_string(p1), sizeof(header->data.ttf.source_file));
    }
    else if(str_icmp(id, "size") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const char* err = "Font script error: a positive integer is expected in size";
        int n;

        nanoparser_expect_string(p1, err);
        n = atoi(nanoparser_get_string(p1));
        if(n <= 0)
            fatal_error("%s", err);

        header->data.ttf.size = n;
    }
    else if(str_icmp(id, "antialias") == 0)
        header->data.ttf.antialias = true;
    else if(str_icmp(id, "shadow") == 0)
        header->data.ttf.shadow = true;
    else
        fatal_error("Font script error: unknown keyword '%s' in ttf font", id);

    return 0;
}

int dirfill(const char* vpath, void* param)
{
    (void)param;

    const char* fullpath = asset_path(vpath);
    parsetree_program_t* p = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(p, traverse);
    nanoparser_deconstruct_tree(p);
    return 0;
}


/* ------------------------------------------------- */
/* list of fontdrv_t */
/* ------------------------------------------------- */

void fontdrv_list_init()
{
    fontdrv_list = NULL;
}

void fontdrv_list_add(const char* name, fontdrv_t* drv)
{
    fontdrv_list_t* x = mallocx(sizeof *x);
    x->name = str_dup(name);
    x->data = drv;
    x->next = fontdrv_list;
    fontdrv_list = x;
}

void fontdrv_list_release()
{
    fontdrv_list_t *x, *next;

    for(x = fontdrv_list; x; x = next) {
        next = x->next;
        free(x->name);
        x->data->release(x->data);
        free(x);
    }
}

fontdrv_t* fontdrv_list_find(const char* name)
{
    fontdrv_list_t* x;

    for(x = fontdrv_list; x; x = x->next) {
        if(str_icmp(x->name, name) == 0)
            return x->data;
    }

    return NULL;
}

/* find the language-specific font driver, or
   return the generic one if it doesn't exist */
fontdrv_t* fontdrv_list_find_ex(const char* name, const char* lang_id)
{
    char* language_specific_font_name = join_names(name, lang_id);
    fontdrv_t* drv = fontdrv_list_find(language_specific_font_name);

    if(drv == NULL)
        drv = fontdrv_list_find(name);

    free(language_specific_font_name);
    return drv;
}


/* ------------------------------------------------- */
/* bitmap fonts */
/* ------------------------------------------------- */

fontdrv_t* fontdrv_bmp_new(const char* source_file, charproperties_t chr[], int spacing[2])
{
    fontdrv_bmp_t* f = mallocx(sizeof *f);
    const image_t* img = image_load(source_file);
    const int n = sizeof(f->glyph) / sizeof(image_t*);

    /* initialize the vtable */
    ((fontdrv_t*)f)->textout = fontdrv_bmp_textout;
    ((fontdrv_t*)f)->line_width = fontdrv_bmp_linewidth;
    ((fontdrv_t*)f)->line_height = fontdrv_bmp_lineheight;
    ((fontdrv_t*)f)->filepath = fontdrv_bmp_filepath;
    ((fontdrv_t*)f)->image = fontdrv_bmp_image;
    ((fontdrv_t*)f)->release = fontdrv_bmp_release;

    /* initialize the glyphs */
    for(int j = 0; j < n; j++)
        f->glyph[j] = NULL;

    /* set the image atlas */
    f->atlas = img;

    /* configure the spritesheet */
    f->line_height = 0;
    for(int j = 1 + FONT_COLORBREAKPOINT; j < n; j++) {
        if(chr[j].valid) {
            f->glyph[j] = image_create_shared(img, chr[j].source_rect.x, chr[j].source_rect.y, chr[j].source_rect.width, chr[j].source_rect.height);
            f->line_height = max(f->line_height, chr[j].source_rect.height);
        }
    }
    f->spacing = v2d_new(spacing[0], spacing[1]);
    f->line_height += spacing[1];

    /* validation */
    if(f->line_height <= spacing[1])
        fatal_error("Font script error: bitmap font \"%s\" has no valid characters.", source_file);

    /* copy the source file */
    f->filepath = str_dup(source_file);

    /* done! ;) */
    return (fontdrv_t*)f;
}

void fontdrv_bmp_release(fontdrv_t* fnt)
{
    fontdrv_bmp_t* f = (fontdrv_bmp_t*)fnt;
    const int n = sizeof(f->glyph) / sizeof(image_t*);

    for(int i = 0; i < n; i++) {
        if(f->glyph[i] != NULL)
            image_destroy(f->glyph[i]);
    }

    image_unload(f->atlas);

    free(f->filepath);
    free(f);
}

void fontdrv_bmp_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    int hsp = f->spacing.x;
    int vsp = f->spacing.y;
    uint32_t c = 0;

    for(size_t i = 0; (c = u8_nextchar(text, &i)) != 0; ) {
        const image_t* glyph = find_bmp_glyph(f, c);
        if(glyph != NULL) {
            int dy = f->line_height - vsp - image_height(glyph);
            image_draw_tinted(glyph, x, y + dy, color, IF_NONE);
            x += image_width(glyph) + hsp;
        }
    }
}

int fontdrv_bmp_lineheight(const fontdrv_t* fnt)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    return f->line_height;
}

int fontdrv_bmp_linewidth(const fontdrv_t* fnt, const char* text)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    int hsp = f->spacing.x;
    int space = 0;
    int line_width = 0;
    uint32_t c = 0;

    for(size_t i = 0; (c = u8_nextchar(text, &i)) != 0; ) {
        const image_t* glyph = find_bmp_glyph(f, c);
        if(glyph != NULL) {
            line_width += image_width(glyph) + space;
            space = (text[i] != '\0') ? hsp : 0;
        }
    }

    return line_width;
}

const char* fontdrv_bmp_filepath(const fontdrv_t* fnt)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    return f->filepath;
}

const image_t* fontdrv_bmp_image(const fontdrv_t* fnt)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    return f->atlas;
}

const image_t* find_bmp_glyph(const fontdrv_bmp_t* f, uint32_t codepoint)
{
    /* currently, bitmap fonts only support up
       to 256 characters. That can be expanded
       in the future. */

    return f->glyph[codepoint & 0xFF]; /* possibly NULL */
}

/* ------------------------------------------------- */
/* ttf fonts */
/* ------------------------------------------------- */

fontdrv_t* fontdrv_ttf_new(const char* source_file, int size, bool antialias, bool shadow)
{
    /* basic setup */
    fontdrv_ttf_t* f = mallocx(sizeof *f);
    ((fontdrv_t*)f)->textout = fontdrv_ttf_textout;
    ((fontdrv_t*)f)->line_width = fontdrv_ttf_linewidth;
    ((fontdrv_t*)f)->line_height = fontdrv_ttf_lineheight;
    ((fontdrv_t*)f)->filepath = fontdrv_ttf_filepath;
    ((fontdrv_t*)f)->image = fontdrv_ttf_image;
    ((fontdrv_t*)f)->release = fontdrv_ttf_release;

    /* store font attributes */
    f->filepath = str_dup(source_file);
    f->size = max(size, 0); /* height of glyphs in pixels */
    f->antialias = antialias;
    f->shadow = shadow;
    f->line_height = 0;

    /* lazy loading */
    f->font = NULL;

    /* done! */
    return (fontdrv_t*)f;
}

void fontdrv_ttf_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color)
{
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;
    int flags = ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER;

    if(!has_loaded_ttf(f))
        load_ttf((fontdrv_ttf_t*)f);

    /* draw shadow */
    if(f->shadow) {
        ALLEGRO_COLOR black = al_map_rgb(0, 0, 0);
        al_draw_text(f->font, black, x, y + 1.0f, flags, text);
        al_draw_text(f->font, black, x + 1.0f, y + 1.0f, flags, text);
        if(f->size >= 18) /* TODO: configurable shadows */
            al_draw_text(f->font, black, x + 2.0f, y + 2.0f, flags, text);
    }

    /* draw text */
    al_draw_text(f->font, color._color, x, y, flags, text);
}

void fontdrv_ttf_release(fontdrv_t* fnt)
{
    fontdrv_ttf_t* f = (fontdrv_ttf_t*)fnt;

    if(has_loaded_ttf(f))
        unload_ttf(f);

    free(f->filepath);
    free(f);
}

int fontdrv_ttf_lineheight(const fontdrv_t* fnt)
{
    return ((const fontdrv_ttf_t*)fnt)->line_height;
}

int fontdrv_ttf_linewidth(const fontdrv_t* fnt, const char* text)
{
    char buffer[256];
    const int maxlen = sizeof(buffer) - 1;
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;

    /* empty string? */
    if(*text == '\0')
        return 0;

    /* lazily load the font */
    if(!has_loaded_ttf(f))
        load_ttf((fontdrv_ttf_t*)f);

    /* filter out characters used as breakpoints */
    const char* p = text; int i = 0;
    while(i < maxlen && *p != '\0') {
        if(*p != '\n' && *p != FONT_COLORBREAKPOINT)
            buffer[i++] = *p;
        p++;
    }
    buffer[i] = '\0';

    /* compute the width */
    return al_get_text_width(f->font, buffer);
}

const char* fontdrv_ttf_filepath(const fontdrv_t* fnt)
{
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;
    return f->filepath;
}

const image_t* fontdrv_ttf_image(const fontdrv_t* fnt)
{
    /* there is no image atlas */
    return NULL;
}

bool has_loaded_ttf(const fontdrv_ttf_t* f)
{
    return f->font != NULL;
}

void load_ttf(fontdrv_ttf_t* f)
{
    const char* fullpath = asset_path(f->filepath);

    logfile_message("Loading TrueType font \"%s\"...", fullpath);

    f->font = al_load_ttf_font(fullpath, -(f->size), !(f->antialias) ? ALLEGRO_TTF_MONOCHROME : 0);
    if(f->font == NULL)
        fatal_error("Failed to load TrueType font \"%s\"", fullpath);

    f->line_height = al_get_font_line_height(f->font);
}

void unload_ttf(fontdrv_ttf_t* f)
{
    al_destroy_font(f->font);
}

/* ------------------------------------------------- */
/* callback table */
/* ------------------------------------------------- */

void callbacktable_init()
{
    callback_table = hashtable_fontcallback_t_create();
}

void callbacktable_release()
{
    callback_table = hashtable_fontcallback_t_destroy(callback_table);
}

void callbacktable_add(const char* variable_name, fontcallback_t callback)
{
    hashtable_fontcallback_t_add(callback_table, variable_name, (fontcallback_t*)callback);
}

fontcallback_t callbacktable_find(const char* variable_name)
{
    return (fontcallback_t)hashtable_fontcallback_t_find(callback_table, variable_name);
}


/* ------------------------------------------------- */
/* (legacy) predefined vars */
/* ------------------------------------------------- */
static const char* f_dollar() { return "$"; }
static const char* f_lowerthan() { return "<"; }
static const char* f_greaterthan() { return ">"; }
static const char* f_empty() { return ""; }
static const char* f_level_name() { return level_name(); }
static const char* f_level_version() { return level_version(); }
static const char* f_level_author() { return level_author(); }
static const char* f_level_act() { return str_from_int(level_act(), NULL, 0); }
static const char* f_player_name() { return level_player() != NULL ? player_name(level_player()) : "null"; }
static const char* f_input_type() { return input_is_joystick_enabled() ? "JOY" : "KEYB"; }
static const char* f_engine_name() { return GAME_TITLE; }
static const char* f_engine_version() { return GAME_VERSION_STRING; }
static const char* f_engine_website() { return GAME_WEBSITE; }
static const char* f_engine_year() { return GAME_YEAR; }
static const char* f_game_name() { return opensurge_game_name(); }
static const char* f_game_version() { return opensurge_game_version(); }
static const char* f_game_website() { return GAME_WEBSITE; }
static const char* f_game_year() { return GAME_YEAR; }


/*
 * register_predefined_vars()
 * Registers lots of useful predefined variables
 */
void register_predefined_vars()
{
    font_register_variable("DOLLAR", f_dollar);
    font_register_variable("LT", f_lowerthan);
    font_register_variable("GT", f_greaterthan);
    font_register_variable("EMPTY", f_empty);
    font_register_variable("LEVEL_NAME", f_level_name);
    font_register_variable("LEVEL_VERSION", f_level_version);
    font_register_variable("LEVEL_AUTHOR", f_level_author);
    font_register_variable("LEVEL_ACT", f_level_act);
    font_register_variable("PLAYER_NAME", f_player_name);
    font_register_variable("INPUT_TYPE", f_input_type);
    font_register_variable("ENGINE_NAME", f_engine_name);
    font_register_variable("ENGINE_VERSION", f_engine_version);
    font_register_variable("ENGINE_WEBSITE", f_engine_website);
    font_register_variable("ENGINE_YEAR", f_engine_year);
    font_register_variable("GAME_NAME", f_game_name);
    font_register_variable("GAME_VERSION", f_game_version);
    font_register_variable("GAME_WEBSITE", f_game_website);
    font_register_variable("GAME_YEAR", f_game_year);
}