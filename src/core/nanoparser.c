/*
 * Open Surge Engine
 * nanoparser.c - nanoparser v2
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "nanoparser.h"
#include "../util/darray.h"
#include "../util/util.h"
#include "../util/stringutil.h"

/*
 * nanoparser v2
 *
 * Grammar:
 * <root>      ::= <program> eof
 * <program>   ::= <br*> <statement>
 * <statement> ::= identifier <parameter> \n <br*> <statement> | empty
 * <parameter> ::= string <parameter> | identifier <parameter> | <br*> <block> | empty
 * <block>     ::= { \n <program> }
 * <br*>       ::= \n <br*> | empty
 *
 * where:
 * string is a double-quoted, single-quoted or unquoted text (e.g., "Hello, world!", 'Let\'s go!', hello-world)
 * identifier is a unquoted text that matches /^[A-Za-z_<>][A-Za-z0-9_<>]*$/
 * empty is a symbol of length zero
 * eof is the end of the file
 *
 * Write bytes using "\xhh" (hexadecimal) and
 * unicode code points using "\uhhhh" (will be encoded as UTF-8)
 *
 * // Single-line or / * multi-line
 *                       comments   * / are ignored
 */



/*
 * ERROR FUNCTIONS
 */

static void crash(const char* fmt, ...);
static void crash_fun_default(const char* message);
static void (*crash_fun)(const char*) = crash_fun_default;

static void warning(const char* fmt, ...);
static void warning_fun_default(const char* message);
static void (*warning_fun)(const char*) = warning_fun_default;

#define nanoassert(expr) do { \
    if(!(expr)) { \
        crash("Assertion failed: " #expr " at %s:%d", __FILE__, __LINE__); \
    } \
} while(0)

#define ERROR_MAXLENGTH 1023
#define ERROR_PREFIX "[nanoparser] "
static const char _ERROR_PREFIX[] = ERROR_PREFIX;
static const int ERROR_PREFIX_LENGTH = sizeof(_ERROR_PREFIX) - 1;




/*
 * BASIC TYPES
 */

/* A program contains a list of statements */
struct parsetree_program_t
{
    parsetree_statement_t* statement; /* first statement (head of a linked list; may be NULL) */
    const parsetree_program_t* parent; /* NULL if root */
};

/* The root of a parse tree, which corresponds to a file. It's a program.
   This is for backwards compatibility with the nanoparser v1 API */
typedef struct parsetree_root_t parsetree_root_t;
struct parsetree_root_t
{
    parsetree_program_t program; /* base class */
    char* filepath; /* path to the source file */
};

/* A statement is an identifier followed by a (possibly empty) list of parameters */
struct parsetree_statement_t
{
    char* identifier; /* an identifier */
    parsetree_parameter_t* parameter; /* a list of parameters */
    int line; /* line number in the source file associated with the program */
    const parsetree_program_t* program; /* the program to which this statement belongs */

    parsetree_statement_t* next; /* next node (linked list) */
};

/* A parameter is either: a) a string followed by another parameter or b) another program */
struct parsetree_parameter_t
{
    enum { PARAMETER_STRING, PARAMETER_BLOCK } type;
    union {
        char* string;
        parsetree_program_t* program;
    };
    const parsetree_statement_t* statement; /* the statement to which this parameter belongs */

    parsetree_parameter_t* next; /* next node (linked list) */
};

static const parsetree_parameter_t NULL_PARAMETER = {
    .type = -1,
    .string = "",
    .statement = NULL,
    .next = NULL
};

static int traverse_adapter(const parsetree_statement_t* statement, void* user_data);
static parsetree_program_t* release_program(parsetree_program_t* program);
static parsetree_statement_t* release_statement(parsetree_statement_t* statement);
static parsetree_parameter_t* release_parameter(parsetree_parameter_t* parameter);



/*
 * TOKENS
 */

#define WRITE_TOKEN_TYPE(x, y) x
#define WRITE_TOKEN_NAME(x, y) y
#define FOREACH_TOKEN(F) \
    F(TOKEN_EOF, "end of file"), \
    F(TOKEN_IDENTIFIER, "identifier"), \
    F(TOKEN_STRING, "string"), \
    F(TOKEN_BLOCKSTART, "{"), \
    F(TOKEN_BLOCKEND, "}"), \
    F(TOKEN_LINEBREAK, "line break")

typedef enum nanotokentype_t nanotokentype_t;
enum nanotokentype_t
{
    FOREACH_TOKEN(WRITE_TOKEN_TYPE)
};

