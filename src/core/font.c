/*
 * Open Surge Engine
 * font.c - font module
 * Copyright (C) 2008-2011, 2013, 2018-2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <allegro.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <alfont.h>
#include "font.h"
#include "video.h"
#include "stringutil.h"
#include "assetfs.h"
#include "lang.h"
#include "logfile.h"
#include "hashtable.h"
#include "nanoparser/nanoparser.h"
#include "utf8/utf8.h"

/* private stuff */
#define IMAGE2BITMAP(img)          (*((BITMAP**)(img)))   /* crazy stuff */
#define FONT_TEXTMAXSIZE           16384    /* maximum size for texts */
#define FONT_STACKCAPACITY         8        /* color stack capacity */
static int allow_ttf_aa = TRUE; /* allow antialiasing for all TTF fonts? */

/* ------------------------------- */

/* callback table: used for variable/text interpolation */
typedef const char* (*fontcallback_t)();
HASHTABLE_GENERATE_CODE(fontcallback_t)
hashtable_fontcallback_t *callback_table;
static void callbacktable_init();
static void callbacktable_release();
static void callbacktable_add(const char *variable_name, fontcallback_t callback);
static fontcallback_t callbacktable_find(const char *variable_name);

/* ------------------------------- */

/* font scripts (nanoparser) */
#define IS_IDENTIFIER_CHAR(c)      ((c) != '\0' && ((isalnum((unsigned char)(c))) || ((c) == '_')))
static int traverse(const parsetree_statement_t *stmt);
static int traverse_block(const parsetree_statement_t *stmt, void *data);
static int traverse_bmp(const parsetree_statement_t *stmt, void *data);
static int traverse_bmp_char(const parsetree_statement_t *stmt, void *data);
static int traverse_ttf(const parsetree_statement_t *stmt, void *data);
static int dirfill(const char *vpath, void *param);

typedef struct charproperties_t charproperties_t;
struct charproperties_t {
    uint8 valid; /* whether this character is valid (exists) */
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
            char source_file[1024]; /* source file (relative file path) */
            int source_rect[4]; /* spritesheet rect: x, y, width, height */
            int spacing[2]; /* character spacing: x, y */
            charproperties_t chr[256]; /* properties of char x (0..255) */
        } bmp;

        /* true-type */
        struct {
            char source_file[1024]; /* source file (relative file path) */
            int size; /* font size */
            int antialias; /* enable antialiasing? */
            int shadow; /* enable shadow? */
        } ttf;
    } data;
};

/* ------------------------------- */

/* fontdrv_t: a font driver stores the attributes the font class (bmp, ttf) */
typedef struct fontdrv_t fontdrv_t;
struct fontdrv_t { /* abstract font: base class */
    void (*textout)(const fontdrv_t*,image_t*,const char*,int,int,uint32); /* prints a string */
    v2d_t (*textsize)(const fontdrv_t*,const char*); /* text size, in pixels */
    v2d_t (*charspacing)(const fontdrv_t*); /* a pair (hspace, vspace) */
    void (*release)(fontdrv_t*); /* release the fontdrv_t */
};
static fontdrv_t* fontdrv_bmp_new(const char *source_file, charproperties_t chr[], int spacing[2]);
static fontdrv_t* fontdrv_ttf_new(const char *source_file, int size, int antialias, int shadow);

typedef struct fontdrv_bmp_t fontdrv_bmp_t;
struct fontdrv_bmp_t { /* bitmap font */
    fontdrv_t base;
    image_t *bmp[256]; /* bitmap character indexed by its unicode number */
    v2d_t spacing; /* character spacing */
    int line_height; /* max({ image_height(bmp[j]) | j >= 0 }) */
};
static void fontdrv_bmp_textout(const fontdrv_t *fnt, image_t *img, const char* text, int x, int y, uint32 color);
static v2d_t fontdrv_bmp_textsize(const fontdrv_t *fnt, const char *string);
static v2d_t fontdrv_bmp_charspacing(const fontdrv_t *fnt);
static void fontdrv_bmp_release(fontdrv_t *fnt);

