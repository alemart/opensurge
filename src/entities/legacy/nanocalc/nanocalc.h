/*
 * nanocalc 1.1
 * A tiny stand-alone easy-to-use expression evaluator written in C
 * Copyright (c) 2010, 2012  Alexandre Martins <alemartf@gmail.com>
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

        Just drop nanocalc.c and nanocalc.h into your project and you're done.
        Optionally, you may want to bind some extra functions to the expression
        evaluator by adding nanocalc_addons.c and nanocalc_addons.h to your
        project as well.

II)  Usage

        #include "nanocalc.h"

        int main()
        {
            expression_t *e1, *e2, *e3, *e4, *e5, *e6, *e7;

            nanocalc_init(); // initializes nanocalc. This is very important!

            e1 = expression_new("2+3*4",                 NULL); // basic arithmethic
            e2 = expression_new("(2+3)*4",               NULL); // parenthesis
            e3 = expression_new("$val = 2^3 + 0.5",      NULL); // $val is a variable
            e4 = expression_new("$val + 1.5",            NULL); // $val plus 1.5
            e5 = expression_new("($val+1.5) >= 10",      NULL); // comparsions
            e6 = expression_new("not(($val+1.5) >= 10)", NULL); // logic operators
            e7 = expression_new("$val = $val + 1",       NULL); // the same as $val+=1

            printf("%.2f\n", expression_evaluate(e1));
            printf("%.2f\n", expression_evaluate(e2));
            printf("%.2f\n", expression_evaluate(e3));
            printf("%.2f\n", expression_evaluate(e4));
            printf("%.2f\n", expression_evaluate(e5));
            printf("%.2f\n", expression_evaluate(e6));
            printf("%.2f\n", expression_evaluate(e7));
            printf("%.2f\n", expression_evaluate(e7)); // evaluates e7 again

            expression_destroy(e7);
            expression_destroy(e6);
            expression_destroy(e5);
            expression_destroy(e4);
            expression_destroy(e3);
            expression_destroy(e2);
            expression_destroy(e1);

            nanocalc_release(); // remember to release nanocalc after you're done

            return 0;
        }

III) Output:

        14.00
        20.00
        8.50
        10.00
        1.00
        0.00
        9.50
        10.50
*/


#ifndef _NANOCALC_H
#define _NANOCALC_H

#ifdef __cplusplus
extern "C" {
#endif



/* ================= NANOCALC INTERFACE ======================= */

/* initializes this module. Call this in the beginning of your program */
void nanocalc_init();

/* releases this module. Call this at the end of your program */
void nanocalc_release();

/* registers a built-in function (BIF) */
void nanocalc_register_bif_arity0(const char *name, float (*fun)());
void nanocalc_register_bif_arity1(const char *name, float (*fun)(float));
void nanocalc_register_bif_arity2(const char *name, float (*fun)(float,float));
void nanocalc_register_bif_arity3(const char *name, float (*fun)(float,float,float));
void nanocalc_register_bif_arity4(const char *name, float (*fun)(float,float,float,float));

/* you may optionally define your own error function (it will be called
   when an error arises). It must receive an error string */
void nanocalc_set_error_function(void (*fun)(const char*));

/* calls the function 'fun' defined above and kills the program */
void nanocalc_error(const char *fmt, ...);



/* ==================== SYMBOL TABLE ==================== */

/* symbol table: used to store variables */
typedef struct symboltable_t symboltable_t;

/* creates a new symbol table */
symboltable_t *symboltable_new();

/* destroys an existing symbol table */
void symboltable_destroy(symboltable_t *st);

/* adds or updates an association */
void symboltable_set(symboltable_t *st, const char *key, float value);

/* gets the value of an association */
float symboltable_get(symboltable_t *st, const char *key);

/* does the given variable exist? */
int symboltable_is_defined(symboltable_t *st, const char *key);

/* clears (resets) the symbol table */
void symboltable_clear(symboltable_t *st);

/* returns a fixed, global symbol table */
symboltable_t *symboltable_get_global_table();



/* ============== EXPRESSION FACTORY ================= */

/* expression */
typedef struct expression_t expression_t;

/* creates a new expression */
/* if symbol_table == NULL, variables in this expression will be considered globals */
expression_t *expression_new(const char *expression_string, symboltable_t *symbol_table);

/* destroys an existing expression object */
void expression_destroy(expression_t *expr);

/* evaluates an expression */
float expression_evaluate(expression_t *expr);



/* ============== UTILITIES ================= */

/* interpolates the given string, i.e., replaces all variables of the given string
   by their values. You need to free() the string returned by this function */
char *nanocalc_interpolate_string(const char *str, symboltable_t *symbol_table);


#ifdef __cplusplus
}
#endif

#endif