static const char* TOKEN_NAME[] =
{
    FOREACH_TOKEN(WRITE_TOKEN_NAME)
};

typedef struct nanotoken_t nanotoken_t;
struct nanotoken_t
{
    nanotokentype_t type;
    int line;
    char* value;
    size_t value_size;
};

#define token_create(type, value, value_size, line) (nanotoken_t){ (type), (line), memcpy(malloc(value_size), (value), (value_size)), (value_size) }
#define token_destroy(token) do { free((token).value); (token).value = NULL; } while(0)



/*
 * LEXICAL ANALYSIS
 */

#define LEXER_SYMBOL_MAXLENGTH 4095 /* long strings; use 2^n - 1 */
#define LEXER_BUFFER_SIZE 4096 /* use 2^n */

typedef struct nanofilestate_t nanofilestate_t;
struct nanofilestate_t
{
    ALLEGRO_FILE* fp;
    int cursor;
    char buffer[2 * LEXER_BUFFER_SIZE];
    int line;
    int last;
    bool locked;
};

typedef struct nanolexer_t nanolexer_t;
struct nanolexer_t
{
    char* filepath;
    DARRAY(nanotoken_t, token);
};

static nanolexer_t* lexer_create(const char* filepath);
static nanolexer_t* lexer_destroy(nanolexer_t* lexer);

static bool lexer_read(nanolexer_t* lexer, ALLEGRO_FILE* fp);
static int lexer_getc(nanofilestate_t* state);
static int lexer_ungetc(nanofilestate_t* state);



/*
 * SYNTAX ANALYSIS
 */

typedef struct nanoparser_t nanoparser_t;
struct nanoparser_t
{
    const nanolexer_t* lexer;
    int cursor;
};

static nanoparser_t* parser_create(const nanolexer_t* lexer);
static nanoparser_t* parser_destroy(nanoparser_t* parser);

static const nanotoken_t* parser_lookahead(const nanoparser_t* parser);
static bool parser_check(const nanoparser_t* parser, nanotokentype_t token_type);
static void parser_expect(const nanoparser_t* parser, nanotokentype_t token_type);
static void parser_match(nanoparser_t* parser, nanotokentype_t token_type);
static void parser_unmatch(nanoparser_t* parser, nanotokentype_t token_type);

static parsetree_root_t* parser_parse_root(nanoparser_t* parser);
static parsetree_program_t* parser_parse_program(nanoparser_t* parser, const parsetree_program_t* parent);
static parsetree_statement_t* parser_parse_statement(nanoparser_t* parser, const parsetree_program_t* program);
static parsetree_parameter_t* parser_parse_parameter(nanoparser_t* parser, const parsetree_statement_t* statement);




/*
 * LOADING & UNLOADING
 */


/*
 * nanoparser_construct_tree()
 * Parse a file and construct a parse tree
 */
parsetree_program_t* nanoparser_construct_tree(const char* filepath)
{
    warning("Reading file %s...", filepath);

    nanolexer_t* lexer = lexer_create(filepath);
    nanoparser_t* parser = parser_create(lexer);

    parsetree_root_t* root = parser_parse_root(parser);

    parser_destroy(parser);
    lexer_destroy(lexer);

    return (parsetree_program_t*)root;
}

/*
 * nanoparser_deconstruct_tree()
 * Release a parse tree
 */
parsetree_program_t* nanoparser_deconstruct_tree(parsetree_program_t* root)
{
    free(((parsetree_root_t*)root)->filepath);
    return release_program(root);
}

/*
 * release_program()
 * Release a program
 */
parsetree_program_t* release_program(parsetree_program_t* program)
{
    release_statement(program->statement);
    free(program);

    return NULL;
}

/*
 * release_statement()
 * Release a list of statements
 */
parsetree_statement_t* release_statement(parsetree_statement_t* statement)
{
    while(statement != NULL) {
        parsetree_statement_t* next = statement->next;

        release_parameter(statement->parameter);
        free(statement->identifier);
        free(statement);

        statement = next;
    }

    return NULL;
}

/*
 * release_parameter()
 * Release a list of parameters
 */
parsetree_parameter_t* release_parameter(parsetree_parameter_t* parameter)
{
    while(parameter != NULL) {
        parsetree_parameter_t* next = parameter->next;

        switch(parameter->type) {
            case PARAMETER_STRING:
                free(parameter->string);
                break;

            case PARAMETER_BLOCK:
                release_program(parameter->program);
                break;
        }

        free(parameter);
        parameter = next;
    }

    return NULL;
}



/*
 * TREE TRAVERSAL
 */

/*
 * traverse_adapter()
 * A helper for tree traversal
 */