typedef struct fontdrv_ttf_t fontdrv_ttf_t;
struct fontdrv_ttf_t { /* truetype font */
    fontdrv_t base;
    ALFONT_FONT *ttf;
    image_t *cached_character[96]; /* store characters 32..127 in a table */
    int antialias; /* enable antialiasing? */
    int shadow; /* enable shadow? */
};
static void fontdrv_ttf_textout(const fontdrv_t *fnt, image_t *img, const char* text, int x, int y, uint32 color);
static v2d_t fontdrv_ttf_textsize(const fontdrv_t *fnt, const char *string);
static v2d_t fontdrv_ttf_charspacing(const fontdrv_t *fnt);
static void fontdrv_ttf_release(fontdrv_t *fnt);

/* ------------------------------- */

/* list of fontdrv_t */
typedef struct fontdrv_list_t fontdrv_list_t;
struct fontdrv_list_t {
    char *name;
    fontdrv_t *data;
    fontdrv_list_t *next;
};

static fontdrv_list_t* fontdrv_list = NULL;
static void fontdrv_list_init();
static void fontdrv_list_release();
static void fontdrv_list_add(const char *name, fontdrv_t *drv);
static fontdrv_t* fontdrv_list_find(const char *name);

/* ------------------------------- */

/* font arguments: for a safe printf-like alternative (when dealing with user-provided format strings) */
#define FONTARGS_MAX 3 /* can go up to 9 in the current expand algorithm */
typedef char* fontargs_t[FONTARGS_MAX];

/* font struct: this struct is used by the external world */
struct font_t {
    fontdrv_t *drv; /* font driver */
    char *text; /* text data */
    v2d_t position; /* position */
    int width; /* width (in pixels) for wordwrap */
    int visible; /* is this font visible? */
    int index_of_first_char, length; /* substring (deprecated) */
    fontargs_t argument; /* text arguments: $1, $2 ... $<FONTARGS_MAX> */
    fontalign_t align; /* alignment */
};

/* misc */
static const char* get_variable(const char *key);
static inline int has_variables_to_expand(const char *str, int *passes);
static void expand_variables(char *str, fontargs_t args, size_t size);
static uint8 hex2dec(char digit);
static void convert_to_ascii(char *str);
static int print_line(const fontdrv_t *drv, const char* text, int x, int y, uint32 color_stack[], int* stack_top);
static int print_aligned_line(const fontdrv_t *drv, const char* text, fontalign_t align, int x, int y, uint32 color_stack[], int* stack_top);
static char* find_wordwrap(const fontdrv_t* drv, char* text, int max_width);
static int find_blanks(int blank[], size_t size, const char* text);
static char* tagged_text_offset(char* txt, int charnum);

/*
 * font_init()
 * Initializes the font module
 */
void font_init(int allow_font_smoothing)
{
    parsetree_program_t *fonts = NULL;

    allow_ttf_aa = allow_font_smoothing; /* this comes first */
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

    logfile_message("Unloading alfont...");
    alfont_exit();
}


/*
 * font_register_variable()
 * Variable/text interpolation. Example: font_add_variable("$LEVEL_NAME", level_name) 
 * will replace every occurrence of $LEVEL_NAME in the text of any fonts by the
 * result of level_name()
 * Call this AFTER font_init()
 */
void font_register_variable(const char *variable_name, const char* (*callback)())
{
    callbacktable_add(variable_name, (fontcallback_t)callback);
}


/*
 * font_create()
 * Creates a new font object
 */
font_t *font_create(const char *font_name)
{
    int i;
    font_t *f = mallocx(sizeof *f);

    f->text = str_dup("");
    f->width = 0;
    f->visible = TRUE;
    f->position = v2d_new(0, 0);
    f->index_of_first_char = 0;
    f->length = FONT_TEXTMAXSIZE - 1;
    f->align = FONTALIGN_LEFT;

    f->drv = fontdrv_list_find(font_name);
    if(f->drv == NULL)
        fatal_error("Can't find font \"%s\"", font_name);

    for(i=0; i<FONTARGS_MAX; i++)
        f->argument[i] = NULL;

    return f;
}




