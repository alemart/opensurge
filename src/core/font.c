/*
 * Open Surge Engine
 * font.c - font module
 * Copyright (C) 2008-2011, 2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "osspec.h"
#include "lang.h"
#include "logfile.h"
#include "hashtable.h"
#include "nanoparser/nanoparser.h"

/* private stuff */
#define IMAGE2BITMAP(img)       (*((BITMAP**)(img)))   /* whoooa, this is crazy stuff */
static int allow_ttf_aa = TRUE; /* allow antialiasing for all TTF fonts? */


/* callback table: used for variable/text interpolation */
typedef const char* (*fontcallback_t)();
HASHTABLE_GENERATE_CODE(fontcallback_t)
hashtable_fontcallback_t *callback_table;
static void callbacktable_init();
static void callbacktable_release();
static void callbacktable_add(const char *variable_name, fontcallback_t callback);
static fontcallback_t callbacktable_find(const char *variable_name);

/* fontdata_t: stores the attributes of each font class (set of fonts with the same name) */
typedef struct fontdata_t fontdata_t;
struct fontdata_t { /* abstract font: base class */
    void (*renderchar)(fontdata_t*,image_t*,int,int,int,uint32); /* renders a character */
    void (*release)(fontdata_t*); /* release the fontdata_t */
    v2d_t (*charspacing)(fontdata_t*); /* a pair (hspace, vspace) */
    v2d_t (*textsize)(fontdata_t*,const char*); /* text size, in pixels */
};
static fontdata_t* fontdata_bmp_new(const char *source_file, const char *keymap, int sheet_source_x, int sheet_source_y, int sheet_width, int sheet_height, int char_width, int char_height);
static fontdata_t* fontdata_ttf_new(const char *source_file, int size, int antialias, int shadow);

typedef struct fontdata_bmp_t fontdata_bmp_t;
struct fontdata_bmp_t { /* bitmap font */
    fontdata_t base;
    image_t *bmp[256]; /* bitmap char indexed by ascii code */
    v2d_t charsize;
};
static void fontdata_bmp_renderchar(fontdata_t *fnt, image_t *img, int ch, int x, int y, uint32 color);
static void fontdata_bmp_release(fontdata_t *fnt);
static v2d_t fontdata_bmp_charspacing(fontdata_t *fnt);
static v2d_t fontdata_bmp_textsize(fontdata_t *fnt, const char *string);

typedef struct fontdata_ttf_t fontdata_ttf_t;
struct fontdata_ttf_t { /* truetype font */
    fontdata_t base;
    ALFONT_FONT *ttf;
    image_t *cached_character[96]; /* store characters 0..127 in a table */
    int antialias; /* enable antialiasing? */
    int shadow; /* enable shadow? */
};
static void fontdata_ttf_renderchar(fontdata_t *fnt, image_t *img, int ch, int x, int y, uint32 color);
static void fontdata_ttf_release(fontdata_t *fnt);
static v2d_t fontdata_ttf_charspacing(fontdata_t *fnt);
static v2d_t fontdata_ttf_textsize(fontdata_t *fnt, const char *string);

/* ------------------------------- */

/* list of fontdata_t */
typedef struct fontdata_list_t fontdata_list_t;
struct fontdata_list_t {
    char *name;
    fontdata_t *data;
    fontdata_list_t *next;
};

static fontdata_list_t* fontdata_list = NULL;
static void fontdata_list_init();
static void fontdata_list_release();
static void fontdata_list_add(const char *name, fontdata_t *data);
static fontdata_t* fontdata_list_find(const char *name);

/* ------------------------------- */

/* font arguments: for a safe printf-like alternative (when dealing with user-provided format strings) */
#define FONTARGS_MAX 3
typedef char* fontargs_t[FONTARGS_MAX];