int traverse_adapter(const parsetree_statement_t* statement, void* user_data)
{
    int (*callback)(const parsetree_statement_t*) = (int(*)(const parsetree_statement_t*))user_data;
    return callback(statement);
}

/*
 * nanoparser_traverse_program_ex()
 * Traverse a program with a data field. The callback must return zero to let the enumeration proceed, or any non-zero value to stop it
 */
void nanoparser_traverse_program_ex(const parsetree_program_t* program, void *user_data, int (*callback)(const parsetree_statement_t*,void*))
{
    /* for each statement, traverse in order */
    for(const parsetree_statement_t* statement = program->statement; statement != NULL; statement = statement->next) {
        if(0 != callback(statement, user_data))
            break;
    }
}

/*
 * nanoparser_traverse_program()
 * Traverse a program. The callback must return zero to let the enumeration proceed, or any non-zero value to stop it
 */
void nanoparser_traverse_program(const parsetree_program_t* program, int (*callback)(const parsetree_statement_t*))
{
    nanoparser_traverse_program_ex(program, callback, traverse_adapter);
}



/*
 * DATA RETRIEVAL
 */

/*
 * nanoparser_get_identifier()
 * Read the identifier of a statement
 */
const char* nanoparser_get_identifier(const parsetree_statement_t* statement)
{
    return statement->identifier;
}

/*
 * nanoparser_get_parameter_list()
 * Read the list of parameters of a statement
 */
const parsetree_parameter_t* nanoparser_get_parameter_list(const parsetree_statement_t* statement)
{
    return statement->parameter;
}

/*
 * nanoparser_get_file()
 * Get the file associated with a statement
 */
const char* nanoparser_get_file(const parsetree_statement_t* statement)
{
    const parsetree_program_t* program = statement->program;

    while(program->parent != NULL)
        program = program->parent;

    return ((parsetree_root_t*)program)->filepath;
}

/*
 * nanoparser_get_line_number()
 * Get the line number associated with a statement
 */
int nanoparser_get_line_number(const parsetree_statement_t* statement)
{
    return statement->line;
}

/*
 * nanoparser_get_number_of_parameters()
 * Get the number of parameters of a list of statements
 */
int nanoparser_get_number_of_parameters(const parsetree_parameter_t* param_list)
{
    int counter = 0;

    for(; param_list != NULL; param_list = param_list->next)
        ++counter;

    return counter;
}

/*
 * nanoparser_get_nth_parameter()
 * Get a specific parameter of a list of parameters (n=1: first parameter; n=2: second parameter; and so on)
 */
const parsetree_parameter_t* nanoparser_get_nth_parameter(const parsetree_parameter_t* param_list, int n)
{
    const parsetree_parameter_t* first_node = param_list;
    nanoassert(n >= 1);

    for(int counter = 1; param_list != NULL; counter++, param_list = param_list->next) {
        if(n == counter)
            return param_list;
    }

#if 0
    return NULL;
#else
    /* give better error messages when getting missing parameters */
    if(first_node != NULL) {
        static parsetree_parameter_t tmp;
        tmp = NULL_PARAMETER;
        tmp.statement = first_node->statement;
        return &tmp;
    }

    return NULL;
#endif
}

/*
 * nanoparser_expect_string()
 * Crash if the given parameter is not a string
 */
void nanoparser_expect_string(const parsetree_parameter_t* param, const char* error_message)
{
    if(param != NULL && param->type != PARAMETER_STRING)
        crash("%s at %s:%d", error_message, nanoparser_get_file(param->statement), param->statement->line);
    else if(param == NULL)
        crash("%s at ???", error_message);
}

/*
 * nanoparser_expect_program()
 * Crash if the given parameter is not a program (block)
 */
void nanoparser_expect_program(const parsetree_parameter_t* param, const char* error_message)
{
    if(param != NULL && param->type != PARAMETER_BLOCK)
        crash("%s at %s:%d", error_message, nanoparser_get_file(param->statement), param->statement->line);
    else if(param == NULL)
        crash("%s at ???", error_message);
}

/*
 * nanoparser_get_string()
 * Get the string associated with the given parameter, if any
 */
const char* nanoparser_get_string(const parsetree_parameter_t* param)
{
    if(param != NULL && param->type == PARAMETER_STRING)
        return param->string;
    else
        return "null";
}

/*
 * nanoparser_get_program()
 * Get the program associated with the given parameter, if any
 * Returns NULL if there is no such program
 */
const parsetree_program_t* nanoparser_get_program(const parsetree_parameter_t* param)
{
    if(param != NULL && param->type == PARAMETER_BLOCK)
        return param->program;
    else
        return NULL;
}

