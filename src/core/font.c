/*
 * Open Surge Engine
 * font.c - font module
 * Copyright (C) 2008-2011, 2013, 2018-2019  Alexandre Martins <alemartf@gmail.com>
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

#if defined(A5BUILD)
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#else
#include <allegro.h>
#include <alfont.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "font.h"
#include "video.h"
#include "image.h"
#include "color.h"
#include "stringutil.h"
#include "assetfs.h"
#include "lang.h"
#include "logfile.h"
#include "hashtable.h"
#include "nanoparser/nanoparser.h"
#include "utf8/utf8.h"

/* private stuff */
#if defined(A5BUILD)
#define IMAGE2BITMAP(img)           (*((ALLEGRO_BITMAP**)(img)))
#else
#define IMAGE2BITMAP(img)           (*((BITMAP**)(img)))
#endif
#define FONT_STACKCAPACITY          8        /* color stack capacity */
#define FONT_TEXTMAXSIZE            65536    /* maximum size for texts */
#define FONT_PATHMAX                1024     /* buffer size for multilingual paths */
static bool allow_antialias = true;          /* allow antialiasing for all TTF fonts? */

/* macros */
#define IS_VAR_ANYCHAR(c)           ((c) != '\0' && ((isalnum((unsigned char)(c))) || ((c) == '_')))
#define IS_VAR_1STCHAR(c)           ((c) != '\0' && ((isalpha((unsigned char)(c))) || ((c) == '_')))
#define IS_TAG_1STCHAR(c)           ((c) != '\0' && ((isalpha((unsigned char)(c))) || ((c) == '/')))

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
    void (*textout)(const fontdrv_t*,const char*,int,int,color_t); /* prints a string */
    v2d_t (*textsize)(const fontdrv_t*,const char*); /* text size, in pixels */
    v2d_t (*charspacing)(const fontdrv_t*); /* a pair (hspace, vspace) */
    void (*release)(fontdrv_t*); /* release the fontdrv_t */
};
static fontdrv_t* fontdrv_bmp_new(const char* source_file, charproperties_t chr[], int spacing[2]);
static fontdrv_t* fontdrv_ttf_new(const char* source_file, int size, bool antialias, bool shadow);

typedef struct fontdrv_bmp_t fontdrv_bmp_t;
struct fontdrv_bmp_t { /* bitmap font */
    fontdrv_t base;
    image_t* bmp[256]; /* bitmap character indexed by its unicode number */
    v2d_t spacing; /* character spacing */
    int line_height; /* max({ image_height(bmp[j]) | j >= 0 }) */
};
static void fontdrv_bmp_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color);
static v2d_t fontdrv_bmp_textsize(const fontdrv_t* fnt, const char* string);
static v2d_t fontdrv_bmp_charspacing(const fontdrv_t* fnt);
static void fontdrv_bmp_release(fontdrv_t* fnt);

typedef struct fontdrv_ttf_t fontdrv_ttf_t;
struct fontdrv_ttf_t { /* truetype font */
    fontdrv_t base;
#if defined(A5BUILD)
    ALLEGRO_FONT* font;
#else
    ALFONT_FONT* ttf;
#endif
    int size; /* font size */
    bool antialias; /* enable antialiasing? */
    bool shadow; /* enable shadow? */
    char* source_file; /* relative path */
};
static void fontdrv_ttf_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color);
static v2d_t fontdrv_ttf_textsize(const fontdrv_t* fnt, const char* string);
static v2d_t fontdrv_ttf_charspacing(const fontdrv_t* fnt);
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

/* font struct: this struct is used by the external world */
struct font_t {
    fontdrv_t* drv; /* font driver */
    char* text; /* text data */
    v2d_t position; /* position */
    int width; /* width (in pixels) for wordwrap */
    bool visible; /* is this font visible? */
    int index_of_first_char, length; /* substring (deprecated) */
    fontargs_t argument; /* text arguments: $1, $2 ... ${FONTARGS_MAX} */
    fontalign_t align; /* alignment */
    char* lang_id; /* current language ID (multilingual support) */
    char* name; /* font name (not language specific) */
};

