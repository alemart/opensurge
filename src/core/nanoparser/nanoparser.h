/*
 * nanoparser 1.1
 * A tiny stand-alone easy-to-use parser written in C
 * Copyright (c) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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
        #include "config.cfg" // you may include other files too!

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

#ifndef _NANOPARSER_H
#define _NANOPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/* a program is a list of statements */
typedef struct parsetree_program_t parsetree_program_t;

/* a statement is a line containing an identifier (i.e., a string) followed by a parameter */
typedef struct parsetree_statement_t parsetree_statement_t;

/* a parameter is either:
   i)   another program;
   ii)  a string followed by another parameter */
typedef struct parsetree_parameter_t parsetree_parameter_t;




/* ===== LOADING / UNLOADING ===== */

/* given a filepath, it constructs the parse tree for you. Returns NULL on error. */
parsetree_program_t* nanoparser_construct_tree(const char *filepath);

/* you need to deconstruct the tree in order to free the allocated memory. Always returns NULL. */
parsetree_program_t* nanoparser_deconstruct_tree(parsetree_program_t *tree);




/* ===== OPERATIONS ===== */

/* appends src to dest. Returns dest. */
parsetree_program_t* nanoparser_append_program(parsetree_program_t *dest, parsetree_program_t *src);




/* ===== ERROR FUNCTIONS ===== */

/* you may optionally define your own error function (it will be called when a parsing error arises). It receives an error string */
void nanoparser_set_error_function(void (*fun)(const char*));

/* you may optionally define your own warning function (it will be called when a warning arises). It receives a warning string */
void nanoparser_set_warning_function(void (*fun)(const char*));



/* ===== TREE TRAVERSAL ===== */

/* traverses a given program */
/* the callback must return zero to let the enumeration proceed, or any non-zero value to stop it */
void nanoparser_traverse_program(const parsetree_program_t *program, int (*eval)(const parsetree_statement_t*));

/* traverses a given program, while providing some user-specific data as well */
/* the callback must return zero to let the enumeration proceed, or any non-zero value to stop it */
void nanoparser_traverse_program_ex(const parsetree_program_t *program, void *some_user_data, int (*eval)(const parsetree_statement_t*,void*));



/* ===== STATEMENT HANDLING ===== */

/* the first string of each line is also known as an identifier */
const char* nanoparser_get_identifier(const parsetree_statement_t *stmt);

/* returns the parameter list of a given statement */
const parsetree_parameter_t* nanoparser_get_parameter_list(const parsetree_statement_t *stmt);

/* returns the file name that originated the given statement */
const char* nanoparser_get_file(const parsetree_statement_t *stmt);

/* returns the line number that originated the given statement */
int nanoparser_get_line_number(const parsetree_statement_t *stmt);



/* ===== DATA RETRIEVAL ===== */

/* gets the number of parameters of a given parameter list */
int nanoparser_get_number_of_parameters(const parsetree_parameter_t *param_list);

/* gets the Nth parameter (N >= 1) of a given parameter list, i.e., n=1 returns the 1st parameter; n=2 returns the 2nd, and so on. Returns NULL if the n-th parameter doesn't exist. */
const parsetree_parameter_t* nanoparser_get_nth_parameter(const parsetree_parameter_t *param_list, int n);

/* expects that the given parameter must be a string. Throws an error otherwise. */
void nanoparser_expect_string(const parsetree_parameter_t *param, const char *error_message);

/* expects that the given parameter must be a program. Throws an error otherwise. */
void nanoparser_expect_program(const parsetree_parameter_t *param, const char *error_message);

/* returns the string of the given parameter */
const char* nanoparser_get_string(const parsetree_parameter_t *param);

/* returns the program of the given parameter */
const parsetree_program_t* nanoparser_get_program(const parsetree_parameter_t *param);


#ifdef __cplusplus
}
#endif

#endif