/*
 * nanoparser_get_statement()
 * Get the statement to which the given parameter belongs
 */
const parsetree_statement_t* nanoparser_get_statement(const parsetree_parameter_t* param)
{
    if(param != NULL)
        return param->statement;
    else
        return NULL;
}





/*
 * LEXICAL ANALYSIS
 */



/*
 * lexer_create()
 * Creates a lexer.
 * Returns NULL on error.
 */
nanolexer_t* lexer_create(const char* filepath)
{
    /* open file */
    ALLEGRO_FILE* fp = al_fopen(filepath, "rb"); /* we're requesting binary mode because physfs reads files in binary mode only */
    if(!fp) {
        crash("Can't open file %s for reading", filepath);
        return NULL;
    }

    /* initialize the lexer */
    nanolexer_t* lexer = mallocx(sizeof *lexer);
    lexer->filepath = str_dup(filepath);
    darray_init(lexer->token);

    /* read the tokens */
    if(!lexer_read(lexer, fp)) {
        lexer = lexer_destroy(lexer);
        al_fclose(fp);
        crash("Can't read the tokens of %s", filepath);
        return NULL;
    }

    #if 0
    /* list tokens */
    for(int i = 0; i < darray_length(lexer->token); i++)
        printf("% 3d. (%d,<%s>)\n", lexer->token[i].line, lexer->token[i].type, lexer->token[i].value);
    #endif

    /* done! */
    al_fclose(fp);
    return lexer;
}

/*
 * lexer_destroy()
 * Destroy a lexer
 */
nanolexer_t* lexer_destroy(nanolexer_t* lexer)
{
    for(int i = darray_length(lexer->token) - 1; i >= 0; i--)
        token_destroy(lexer->token[i]);

    darray_release(lexer->token);
    free(lexer->filepath);
    free(lexer);

    return NULL;
}

/*
 * lexer_getc()
 * Get a character from the file
 */
int lexer_getc(nanofilestate_t* state)
{
    int* j = &(state->cursor);
    size_t read_bytes;
    char c;

    if(state->locked) {
        state->locked = false;
        return state->last;
    }

    if(state->last == '\n')
        ++state->line;

    while(true) {

        if(0 == (c = state->buffer[(*j)++])) { /* end of buffer or file */

            if(*j == LEXER_BUFFER_SIZE) {
                /* the first buffer has been processed; now read to the second buffer */
                /*memset(state->buffer + LEXER_BUFFER_SIZE, 0, LEXER_BUFFER_SIZE);*/
                read_bytes = al_fread(state->fp, state->buffer + LEXER_BUFFER_SIZE, LEXER_BUFFER_SIZE - 1);
                state->buffer[LEXER_BUFFER_SIZE + read_bytes] = 0;
            }
            else if(*j == 2 * LEXER_BUFFER_SIZE) {
                /* the second buffer has been processed; now read to the first buffer */
                /*memset(state->buffer, 0, LEXER_BUFFER_SIZE);*/
                read_bytes = al_fread(state->fp, state->buffer, LEXER_BUFFER_SIZE - 1);
                state->buffer[read_bytes] = 0;
                *j = 0;
            }
            else {
                /* end of file */
                return (state->last = EOF);
            }

            /* end of buffer: read next char */
            /* loop */ ;

        }
        else {

            /* ignore CR; physfs reads files in binary mode only */
            if(c == '\r')
                continue;

            /* we're not done reading yet */
            /*putchar(c);*/
            return (state->last = (int)c); /* not 0 */

        }

    }

    return EOF;
}

/*
 * lexer_ungetc()
 * Puts the last retrieved character back on the stream
 */
int lexer_ungetc(nanofilestate_t* state)
{
    nanoassert(!state->locked);
    state->locked = true;
    return state->last;
}

/*
 * lexer_read()
 * Reads all tokens from the file
 */