/*
 * font_destroy()
 * Destroys an existing font object
 */
void font_destroy(font_t *f)
{
    int i;

    for(i=0; i<FONTARGS_MAX; i++) {
        if(f->argument[i])
            free(f->argument[i]);
    }

    free(f->text);
    free(f);
}




/*
 * font_set_text()
 * Sets the text... printf style. Be careful with unsanitized format strings.
 */
void font_set_text(font_t *f, const char *fmt, ...)
{
    static char buf[FONT_TEXTMAXSIZE];
    va_list args;
    int passes = 0;

    /* printf */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* expand variables */
    while(has_variables_to_expand(buf, &passes))
        expand_variables(buf, f->argument, sizeof(buf));

    /* utf8 check */
    if(!u8_isvalid(buf, strlen(buf)))
        convert_to_ascii(buf);

    /* allocate text */
    if(f->text != NULL)
        free(f->text);
    f->text = str_dup(buf);
}


/*
 * font_set_textarguments()
 * Kinda like a safe, sanitazed printf-format stuff.
 * pass <amount> of const char*'s; they'll be stored
 * in $1, $2, ... up to $<FONTARGS_MAX>
 */
void font_set_textarguments(font_t *f, int amount, ...)
{
    va_list ap;
    int i, m = min(FONTARGS_MAX, amount);
    
    va_start(ap, amount);
    for(i=0; i<m; i++) {
        if(f->argument[i])
            free(f->argument[i]);
        f->argument[i] = str_dup(va_arg(ap, const char*));
    }
    va_end(ap);
}



/*
 * font_get_text()
 * Returns the text
 */
const char *font_get_text(const font_t *f)
{
    return f->text ? f->text : "";
}



/*
 * font_set_width()
 * Sets the width of the font (useful if
 * you want wordwrap). If w == 0, then
 * there's no wordwrap.
 */
void font_set_width(font_t *f, int w)
{
    f->width = max(0, w);
}


/*
 * font_render()
 * Renders the text
 */
