/*
 * Open Surge Engine
 * nanoparser.h - nanoparser v2
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _NANOPARSER_H
#define _NANOPARSER_H



/*
 * BASIC TYPES
 */

/* a program is a list of statements */
typedef struct parsetree_program_t parsetree_program_t;

/* a statement is a line containing an identifier (string) followed by a list of parameters */
typedef struct parsetree_statement_t parsetree_statement_t;

/* a list of parameters is another program (block), a string followed by another list of parameters, or null */
typedef struct parsetree_parameter_t parsetree_parameter_t;



/*
 * LOADING & UNLOADING
 */

/* Parse a file and construct a parse tree */
parsetree_program_t* nanoparser_construct_tree(const char* filepath);

/* Release a parse tree */
parsetree_program_t* nanoparser_deconstruct_tree(parsetree_program_t* root);




/*
 * TREE TRAVERSAL
 */

/* Traverse a program. The callback must return zero to let the enumeration proceed, or any non-zero value to stop it */
void nanoparser_traverse_program(const parsetree_program_t* program, int (*callback)(const parsetree_statement_t*));

/* Traverse a program with a data field. The callback must return zero to let the enumeration proceed, or any non-zero value to stop it */
void nanoparser_traverse_program_ex(const parsetree_program_t* program, void *user_data, int (*callback)(const parsetree_statement_t*,void*));



/*
 * STATEMENT HANDLING
 */

/* Read the identifier of a statement */
const char* nanoparser_get_identifier(const parsetree_statement_t* statement);

/* Read the list of parameters of a statement */
const parsetree_parameter_t* nanoparser_get_parameter_list(const parsetree_statement_t* statement);

/* Get the file associated with a statement */
const char* nanoparser_get_file(const parsetree_statement_t* statement);

/* Get the line number associated with a statement */
int nanoparser_get_line_number(const parsetree_statement_t* statement);



/*
 * DATA RETRIEVAL
 */

/* Get the number of parameters of a list of statements */
int nanoparser_get_number_of_parameters(const parsetree_parameter_t* param_list);

/* Get a specific parameter of a list of parameters (n=1: first parameter; n=2: second parameter; and so on) */
const parsetree_parameter_t* nanoparser_get_nth_parameter(const parsetree_parameter_t* param_list, int n);

/* Crash if the given parameter is not a string */
void nanoparser_expect_string(const parsetree_parameter_t* param, const char* error_message);

/* Crash if the given parameter is not a program (block) */
void nanoparser_expect_program(const parsetree_parameter_t* param, const char* error_message);

/* Get the string associated with the given parameter, if any */
const char* nanoparser_get_string(const parsetree_parameter_t* param);

/* Get the program associated with the given parameter, if any. Returns NULL if there is no such program */
const parsetree_program_t* nanoparser_get_program(const parsetree_parameter_t* param);

/* Get the statement to which the given parameter belongs */
const parsetree_statement_t* nanoparser_get_statement(const parsetree_parameter_t* param);



/*
 * ERROR FUNCTIONS
 */

/* Set an error function */
void nanoparser_set_error_function(void (*fun)(const char*, void*), void* context);

/* Set a warning function */
void nanoparser_set_warning_function(void (*fun)(const char*, void*), void* context);

/* Trigger a crash related to a statement */
void nanoparser_crash(const parsetree_statement_t* statement, const char* fmt, ...);

/* Trigger a warning related to a statement */
void nanoparser_warn(const parsetree_statement_t* statement, const char* fmt, ...);



#endif