/* font struct: this struct is used by the external world */
struct font_t {
    fontdata_t *my_class;
    char *text; /* text data */
    v2d_t position; /* position */
    int width; /* width (in pixels) - wordwrap? */
    int visible; /* is this font visible? */
    int index_of_first_char, length; /* substring */
    fontargs_t argument; /* text arguments: $1, $2 ... $<FONTARGS_MAX> */
};

/* ------------------------------- */

/* private data */
#define IS_IDENTIFIER_CHAR(c)      ((c) != '\0' && ((isalnum((unsigned char)(c))) || ((c) == '_')))
#define FONT_STACKCAPACITY  32
#define FONT_TEXTMAXLENGTH  20480
static int dirfill(const char *filename, void *param);
static int traverse(const parsetree_statement_t *stmt);
static int traverse_block(const parsetree_statement_t *stmt, void *data);
static int traverse_bmp(const parsetree_statement_t *stmt, void *data);
static int traverse_ttf(const parsetree_statement_t *stmt, void *data);
static const char* get_variable(const char *key);
static int has_variables_to_expand(const char *str);
static void expand_variables(char *str, fontargs_t args);
static uint8 hex2dec(char digit);
static char* remove_tags(const char *str);

typedef struct fontscript_t fontscript_t;
struct fontscript_t {
    enum { FONTSCRIPTTYPE_TTF, FONTSCRIPTTYPE_BMP } type;
    union {