void font_render(const font_t *f, v2d_t camera_position)
{
    uint32 stack[FONT_STACKCAPACITY] = { image_rgb(255, 255, 255) }; /* color stack */
    int stack_top = 0, offset = -1;
    char *p, *q, *s, *w, r = 0, t = 0;
    char *text = f->text;
    v2d_t pos;

    /* not visible? */
    if(!f->visible)
        return;

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

    /* compute screen position */
    pos = v2d_subtract(f->position, v2d_subtract(camera_position,
        v2d_multiply(video_get_screen_size(), 0.5f)
    ));

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
 * Returns the size (in pixels) of a given text
 */
v2d_t font_get_textsize(const font_t *f)
{
    return f->drv->textsize(f->drv, f->text);
}

/*
 * font_get_charspacing()
 * Returns the spacing between the characters of a given font
 */
v2d_t font_get_charspacing(const font_t *f)
{
    return f->drv->charspacing(f->drv);
}


/*
 * font_set_position()
 * Sets the position of the font
 */
void font_set_position(font_t *f, v2d_t position)
{
    f->position = position;
}


/*
 * font_get_position()
 * Gets the position of the font
 */
v2d_t font_get_position(const font_t *f)
{
    return f->position;
}


/*
 * font_is_visible()
 * Is the font visible?
 */
int font_is_visible(const font_t *f)
{
    return f->visible;
}


/*
 * font_set_visible()
 * Sets the visibility of the font
 */
void font_set_visible(font_t *f, int is_visible)
{
    f->visible = is_visible;
}


/*
 * font_use_substring()
 * Since fonts may have color tags, variables, etc. , use this
 * to display a substring of the font (not the whole text)
 */
void font_use_substring(font_t *f, int index_of_first_char, int length)
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
const char* get_variable(const char *key)
{
    fontcallback_t fun = callbacktable_find(key);
    if(fun != NULL)
        return fun();
    else
        return lang_get(key+1); /* skip the '$' */
}


/* expands the variables, e.g.,

   "Please press the $INPUT_LEFT to go left"
            ... becomes ...
   "Please press the LEFT CTRL KEY to go left"
*/
void expand_variables(char *str, fontargs_t args, size_t size)
{
    static char buf[FONT_TEXTMAXSIZE];
    char *p = str, *q = buf;

    /* expand into buf */
    while(*p && (q - buf < sizeof(buf) - 1)) {
        /* scanning... */
        while(*p && *p != '$' && (q - buf < sizeof(buf) - 1))
            *(q++) = *(p++);

        /* found a variable! */
        if(*p == '$') {
            char varname[64], *r;
            const char *value = NULL, *u;

            /* detect the name of this variable */
            r = varname;
            do {
                *(r++) = *(p++);
            } while(IS_IDENTIFIER_CHAR(*p) && (r - varname < sizeof(varname) - 1));
            *r = 0;

            /* get the contents of varname */
            if(isdigit(varname[1]) && varname[2] == 0) {
                /* match $1, $2, ... */
                if(varname[1] >= '1' && varname[1] <= '0' + FONTARGS_MAX)
                    value = args[ varname[1] - '1' ]; /* may be NULL */
                if(value == NULL)
                    value = varname; /* get the digit */
            }
            else {
                /* match other variables */
                value = get_variable(varname);
            }

            /* put it into buf */
            if((u = value) != NULL) {
                while(*u && (q - buf < sizeof(buf) - 1))
                    *(q++) = *(u++);
            }
        }
    }
    *q = 0;

    /* copy back into str */
    str_cpy(str, buf, size);
}


/* has_variables_to_expand() */
int has_variables_to_expand(const char *str, int *passes)
{
    static const int MAX_PASSES = 3; /* won't expand more than this */
    if(++(*passes) > MAX_PASSES)
        return FALSE;

    while(*str) {
        if(*str == '$' && IS_IDENTIFIER_CHAR(str[1]))
            return TRUE;
        str++;
    }

    return FALSE;
}


/* hex2dec() */
uint8 hex2dec(char digit)
{
    digit = tolower(digit);
    if(digit >= '0' && digit <= '9')
        return digit-'0';
    else if(digit >= 'a' && digit <= 'f')
        return (digit-'a')+10;
    else
        return 255; /* error */
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
int print_line(const fontdrv_t *drv, const char* text, int x, int y, uint32 color_stack[], int* stack_top)
{
    static char linebuf[FONT_TEXTMAXSIZE];
    char *p, *q;
    size_t j = 0, prev_j = 0;
    uint32_t chr, tag = 0;
    #define _print_linebuf() \
        do { \
            if(*linebuf) { \
                drv->textout(drv, video_get_backbuffer(), linebuf, x, y, color_stack[*stack_top]); \
                x += drv->textsize(drv, linebuf).x + drv->charspacing(drv).x; \
            } \
            *(p = linebuf) = 0; \
        } while(0)

    *(p = linebuf) = 0;
    while((chr = u8_nextchar(text, &j)) != 0) {
        if(!tag && chr == '<') {
            /* read tag */
            const char* tag_name = text + prev_j + 1;
            if(isalpha(tag_name[0])) {
                /* open tag */
                if(strncmp(tag_name, "color=", 6) == 0) {
                    /* color tag */
                    char hex_code[6] = { 0 }; int i = 0;
                    const char* color_code = tag_name + 6;
                    if(*color_code == '#') /* skip '#', if any */
                        ++color_code;
                    while(*color_code && *color_code != '>' && i < sizeof(hex_code)) /* read color */
                        hex_code[i++] = *(color_code++);
                    if(i == 3) { /* accept short color notation (e.g., fff, eee, etc.) */
                        hex_code[5] = hex_code[4] = hex_code[2];
                        hex_code[3] = hex_code[2] = hex_code[1];
                        hex_code[1] = hex_code[0];
                    }
                    else if(i != 6) /* invalid color code length */
                        ; /* do nothing? */
                    if(*stack_top + 1 < FONT_STACKCAPACITY) { /* push color */
                        uint8 r = (hex2dec(hex_code[0]) << 4) | hex2dec(hex_code[1]);
                        uint8 g = (hex2dec(hex_code[2]) << 4) | hex2dec(hex_code[3]);
                        uint8 b = (hex2dec(hex_code[4]) << 4) | hex2dec(hex_code[5]);
                        _print_linebuf();
                        color_stack[++(*stack_top)] = image_rgb(r, g, b);
                    }
                }
                tag = 1;
            }
            else if(tag_name[0] == '/') {
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
        prev_j = j;
    }
    _print_linebuf();

    /* return the height of the printed line */
    return drv->textsize(drv, " ").y + drv->charspacing(drv).y;
}

/* print a line with a certain alignment, returning its height */
int print_aligned_line(const fontdrv_t *drv, const char* text, fontalign_t align, int x, int y, uint32 color_stack[], int* stack_top)
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
        static int blank[384];
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
        if(!tag && chr == '<' && (isalpha(txt[i]) || txt[i] == '/'))
            tag = 1;
        else if(tag && chr == '>')
            tag = 0;
        else if(!tag && !isspace(chr) && charnum-- == 0)
            return txt + prev_i;
    }

    /* character not found */
    return NULL;
}


/* ------------------------------------------------- */
/* read the font scripts */
/* ------------------------------------------------- */

int traverse(const parsetree_statement_t *stmt)
{
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "font") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);
        const char *name = NULL;
        fontdrv_t *drv = NULL;
        fontscript_t header;

        /* read the data */
        nanoparser_expect_string(p1, "Font script error: font name is expected");
        name = nanoparser_get_string(p1);
        logfile_message("Loading font \"%s\"...", name);
        nanoparser_expect_program(p2, "Font script error: font block is expected after the font name");
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)(&header), traverse_block);

        /* duplicate font? */
        if(NULL != fontdrv_list_find(name)) {
            /* fail silently */
            logfile_message("Font script error: can't redefine font \"%s\"\nin \"%s\" near line %d", name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
            return 0;
        }

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
            fatal_error("Font script error: unknown font type");
            break;
        }

        /* register the fontdrv_t */
        fontdrv_list_add(name, drv);

    }
    else
        fatal_error("Font script error: unknown identifier \"%s\"\nin \"%s\" near line %d", id, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int traverse_block(const parsetree_statement_t *stmt, void *data)
{
    fontscript_t *header = (fontscript_t*)data;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);

    nanoparser_expect_program(p1, "Font script error: block data is expected after the type of the font");

    if(str_icmp(id, "truetype") == 0) {
        /* default configuration */
        header->type = FONTSCRIPTTYPE_TTF;
        strcpy(header->data.ttf.source_file, "");
        header->data.ttf.size = 12;
        header->data.ttf.antialias = FALSE;
        header->data.ttf.shadow = FALSE;

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
            header->data.bmp.chr[i].valid = 0;
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

int traverse_bmp(const parsetree_statement_t *stmt, void *data)
{
    fontscript_t *header = (fontscript_t*)data;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "source_file") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_string(p1, "Font script error: a relative filepath is expected in source_file");

        str_cpy(header->data.bmp.source_file, nanoparser_get_string(p1), sizeof(header->data.bmp.source_file));
    }
    else if(str_icmp(id, "source_rect") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);
        const parsetree_parameter_t *p3 = nanoparser_get_nth_parameter(param_list, 3);
        const parsetree_parameter_t *p4 = nanoparser_get_nth_parameter(param_list, 4);

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
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);
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
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const char *keymap;
        size_t i, prev_i;
        uint32_t chr;

        nanoparser_expect_string(p1, "Font script error: a sequence of characters is expected in keymap");

        keymap = nanoparser_get_string(p1);
        for(prev_i = i = 0; (chr = u8_nextchar(keymap, &i)) != 0; prev_i = i) {
            if(chr <= 0xFF) {
                if(!header->data.bmp.chr[chr].valid) {
                    header->data.bmp.chr[chr].index = prev_i;
                    header->data.bmp.chr[chr].valid = 1;
                }
            }
        }
    }
    else if(str_icmp(id, "spacing") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "Font script error: spacing expects two parameters: x, y");
        nanoparser_expect_string(p2, "Font script error: spacing expects two parameters: x, y");

        header->data.bmp.spacing[0] = atoi(nanoparser_get_string(p1));
        header->data.bmp.spacing[1] = atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(id, "char") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);
        int c;

        nanoparser_expect_string(p1, "Font script error: a character is expected in char");
        c = *(nanoparser_get_string(p1));

        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)(header->data.bmp.chr + c), traverse_bmp_char);
    }
    else
        fatal_error("Font script error: unknown keyword '%s' in bitmap font", id);

    return 0;
}

