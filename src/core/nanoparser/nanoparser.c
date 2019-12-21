/*
 * nanoparser 1.1
 * A tiny stand-alone easy-to-use parser written in C
 * Copyright (c) 2010  Alexandre Martins <alemartf@gmail.com>
 * http://opensurge2d.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
 * to whom the Software is furnished to do so, subject to the following conditions:
 *   
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

/*
                                    INSTRUCTIONS

I)   Installation

        Just drop nanoparser.c and nanoparser.h into your project and you're done.

II)  Usage

        parsetree_program_t *tree;

        tree = nanoparser_construct_tree("my text file.txt");
        interpret(tree); // you write this
        tree = nanoparser_deconstruct_tree(tree);

III) "my text file.txt" example:

        // hello, this is a comment!

        resource "skybox"
        {
            type                TEXTURE
            properties {
                file            "images/space skybox.jpg"
                color           32 48 64        // rgb color, r=32, g=48, b=64
                speed           0.5 0.3         // x-speed, y-speed
                dimensions {
                    width       128
                    height      128
                }
            }
        }
*/


/*

Context-free grammar:

<program> ::= <statement> <program> | EMPTY
<statement> ::= STRING <parameter> <nl>
<parameter> ::= STRING <parameter> | <block> | EMPTY
<block> ::= <nq> '{' <nl> <program> '}'
<nl> ::= '\n' <nl> | '\n'
<nq> := '\n' | EMPTY

where:

    STRING is:
            a single-line double-quoted text (e.g., "Hello, world! Texts can be \"quoted\".")
            or
            a sequence of printable characters not in { ' ', '{', '}' } (e.g., hello_world)
            http://en.wikipedia.org/wiki/ASCII

    EMPTY is a zero-length symbol

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "nanoparser.h"

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE  1
#define FALSE 0

/*#define NANOPARSER_DEBUG_MODE*/
/*#define NANOPARSER_DISABLE_DOUBLE_QUOTES*/

#define GENERATE_INTERFACE_OF_EXPANDABLE_ARRAY(T)                                               \
                                                                                                \
    typedef struct expandable_array_##T {                                                       \
        T *_data; int _size; int _capacity;                                                     \
    } expandable_array_##T ;                                                                    \
                                                                                                \
    expandable_array_##T *expandable_array_##T##_new();                                         \
    expandable_array_##T *expandable_array_##T##_delete(expandable_array_##T *array);           \
    void expandable_array_##T##_push_back(expandable_array_##T *array, T element);              \
    void expandable_array_##T##_pop_back(expandable_array_##T *array);                          \
    T *expandable_array_##T##_at(expandable_array_##T *array, int index);                       \
    int expandable_array_##T##_size(expandable_array_##T *array)                           

#define GENERATE_IMPLEMENTATION_OF_EXPANDABLE_ARRAY(T)                                          \
                                                                                                \
    expandable_array_##T *expandable_array_##T##_new()                                          \
    {                                                                                           \
        expandable_array_##T *a = malloc(sizeof *a);                                            \
        a->_size = 0; a->_capacity = 8; a->_data = malloc_x(a->_capacity * sizeof(T));          \
        return a;                                                                               \
    }                                                                                           \
                                                                                                \
    expandable_array_##T *expandable_array_##T##_delete(expandable_array_##T *array)            \
    {                                                                                           \
        if(array) { free(array->_data); free(array); }                                          \
        return NULL;                                                                            \
    }                                                                                           \
                                                                                                \
    void expandable_array_##T##_push_back(expandable_array_##T *array, T element)               \
    {                                                                                           \
        if(array->_size >= array->_capacity) {                                                  \
            array->_capacity *= 2;                                                              \
            array->_data = realloc_x(array->_data, array->_capacity * sizeof(T));               \
        }                                                                                       \
        array->_data[ array->_size++ ] = element;                                               \
    }                                                                                           \
                                                                                                \
    void expandable_array_##T##_pop_back(expandable_array_##T *array)                           \
    {                                                                                           \
        if(array->_size > 0) array->_size--;                                                    \
    }                                                                                           \
                                                                                                \
    T *expandable_array_##T##_at(expandable_array_##T *array, int index)                        \
    {                                                                                           \
        int i = index;                                                                          \
        if(i < 0) { i = 0; } else if(i >= array->_size) { i = array->_size - 1; }               \
        return &(array->_data[i]);                                                              \
    }                                                                                           \
                                                                                                \
    int expandable_array_##T##_size(expandable_array_##T *array)                                \
    {                                                                                           \
        return array->_size;                                                                    \
    }                                                                                           \
                                                                                                \
    