        /* bitmap */
        struct {
            char source_file[1024]; /* source file (relative file path) */
            char keymap[10240]; /* character-image mapping */
            int source_rect[4]; /* x, y, width, height */
            int char_size[2]; /* width, height */
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


/*
 * font_init()
 * Initializes the font module
 */
void font_init(int allow_font_smoothing)
{
    const char *path = "fonts/*.fnt";
    parsetree_program_t *fonts = NULL;

    allow_ttf_aa = allow_font_smoothing; /* this comes first */
    logfile_message("Initializing alfont...");
    if(0 != alfont_init())
        fatal_error("alfont_init() has failed. %s", allegro_error);

    logfile_message("Loading font scripts...");
    fontdata_list_init();

    /* reading the parse tree */
    foreach_resource(path, dirfill, (void*)&fonts, TRUE);

    /* loading the fontdata list */
    nanoparser_traverse_program(fonts, traverse);

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
    fontdata_list_release();

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
    f->length = INFINITY;

    f->my_class = fontdata_list_find(font_name);
    if(f->my_class == NULL)
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
    static char buf[FONT_TEXTMAXLENGTH];
    va_list args;
    char *p, *q;

    va_start(args, fmt);
    vsnprintf(buf, (FONT_TEXTMAXLENGTH*2)/3, fmt, args);
    va_end(args);

    while(has_variables_to_expand(buf))
        expand_variables(buf, f->argument);

    if(f->text) free(f->text);
    f->text = mallocx(sizeof(char) * (strlen(buf) + 1));

    for(p=buf,q=f->text; *p; p++,q++) {
        if(*p == '\\') {
            switch( *(p+1) ) {
                case 'n':
                    *q = '\n';
                    p++;
                    break;

                case '\\':
                    *q = '\\';
                    p++;
                    break;

                default:
                    *q = *p;
                    break;
            }
        }
        else
            *q = *p;
    }

    *q = 0;
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
    /* this routine is horrible (it has suffered too many mutations through time).
       it should be rewritten some time... :(
       
       ...but it works and I'm lazy ;) */

    int offx = 0, offy = 0;
    char *p, s[8];
    uint32 color[FONT_STACKCAPACITY];
    int i, top = 0, w = 0, h = 0;
    int wordwrap;
    v2d_t textsize, charspacing = font_get_charspacing(f);
    int hspace = charspacing.x, vspace = charspacing.y;
    char *text = f->text;
    int wide_char = 0;
    int idx = 0;

    color[top++] = image_rgb(255,255,255);
    if(f->visible && text) {
        for(p=text; *p; p++) {
            /* wordwrap */
            wordwrap = FALSE;
            if(p == text || (p != text && isspace((unsigned char)*(p-1)))) {
                char *q;
                int tag = FALSE;
                int line_width = 0;

                for(q=p; !(*q=='\0' || isspace((unsigned char)*q)); q++) {
                    if(*q == '<') tag = TRUE;
                    if(!tag) {
                        uszprintf(s, sizeof(s), "%lc", ugetat(q, 0));
                        line_width += (int)(f->my_class->textsize(f->my_class, s).x) + hspace;
                    }
                    if(*q == '>') tag = FALSE;
                }

                wordwrap = ((f->width > 0) && ((offx + line_width - hspace) > f->width));
            }

            /* tags */
            if(*p == '<') {

                if(strncmp(p+1, "color=", 6) == 0) {
                    char *orig = p;
                    uint8 r, g, b;
                    char tc;
                    int valid = TRUE;

                    p += 7;
                    for(i=0; i<6 && valid; i++) {
                        tc = tolower( *(p+i) );
                        valid = ((tc >= '0' && tc <= '9') || (tc >= 'a' && tc <= 'f'));
                    }
                    valid = valid && (*(p+6) == '>');

                    if(valid) {
                        r = (hex2dec(*(p+0)) << 4) | hex2dec(*(p+1));
                        g = (hex2dec(*(p+2)) << 4) | hex2dec(*(p+3));
                        b = (hex2dec(*(p+4)) << 4) | hex2dec(*(p+5));
                        p += 7;
                        if(top < FONT_STACKCAPACITY)
                            color[top++] = image_rgb(r,g,b);
                    }
                    else
                        p = orig;
                }

                if(strncmp(p+1, "/color>", 7) == 0) {
                    p += 8;
                    if(top >= 2) /* we must not clear the color stack */
                        top--;
                }

                if(!*p)
                    break;
            }

            /* skip it! */
            if(idx < f->index_of_first_char) { idx++; continue; }
            if(idx >= f->index_of_first_char + f->length) { break; }
            idx++;

            /* character size */
            wide_char = ugetat(p, 0);
            uszprintf(s, sizeof(s), "%lc", wide_char);
            textsize = f->my_class->textsize(f->my_class, s);
            w = (int)textsize.x; h = (int)textsize.y;

            /* printing text */
            if(wordwrap) { offx = 0; offy += h + vspace; }
            if(*p != '\n') {
                p += uoffset(p, 1) - 1; /* ugly hack */
                f->my_class->renderchar(f->my_class, video_get_backbuffer(), wide_char, (int)(f->position.x+offx-(camera_position.x-VIDEO_SCREEN_W/2)), (int)(f->position.y+offy-(camera_position.y-VIDEO_SCREEN_H/2)), color[top-1]);
                offx += w + hspace;

                /* gulp... o_o' */
                if(wide_char >= 0x80 && f->my_class->renderchar == fontdata_bmp_renderchar)
                    offx += -w + (w>>1);
            }
            else {
                offx = 0;
                offy += h + vspace;
            }
        }
    }
}



/*
 * font_get_textsize()
 * Returns the size (in pixels) of a given text
 */
v2d_t font_get_textsize(const font_t *f)
{
    return f->my_class->textsize(f->my_class, f->text);
}


/*
 * font_get_charspacing()
 * Returns the spacing between the characters of a given font
 */
v2d_t font_get_charspacing(const font_t *f)
{
    return f->my_class->charspacing(f->my_class);
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
 * 1) Please press the $INPUT_LEFT to go left
 * 2) Please press the LEFT CTRL KEY to go left */
void expand_variables(char *str, fontargs_t args)
{
    static char buf[FONT_TEXTMAXLENGTH];
    static char varname[FONT_TEXTMAXLENGTH];
    char *p=str, *q=buf, *r;
    const char *u;
    const char *varcontent;

    while(*p) {
        /* looking for variables... */
        while(*p && *p != '$' && (q-buf < FONT_TEXTMAXLENGTH))
            *(q++) = *(p++);

        /* I found a variable! */
        if(*p == '$') {
            /* detect the name of this variable */
            r = varname;
            do {
                *(r++) = *(p++);
            } while(IS_IDENTIFIER_CHAR(*p));
            *r = 0;

            /* get the contents of varname */
            if(varname[1] >= '1' && varname[1] <= ('0' + FONTARGS_MAX) && varname[2] == '\0')
                varcontent = args[ varname[1] - '1' ]; /* may be NULL */
            else
                varcontent = get_variable(varname);

            /* put it into buf */
            for(u = varcontent; u && *u && (q-buf < FONT_TEXTMAXLENGTH); u++, q++)
                *q = *u;
        }
    }
    *q = 0;

    strcpy(str, buf);
}


/* has_variables_to_expand() */
int has_variables_to_expand(const char *str)
{
    const char *p;

    for(p=str; *p; p++) {
        if(*p == '$' && IS_IDENTIFIER_CHAR(*(p+1)))
            return TRUE;
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

/* returns a copy of str without things between < and > */
/* example: <color=ff0000>hi</color> becomes hi */
char* remove_tags(const char *str)
{
    int len, tag;
    char *s;
    const char *p;

    for(len=0,tag=FALSE,p=str; *p; p++) {
        if(tag && (*p == '>')) tag = FALSE;
        else if(!tag && (*p == '<')) tag = TRUE;
        else if(!tag && (*p != '<')) len++;
    }

    s = mallocx((1+len) * sizeof(*s));
    for(len=0,tag=FALSE,p=str; *p; p++) {
        if(tag && (*p == '>')) tag = FALSE;
        else if(!tag && (*p == '<')) tag = TRUE;
        else if(!tag && (*p != '<')) s[len++] = *p;
    }
    s[len] = '\0';

    return s;
}

/* dirfill() */
int dirfill(const char *filename, void *param)
{
    parsetree_program_t** p = (parsetree_program_t**)param;
    *p = nanoparser_append_program(*p, nanoparser_construct_tree(filename));
    return 0;
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
        fontdata_t *data = NULL;
        fontscript_t header;

        /* read the data */
        nanoparser_expect_string(p1, "Font script error: font name is expected");
        name = nanoparser_get_string(p1);
        logfile_message("Loading font '%s'...", name);

        nanoparser_expect_program(p2, "Font script error: font block is expected after the font name");
        nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)(&header), traverse_block);

        /* create the fontdata_t */
        switch(header.type) {
        case FONTSCRIPTTYPE_TTF:
            data = fontdata_ttf_new(
                header.data.ttf.source_file,
                header.data.ttf.size,
                header.data.ttf.antialias,
                header.data.ttf.shadow
            );
            break;

        case FONTSCRIPTTYPE_BMP:
            data = fontdata_bmp_new(
                header.data.bmp.source_file,
                header.data.bmp.keymap,
                header.data.bmp.source_rect[0],
                header.data.bmp.source_rect[1],
                header.data.bmp.source_rect[2],
                header.data.bmp.source_rect[3],
                header.data.bmp.char_size[0],
                header.data.bmp.char_size[1]
            );
            break;

        default:
            fatal_error("Font script error: unknown font type");
            break;
        }

        /* register the fontdata_t */
        fontdata_list_add(name, data);
    }
    else
        fatal_error("Font script error: unknown keyword '%s'", id);

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
        header->type = FONTSCRIPTTYPE_BMP;
        strcpy(header->data.bmp.source_file, "");
        strcpy(header->data.bmp.keymap, " ");
        header->data.bmp.source_rect[0] = 0;
        header->data.bmp.source_rect[1] = 0;
        header->data.bmp.source_rect[2] = 1;
        header->data.bmp.source_rect[3] = 1;
        header->data.bmp.char_size[0] = 1;
        header->data.bmp.char_size[1] = 1;