bool lexer_read(nanolexer_t* lexer, ALLEGRO_FILE* fp)
{
    char symbol_buffer[LEXER_SYMBOL_MAXLENGTH + 1];
    int symbol_length;
    int peek, next;
    nanotoken_t token;
    nanofilestate_t state = {
        .fp = fp,
        .cursor = 0,
        .buffer = { 0 },
        .line = 1,
        .last = EOF,
        .locked = false
    };

    /* initialize */
    /*memset(state.buffer, 0, LEXER_BUFFER_SIZE);*/
    size_t read_bytes = al_fread(fp, state.buffer, LEXER_BUFFER_SIZE - 1);
    state.buffer[read_bytes] = state.buffer[LEXER_BUFFER_SIZE - 1] = state.buffer[2 * LEXER_BUFFER_SIZE - 1] = 0;

    #if 0
    /* debug */
    while(EOF != (peek = lexer_getc(&state)))
        putchar(peek);
    return true;
    #endif

    for(peek = 0; peek != EOF; ) {

        /* skip spaces */
        while(EOF != (peek = lexer_getc(&state)) && isspace(peek)) {
            if(peek == '\n') {
                token = token_create(TOKEN_LINEBREAK, "\n", 2, state.line);
                darray_push(lexer->token, token);
            }
        }

        /* skip comments */
        if(peek == '/') {
            next = lexer_getc(&state);

            /* single-line comment */
            if(next == '/') {

                /* consume the characters */
                while(EOF != (peek = lexer_getc(&state))) {
                    if(peek == '\n') {
                        lexer_ungetc(&state);
                        break;
                    }
                }

                /* skip spaces */
                continue;

            }

            /* multi-line comment */
            else if(next == '*') {

                int start_line = state.line;

                /* consume the characters */
                while(EOF != (peek = lexer_getc(&state))) {
                    if(peek == '*') {
                        peek = lexer_getc(&state);
                        if('/' == peek || EOF == peek)
                            break;
                    }

                }

                /* need to close the comment? */
                if(peek == EOF) {
                    crash("Please close the open /* comment */ at %s:%d", lexer->filepath, start_line);
                    return false;
                }

                /* skip spaces */
                continue;

            }

            /* not a comment */
            else {
                lexer_ungetc(&state);
            }
        }

        /* preprocessor (backwards compatibility) */
        else if(peek == '#') {

            /* line start? */
            if(darray_length(lexer->token) <= 0 || lexer->token[darray_length(lexer->token)-1].type == TOKEN_LINEBREAK) {

                /* warning */
                warning("Obsolete: ignored preprocessor directive at %s:%d", lexer->filepath, state.line);

                /* treat it as a single-line comment */
                while(EOF != (peek = lexer_getc(&state))) {
                    if(peek == '\n') {
                        lexer_ungetc(&state);
                        break;
                    }
                }

                /* skip spaces */
                continue;
            }

        }

        /* read token */
        switch(peek) {
            case EOF:
                /* end of file */
                break;

            case '{':
                /* open block */
                token = token_create(TOKEN_BLOCKSTART, "{", 2, state.line);
                darray_push(lexer->token, token);
                break;

            case '}':
                /* close block */
                token = token_create(TOKEN_BLOCKEND, "}", 2, state.line);
                darray_push(lexer->token, token);
                break;

            case '"':
            case '\'': {
                /* quoted string */
                char quote = peek;

                /* accumulate characters */
                symbol_length = 0;
                for(
                    peek = lexer_getc(&state); /* skip opening quote */
                    peek != quote && peek != EOF;
                    peek = lexer_getc(&state)
                ) {
                    if(symbol_length + 4 >= sizeof(symbol_buffer)) { /* 4: max size in bytes of a utf-8 code point */
                        symbol_buffer[symbol_length++] = '\0';
                        crash("String is too long at %s:%d", lexer->filepath, state.line);
                        return false;
                    }
                    else if(peek == '\n') {
                        symbol_buffer[symbol_length++] = '\0';
                        crash("Unexpected line break at %s:%d", lexer->filepath, state.line);
                        return false;
                    }
                    else if(peek != '\\') {
                        /* any character, except the closing quote and a backslash */
                        symbol_buffer[symbol_length++] = peek;
                    }
                    else {
                        /* read escape sequences */
                        switch((peek = lexer_getc(&state))) {
                            case 'n':
                                /* line break */
                                symbol_buffer[symbol_length++] = '\n';
                                break;

                            case 't':
                                /* tab */
                                symbol_buffer[symbol_length++] = '\t';
                                break;

                            case '\\':
                                /* backslash */
                                symbol_buffer[symbol_length++] = '\\';
                                break;

                            case 'u':
                            case 'x': {
                                /* hex byte or unicode code-point: \xhh or \uhhhh */
                                uint32_t hex = 0;
                                bool unicode = (peek != 'x');
                                int digits, expected_digits = unicode ? 4 : 2;

                                for(digits = expected_digits; digits > 0; digits--) {
                                    peek = lexer_getc(&state);
                                    if(!isxdigit(peek)) {
                                        lexer_ungetc(&state);
                                        break;
                                    }
                                    else if(isdigit(peek))
                                        hex = (hex << 4) | (peek - '0');
                                    else
                                        hex = (hex << 4) | ((tolower(peek) - 'a') + 10);
                                }

                                if(digits != 0) {
                                    crash("Use %s at %s:%d", unicode ? "\\uhhhh" : "\\xhh", lexer->filepath, state.line);
                                    return false;
                                }

                                #if 1
                                if(hex == 0) /* ignore the null character */
                                    break;
                                #endif

                                if(!unicode) { /* write byte */
                                    ((unsigned char*)symbol_buffer)[symbol_length++] = hex & 0xFF;
                                    break;
                                }

                                /* encode in UTF-8 a unicode code point */
                                size_t written_bytes = al_utf8_encode(symbol_buffer + symbol_length, hex);
                                nanoassert(written_bytes <= 4);
                                symbol_length += written_bytes;
                                break;
                            }

                            default: {
                                /* quote */
                                if(peek == quote) {
                                    symbol_buffer[symbol_length++] = quote;
                                    break;
                                }

                                /* invalid escape sequence */
                                crash("Invalid escape sequence '\\%c' at %s:%d", (char)peek, lexer->filepath, state.line);
                                return false;
                            }
                        }
                    }
                }
                symbol_buffer[symbol_length++] = '\0';

                /* validate closing quote */
                if(peek != quote) {
                    crash("Invalid string at %s:%d\n\n\"%s\"", lexer->filepath, state.line, symbol_buffer);
                    return false;
                }

                /* add token */
                token = token_create(TOKEN_STRING, symbol_buffer, symbol_length, state.line);
                darray_push(lexer->token, token);
                break;
            }

            default: {
                /* unquoted string */

                /* printable characters (ASCII), no whitespaces, no quotes, no curly braces */
                #define isvalidchar(c) ((c) > 0x20 && (c) < 0x7f && (c) != '"' && (c) != '\'' && (c) != '{' && (c) != '}')

                /* only valid chars (as defined above) are accepted. UTF-8 strings must be quoted */
                if(!isvalidchar(peek)) {
                    crash("Invalid character 0x%x '%c' at %s:%d", peek, (char)peek, lexer->filepath, state.line);
                    return false;
                }

                /* maybe this token will be an identifier? */
                bool is_identifier = (isalpha(peek) || peek == '_' || peek == '<' || peek == '>');

                /* accumulate characters */
                symbol_length = 0;
                do {

                    if(symbol_length + 1 >= sizeof(symbol_buffer)) {
                        symbol_buffer[symbol_length++] = '\0';
                        crash("Token is too long at %s:%d\n\n\"%s\"", lexer->filepath, state.line, symbol_buffer);
                        return false;
                    }

                    symbol_buffer[symbol_length++] = (char)peek;
                    is_identifier = is_identifier && (isalnum(peek) || peek == '_' || peek == '<' || peek == '>');
                    peek = lexer_getc(&state);

                } while(isvalidchar(peek) && peek != EOF);
                symbol_buffer[symbol_length++] = '\0';

                /* ungetc */
                lexer_ungetc(&state);

                /* add token */
                nanotokentype_t token_type = is_identifier ? TOKEN_IDENTIFIER : TOKEN_STRING;
                token = token_create(token_type, symbol_buffer, symbol_length, state.line);
                darray_push(lexer->token, token);
                break;
            }
        }

    }

    /* add a line break at the end to simplify the syntax analysis */
    token = token_create(TOKEN_LINEBREAK, "\n", 2, state.line);
    darray_push(lexer->token, token);

    /* EOF */
    token = token_create(TOKEN_EOF, "EOF", 4, state.line);
    darray_push(lexer->token, token);

    /* success */
    return true;
}