#ifdef __cplusplus
extern "C" {
#endif

/* forward declarations */
typedef struct sourcelocation_t sourcelocation_t;



/* private structures */


/* a program is a list of statements */
struct parsetree_program_t {
    parsetree_statement_t *statement;
    parsetree_program_t *next;
};

/* a statement is a line containing an identifier (i.e., a string) followed by a parameter */
struct parsetree_statement_t {
    char *string;
    parsetree_parameter_t *parameter;
    sourcelocation_t *source_location; /* where is this statement located in the source code? */
};

/* a parameter is either:
   i)   another program;
   ii)  a string followed by another parameter */
struct parsetree_parameter_t {
    enum { VALUE, PROGRAM } type;
    union {
        struct {
            char *string;
            parsetree_parameter_t *next;
        } value;
        parsetree_program_t *program;
    } data;
    const parsetree_statement_t *stmt; /* I belong to stmt */
};



/* utilities */
static void (*error_fun)(const char*) = NULL;
static void (*warning_fun)(const char*) = NULL;
static void error(const char *fmt, ...); /* fatal error */
static void warning(const char *fmt, ...); /* warning */
static char* dirpath(const char *filepath); /* dirpath("f/folder/file.txt") = "f/folder/" */
static void* malloc_x(size_t bytes); /* our version of malloc */
static void* realloc_x(void *ptr, size_t bytes); /* our version of realloc */
static char* str_dup(const char *s); /* our version of strdup: duplicates s */
static char* r_trim(char *s); /* r_trim */



/* parse tree */
static parsetree_parameter_t* parsetree_parameter_new_value(const char *str, parsetree_parameter_t *nextparam);
static parsetree_parameter_t* parsetree_parameter_new_program(parsetree_program_t *prog);
static parsetree_parameter_t* parsetree_parameter_delete(parsetree_parameter_t* param);
static void parsetree_parameter_show(parsetree_parameter_t* param);

static parsetree_statement_t* parsetree_statement_new(const char *str, parsetree_parameter_t *parameter);
static parsetree_statement_t* parsetree_statement_delete(parsetree_statement_t* stmt);
static void parsetree_statement_show(parsetree_statement_t* stmt);

static parsetree_program_t* parsetree_program_new(parsetree_statement_t* stmt, parsetree_program_t* nextprog);
static parsetree_program_t* parsetree_program_delete(parsetree_program_t* prog);
static void parsetree_program_show(parsetree_program_t* prog);



/* virtual file (in-memory) */
GENERATE_INTERFACE_OF_EXPANDABLE_ARRAY(int);
GENERATE_IMPLEMENTATION_OF_EXPANDABLE_ARRAY(int);
static char* vfile_name; /* filename */
static expandable_array_int* vfile_contents; /* file contents */
static int vfile_ptr; /* current file pointer */

static void vfile_create(const char *name); /* creates the virtual file */
static void vfile_destroy(); /* destroys the virtual file */
static int vfile_getc(); /* getchar */
static int vfile_ungetc(int c); /* ungetchar */
static int vfile_putc(int c); /* putchar */
static void vfile_rewind(); /* rewind */



/* preprocessor */
typedef char* pchar;
GENERATE_INTERFACE_OF_EXPANDABLE_ARRAY(pchar);
GENERATE_IMPLEMENTATION_OF_EXPANDABLE_ARRAY(pchar);
static expandable_array_pchar* preprocessor_include_table; /* avoids infinite recursive inclusions */
static int preprocessor_line; /* current line number */

static void preprocessor_init();
static void preprocessor_release();
static void preprocessor_run(FILE *in, int depth); /* runs the preprocessor */
static void preprocessor_show(); /* shows the pre-processed file */
static void preprocessor_add_to_include_table(const char *filepath);
static int preprocessor_has_file_been_included(const char *filepath);



/* error detection: this is used to detect WHERE errors are located (after preprocessing) */
typedef struct {
    /* filename */
    char *filename;

    /* file [filename] starts at line [vline_start_line] of the virtual preprocessed file */
    int vfile_start_line;

    /* vfile_line_offset: used if there's another file included within filename (otherwise it's 0) */
    int vfile_line_offset;
} errorcontext;
GENERATE_INTERFACE_OF_EXPANDABLE_ARRAY(errorcontext);
GENERATE_IMPLEMENTATION_OF_EXPANDABLE_ARRAY(errorcontext);
static expandable_array_errorcontext* errorcontext_table;

static void errorcontext_init(); /* initializes the error context module */
static void errorcontext_release(); /* releases the erro context module */
static void errorcontext_add_to_table(const char *filename, int vfile_start_line, int vfile_line_offset); /* adds a new error context to its internal table */
static int errorcontext_detect_file_line(int vfile_line); /* provide a line number of the virtual file, and it will return you the line number of the real file at that place */
static const char* errorcontext_detect_file_name(int vfile_line); /* provide a line number of the virtual file, and it will return you the name of the real file at that place */
static errorcontext* errorcontext_find(int idx, int vfile_line); /* internal use only */





/* source location (used for improved error detection) */
/* this is an easy-to-use facade for errorcontext */
struct sourcelocation_t {
    char *file;
    int line;
};
static sourcelocation_t* sourcelocation_new(int vfile_line); /* must be called before errorcontext_release() */
static sourcelocation_t* sourcelocation_delete(sourcelocation_t* s);
static const char* sourcelocation_get_file(sourcelocation_t* s);
static int sourcelocation_get_line(sourcelocation_t* s);


/* lexical analyzer */
#define SYMBOL_MAXLENGTH        2048

typedef enum {
    SYM_EOF,
    SYM_NEWLINE,
    SYM_STRING,
    SYM_BEGINBLOCK,
    SYM_ENDBLOCK
} symbol_t;

static int line; /* current line number (after preprocessing phase) */
static symbol_t sym, oldsym; /* current/old token */
static char symdata[SYMBOL_MAXLENGTH+1], oldsymdata[SYMBOL_MAXLENGTH+1]; /* current/old token textual data */

static void getsym(); /* read the next token */
static void ungetsym(); /* put the last read token back into the stream */
static int read_hex_char(); /* reads a token of the form \xAB */



/* syntatic analyzer */
static parsetree_program_t* parse(); /* main parsing routine */
static int accept(symbol_t s); /* reads the next token and returns true iff s is found */
static int expect(symbol_t s); /* throws an error if s doesn't get accepted */



/* grammar rules */
static parsetree_program_t* program();
static parsetree_statement_t* statement();
static parsetree_parameter_t* parameter();
static parsetree_program_t* block();
static void nl();
static void nq();


/* tree traversal */
static int traversal_adapter(const parsetree_statement_t *stmt, void *eval_fun);





/* ---------------------------------------------
 * public methods of the parser
 * ---------------------------------------------- */

parsetree_program_t* nanoparser_construct_tree(const char *filepath)
{
    FILE *fp;
    parsetree_program_t *prog;

    fp = fopen(filepath, "r");
    if(fp != NULL) {
        /* creates the temporary virtual file */
        vfile_create(filepath);

        /* initializes the error context module (used for improved error detection) */
        errorcontext_init();
        errorcontext_add_to_table(filepath, 1, 0);

        /* calls the preprocessor */
        preprocessor_init();
        preprocessor_add_to_include_table(filepath); /* you can't #include yourself */
        preprocessor_run(fp, 0);
        preprocessor_release();

        /* calls the parser */
        prog = parse();

        /* releases the error context module */
        errorcontext_release();

        /* destroys the temporary virtual file */
        vfile_destroy();

        /* done! */
        fclose(fp);
    }
    else {
        prog = NULL;
        error("Couldn't open file \"%s\" for reading.", filepath);
    }

    return prog;
}

parsetree_program_t* nanoparser_deconstruct_tree(parsetree_program_t *tree)
{
    tree = parsetree_program_delete(tree);
    return tree;
}

void nanoparser_set_error_function(void (*fun)(const char*))
{
    error_fun = fun;
}

void nanoparser_set_warning_function(void (*fun)(const char*))
{
    warning_fun = fun;
}


/* ---------------------------------------------
 * lexical analysis
 * ---------------------------------------------- */

/* this is the lexer */
void getsym()
{
    int i = 0;
    unsigned int c;
    char *p = symdata;

    /* create a backup */
    oldsym = sym;
    strcpy(oldsymdata, symdata);

    /* skip white spaces */
    do {
        c = vfile_getc();
    } while(c != '\n' && isspace(c));

    /* deciding which symbol comes next */
    if(c == EOF) {
        sym = SYM_EOF;
        *(p++) = (char)c;
    }
    else if(c == '\n') {
        sym = SYM_NEWLINE;
        *(p++) = (char)c;
        ++line;
    }
    else if(c == '{') {
        sym = SYM_BEGINBLOCK;
        *(p++) = (char)c;
    }
    else if(c == '}') {
        sym = SYM_ENDBLOCK;
        *(p++) = (char)c;
    }
    else if(c >= 0x20) {
        sym = SYM_STRING;
#ifndef NANOPARSER_DISABLE_DOUBLE_QUOTES
        if(c != '"') {
#endif
            /* non-quoted string */
            while(c >= 0x20 && c != EOF && !isspace(c) && c != '{' && c != '}' && ++i <= SYMBOL_MAXLENGTH) { /* printable character */
                *(p++) = (char)c;
                c = vfile_getc();
            }
            vfile_ungetc(c);
#ifndef NANOPARSER_DISABLE_DOUBLE_QUOTES
        }
        else {
            /* double-quoted string */
            c = vfile_getc(); /* discard '"' */
            while(c >= 0x20 && c != '"' && c != EOF && ++i <= SYMBOL_MAXLENGTH) {
                if(c == '\n') {
                    error(
                        "Unexpected end of string in \"%s\" on line %d.",
                        errorcontext_detect_file_name(line),
                        errorcontext_detect_file_line(line)
                    );
                }
                else if(c == '\\') {
                    int h = vfile_getc();
                    switch(h) {
                        case '"':  c = '"';  break;
                        case 'n':  c = '\n'; break;
                        case 't':  c = '\t'; break;
                        case '\\': c = '\\'; break;
                        case 'x': c = read_hex_char(); break;
                        default:
                            error(
                                "Invalid character '\\%c' in \"%s\" on line %d. Did you mean '\\\\'?",
                                h,
                                errorcontext_detect_file_name(line),
                                errorcontext_detect_file_line(line)
                            );
                            break;
                    }
                }

                *(p++) = (char)c;
                c = vfile_getc();
            }

            if(c != '"') /* discard '"' */
                vfile_ungetc(c);
        }
#endif
    }
    else {
        error(
            "Lexical error in \"%s\" on line %d: unknown symbol \"%c\" (%d).",
            errorcontext_detect_file_name(line),
            errorcontext_detect_file_line(line),
            c,
            c
        );
    }

    *p = 0;
}

void ungetsym()
{
    char *str = symdata;
    int i;

    /* putting the symbol back into the stream */
    vfile_ungetc(' ');
    for(i=strlen(str)-1; i>=0; i--) {
        vfile_ungetc((int)str[i]);
        if(str[i] == '\n')
            --line;
    }

    /* restoring the backup */
    strcpy(symdata, oldsymdata);
    sym = oldsym;
}

int accept(symbol_t s)
{
    if(sym == s) {
        getsym();
        return TRUE;
    }
    else
        return FALSE;
}

int expect(symbol_t s)
{
    if(!accept(s)) {
        error(
            "Syntax error in \"%s\" on line %d: unexpected symbol \"%s\".",
            errorcontext_detect_file_name(line),
            errorcontext_detect_file_line(line),
            symdata
        );
        return FALSE;
    }
    else
        return TRUE;
}

int read_hex_char()
{
    int a, b;

    a = tolower(vfile_getc());
    if(isdigit(a) || (a >= 'a' && a <= 'f')) {
        b = tolower(vfile_getc());
        if(isdigit(b) || (b >= 'a' && b <= 'f'))
            return ((isdigit(a) ? a-'0' : 10+(a-'a')) << 4) | (isdigit(b) ? b-'0' : 10+(b-'a'));
    }

    error(
        "Invalid token in \"%s\" on line %d.",
        errorcontext_detect_file_name(line),
        errorcontext_detect_file_line(line)
    );

    return '?';
}


/* ---------------------------------------------
 * syntatic analysis: parser
 * ---------------------------------------------- */

parsetree_program_t* parse()
{
    parsetree_program_t *prog;

    line = 1;
    getsym(); /* reads the first symbol */
    while(accept(SYM_NEWLINE)); /* skips newlines */
    prog = program(); /* generates the syntatic tree */
    expect(SYM_EOF); /* expects an EOF character */

    return prog;
}





/* ---------------------------------------------
 * syntax analysis: grammar rules
 * ---------------------------------------------- */

parsetree_program_t* program()
{
    parsetree_program_t *prog = NULL;

    if(sym != SYM_EOF && sym != SYM_ENDBLOCK) {
        parsetree_statement_t *stmt = statement();
        prog = parsetree_program_new(
            stmt,
            program()
        );
    }
    else
        ; /* empty */

    return prog;
}

parsetree_statement_t* statement()
{
    parsetree_statement_t *stmt = NULL;
    char *str = str_dup(symdata);

    expect(SYM_STRING);
    stmt = parsetree_statement_new(
        str,
        parameter()
    );
    if(sym != SYM_EOF)
        nl();

    free(str);
    return stmt;
}

parsetree_parameter_t* parameter()
{
    parsetree_parameter_t *param = NULL;

    if(sym == SYM_STRING) {
        char *str = str_dup(symdata);
        accept(SYM_STRING);
        param = parsetree_parameter_new_value(
            str,
            parameter()
        );
        free(str);
    }
    else if(sym == SYM_BEGINBLOCK) {
        param = parsetree_parameter_new_program(
            block()
        );
    }
    else if(sym == SYM_NEWLINE) {
        /* lookahead: do we have a block? */
        int blk;

        getsym();
        blk = (sym == SYM_BEGINBLOCK);
        ungetsym();

        if(blk) {
            param = parsetree_parameter_new_program(
                block()
            );
        }
    }
    else
        ; /* empty */

    return param;
}

parsetree_program_t* block()
{
    parsetree_program_t *prog = NULL;

    nq();
    expect(SYM_BEGINBLOCK);
    nl();
    prog = program();
    expect(SYM_ENDBLOCK);

    return prog;
}

void nq()
{
    accept(SYM_NEWLINE);
}

void nl()
{
    expect(SYM_NEWLINE);
    while(accept(SYM_NEWLINE));
}




/* ---------------------------------------------
 * syntax analysis: parse tree manipulation
 * ---------------------------------------------- */

parsetree_parameter_t* parsetree_parameter_new_value(const char *str, parsetree_parameter_t *nextparam)
{
    parsetree_parameter_t *p = malloc_x(sizeof *p);
    p->type = VALUE;
    p->data.value.string = str_dup(str);
    p->data.value.next = nextparam;
    p->stmt = NULL;
    return p;
}

parsetree_parameter_t* parsetree_parameter_new_program(parsetree_program_t *prog)
{
    parsetree_parameter_t *p = malloc_x(sizeof *p);
    p->type = PROGRAM;
    p->data.program = prog;
    p->stmt = NULL;
    return p;
}

parsetree_parameter_t* parsetree_parameter_delete(parsetree_parameter_t* param)
{
    if(param != NULL) {
        if(param->type == VALUE) {
            free(param->data.value.string);
            param->data.value.string = NULL;
            param->data.value.next = parsetree_parameter_delete(param->data.value.next);
        }
        else
            param->data.program = parsetree_program_delete(param->data.program);

        free(param);
    }

    return NULL;
}

void parsetree_parameter_show(parsetree_parameter_t* param)
{
    printf("[ ");
    if(param != NULL) {
        if(param->type == PROGRAM) {
            printf("\n");
            parsetree_program_show(param->data.program);
            printf("\n");
        }
        else {
            printf("%s ", param->data.value.string);
            parsetree_parameter_show(param->data.value.next);
        }
    }
    printf(" ] ");
}

parsetree_statement_t* parsetree_statement_new(const char *str, parsetree_parameter_t *parameter)
{
    parsetree_statement_t *p = malloc_x(sizeof *p);
    parsetree_parameter_t *element;
    int j, n;

    p->string = str_dup(str);
    p->parameter = parameter;
    p->source_location = sourcelocation_new(line-1);

    n = nanoparser_get_number_of_parameters(parameter);
    for(j=1; j<=n; j++) {
        element = (parsetree_parameter_t*)nanoparser_get_nth_parameter(parameter, j);
        element->stmt = p;
    }

    return p;
}

parsetree_statement_t* parsetree_statement_delete(parsetree_statement_t* stmt)
{
    if(stmt != NULL) {
        stmt->source_location = sourcelocation_delete(stmt->source_location);
        free(stmt->string);
        stmt->string = NULL;
        stmt->parameter = parsetree_parameter_delete(stmt->parameter);
        free(stmt);
    }

    return NULL;
}

void parsetree_statement_show(parsetree_statement_t* stmt)
{
    if(stmt != NULL) {
        printf("%s := ", stmt->string);
        parsetree_parameter_show(stmt->parameter);
        printf("\n");
    }
}

parsetree_program_t* parsetree_program_new(parsetree_statement_t *stmt, parsetree_program_t *nextprog)
{
    parsetree_program_t *p = malloc_x(sizeof *p);
    p->statement = stmt;
    p->next = nextprog;
    return p;
}

parsetree_program_t* parsetree_program_delete(parsetree_program_t *prog)
{
    parsetree_program_t *next = NULL;

    for(; prog; prog = next) { /* we should avoid using recursion here... 'prog' can be HUGE! */
        next = prog->next;
        parsetree_statement_delete(prog->statement);
        free(prog);
    }

    return NULL;
}

void parsetree_program_show(parsetree_program_t* prog)
{
    if(prog != NULL) {
        parsetree_statement_show(prog->statement);
        parsetree_program_show(prog->next);
    }
}



/* ---------------------------------------------
 * virtual files
 * ---------------------------------------------- */

void vfile_create(const char *name)
{
    vfile_ptr = 0;
    vfile_name = str_dup(name);
    vfile_contents = expandable_array_int_new();
}

void vfile_destroy()
{
    vfile_contents = expandable_array_int_delete(vfile_contents);
    free(vfile_name);
    vfile_name = NULL;
    vfile_ptr = 0;
}

int vfile_getc()
{
    if(vfile_ptr < expandable_array_int_size(vfile_contents)) {
        int *ptr = expandable_array_int_at(vfile_contents, vfile_ptr++);
        return *ptr;
    }
    else
        return EOF;
}

int vfile_ungetc(int c)
{
    if(vfile_ptr > 0 && c != EOF) {
        int *ptr = expandable_array_int_at(vfile_contents, --vfile_ptr);
        *ptr = c;
        return *ptr;
    }
    else
        return EOF;
}

int vfile_putc(int c)
{
    int size = expandable_array_int_size(vfile_contents);

    if(vfile_ptr < size) {
        int *ptr = expandable_array_int_at(vfile_contents, vfile_ptr++);
        *ptr = c;
    }
    else {
        expandable_array_int_push_back(vfile_contents, c);
        vfile_ptr = size+1;
    }

    return c;
}

void vfile_rewind()
{
    vfile_ptr = 0;
}




/* ---------------------------------------------
 * preprocessor
 * ---------------------------------------------- */

void preprocessor_init()
{
    preprocessor_line = 1;
    preprocessor_include_table = expandable_array_pchar_new();
}

void preprocessor_release()
{
    int i, len = expandable_array_pchar_size(preprocessor_include_table);

    for(i=0; i<len; i++) {
        char **p = expandable_array_pchar_at(preprocessor_include_table, i);
        free(*p);
        *p = NULL;
    }

    preprocessor_include_table = expandable_array_pchar_delete(preprocessor_include_table);
    preprocessor_line = 1;
    vfile_rewind();
}

void preprocessor_show()
{
#ifdef NANOPARSER_DEBUG_MODE
    int c;

    vfile_rewind();
    while(EOF != (c=vfile_getc()))
        putchar(c);
    vfile_rewind();
#endif
}

void preprocessor_run(FILE *in, int depth)
{
    int c;
    int line_start = TRUE;

    while(EOF != (c = fgetc(in))) {
        /* do nothing with double-quoted strings */
        if(c == '"') {
            int old = c;
            vfile_putc(c);
            c = fgetc(in);
            while(((c != '"') || (old == '\\' && c == '"')) && c != EOF && c != '\n') {
                vfile_putc(c);
                old = c;
                c = fgetc(in);
            }
        }

        /* ignore comments */
        if(c == '/') {
            int h = fgetc(in);
            if(h == '/') {
                do {
                    c = fgetc(in);
                } while(c != '\n' && c != EOF);
            }
            else
                ungetc(h, in);
        }

        /* preprocessor directives */
        if(c == '#' && line_start) {
            char key[1+512]="", value[1+512]="";
            int key_len = 0, value_len = 0;
            int quot = FALSE;
            char *p;

            /* read key */
            p = key;
            do {
                *(p++) = c;
                c = fgetc(in);
            } while(!isspace(c) && c != '\n' && c != EOF && ++key_len < 512);
            *p = 0;

            /* read value */
            p = value;
            while(c != '\n' && isspace(c)) /* skip spaces */
                c = fgetc(in);
            while(c != '\n' && c != EOF && value_len++ < 512) {
                if(c == '/' && !quot) {
                    int h = fgetc(in);
                    if(h == '/')
                        break;
                    else
                        ungetc(h, in);
                }
                if(c != '"')
                    *(p++) = c;
                else
                    quot = !quot;
                c = fgetc(in);
            }
            *p = 0;
            r_trim(value);

            /* #include has been deprecated */
            if(strcmp(key, "#include") == 0) {
                char *dir, *fullpath;
                const char *ext = strrchr(value, '.');
                const char *vfile_ext = strrchr(vfile_name, '.');
                void (*deprecated_error)(const char*, ...) = error;

                if(vfile_ext != NULL && ext != NULL && strcmp(vfile_ext, ".obj") == 0 && strcmp(ext, ".inc") == 0)
                    deprecated_error = warning; /* soft error (legacy) */

                deprecated_error(
                    "The #include directive has been deprecated and must no longer be used (see %s:%d)",
                    errorcontext_detect_file_name(preprocessor_line),
                    errorcontext_detect_file_line(preprocessor_line)
                );

                dir = dirpath(vfile_name);
                fullpath = malloc_x((1+strlen(value)+strlen(dir)) * sizeof(*fullpath));
                strcpy(fullpath, dir);
                strcat(fullpath, value);

                if(strstr(value, "..") != NULL || !isalnum(value[0])) {
                    error(
                        "Preprocessor error in \"%s\" on line %d: couldn't include file \"%s\".",
                        errorcontext_detect_file_name(preprocessor_line),
                        errorcontext_detect_file_line(preprocessor_line),
                        fullpath
                    );
                }

                if(!preprocessor_has_file_been_included(fullpath)) {
                    FILE *fp = fopen(fullpath, "r");
                    preprocessor_add_to_include_table(fullpath);
                    if(fp != NULL) {
                        char *old_vfile_name = vfile_name;
                        const char *me = errorcontext_detect_file_name(preprocessor_line);
                        int mel = errorcontext_detect_file_line(preprocessor_line);
                        errorcontext_add_to_table(fullpath, preprocessor_line, 0);

                        vfile_name = str_dup(fullpath);
                        preprocessor_run(fp, depth+1);
                        free(vfile_name);
                        vfile_name = old_vfile_name;

                        errorcontext_add_to_table(me, preprocessor_line, mel);
                        fclose(fp);
                    }
                    else {
                        error(
                            "Preprocessor error in \"%s\" on line %d: couldn't include file \"%s\".",
                            errorcontext_detect_file_name(preprocessor_line),
                            errorcontext_detect_file_line(preprocessor_line),
                            fullpath
                        );
                    }

                    free(fullpath);
                    free(dir);

                    line_start = TRUE;
                    continue;
                }
                else {
                    error(
                        "Preprocessor error in \"%s\" on line %d: file \"%s\" has already been included.",
                        errorcontext_detect_file_name(preprocessor_line),
                        errorcontext_detect_file_line(preprocessor_line),
                        fullpath
                    );
                }
            }
            else if(strcmp(key, "#") == 0) {
                /* ignore comments */
                ;
            }
            else {
                /* we'll consider unknown pre-processor commands as being comments */
                warning(
                    "Preprocessor error in \"%s\" on line %d: unknown command \"%s %s\".",
                    errorcontext_detect_file_name(preprocessor_line),
                    errorcontext_detect_file_line(preprocessor_line),
                    key,
                    value
                );
            }

        }

        /* new line... */
        if(c == '\n') {
            line_start = TRUE;
            preprocessor_line++;
        }
        else if(!isspace(c))
            line_start = FALSE;

        /* accept this character */
        if(c != EOF)
            vfile_putc(c);
        else if(feof(in))
            vfile_putc(c);
        else if(ferror(in))
            error("Error reading from stream '%s'", vfile_name);
    }

    if(depth == 0) {
        /* rewinds the virtual file */
        vfile_rewind();

        /* debug information */
        preprocessor_show();
    }
}

int preprocessor_has_file_been_included(const char *filename)
{
    int i, len = expandable_array_pchar_size(preprocessor_include_table);
    char *file;

    for(i=0; i<len; i++) {
        file = *(expandable_array_pchar_at(preprocessor_include_table, i));
        if(strcmp(file, filename) == 0)
            return TRUE;
    }

    return FALSE;
}

void preprocessor_add_to_include_table(const char *filepath)
{
    expandable_array_pchar_push_back(preprocessor_include_table, (pchar)str_dup(filepath));
}



/* ---------------------------------------------
 * improved error detection
 * ---------------------------------------------- */

void errorcontext_init()
{
    errorcontext_table = expandable_array_errorcontext_new();
}

void errorcontext_release()
{
    int i, size = expandable_array_errorcontext_size(errorcontext_table);

    for(i=0; i<size; i++)
        free( expandable_array_errorcontext_at(errorcontext_table, i)->filename );

    errorcontext_table = expandable_array_errorcontext_delete(errorcontext_table);
}

void errorcontext_add_to_table(const char *filename, int vfile_start_line, int vfile_line_offset)
{
    errorcontext ctx;
    ctx.filename = str_dup(filename);
    ctx.vfile_start_line = vfile_start_line;
    ctx.vfile_line_offset = vfile_line_offset;
    expandable_array_errorcontext_push_back(errorcontext_table, ctx);
}

errorcontext* errorcontext_find(int idx, int vfile_line)
{
    int size = expandable_array_errorcontext_size(errorcontext_table);

    if(idx < 0)
        return expandable_array_errorcontext_at(errorcontext_table, 0);
    else if(idx >= size)
        return expandable_array_errorcontext_at(errorcontext_table, size-1);
    else if(vfile_line < expandable_array_errorcontext_at(errorcontext_table, idx)->vfile_start_line)
        return expandable_array_errorcontext_at(errorcontext_table, idx>0 ? idx-1 : 0);
    else
        return errorcontext_find(idx+1, vfile_line);
}

int errorcontext_detect_file_line(int vfile_line)
{
    errorcontext *c = errorcontext_find(0, vfile_line);
    return 1 + vfile_line - c->vfile_start_line + c->vfile_line_offset;
}

const char* errorcontext_detect_file_name(int vfile_line)
{
    errorcontext *c = errorcontext_find(0, vfile_line);
    return c->filename;
}



/* ---------------------------------------------
 * tree traversal
 * ---------------------------------------------- */
void nanoparser_traverse_program(const parsetree_program_t *program, int (*eval)(const parsetree_statement_t*))
{
    nanoparser_traverse_program_ex(program, (void*)eval, traversal_adapter);
}

void nanoparser_traverse_program_ex(const parsetree_program_t *program, void *some_user_data, int (*eval)(const parsetree_statement_t*,void*))
{
    const parsetree_program_t *p;

    for(p = program; p != NULL; p = p->next) {
        if(eval(p->statement, some_user_data) != 0)
            break;
    }
}

int traversal_adapter(const parsetree_statement_t *stmt, void *eval_fun)
{
    int (*eval)(const parsetree_statement_t*) = (int (*)(const parsetree_statement_t*))eval_fun;
    return eval(stmt);
}


/* ---------------------------------------------
 * source location
 * ---------------------------------------------- */
sourcelocation_t* sourcelocation_new(int vfile_line)
{
    sourcelocation_t *s = malloc_x(sizeof *s);
    s->file = str_dup(errorcontext_detect_file_name(vfile_line));
    s->line = errorcontext_detect_file_line(vfile_line);
    return s;
}

sourcelocation_t* sourcelocation_delete(sourcelocation_t* s)
{
    free(s->file);
    free(s);
    return NULL;
}

const char* sourcelocation_get_file(sourcelocation_t* s)
{
    return s->file;
}

int sourcelocation_get_line(sourcelocation_t* s)
{
    return s->line;
}


/* ---------------------------------------------
 * statement handling
 * ---------------------------------------------- */

const char* nanoparser_get_identifier(const parsetree_statement_t *stmt)
{
    return stmt->string;
}

const parsetree_parameter_t* nanoparser_get_parameter_list(const parsetree_statement_t *stmt)
{
    return stmt->parameter;
}

const char* nanoparser_get_file(const parsetree_statement_t *stmt)
{
    return (stmt != NULL) ? sourcelocation_get_file(stmt->source_location) : "null";
}

int nanoparser_get_line_number(const parsetree_statement_t *stmt)
{
    return (stmt != NULL) ? sourcelocation_get_line(stmt->source_location) : -1;
}


/* ---------------------------------------------
 * data retrieval
 * ---------------------------------------------- */

int nanoparser_get_number_of_parameters(const parsetree_parameter_t *param_list)
{
    if(param_list != NULL) {
        if(param_list->type == VALUE)
            return 1 + nanoparser_get_number_of_parameters(param_list->data.value.next);
        else
            return 1;
    }
    else
        return 0;
}

const parsetree_parameter_t* nanoparser_get_nth_parameter(const parsetree_parameter_t *param_list, int n)
{
    if(param_list != NULL && n >= 1) {
        if(n == 1)
            return param_list;
        else if(param_list->type == VALUE)
            return nanoparser_get_nth_parameter(param_list->data.value.next, n-1);
    }

    return NULL;
}

void nanoparser_expect_string(const parsetree_parameter_t *param, const char *error_message)
{
    if(param == NULL)
        error("%s", error_message);
    else if(param != NULL && param->type != VALUE)
        error("%s\nin \"%s\" near line %d", error_message, nanoparser_get_file(param->stmt), nanoparser_get_line_number(param->stmt));
}

void nanoparser_expect_program(const parsetree_parameter_t *param, const char *error_message)
{
    if(param == NULL)
        error("%s", error_message);
    else if(param != NULL && param->type != PROGRAM)
        error("%s\nin \"%s\" near line %d", error_message, nanoparser_get_file(param->stmt), nanoparser_get_line_number(param->stmt));
}

const char* nanoparser_get_string(const parsetree_parameter_t *param)
{
    if(param != NULL && param->type == VALUE)
        return param->data.value.string;
    else
        return "null";
}

const parsetree_program_t* nanoparser_get_program(const parsetree_parameter_t *param)
{
    if(param != NULL && param->type == PROGRAM)
        return param->data.program;
    else
        return NULL;
}


/* ---------------------------------------------
 * operations
 * --------------------------------------------- */
parsetree_program_t* nanoparser_append_program(parsetree_program_t *dest, parsetree_program_t *src)
{
    if(dest != NULL) {
        parsetree_program_t *node = dest;
        while(node->next != NULL)
            node = node->next;
        node->next = src;
        return dest;
    }
    else
        return src;
}


/* ---------------------------------------------
 * utilities
 * ---------------------------------------------- */

char* dirpath(const char *filepath)
{
    char *p, *str = str_dup(filepath);

    if(NULL == (p=strrchr(str, '/'))) {
        if(NULL == (p=strrchr(str, '\\'))) {
            *str = 0;
            return str;
        }
    }

    *(++p) = 0;
    return str;
}

void error(const char *fmt, ...)
{
    char buf[1024] = "nanoparser error! ";
    int len = strlen(buf);
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
    va_end(args);

    if(error_fun)
        error_fun(buf);
    else
        fprintf(stderr, "%s\n", buf);

    exit(1);
}

void warning(const char *fmt, ...)
{
    char buf[1024] = "nanoparser warning! ";
    int len = strlen(buf);
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
    va_end(args);

    if(warning_fun)
        warning_fun(buf);
    else
        fprintf(stderr, "%s\n", buf);
}

char* str_dup(const char *s)
{
    char *p = malloc_x( (1 + strlen(s)) * sizeof(char) );
    return strcpy(p, s);
}

char* r_trim(char *s)
{
    char *p;

    if(NULL != (p=strrchr(s, ' ')))
        *p = 0;

    if(NULL != (p=strrchr(s, '\t')))
        *p = 0;

    if(NULL != (p=strrchr(s, '\r')))
        *p = 0;

    if(NULL != (p=strrchr(s, '\n')))
        *p = 0;

    return s;
}

void* malloc_x(size_t bytes)
{
    void *m = malloc(bytes);

    if(m == NULL) {
        fprintf(stderr, __FILE__ ": Out of memory");
        error(__FILE__ ": Out of memory");
        exit(1);
    }

    return m;
}

void* realloc_x(void *ptr, size_t bytes)
{
    void *m = realloc(ptr, bytes);

    if(m == NULL) {
        fprintf(stderr, __FILE__ ": Out of memory (realloc_x)");
        error(__FILE__ ": Out of memory (realloc_x)");
        exit(1);
    }

    return m;
}

#ifdef __cplusplus
}
#endif