        nanoparser_traverse_program_ex(nanoparser_get_program(p1), data, traverse_bmp);
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

        header->data.bmp.source_rect[0] = atoi(nanoparser_get_string(p1));
        header->data.bmp.source_rect[1] = atoi(nanoparser_get_string(p2));
        header->data.bmp.source_rect[2] = atoi(nanoparser_get_string(p3));
        header->data.bmp.source_rect[3] = atoi(nanoparser_get_string(p4));
    }
    else if(str_icmp(id, "frame_size") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "Font script error: frame_size expects two parameters: char_width, char_height");
        nanoparser_expect_string(p2, "Font script error: frame_size expects two parameters: char_width, char_height");

        header->data.bmp.char_size[0] = atoi(nanoparser_get_string(p1));
        header->data.bmp.char_size[1] = atoi(nanoparser_get_string(p2));
    }
    else if(str_icmp(id, "keymap") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);

        nanoparser_expect_string(p1, "Font script error: a sequence of characters is expected in keymap");

        str_cpy(header->data.bmp.keymap, nanoparser_get_string(p1), sizeof(header->data.bmp.keymap));
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
/* list of fontdata_t */
/* ------------------------------------------------- */

void fontdata_list_init()
{
    fontdata_list = NULL;
}

void fontdata_list_add(const char *name, fontdata_t *data)
{
    fontdata_list_t *x;

    if(NULL != fontdata_list_find(name))
        fatal_error("Font script error: can't redefine font \"%s\"", name);

    x = mallocx(sizeof *x);
    x->name = str_dup(name);
    x->data = data;
    x->next = fontdata_list;
    fontdata_list = x;
}