int traverse_bmp_char(const parsetree_statement_t *stmt, void *data)
{
    charproperties_t *chr = (charproperties_t*)data;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "source_rect") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);
        const parsetree_parameter_t *p3 = nanoparser_get_nth_parameter(param_list, 3);
        const parsetree_parameter_t *p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p2, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p3, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");
        nanoparser_expect_string(p4, "Font script error: source_rect expects four parameters: source_x, source_y, width, height");

        chr->source_rect.x = max(0, atoi(nanoparser_get_string(p1)));
        chr->source_rect.y = max(0, atoi(nanoparser_get_string(p2)));
        chr->source_rect.width = max(0, atoi(nanoparser_get_string(p3)));
        chr->source_rect.height = max(0, atoi(nanoparser_get_string(p4)));
        chr->index = -1;
        chr->valid = 1;
    }
    else
        fatal_error("Font script error: unknown keyword '%s' in bitmap font", id);

    return 0;
}

int traverse_ttf(const parsetree_statement_t *stmt, void *data)
{
    fontscript_t *header = (fontscript_t*)data;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "source_file") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_string(p1, "Font script error: a relative filepath is expected in source_file");

        str_cpy(header->data.ttf.source_file, nanoparser_get_string(p1), sizeof(header->data.ttf.source_file));
    }
    else if(str_icmp(id, "size") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const char *err = "Font script error: a positive integer is expected in size";
        int n;

        nanoparser_expect_string(p1, err);
        n = atoi(nanoparser_get_string(p1));
        if(n <= 0)
            fatal_error("%s", err);

        header->data.ttf.size = n;
    }
    else if(str_icmp(id, "antialias") == 0)
        header->data.ttf.antialias = TRUE;
    else if(str_icmp(id, "shadow") == 0)
        header->data.ttf.shadow = TRUE;
    else
        fatal_error("Font script error: unknown keyword '%s' in ttf font", id);

    return 0;
}