/*
 * SYNTAX ANALYSIS
 */

/*
 * parser_create()
 * Create a parser
 */
nanoparser_t* parser_create(const nanolexer_t* lexer)
{
    nanoparser_t* parser = mallocx(sizeof *parser);

    parser->lexer = lexer;
    parser->cursor = 0;

    return parser;
}

/*
 * parser_destroy()
 * Destroy a parser
 */
nanoparser_t* parser_destroy(nanoparser_t* parser)
{
    free(parser);
    return NULL;
}

/*
 * parser_lookahead()
 * Get the lookahead symbol
 */
const nanotoken_t* parser_lookahead(const nanoparser_t* parser)
{
    int cursor = parser->cursor;

    /* check boundaries */
    nanoassert(cursor >= 0);
    if(cursor >= darray_length(parser->lexer->token)) /* eof */
        cursor = darray_length(parser->lexer->token) - 1;

    /* return token */
    return parser->lexer->token + cursor;
}

/*
 * parser_check()
 * Check if the lookahead has the given type
 */
bool parser_check(const nanoparser_t* parser, nanotokentype_t token_type)
{
    const nanotoken_t* lookahead = parser_lookahead(parser);
    return lookahead->type == token_type;
}

/*
 * parser_expect()
 * Require that the lookahead has the given type
 */