void fontdata_list_release()
{
    fontdata_list_t *x, *next;

    for(x=fontdata_list; x; x=next) {
        next = x->next;
        free(x->name);
        x->data->release(x->data);
        free(x);
    }
}

fontdata_t* fontdata_list_find(const char *name)
{
    fontdata_list_t *x;

    for(x=fontdata_list; x; x=x->next) {
        if(str_icmp(x->name, name) == 0)
            return x->data;
    }

    return NULL;
}


/* ------------------------------------------------- */
/* bitmap fonts */
/* ------------------------------------------------- */

fontdata_t* fontdata_bmp_new(const char *source_file, const char *keymap, int sheet_source_x, int sheet_source_y, int sheet_width, int sheet_height, int char_width, int char_height)
{
    int i, len;
    const char *p;
    fontdata_bmp_t *f = mallocx(sizeof *f);
    image_t *img = image_load(source_file);

    ((fontdata_t*)f)->renderchar = fontdata_bmp_renderchar;
    ((fontdata_t*)f)->release = fontdata_bmp_release;
    ((fontdata_t*)f)->charspacing = fontdata_bmp_charspacing;
    ((fontdata_t*)f)->textsize = fontdata_bmp_textsize;

    /* validating */
    if(sheet_source_x < 0 || sheet_source_y < 0)
        fatal_error("Font script error: invalid (sheet_source_x,sheet_source_y) = (%d,%d) in '%s'. Both must be non-negative integers.", sheet_source_x, sheet_source_y, source_file);

    if(sheet_width <= 0 || sheet_height <= 0)
        fatal_error("Font script error: invalid (sheet_width,sheet_height) = (%d,%d) in '%s'. Both must be positive integers.", sheet_width, sheet_height, source_file);

    if(char_width <= 0 || char_height <= 0)
        fatal_error("Font script error: invalid (char_width, char_height) = (%d,%d) in '%s'. Both must be positive integers.", char_width, char_height, source_file);

    if(sheet_width % char_width != 0 || sheet_height % char_height != 0)
        fatal_error("Font script error: in '%s', sheet_width (%d) must be divisible by char_width (%d) and sheet_height (%d) must be divisible by char_height (%d).", source_file, sheet_width, char_width, sheet_height, char_height);

    /* initializing */
    f->charsize = v2d_new(char_width, char_height);
    for(i=0; i<256; i++)
        f->bmp[i] = NULL;

    /* creating the spritesheet */
    len = sheet_width / char_width;
    for(i=0,p=keymap; *p; p++,i++) {
        int x = i % len;
        int y = i / len;
        int c = (*p) & 0xFF;

        if(NULL == f->bmp[c])
            f->bmp[c] = image_create_shared(img, sheet_source_x + x*char_width, sheet_source_y + y*char_height, char_width, char_height);
    }

    /* done! ;) */
    image_unload(img);
    return (fontdata_t*)f;
}