/* misc */
static const char* read_variable(const char* key, void* data);
static int expand_vars(char* dest, const char* src, size_t dest_size, const char* (*callback)(const char*,void*), void* data);
static inline bool has_vars_to_expand(const char* str);
static void convert_to_ascii(char* str);
static int print_line(const fontdrv_t* drv, const char* text, int x, int y, color_t color_stack[], int* stack_top);
static int print_aligned_line(const fontdrv_t* drv, const char* text, fontalign_t align, int x, int y, color_t color_stack[], int* stack_top);
static char* find_wordwrap(const fontdrv_t* drv, char* text, int max_width);
static int find_blanks(int blank[], size_t size, const char* text);
static char* tagged_text_offset(char* txt, int charnum);
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
void font_init(bool allow_font_smoothing)
{
#if defined(A5BUILD)
    parsetree_program_t* fonts = NULL;

    /* initializing Allegro's TTF addon */
    if(!al_init_ttf_addon())
        fatal_error("Can't initialize Allegro's TTF addon");

    /* basic initialization */
    allow_antialias = allow_font_smoothing;
    fontdrv_list_init();

    /* reading the parse tree */
    logfile_message("Loading fonts...");
    assetfs_foreach_file("fonts", ".fnt", dirfill, &fonts, true);
    nanoparser_traverse_program(fonts, traverse);
    logfile_message("All fonts have been loaded.");

    /* initializing the font callback table */
    callbacktable_init();

    /* done */
    fonts = nanoparser_deconstruct_tree(fonts);
#else
    parsetree_program_t* fonts = NULL;

    allow_antialias = allow_font_smoothing; /* this comes first */
    logfile_message("Initializing alfont...");
    if(0 != alfont_init())
        fatal_error("alfont_init() has failed. %s", allegro_error);

    logfile_message("Loading font scripts...");
    fontdrv_list_init();

    /* reading the parse tree */
    assetfs_foreach_file("fonts", ".fnt", dirfill, &fonts, true);
    nanoparser_traverse_program(fonts, traverse);
    logfile_message("All fonts have been loaded.");

    /* initializing the font callback table */
    callbacktable_init();

    /* done */
    fonts = nanoparser_deconstruct_tree(fonts);
#endif
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

#if !defined(A5BUILD)
    logfile_message("Unloading alfont...");
    alfont_exit();
#endif
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
 * font_create()
 * Creates a new font object
 */
font_t* font_create(const char* font_name)
{
    int i;
    font_t* f = mallocx(sizeof *f);

    f->text = str_dup("");
    f->width = 0;
    f->visible = true;
    f->position = v2d_new(0, 0);
    f->index_of_first_char = 0;
    f->length = FONT_TEXTMAXSIZE - 1;
    f->align = FONTALIGN_LEFT;
    f->name = str_dup(font_name);
    f->lang_id = str_dup(lang_getid());

    f->drv = fontdrv_list_find_ex(f->name, f->lang_id);
    if(f->drv == NULL)
        fatal_error("Can't find font \"%s\"", f->name);

    for(i=0; i<FONTARGS_MAX; i++)
        f->argument[i] = NULL;

    return f;
}




/*
 * font_destroy()
 * Destroys an existing font object
 */
void font_destroy(font_t* f)
{
    for(int i = 0; i < FONTARGS_MAX; i++) {
        if(f->argument[i])
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
    const int MAX_PASSES = 2;
    static char buf[FONT_TEXTMAXSIZE], pre[FONT_TEXTMAXSIZE];
    va_list args;

    /* printf */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* expand variables (call AFTER printf) */
    for(int k = 0; k < MAX_PASSES && has_vars_to_expand(buf); k++) {
        str_cpy(pre, buf, sizeof(pre));
        expand_vars(buf, pre, sizeof(buf), read_variable, f->argument);
    }

    /* utf8 check */
    if(!u8_isvalid(buf, strlen(buf)))
        convert_to_ascii(buf);

    /* allocate text */
    if(f->text != NULL) {
        /* skip if the text is the same */
        if(strcmp(buf, f->text) != 0) {
            free(f->text);
            f->text = str_dup(buf);
        }
    }
    else
        f->text = str_dup(buf);
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
    
    va_start(ap, amount);
    for(i = 0; i < m; i++) {
        if(f->argument[i])
            free(f->argument[i]);
        f->argument[i] = str_dup(va_arg(ap, const char*));
    }
    va_end(ap);
}


/*
 * font_set_textargumentsv()
 * An alternative to font_set_textarguments().
 * Pass an array of strings instead of a variadic argument
 */
void font_set_textargumentsv(font_t* f, int argc, const char** argv)
{
    int m = min(FONTARGS_MAX, argc);

    for(int i = 0; i < m; i++) {
        if(f->argument[i])
            free(f->argument[i]);
        f->argument[i] = str_dup(argv[i]);
    }
}



/*
 * font_get_text()
 * Returns the text
 */
const char* font_get_text(const font_t* f)
{
    return f->text ? f->text : "";
}



/*
 * font_set_width()
 * Sets the width of the font (useful if
 * you want wordwrap). If w == 0, then
 * there's no wordwrap.
 */
void font_set_width(font_t* f, int w)
{
    f->width = max(0, w);
}


/*
 * font_render()
 * Renders the text
 */
void font_render(font_t* f, v2d_t camera_position)
{
    color_t stack[FONT_STACKCAPACITY] = { color_rgb(255, 255, 255) }; /* color stack */
    int stack_top = 0, offset = -1;
    char *p, *q, *s, *w, r = 0, t = 0;
    char* text = f->text;
    v2d_t pos;

    /* not visible? */
    if(!f->visible)
        return;

    /* multilingual support */
    if(must_refresh_driver(f))
        refresh_driver(f);

    /* compute screen position */
    pos = v2d_subtract(f->position, v2d_subtract(camera_position,
        v2d_multiply(video_get_screen_size(), 0.5f)
    ));

    /* use substring? */
    if(f->index_of_first_char > 0) {
        text = tagged_text_offset(text, f->index_of_first_char);
        if(text == NULL)
            return;
    }
    if(f->length < FONT_TEXTMAXSIZE - 1) {
        char* x = tagged_text_offset(text, f->length);
        if(x != NULL) {
            offset = x - text;
            t = text[offset];
            text[offset] = 0;
        }
    }

    /* for each line p of text (split using '\n') */
    for(p = q = text; q != NULL; ) {
        if((q = strchr(p, '\n')) != NULL) { *q = 0; }
        for(s = w = p; w != NULL; ) {
            /* for each line s of text (split using wordwrap rules) */
            if((w = find_wordwrap(f->drv, s, f->width)) != NULL) { r = *w; *w = 0; }
            pos.y += print_aligned_line(f->drv, s, f->align, pos.x, pos.y, stack, &stack_top);
            if(w != NULL) { *w = r; s = w; }
        }
        if(q != NULL) { *q = '\n'; p = q+1; }
    }

    /* undo substring */
    if(offset >= 0)
        text[offset] = t;
}


/*
 * font_get_textsize()
 * Returns the size (in pixels) of the rendered text
 * (considering wordwrap)
 */
v2d_t font_get_textsize(const font_t* f)
{
    int offset = -1;
    char *p, *q, *s, *w, r = 0, t = 0;
    char* text = f->text;
    v2d_t size = v2d_new(0, 0), tmp;

    /* use substring? */
    if(f->index_of_first_char > 0) {
        text = tagged_text_offset(text, f->index_of_first_char);
        if(text == NULL)
            return size;
    }
    if(f->length < FONT_TEXTMAXSIZE - 1) {
        char* x = tagged_text_offset(text, f->length);
        if(x != NULL) {
            offset = x - text;
            t = text[offset];
            text[offset] = 0;
        }
    }

    /* for each line p of text (split using '\n') */
    for(p = q = text; q != NULL; ) {
        if((q = strchr(p, '\n')) != NULL) { *q = 0; }
        for(s = w = p; w != NULL; ) {
            /* for each line s of text (split using wordwrap rules) */
            if((w = find_wordwrap(f->drv, s, f->width)) != NULL) { r = *w; *w = 0; }
            tmp = f->drv->textsize(f->drv, s);
            size.x = max(size.x, tmp.x);
            size.y += tmp.y;
            if(w != NULL) { *w = r; s = w; }
        }
        if(q != NULL) { *q = '\n'; p = q+1; }
    }

    /* undo substring */
    if(offset >= 0)
        text[offset] = t;

    /* done */
    return size;
}


/*
 * font_get_charspacing()
 * Returns the spacing between the characters of a given font
 */
v2d_t font_get_charspacing(const font_t* f)
{
    return f->drv->charspacing(f->drv);
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
void font_use_substring(font_t* f, int index_of_first_char, int length)
{
    f->index_of_first_char = max(0, index_of_first_char);
    f->length = max(0, length);
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
    f->align = align;
}

/*
 * font_get_maxlength()
 * Get the maximum number of characters that can be printed, ignoring color tags and blanks
 */
int font_get_maxlength(const font_t* f)
{
    return f->length;
}

/*
 * font_set_maxlength()
 * Set the maximum number of characters that can be printed, ignoring color tags and blanks
 */
void font_set_maxlength(font_t* f, int maxlength)
{
    f->length = clip(maxlength, 0, FONT_TEXTMAXSIZE - 1);
}


/* ------------------------------------------------- */
/* private utilities */
/* ------------------------------------------------- */

/* returns a static char* (case insensitive search) */
const char* read_variable(const char* key, void* data)
{
    const char* value = NULL;

    if(!isdigit(key[0])) {
        /* read $IDENTIFIER */
        fontcallback_t fun = callbacktable_find(key);
        static char buf[1024];

        if(fun != NULL)
            value = fun();
        else
            value = lang_getstring(key, buf, sizeof(buf));
    }
    else {
        /* read $1, $2 ... $9 */
        const char** args = (const char**)data;
        int index = key[0] - '1';

        if(index >= 0 && index < FONTARGS_MAX)
            value = args[index]; /* may be NULL */
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
    enum { COPYING, ACCUMULATING_DIGIT, ACCUMULATING_IDENTIFIER, ACCUMULATING_EXPRESSION } state = COPYING;
    int curly_counter = 0, number_of_substitutions = 0;
    int m = (int)dest_size - 1, accsize = sizeof(acc) - 1;
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
void convert_to_ascii(char* str)
{
    char *p, *q;

    for(q = p = str; *p; p++) {
        if(!(*p & 0x80))
            *(q++) = *p;
    }

    *q = 0;
}

/* will print a single line of text with the specified font,
   returning its height */
int print_line(const fontdrv_t* drv, const char* text, int x, int y, color_t color_stack[], int* stack_top)
{
    static char linebuf[FONT_TEXTMAXSIZE];
    char *p, *q;
    size_t j = 0;
    uint32_t chr, tag = 0;
    v2d_t sp = drv->charspacing(drv);
    int line_height = drv->textsize(drv, " ").y + sp.y;
    const image_t* target = image_drawing_target();
    #define _print_linebuf() \
        do { \
            if(*linebuf) { \
                drv->textout(drv, linebuf, x, y, color_stack[*stack_top]); \
                x += drv->textsize(drv, linebuf).x + sp.x; \
            } \
            *(p = linebuf) = 0; \
        } while(0)

    /* clip */
    if(y < -line_height || y > image_height(target) + line_height || x > image_width(target) + sp.x)
        return line_height;

    /* print */
    *(p = linebuf) = 0;
    while((chr = u8_nextchar(text, &j)) != 0) {
        if(!tag && chr == '<' && IS_TAG_1STCHAR(text[j])) {
            /* read tag */
            const char* tag_name = text + j;
            if(tag_name[0] != '/') {
                /* open tag */
                if(strncmp(tag_name, "color=", 6) == 0) {
                    /* color tag */
                    const char* color_code = tag_name + 6;
                    char hex_code[7] = { 0 }; int i = 0;
                    if(*color_code == '#') /* skip '#', if any */
                        ++color_code;
                    while(*color_code && *color_code != '>' && i < sizeof(hex_code) - 1) /* read color */
                        hex_code[i++] = *(color_code++);
                    if(*stack_top + 1 < FONT_STACKCAPACITY) { /* push color */
                        _print_linebuf();
                        color_stack[++(*stack_top)] = color_hex(hex_code);
                    }
                }
                tag = 1;
            }
            else {
                /* close tag */
                if(strncmp(tag_name, "/color>", 7) == 0) {
                    if(*stack_top > 0) {
                        _print_linebuf();
                        --(*stack_top);
                    }
                }
                tag = 1;
            }
        }
        else if(tag) {
            /* skip tag */
            if(chr == '>')
                tag = 0;
        }
        else {
            /* buffer character */
            char buf[5] = { 0 };
            u8_wc_toutf8(buf, chr);
            for(q = buf; *q && p - linebuf < sizeof(linebuf) - 1; *p = 0)
                *(p++) = *(q++);
        }
    }
    _print_linebuf();

    /* done */
    return line_height;
}

/* print a line with a certain alignment, returning its height */
int print_aligned_line(const fontdrv_t* drv, const char* text, fontalign_t align, int x, int y, color_t color_stack[], int* stack_top)
{
    int dx = 0;

    switch(align) {
        case FONTALIGN_LEFT:
            break;

        case FONTALIGN_CENTER:
            dx = drv->textsize(drv, text).x * 0.5f;
            break;

        case FONTALIGN_RIGHT:
            dx = drv->textsize(drv, text).x;
            break;
    }

    return print_line(drv, text, x - dx, y, color_stack, stack_top);
}

/* find the next point where a wordwrap should be placed */
char* find_wordwrap(const fontdrv_t* drv, char* text, int max_width)
{
    if(max_width > 0) {
        static int blank[1024];
        int blanks = find_blanks(blank, sizeof(blank), text);
        int m, l = 0, r = blanks - 1;
        int best_m = 0, width;
        char *wordwrap, chr;

        /* check if there is no wordwrap */
        if(blanks == 0 || (int)drv->textsize(drv, text).x <= max_width)
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
            width = drv->textsize(drv, text).x;
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
        for(wordwrap = text + blank[best_m] + 1; *wordwrap && isspace(*wordwrap); wordwrap++);

        /* done */
        return wordwrap;
    }
    else
        return NULL; /* no wordwrap */
}

/* find all indexes of the text containing blank spaces */
int find_blanks(int blank[], size_t size, const char* text)
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
static char* tagged_text_offset(char* txt, int charnum)
{
    uint32_t chr;
    size_t i, prev_i;
    int tag = 0;

    /* lookup */
    for(prev_i = i = 0; (chr = u8_nextchar(txt, &i)) != 0; prev_i = i) {
        if(!tag && chr == '<' && IS_TAG_1STCHAR(txt[i]))
            tag = 1;
        else if(tag && chr == '>')
            tag = 0;
        else if(!tag && !isspace(chr) && charnum-- == 0)
            return txt + prev_i;
    }

    /* character not found */
    return NULL;
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
    const char* fullpath = assetfs_fullpath(vpath);
    parsetree_program_t** p = (parsetree_program_t**)param;
    *p = nanoparser_append_program(*p, nanoparser_construct_tree(fullpath));
    return 0;
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
    int j, n = sizeof(f->bmp) / sizeof(image_t*);

    ((fontdrv_t*)f)->textout = fontdrv_bmp_textout;
    ((fontdrv_t*)f)->textsize = fontdrv_bmp_textsize;
    ((fontdrv_t*)f)->charspacing = fontdrv_bmp_charspacing;
    ((fontdrv_t*)f)->release = fontdrv_bmp_release;

    /* configure the spritesheet */
    f->line_height = 0;
    for(j = 0; j < n; j++) {
        f->bmp[j] = NULL;
        if(chr[j].valid) {
            f->bmp[j] = image_create_shared(img, chr[j].source_rect.x, chr[j].source_rect.y, chr[j].source_rect.width, chr[j].source_rect.height);
            f->line_height = max(f->line_height, chr[j].source_rect.height);
        }
    }
    f->spacing = v2d_new(spacing[0], spacing[1]);

    /* validation */
    if(f->line_height == 0)
        fatal_error("Font script error: font \"%s\" has got no valid characters.", source_file);

    /* done! ;) */
    return (fontdrv_t*)f;
}

void fontdrv_bmp_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    uint32_t chr, n = sizeof(f->bmp) / sizeof(image_t*);
    color_t white = color_rgb(255, 255, 255);
    image_t* chimg;

    /* currently, bitmap fonts only support the
       first 256 Unicode characters, though that
       can be expanded in the future */

    for(size_t i = 0; (chr = u8_nextchar(text, &i)) != 0; ) {
        if(chr < n && (chimg = f->bmp[chr]) != NULL) {
            if(!color_equals(color, white))
                image_draw_tinted(chimg, x, y + f->line_height - image_height(chimg), color, IF_NONE);
            else
                image_draw(chimg, x, y + f->line_height - image_height(chimg), IF_NONE);
            x += image_width(chimg) + (int)f->spacing.x;
        }
    }
}

void fontdrv_bmp_release(fontdrv_t* fnt)
{
    fontdrv_bmp_t* f = (fontdrv_bmp_t*)fnt;
    int i, n = sizeof(f->bmp) / sizeof(image_t*);

    for(i = 0; i < n; i++) {
        if(f->bmp[i] != NULL)
            image_destroy(f->bmp[i]);
    }

    free(f);
}

v2d_t fontdrv_bmp_charspacing(const fontdrv_t* fnt)
{
    return ((const fontdrv_bmp_t*)fnt)->spacing;
}

v2d_t fontdrv_bmp_textsize(const fontdrv_t* fnt, const char* string)
{
    const fontdrv_bmp_t* f = (const fontdrv_bmp_t*)fnt;
    v2d_t sp = f->spacing;
    int width = 0, line_width = 0;
    int height = f->line_height;
    bool tag = false;
    const char* p;

    for(p = string; *p; p++) {
        if(!tag && *p == '<' && IS_TAG_1STCHAR(p[1]))
            tag = true;
        else if(tag && *p == '>')
            tag = false;
        else if(tag)
            continue;
        else if(*p == '\n') {
            height += f->line_height + (int)sp.y;
            line_width = 0;
        }
        else if(f->bmp[(int)(*p) & 0xFF] != NULL) {
            line_width += image_width(f->bmp[(int)(*p) & 0xFF]);
            if(p[1])
                line_width += (int)sp.x;
        }
        width = max(width, line_width);
    }

    return v2d_new(width, height);
}

/* ------------------------------------------------- */
/* ttf fonts */
/* ------------------------------------------------- */

fontdrv_t* fontdrv_ttf_new(const char* source_file, int size, bool antialias, bool shadow)
{
    /* basic setup */
    fontdrv_ttf_t* f = mallocx(sizeof *f);
    ((fontdrv_t*)f)->textout = fontdrv_ttf_textout;
    ((fontdrv_t*)f)->textsize = fontdrv_ttf_textsize;
    ((fontdrv_t*)f)->charspacing = fontdrv_ttf_charspacing;
    ((fontdrv_t*)f)->release = fontdrv_ttf_release;

    /* store font attributes */
    f->source_file = str_dup(source_file);
    f->size = max(size, 0); /* height of glyphs in pixels */
    f->antialias = allow_antialias && antialias;
    f->shadow = shadow;

    /* lazy loading */
#if defined(A5BUILD)
    f->font = NULL;
#else
    f->ttf = NULL;
#endif

    /* done! */
    return (fontdrv_t*)f;
}

void fontdrv_ttf_textout(const fontdrv_t* fnt, const char* text, int x, int y, color_t color)
{
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;

    if(!has_loaded_ttf(f))
        load_ttf((fontdrv_ttf_t*)f);

#if defined(A5BUILD)
    /* draw shadow */
    if(f->shadow) {
        ALLEGRO_COLOR black = al_map_rgb(0, 0, 0);
        al_draw_text(f->font, black, x, y + 1.0f, ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER, text);
        al_draw_text(f->font, black, x + 1.0f, y + 1.0f, ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER, text);
        if(f->size >= 18) /* TODO: configurable shadows */
            al_draw_text(f->font, black, x + 2.0f, y + 2.0f, ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER, text);
    }

    /* draw text */
    al_draw_text(f->font, color._color, x, y, ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER, text);
#else
    image_t* img = video_get_backbuffer();

    /* draw shadow */
    if(f->shadow) {
        color_t black = color_rgb(0, 0, 0);
        alfont_set_font_style(f->ttf, STYLE_BOLD);
        alfont_textout_ex(IMAGE2BITMAP(img), f->ttf, text, x, y+1, black._value, -1);
        alfont_set_font_style(f->ttf, STYLE_STANDARD);
    }

    /* draw text */
    if(f->antialias)
        alfont_textout_aa_ex(IMAGE2BITMAP(img), f->ttf, text, x, y, color._value, -1);
    else
        alfont_textout_ex(IMAGE2BITMAP(img), f->ttf, text, x, y, color._value, -1);
#endif
}

void fontdrv_ttf_release(fontdrv_t* fnt)
{
    fontdrv_ttf_t* f = (fontdrv_ttf_t*)fnt;

    if(has_loaded_ttf(f))
        unload_ttf(f);

    free(f->source_file);
    free(f);
}

v2d_t fontdrv_ttf_charspacing(const fontdrv_t* fnt)
{
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;

    if(!has_loaded_ttf(f))
        load_ttf((fontdrv_ttf_t*)f);

#if defined(A5BUILD)
    /* FIXME: is this function still needed? */
    return v2d_new(0, 0);
#else
    return v2d_new(alfont_get_char_extra_spacing(f->ttf), 0);
#endif
}

v2d_t fontdrv_ttf_textsize(const fontdrv_t* fnt, const char* string)
{
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;

    if(!has_loaded_ttf(f))
        load_ttf((fontdrv_ttf_t*)f);

#if defined(A5BUILD)
    static char linebuf[FONT_TEXTMAXSIZE]; char *p, *q;
    int width = 0, line_width = 0;
    int line_height = al_get_font_line_height(f->font);
    int height = line_height;
    bool tag = false;
    size_t i = 0;
    uint32_t ch;

    *(p = linebuf) = 0;
    while(string[i]) {
        ch = u8_nextchar(string, &i);
        if(!tag && ch == '<' && IS_TAG_1STCHAR(string[i]))
            tag = true;
        else if(tag && ch == '>')
            tag = false;
        else if(tag)
            continue;
        else if(ch == '\n') {
            line_width += *linebuf ? al_get_text_width(f->font, linebuf) : 0;
            width = max(width, line_width);
            *(p = linebuf) = 0;
            line_width = 0;
            height += line_height;
        }
        else {
            char buf[5] = { 0 };
            u8_wc_toutf8(buf, ch);
            for(q = buf; *q && p - linebuf < sizeof(linebuf) - 1; *p = 0)
                *(p++) = *(q++);
        }
    }

    line_width += *linebuf ? al_get_text_width(f->font, linebuf) : 0;
    width = max(width, line_width);
    return v2d_new(width, height);
#else
    static char linebuf[FONT_TEXTMAXSIZE]; char *p, *q;
    const fontdrv_ttf_t* f = (const fontdrv_ttf_t*)fnt;
    v2d_t sp = v2d_new(alfont_get_char_extra_spacing(f->ttf), 0);
    int width = 0, line_width = 0;
    int line_height = alfont_text_height(f->ttf);
    int height = line_height;
    bool tag = false;
    size_t i = 0;
    uint32_t ch;

    *(p = linebuf) = 0;
    while(string[i]) {
        ch = u8_nextchar(string, &i);
        if(!tag && ch == '<' && IS_TAG_1STCHAR(string[i]))
            tag = true;
        else if(tag && ch == '>')
            tag = false;
        else if(tag)
            continue;
        else if(ch == '\n') {
            line_width += *linebuf ? alfont_text_length(f->ttf, linebuf) : 0;
            width = max(width, line_width);
            *(p = linebuf) = 0;
            line_width = 0;
            height += line_height + (int)sp.y;
        }
        else {
            char buf[5] = { 0 };
            u8_wc_toutf8(buf, ch);
            for(q = buf; *q && p - linebuf < sizeof(linebuf) - 1; *p = 0)
                *(p++) = *(q++);
        }
    }

    line_width += *linebuf ? alfont_text_length(f->ttf, linebuf) : 0;
    width = max(width, line_width);
    return v2d_new(width, height);
#endif
}

bool has_loaded_ttf(const fontdrv_ttf_t* f)
{
#if defined(A5BUILD)
    return f->font != NULL;
#else
    return f->ttf != NULL;
#endif
}

void load_ttf(fontdrv_ttf_t* f)
{
    const char* fullpath = assetfs_fullpath(f->source_file);

    logfile_message("Loading TrueType font \"%s\"...", fullpath);

#if defined(A5BUILD)
    f->font = al_load_ttf_font(fullpath, -(f->size), !(f->antialias) ? ALLEGRO_TTF_MONOCHROME : 0);
    if(f->font == NULL)
        fatal_error("Failed to load TrueType font \"%s\"", fullpath);
#else
    f->ttf = alfont_load_font(fullpath);
    if(f->ttf != NULL)
        alfont_set_font_size(f->ttf, f->size);
    else
        fatal_error("Couldn't load TrueType font \"%s\"", fullpath);
#endif
}

void unload_ttf(fontdrv_ttf_t* f)
{
#if defined(A5BUILD)
    al_destroy_font(f->font);
#else
    alfont_destroy_font(f->ttf);
#endif
}