void parser_expect(const nanoparser_t* parser, nanotokentype_t token_type)
{
    const nanotoken_t* lookahead = parser_lookahead(parser);

    if(lookahead->type != token_type)
        crash("Syntax error: expected %s at %s:%d", TOKEN_NAME[token_type], parser->lexer->filepath, lookahead->line);
}

/*
 * parser_match()
 * Require that the lookahead has the given type and advance the cursor
 */
void parser_match(nanoparser_t* parser, nanotokentype_t token_type)
{
    parser_expect(parser, token_type);
    /*printf("match %s <%s>\n",TOKEN_NAME[token_type], parser_lookahead(parser)->value);*/
    ++parser->cursor;
}

/*
 * parser_unmatch()
 * Put the last symbol back into the stream, provided that its type matches the given type
 */
void parser_unmatch(nanoparser_t* parser, nanotokentype_t token_type)
{
    --parser->cursor;
    nanoassert(parser->cursor >= 0);
    parser_expect(parser, token_type);
}

/*
 * parser_parse_root()
 * Create the root of the parse tree
 */
parsetree_root_t* parser_parse_root(nanoparser_t* parser)
{
    /* create root program */
    parsetree_root_t* root = mallocx(sizeof *root);
    root->filepath = str_dup(parser->lexer->filepath);



    /* read program */
    parsetree_program_t* program = (parsetree_program_t*)root;
    program->parent = NULL;

    /* skip empty lines */
    while(parser_check(parser, TOKEN_LINEBREAK))
        parser_match(parser, TOKEN_LINEBREAK);

    /* read statements */
    program->statement = parser_parse_statement(parser, program);



    /* validate */
    parser_match(parser, TOKEN_EOF);

    /* reset cursor? */
    /*parser->cursor = 0;*/ /* we won't read the same file twice */

    /* done! */
    return root;
}

/*
 * parser_parse_program()
 * Parse a program
 */
parsetree_program_t* parser_parse_program(nanoparser_t* parser, const parsetree_program_t* parent)
{
    /* create program */
    parsetree_program_t* program = mallocx(sizeof *program);
    program->parent = parent;

    /* skip empty lines */
    while(parser_check(parser, TOKEN_LINEBREAK))
        parser_match(parser, TOKEN_LINEBREAK);

    /* read statements */
    program->statement = parser_parse_statement(parser, program);

    /* done! */
    return program;
}

/*
 * parser_parse_statement()
 * Parse a statement
 */
parsetree_statement_t* parser_parse_statement(nanoparser_t* parser, const parsetree_program_t* program)
{
    /* no more statements? */
    if(parser_check(parser, TOKEN_EOF) || parser_check(parser, TOKEN_BLOCKEND))
        return NULL;

    /* expect an identifier */
    parser_expect(parser, TOKEN_IDENTIFIER);

    /* read statement(s) */
    parsetree_statement_t* head = mallocx(sizeof *head);
    parsetree_statement_t* statement = head;
    do {
        const nanotoken_t* lookahead = parser_lookahead(parser);

        /* read the identifier */
        statement->program = program;
        statement->identifier = memcpy(mallocx(lookahead->value_size), lookahead->value, lookahead->value_size);
        statement->line = lookahead->line;
        statement->next = NULL;
        parser_match(parser, TOKEN_IDENTIFIER);

        /* read the parameters */
        statement->parameter = parser_parse_parameter(parser, statement);

        /* skip empty lines */
        do {
            parser_match(parser, TOKEN_LINEBREAK);
        } while(parser_check(parser, TOKEN_LINEBREAK));

        /* prepare to read the next statement */
        if(parser_check(parser, TOKEN_IDENTIFIER))
            statement->next = mallocx(sizeof *(statement->next));

        /* next node */
        statement = statement->next;

    } while(statement != NULL);

    /* expect no more statements */
    if(!parser_check(parser, TOKEN_BLOCKEND) && !parser_check(parser, TOKEN_EOF)) {
        const nanotoken_t* lookahead = parser_lookahead(parser);
        crash("Syntax error: unexpected %s at %s:%d", TOKEN_NAME[lookahead->type], parser->lexer->filepath, lookahead->line);
    }

    /* done! */
    return head;
}