void fontdata_bmp_renderchar(fontdata_t *fnt, image_t *img, int ch, int x, int y, uint32 color)
{
    fontdata_bmp_t *f = (fontdata_bmp_t*)fnt;
    image_t *char_image = f->bmp[ch & 0xFF];

    if(char_image != NULL) {
        if(color != image_rgb(255,255,255)) {
            int l, c;
            uint8 r, g, b;
            uint8 cr, cg, cb;
            uint32 px, mask = video_get_maskcolor();

            image_color2rgb(color, &cr, &cg, &cb);
            for(l=0; l<image_height(char_image); l++) {
                for(c=0; c<image_width(char_image); c++) {
                    px = image_getpixel(char_image, c, l);
                    if(px != mask) {
                        image_color2rgb(px, &r, &g, &b);
                        r &= cr; g &= cg; b &= cb;
                        image_putpixel(img, x+c, y+l, image_rgb(r,g,b));
                    }
                }
            }
        }
        else
            image_draw(char_image, img, x, y, IF_NONE);
    }
}

void fontdata_bmp_release(fontdata_t *fnt)
{
    fontdata_bmp_t *f = (fontdata_bmp_t*)fnt;
    int i;

    for(i=0; i<256; i++) {
        if(f->bmp[i] != NULL)
            image_destroy(f->bmp[i]);
    }

    free(f);
}

v2d_t fontdata_bmp_charspacing(fontdata_t *fnt)
{
    return v2d_new(1,1);
}

v2d_t fontdata_bmp_textsize(fontdata_t *fnt, const char *string)
{
    fontdata_bmp_t *f = (fontdata_bmp_t*)fnt;
    int cw = fnt->charspacing(fnt).x;
    char *s = remove_tags(string);
    int len = strlen(s);

    free(s);
    return v2d_new((f->charsize.x+cw) * len - cw, f->charsize.y);
}

/* ------------------------------------------------- */
/* ttf fonts */
/* ------------------------------------------------- */

fontdata_t* fontdata_ttf_new(const char *source_file, int size, int antialias, int shadow)
{
    char abs_path[1024], buf[16];
    int ch, w, h;
    fontdata_ttf_t *f = mallocx(sizeof *f);

    ((fontdata_t*)f)->renderchar = fontdata_ttf_renderchar;
    ((fontdata_t*)f)->release = fontdata_ttf_release;
    ((fontdata_t*)f)->charspacing = fontdata_ttf_charspacing;
    ((fontdata_t*)f)->textsize = fontdata_ttf_textsize;

    resource_filepath(abs_path, source_file, sizeof(abs_path), RESFP_READ);
    logfile_message("Loading TrueType font '%s'...", abs_path);

    f->ttf = alfont_load_font(abs_path);
    if(NULL != f->ttf) {
        /* configuring */
        alfont_set_font_size(f->ttf, size);
        f->antialias = allow_ttf_aa ? antialias : FALSE;
        f->shadow = shadow;

        /* caching commonly used characters */
        if(!(f->antialias)) {
            for(ch=32; ch<=127; ch++) {
                uszprintf(buf, sizeof(buf), "%c", ch); /* UTF-8 / ASCII compatibility */
                w = alfont_text_length(f->ttf, buf);
                h = alfont_text_height(f->ttf);
                f->cached_character[ch-32] = image_create(w, h);
                image_clear(f->cached_character[ch-32], video_get_maskcolor());
                alfont_textout_ex(IMAGE2BITMAP(f->cached_character[ch-32]), f->ttf, buf, 0, 0, image_rgb(255,255,255), -1);
            }
        }
    }
    else
        fatal_error("Couldn't load TrueType font '%s'", source_file);

    return (fontdata_t*)f;
}