int dirfill(const char *vpath, void *param)
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
    callback_table = hashtable_fontcallback_t_create(NULL);
}

void callbacktable_release()
{
    callback_table = hashtable_fontcallback_t_destroy(callback_table);
}

void callbacktable_add(const char *variable_name, fontcallback_t callback)
{
    hashtable_fontcallback_t_add(callback_table, variable_name, (fontcallback_t*)callback);
}

fontcallback_t callbacktable_find(const char *variable_name)
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

void fontdrv_list_add(const char *name, fontdrv_t *drv)
{
    fontdrv_list_t *x = mallocx(sizeof *x);
    x->name = str_dup(name);
    x->data = drv;
    x->next = fontdrv_list;
    fontdrv_list = x;
}

void fontdrv_list_release()
{
    fontdrv_list_t *x, *next;

    for(x=fontdrv_list; x; x=next) {
        next = x->next;
        free(x->name);
        x->data->release(x->data);
        free(x);
    }
}

fontdrv_t* fontdrv_list_find(const char *name)
{
    fontdrv_list_t *x;

    for(x=fontdrv_list; x; x=x->next) {
        if(str_icmp(x->name, name) == 0)
            return x->data;
    }

    return NULL;
}


/* ------------------------------------------------- */
/* bitmap fonts */
/* ------------------------------------------------- */