/*
 * parser_parse_parameter()
 * Parse the parameters of a statement
 */
parsetree_parameter_t* parser_parse_parameter(nanoparser_t* parser, const parsetree_statement_t* statement)
{
    const nanotoken_t* lookahead = parser_lookahead(parser);

    if(parser_check(parser, TOKEN_STRING)) {
        /* read string */
        parsetree_parameter_t* parameter = mallocx(sizeof *parameter);

        parameter->type = PARAMETER_STRING;
        parameter->statement = statement;
        parameter->string = memcpy(mallocx(lookahead->value_size), lookahead->value, lookahead->value_size);
        parser_match(parser, TOKEN_STRING);
        parameter->next = parser_parse_parameter(parser, statement);

        return parameter;
    }
    else if(parser_check(parser, TOKEN_IDENTIFIER)) {
        /* read identifier */
        parsetree_parameter_t* parameter = mallocx(sizeof *parameter);

        parameter->type = PARAMETER_STRING;
        parameter->statement = statement;
        parameter->string = memcpy(mallocx(lookahead->value_size), lookahead->value, lookahead->value_size);
        parser_match(parser, TOKEN_IDENTIFIER);
        parameter->next = parser_parse_parameter(parser, statement);

        return parameter;
    }
    else {
        bool newline = parser_check(parser, TOKEN_LINEBREAK);

        /* skip newlines */
        while(parser_check(parser, TOKEN_LINEBREAK))
            parser_match(parser, TOKEN_LINEBREAK);

        /* read block */
        if(parser_check(parser, TOKEN_BLOCKSTART)) {
            parsetree_parameter_t* parameter = mallocx(sizeof *parameter);

            parameter->type = PARAMETER_BLOCK;
            parameter->statement = statement;
            parser_match(parser, TOKEN_BLOCKSTART);
            parser_match(parser, TOKEN_LINEBREAK);
            parameter->program = parser_parse_program(parser, statement->program);
            parser_match(parser, TOKEN_BLOCKEND);
            parameter->next = NULL;

            return parameter;
        }

        /* end of statement */
        else {

            /* put a \n back */
            if(newline)
                parser_unmatch(parser, TOKEN_LINEBREAK);

            /* empty list */
            return NULL;

        }
    }
}



/*
 * ERROR FUNCTIONS
 */

/*
 * nanoparser_set_error_function()
 * Set an error function
 */
void nanoparser_set_error_function(void (*fun)(const char*))
{
    crash_fun = fun != NULL ? fun : crash_fun_default;
}

/*
 * nanoparser_set_warning_function()
 * Set a warning function
 */
void nanoparser_set_warning_function(void (*fun)(const char*))
{
    warning_fun = fun != NULL ? fun : warning_fun_default;
}

/*
 * nanoparser_crash()
 * Trigger a crash related to a statement
 */
void nanoparser_crash(const parsetree_statement_t* statement, const char* fmt, ...)
{
    char message[ERROR_MAXLENGTH + 1];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    const char* file = nanoparser_get_file(statement);
    int line = nanoparser_get_line_number(statement);
    crash("In \"%s\" at line %d: %s", file, line, message);
}

/*
 * nanoparser_warn()
 * Trigger a warning related to a statement
 */
void nanoparser_warn(const parsetree_statement_t* statement, const char* fmt, ...)
{
    char message[ERROR_MAXLENGTH + 1];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    const char* file = nanoparser_get_file(statement);
    int line = nanoparser_get_line_number(statement);
    warning("In \"%s\" at line %d: %s", file, line, message);
}





/*
 * crash()
 * Internal crash function: crash the program with a formatted error message
 */
void crash(const char* fmt, ...)
{
    char buffer[ERROR_MAXLENGTH + 1] = ERROR_PREFIX;
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer + ERROR_PREFIX_LENGTH, sizeof(buffer) - ERROR_PREFIX_LENGTH, fmt, args);
    va_end(args);

    crash_fun(buffer);
    exit(1);
}

/*
 * crash_fun_default()
 * Crash the program with an error message
 */
void crash_fun_default(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

/*
 * warning()
 * Report a formatted message
 */
void warning(const char* fmt, ...)
{
    char buffer[ERROR_MAXLENGTH + 1] = ERROR_PREFIX;
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer + ERROR_PREFIX_LENGTH, sizeof(buffer) - ERROR_PREFIX_LENGTH, fmt, args);
    va_end(args);

    warning_fun(buffer);
}

/*
 * warning_fun_default()
 * Report a message
 */
void warning_fun_default(const char* message)
{
    fprintf(stderr, "%s\n", message);
}