void fontdata_ttf_renderchar(fontdata_t *fnt, image_t *img, int ch, int x, int y, uint32 color)
{
    fontdata_ttf_t *f = (fontdata_ttf_t*)fnt;
    int aa = f->antialias;
    uint32 black = image_rgb(0,0,0);
    static int sh = 0;

    /* sh effect */
    if(f->shadow && !sh++) {
        fontdata_ttf_renderchar(fnt, img, ch, 1+x, 1+y, black);
        fontdata_ttf_renderchar(fnt, img, ch, 0+x, 1+y, black);
        fontdata_ttf_renderchar(fnt, img, ch, 1+x, 0+y, black);
        sh = 0;
    }

    /* renderchar */
    if(!aa && (ch >= 32 && ch <= 127)) {
        /* this character is in cache */
        image_t *char_image = f->cached_character[ch-32];
        if(color != image_rgb(255,255,255)) {
            int l, c;
            uint8 r, g, b;
            uint8 cr, cg, cb;
            uint32 px, mask = video_get_maskcolor();

            image_color2rgb(color, &cr, &cg, &cb);
            for(l=0; l<image_height(char_image); l++) {
                for(c=0; c<image_width(char_image); c++) {
                    px = image_getpixel(char_image, c, l);
                    if(px != mask) {
                        image_color2rgb(px, &r, &g, &b);
                        r &= cr; g &= cg; b &= cb;
                        image_putpixel(img, x+c, y+l, image_rgb(r,g,b));
                    }
                }
            }
        }
        else
            image_draw(char_image, img, x, y, IF_NONE);
    }
    else {
        /* this character is not in cache */
        char buf[16];
        uszprintf(buf, sizeof(buf), "%lc", ch);
        (aa ? alfont_textout_aa_ex : alfont_textout_ex)(IMAGE2BITMAP(img), f->ttf, buf, x, y, (int)color, -1);
    }
}

void fontdata_ttf_release(fontdata_t *fnt)
{
    int ch;
    fontdata_ttf_t *f = (fontdata_ttf_t*)fnt;
    int aa = f->antialias;

    if(!aa) {
        for(ch=32; ch<=127; ch++)
            image_destroy(f->cached_character[ch-32]);
    }

    alfont_destroy_font(f->ttf);
    free(f);
}

v2d_t fontdata_ttf_charspacing(fontdata_t *fnt)
{
    fontdata_ttf_t *f = (fontdata_ttf_t*)fnt;
    return v2d_new(alfont_get_char_extra_spacing(f->ttf), 0);
}

v2d_t fontdata_ttf_textsize(fontdata_t *fnt, const char *string)
{
    fontdata_ttf_t *f = (fontdata_ttf_t*)fnt;
    v2d_t v = v2d_new(0,0);
    int aa = f->antialias;
    char *p, *s = remove_tags(string);

    /* if all characters in the string are cached, don't call alfont */
    for(p=s; *p; p++) {
        if(!aa && (*p >= 32/* && *p <= 127*/)) {
            image_t *char_image = f->cached_character[(int)(*p)-32];
            v.x += image_width(char_image);
            v.y = max(v.y, image_height(char_image));
        }
        else {
            v = v2d_new(alfont_text_length(f->ttf, s), alfont_text_height(f->ttf));
            break;
        }
    }

    free(s);
    return v;
}