fontdrv_t* fontdrv_bmp_new(const char *source_file, charproperties_t chr[], int spacing[2])
{
    fontdrv_bmp_t *f = mallocx(sizeof *f);
    image_t *img = image_load(source_file);
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

void fontdrv_bmp_textout(const fontdrv_t *fnt, image_t *img, const char* text, int x, int y, uint32 color)
{
    const fontdrv_bmp_t *f = (const fontdrv_bmp_t*)fnt;
    uint32 chr, n = sizeof(f->bmp) / sizeof(image_t*);
    uint32 white = image_rgb(255, 255, 255);
    image_t* chimg;

    /* currently, bitmap fonts only support the
       first 256 Unicode characters, though that
       can be expanded in the future */

    for(size_t i = 0; (chr = u8_nextchar(text, &i)) != 0; ) {
        if(chr < n && (chimg = f->bmp[chr]) != NULL) {
            if(color != white)
                image_draw_multiply(chimg, img, x, y + f->line_height - image_height(chimg), color, 1.0f, IF_NONE);
            else
                image_draw(chimg, img, x, y + f->line_height - image_height(chimg), IF_NONE);
            x += image_width(chimg) + (int)f->spacing.x;
        }
    }
}

void fontdrv_bmp_release(fontdrv_t *fnt)
{
    fontdrv_bmp_t *f = (fontdrv_bmp_t*)fnt;
    int i, n = sizeof(f->bmp) / sizeof(image_t*);

    for(i=0; i<n; i++) {
        if(f->bmp[i] != NULL)
            image_destroy(f->bmp[i]);
    }

    free(f);
}

v2d_t fontdrv_bmp_charspacing(const fontdrv_t *fnt)
{
    return ((const fontdrv_bmp_t*)fnt)->spacing;
}

v2d_t fontdrv_bmp_textsize(const fontdrv_t *fnt, const char *string)
{
    const fontdrv_bmp_t *f = (const fontdrv_bmp_t*)fnt;
    v2d_t sp = f->spacing;
    int width = 0, line_width = 0;
    int height = f->line_height;
    int tag = FALSE;
    const char* p;

    for(p = string; *p; p++) {
        if(!tag && *p == '<' && (isalpha(p[1]) || p[1] == '/'))
            tag = TRUE;
        else if(tag && *p == '>')
            tag = FALSE;
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

fontdrv_t* fontdrv_ttf_new(const char *source_file, int size, int antialias, int shadow)
{
    fontdrv_ttf_t *f = mallocx(sizeof *f);
    const char* fullpath;

    ((fontdrv_t*)f)->textout = fontdrv_ttf_textout;
    ((fontdrv_t*)f)->textsize = fontdrv_ttf_textsize;
    ((fontdrv_t*)f)->charspacing = fontdrv_ttf_charspacing;
    ((fontdrv_t*)f)->release = fontdrv_ttf_release;

    fullpath = assetfs_fullpath(source_file);
    logfile_message("Loading TrueType font '%s'...", fullpath);

    f->ttf = alfont_load_font(fullpath);
    if(NULL != f->ttf) {
        /* configuring */
        alfont_set_font_size(f->ttf, size);
        f->antialias = allow_ttf_aa ? antialias : FALSE;
        f->shadow = shadow;

        /* caching commonly used characters */
        if(1 || !(f->antialias)) {
            char buf[2] = { 0, 0 };
            int ch, w, h = alfont_text_height(f->ttf);
            uint32 white = image_rgb(255, 255, 255);
            for(ch=32; ch<=127; ch++) {
                buf[0] = ch;
                w = alfont_text_length(f->ttf, buf);
                f->cached_character[ch-32] = image_create(w, h);
                image_clear(f->cached_character[ch-32], video_get_maskcolor());
                alfont_textout_ex(IMAGE2BITMAP(f->cached_character[ch-32]), f->ttf, buf, 0, 0, white, -1);
            }
        }
        else {
            int i = 0, n = sizeof(f->cached_character) / sizeof(image_t*);
            for(i = 0; i < n; i++)
                f->cached_character[i] = NULL;
        }
    }
    else
        fatal_error("Couldn't load TrueType font '%s'", source_file);

    return (fontdrv_t*)f;
}

void fontdrv_ttf_textout(const fontdrv_t *fnt, image_t *img, const char* text, int x, int y, uint32 color)
{
    const fontdrv_ttf_t *f = (const fontdrv_ttf_t*)fnt;

    /* draw shadow */
    if(f->shadow) {
        uint32 black = image_rgb(0, 0, 0);
        /*alfont_set_font_outline_color(f->ttf, *((int*)&black));
        alfont_set_font_outline_right(f->ttf, 1);*/
        alfont_set_font_style(f->ttf, STYLE_BOLD);
        alfont_textout_ex(IMAGE2BITMAP(img), f->ttf, text, x, y+1, *((int*)&black), -1);
        alfont_set_font_style(f->ttf, STYLE_STANDARD);
        /*alfont_set_font_outline_right(f->ttf, 0);*/
    }

    /* draw text */
    if(f->antialias)
        alfont_textout_aa_ex(IMAGE2BITMAP(img), f->ttf, text, x, y, *((int*)&color), -1);
    else
        alfont_textout_ex(IMAGE2BITMAP(img), f->ttf, text, x, y, *((int*)&color), -1);
}

void fontdrv_ttf_release(fontdrv_t *fnt)
{
    fontdrv_ttf_t *f = (fontdrv_ttf_t*)fnt;
    int i = 0, n = sizeof(f->cached_character) / sizeof(image_t*);

    for(i = 0; i < n; i++) {
        if(f->cached_character[i] != NULL)
            image_destroy(f->cached_character[i]);
    }

    alfont_destroy_font(f->ttf);
    free(f);
}

v2d_t fontdrv_ttf_charspacing(const fontdrv_t *fnt)
{
    const fontdrv_ttf_t *f = (const fontdrv_ttf_t*)fnt;
    return v2d_new(alfont_get_char_extra_spacing(f->ttf), 0);
}

v2d_t fontdrv_ttf_textsize(const fontdrv_t *fnt, const char *string)
{
    static char linebuf[FONT_TEXTMAXSIZE]; char *p, *q;
    const fontdrv_ttf_t *f = (const fontdrv_ttf_t*)fnt;
    v2d_t sp = v2d_new(alfont_get_char_extra_spacing(f->ttf), 0);
    int width = 0, line_width = 0;
    int line_height = alfont_text_height(f->ttf);
    int height = line_height;
    int tag = FALSE;
    size_t i = 0;
    uint32 ch;

    *(p = linebuf) = 0;
    while(string[i]) {
        ch = u8_nextchar(string, &i);
        if(!tag && ch == '<' && (isalpha(string[i]) || string[i] == '/'))
            tag = TRUE;
        else if(tag && ch == '>')
            tag = FALSE;
        else if(tag)
            continue;
        else if(ch == '\n') {
            line_width += *linebuf ? alfont_text_length(f->ttf, linebuf) : 0;
            width = max(width, line_width);
            *(p = linebuf) = 0;
            line_width = 0;
            height += line_height + (int)sp.y;
        }
        else if((ch >= 32 && ch <= 127) && (f->cached_character[(int)ch - 32] != NULL)) {
            line_width += image_width(f->cached_character[(int)ch - 32]);
            if(string[i])
                line_width += (int)sp.x;
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
}