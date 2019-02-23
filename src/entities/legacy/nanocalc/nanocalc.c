/*
 * nanocalc 1.1
 * A tiny stand-alone easy-to-use expression evaluator written in C
 * Copyright (c) 2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "nanocalc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define error nanocalc_error

/* ============ UTILITIES ================= */
static void (*error_fun)(const char*) = NULL;
static void* malloc_x(size_t bytes); /* our version of malloc */
static char* str_dup(const char *s); /* our version of strdup: duplicates s */
static void float2string(float f, char *buf, size_t buf_size);

char* str_dup(const char *s)
{
    char *p = malloc_x( (1 + strlen(s)) * sizeof(char) );
    return strcpy(p, s);
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

void float2string(float f, char *buf, size_t buf_size)
{
    if(fabs(f-floor(f)) < 1e-5)
        snprintf(buf, buf_size, "%d", (int)f);
    else
        snprintf(buf, buf_size, "%.5f", f);
}

/* interpolates the given string, i.e., replaces all variables of the given string
   by their values. You need to free() the string returned by this function */
char *nanocalc_interpolate_string(const char *str, symboltable_t *symbol_table)
{
    char *result = malloc_x(sizeof(char) * 10241), *r;
    char varname[65], varvalue[65];
    const char *p, *q;
    int i, j;

    /* initializing... */
    *result = 0;
    r = result;

    /* looking for variables */
    for(p=str, j=0; *p && j<10240; ) {
        if(*p == '$') {
            if(isalpha((unsigned char)*(p+1)) || (*(p+1) == '_')) {
                varname[0] = *p;
                for(q=p+1,i=1; (isalnum((unsigned char)*q) || (*q=='_')) && i<64; )
                    varname[i++] = *(q++);
                varname[i] = 0;

                if(symboltable_is_defined(symbol_table, varname)) {
                    float2string(symboltable_get(symbol_table, varname), varvalue, sizeof(varvalue));
                    for(q=varvalue; *q && j++<10240; )
                        *(r++) = *(q++);
                    p += strlen(varname);
                    continue;
                }
            }
        }

        *(r++) = *(p++);
        j++;
    }

    /* ok */
    *r = 0;
    return (char*)realloc(result, sizeof(char) * (1+strlen(result)));
}



/* ============ SYMBOL TABLE ============== */

/* data structure: variables are stored in a linked list */
/* we estimate that each symbol table in an object will hold a small number of variables */
typedef struct association_t association_t;
struct association_t {
    char *key;
    float value;
    association_t *next;
};

struct symboltable_t {
    association_t *data;
};

static symboltable_t* global_st = NULL; /* fixed, global symbol table */
#define IS_GLOBAL_VARIABLE(varname) (*((varname)+1) == '_') /* vars starting with '_' are global */

/* creates a new symbol table */
symboltable_t *symboltable_new()
{
    symboltable_t *st = malloc_x(sizeof *st);
    st->data = NULL;
    return st;
}

/* destroys an existing symbol table */
void symboltable_destroy(symboltable_t *st)
{
    symboltable_clear(st);
    free(st);
}

/* clears an existing symbol table */
void symboltable_clear(symboltable_t *st)
{
    association_t *node, *next;

    for(node=st->data; node!=NULL; node=next) {
        next = node->next;
        free(node->key);
        free(node);
    }

    st->data = NULL;
}

/* adds or updates an association */
void symboltable_set(symboltable_t *st, const char *key, float value)
{
    association_t *node, *prev = NULL;

    /* global variable? */
    if(IS_GLOBAL_VARIABLE(key))
        st = symboltable_get_global_table();
    if(!st) return;

    /* searching... */
    for(node=st->data; node!=NULL; node=node->next) {
        if(strcmp(node->key, key) == 0) {
            node->value = value;
            return;
        }
        prev = node;
    }

    node = malloc_x(sizeof *node);
    node->key = str_dup(key);
    node->value = value;
    node->next = NULL;

    if(prev != NULL)
        prev->next = node;
    else
        st->data = node;
}

/* gets the value of an association */
float symboltable_get(symboltable_t *st, const char *key)
{
    association_t *node;

    /* global variable? */
    if(IS_GLOBAL_VARIABLE(key))
        st = symboltable_get_global_table();
    if(!st) return 0.0f;

    /* searching... */
    if(!IS_GLOBAL_VARIABLE(key)) {
        /* linear search */
        for(node=st->data; node; node=node->next) {
            if(strcmp(node->key, key) == 0)
                return node->value;
        }
    }
    else {
        /* linear search + MoveToFront heuristic for linked lists */
        association_t *prev = NULL;
        for(node=st->data; node; node=node->next) {
            if(strcmp(node->key, key) == 0) {
                float value = node->value;
                if(prev != NULL) {
                    prev->next = node->next;
                    node->next = st->data;
                    st->data = node;
                }
                return value;
            }
            else
                prev = node;
        }
    }

    /* element not found */
    return 0.0f;
}

/* does the given variable exist? */
int symboltable_is_defined(symboltable_t *st, const char *key)
{
    association_t *node;

    /* global variable? */
    if(IS_GLOBAL_VARIABLE(key))
        st = symboltable_get_global_table();
    if(!st) return 0;

    /* searching... */
    for(node=st->data; node!=NULL; node=node->next) {
        if(strcmp(node->key, key) == 0)
            return 1;
    }

    return 0; /* element not found */
}

/* returns a fixed, global symbol table */
symboltable_t *symboltable_get_global_table()
{
    return global_st;
}





/* =============== BUILT-IN FUNCTIONS (BIFs) ======================== */
#define CALL_BIF(name, bif_arity, ...) (bif_find/*_or_die*/(name)->call.arity##bif_arity(__VA_ARGS__)) /* calls the given BIF */

typedef struct bif_t bif_t;
struct bif_t {
    int arity;
    union {
        float (*arity0)();
        float (*arity1)(float);
        float (*arity2)(float,float);
        float (*arity3)(float,float,float);
        float (*arity4)(float,float,float,float);
    } call;
};

#define BIF_CAPACITY 256 /* can't have more than BIF_CAPACITY bifs */
static int bif_length = 0; /* number of registered BIFs */
static struct {
    char *name; /* function name */
    bif_t fun; /* function data */
} bif[BIF_CAPACITY]; /* BIF table */

static void bif_init()
{
    bif_length = 0;
}

static void bif_release()
{
    int i;

    for(i=0; i<bif_length; i++)
        free(bif[i].name);
}

static bif_t* bif_find(const char *name)
{
    int i;

    for(i=0; i<bif_length; i++) {
        if(strcmp(bif[i].name, name) == 0)
            return &(bif[i].fun);
    }

    return NULL;
}

/*static bif_t* bif_find_or_die(const char *name)
{
    bif_t *bif = bif_find(name);

    if(!bif)
        error("Can't find built-in function '%s'", name);

    return bif;
}*/

static void bif_register_arity0(const char *name, float (*fun)())
{
    if(bif_length < BIF_CAPACITY) {
        if(!bif_find(name)) {
            bif[bif_length].name = str_dup(name);
            bif[bif_length].fun.arity = 0;
            bif[bif_length].fun.call.arity0 = fun;
            bif_length++;
        }
        else
            error("Redefinition of built-in function '%s'", name);
    }
    else
        error("Can't register more than %d built-in functions", BIF_CAPACITY);
}

static void bif_register_arity1(const char *name, float (*fun)(float))
{
    if(bif_length < BIF_CAPACITY) {
        if(!bif_find(name)) {
            bif[bif_length].name = str_dup(name);
            bif[bif_length].fun.arity = 1;
            bif[bif_length].fun.call.arity1 = fun;
            bif_length++;
        }
        else
            error("Redefinition of built-in function '%s'", name);
    }
    else
        error("Can't register more than %d built-in functions", BIF_CAPACITY);
}

static void bif_register_arity2(const char *name, float (*fun)(float,float))
{
    if(bif_length < BIF_CAPACITY) {
        if(!bif_find(name)) {
            bif[bif_length].name = str_dup(name);
            bif[bif_length].fun.arity = 2;
            bif[bif_length].fun.call.arity2 = fun;
            bif_length++;
        }
        else
            error("Redefinition of built-in function '%s'", name);
    }
    else
        error("Can't register more than %d built-in functions", BIF_CAPACITY);
}

static void bif_register_arity3(const char *name, float (*fun)(float,float,float))
{
    if(bif_length < BIF_CAPACITY) {
        if(!bif_find(name)) {
            bif[bif_length].name = str_dup(name);
            bif[bif_length].fun.arity = 3;
            bif[bif_length].fun.call.arity3 = fun;
            bif_length++;
        }
        else
            error("Redefinition of built-in function '%s'", name);
    }
    else
        error("Can't register more than %d built-in functions", BIF_CAPACITY);
}

static void bif_register_arity4(const char *name, float (*fun)(float,float,float,float))
{
    if(bif_length < BIF_CAPACITY) {
        if(!bif_find(name)) {
            bif[bif_length].name = str_dup(name);
            bif[bif_length].fun.arity = 4;
            bif[bif_length].fun.call.arity4 = fun;
            bif_length++;
        }
        else
            error("Redefinition of built-in function '%s'", name);
    }
    else
        error("Can't register more than %d built-in functions", BIF_CAPACITY);
}




/* =============== EXPRESSION PARSE TREE ======================== */

/* data structure: base class */
typedef struct exprtree_t exprtree_t;
struct exprtree_t {
    float (*eval)(exprtree_t*); /* evaluates this tree node */
    void (*del)(exprtree_t*); /* deletes this tree node */
};

/* number */
typedef struct exprtree_number_t exprtree_number_t;
struct exprtree_number_t {
    exprtree_t base;
    float value;
};

static float exprtree_number_eval(exprtree_t *tree)
{
    return ((exprtree_number_t*)tree)->value;
}

static void exprtree_number_delete(exprtree_t *tree)
{
    free(tree);
}

static exprtree_t *exprtree_number_new(float value)
{
    exprtree_number_t *node = malloc_x(sizeof *node);
    node->value = value;
    ((exprtree_t*)node)->eval = exprtree_number_eval;
    ((exprtree_t*)node)->del = exprtree_number_delete;
    return (exprtree_t*)node;
}

/* variable */
typedef struct exprtree_variable_t exprtree_variable_t;
struct exprtree_variable_t {
    exprtree_t base;
    char *variable_name;
    symboltable_t *symbol_table; /* pointer to the symbol table */
};

static float exprtree_variable_eval(exprtree_t *tree)
{
    exprtree_variable_t *node = (exprtree_variable_t*)tree;
    return symboltable_get(node->symbol_table, node->variable_name);
}

static void exprtree_variable_delete(exprtree_t *tree)
{
    free(((exprtree_variable_t*)tree)->variable_name);
    free(tree);
}

static exprtree_t *exprtree_variable_new(const char *variable_name, symboltable_t *symbol_table)
{
    exprtree_variable_t *node = malloc_x(sizeof *node);
    node->variable_name = str_dup(variable_name);
    node->symbol_table = symbol_table;
    ((exprtree_t*)node)->eval = exprtree_variable_eval;
    ((exprtree_t*)node)->del = exprtree_variable_delete;
    return (exprtree_t*)node;
}

/* unary operator */
typedef struct exprtree_unaryop_t exprtree_unaryop_t;
struct exprtree_unaryop_t {
    exprtree_t base;
    char *operator;
    exprtree_t *expression;
};

static float exprtree_unaryop_eval(exprtree_t *tree)
{
    exprtree_t *child = ((exprtree_unaryop_t*)tree)->expression;
    const char *op = ((exprtree_unaryop_t*)tree)->operator;
    float val = child->eval(child);

    if(strcmp(op, "-") == 0)
        return -val;
    else if(strcmp(op, "not") == 0)
        return fabs(val) > 1e-5 ? 0.0f : 1.0;
    else
        error("Can't evaluate expression: invalid unary operator '%s'", op);

    return 0.0f;
}

static void exprtree_unaryop_delete(exprtree_t *tree)
{
    exprtree_t *child;

    free(((exprtree_unaryop_t*)tree)->operator);

    child = ((exprtree_unaryop_t*)tree)->expression;
    child->del(child);

    free(tree);
}

static exprtree_t *exprtree_unaryop_new(const char *operator, exprtree_t *expression)
{
    exprtree_unaryop_t *node = malloc_x(sizeof *node);
    node->operator = str_dup(operator);
    node->expression = expression;
    ((exprtree_t*)node)->eval = exprtree_unaryop_eval;
    ((exprtree_t*)node)->del = exprtree_unaryop_delete;
    return (exprtree_t*)node;
}

/* binary operator */
typedef struct exprtree_binaryop_t exprtree_binaryop_t;
struct exprtree_binaryop_t {
    exprtree_t base;
    char *operator;
    exprtree_t *left_expr;
    exprtree_t *right_expr;
};

static float exprtree_binaryop_eval(exprtree_t *tree)
{
    exprtree_t *expr1 = ((exprtree_binaryop_t*)tree)->left_expr;
    exprtree_t *expr2 = ((exprtree_binaryop_t*)tree)->right_expr;
    const char *op = ((exprtree_binaryop_t*)tree)->operator;
    float val1, val2;

    /* short-circuit boolean operations */
    val1 = expr1->eval(expr1);
    if((strcmp(op, "and") == 0) && (fabs(val1) <= 1e-5))
        return 0.0f;
    else if((strcmp(op, "or") == 0) && (fabs(val1) > 1e-5))
        return 1.0f;
    val2 = expr2->eval(expr2);

    /* evaluating the binary operation */
    if(strcmp(op, "+") == 0)
        return val1 + val2;
    else if(strcmp(op, "-") == 0)
        return val1 - val2;
    else if(strcmp(op, "*") == 0)
        return val1 * val2;
    else if(strcmp(op, "/") == 0)
        return fabs(val2) > 1e-5 ? val1 / val2 : 1.0f;
    else if(strcmp(op, "mod") == 0)
        return fabs(val2) > 1e-5 ? fmod(val1, val2) : 0.0f;
    else if(strcmp(op, "^") == 0)
        return pow(val1, val2);
    else if(strcmp(op, "==") == 0)
        return fabs(val1-val2) <= 1e-5 ? 1.0f : 0.0f;
    else if(strcmp(op, "<>") == 0)
        return fabs(val1-val2) > 1e-5 ? 1.0f : 0.0f;
    else if(strcmp(op, ">") == 0)
        return val1 > val2 ? 1.0f : 0.0f;
    else if(strcmp(op, "<") == 0)
        return val1 < val2 ? 1.0f : 0.0f;
    else if(strcmp(op, ">=") == 0)
        return val1 >= val2 ? 1.0f : 0.0f;
    else if(strcmp(op, "<=") == 0)
        return val1 <= val2 ? 1.0f : 0.0f;
    else if(strcmp(op, "and") == 0)
        return (fabs(val1) > 1e-5 && fabs(val2) > 1e-5) ? 1.0f : 0.0f;
    else if(strcmp(op, "or") == 0)
        return (fabs(val1) > 1e-5 || fabs(val2) > 1e-5) ? 1.0f : 0.0f;
    else if(strcmp(op, ",") == 0)
        return val2;
    else
        error("Can't evaluate expression: invalid binary operator '%s'", op);

    return 0.0f;
}

static void exprtree_binaryop_delete(exprtree_t *tree)
{
    exprtree_t *child;

    free(((exprtree_binaryop_t*)tree)->operator);

    child = ((exprtree_binaryop_t*)tree)->left_expr;
    child->del(child);

    child = ((exprtree_binaryop_t*)tree)->right_expr;
    child->del(child);

    free(tree);
}

static exprtree_t *exprtree_binaryop_new(const char *operator, exprtree_t *lexpr, exprtree_t *rexpr)
{
    exprtree_binaryop_t *node = malloc_x(sizeof *node);
    node->operator = str_dup(operator);
    node->left_expr = lexpr;
    node->right_expr = rexpr;
    ((exprtree_t*)node)->eval = exprtree_binaryop_eval;
    ((exprtree_t*)node)->del = exprtree_binaryop_delete;
    return (exprtree_t*)node;
}

/* assignment operator */
typedef struct exprtree_assignmentop_t exprtree_assignmentop_t;
struct exprtree_assignmentop_t {
    exprtree_t base;
    char *operator;
    exprtree_variable_t *left_expr;
    exprtree_t *right_expr;
};

static float exprtree_assignmentop_eval(exprtree_t *tree)
{
    exprtree_assignmentop_t *node = (exprtree_assignmentop_t*)tree;
    exprtree_variable_t *var_derived = node->left_expr;
    exprtree_t *var = (exprtree_t*)var_derived;
    exprtree_t *right_expr = node->right_expr;
    char *op = node->operator;
    float value = 0.0f;

    if(strcmp(op, "=") == 0)
        value = right_expr->eval(right_expr);
    else if(strcmp(op, "+=") == 0)
        value = var->eval(var) + right_expr->eval(right_expr);
    else if(strcmp(op, "-=") == 0)
        value = var->eval(var) - right_expr->eval(right_expr);
    else if(strcmp(op, "*=") == 0)
        value = var->eval(var) * right_expr->eval(right_expr);
    else if(strcmp(op, "/=") == 0) {
        float y = right_expr->eval(right_expr);
        value = fabs(y) > 1e-5 ? var->eval(var) / y : 1.0f;
    }
    else if(strcmp(op, "^=") == 0) {
        float x = var->eval(var);
        float y = right_expr->eval(right_expr);
        value = x >= 0.0f ? pow(x, y) : -pow(-x, y);
    }
    else
        error("Can't evaluate expression: invalid assignment operator '%s'", op);

    symboltable_set(var_derived->symbol_table, var_derived->variable_name, value);
    return value;
}

static void exprtree_assignmentop_delete(exprtree_t *tree)
{
    exprtree_t *child;

    child = (exprtree_t*)(((exprtree_assignmentop_t*)tree)->left_expr);
    child->del(child);

    child = ((exprtree_assignmentop_t*)tree)->right_expr;
    child->del(child);

    free(((exprtree_assignmentop_t*)tree)->operator);
    free(tree);
}

static exprtree_t* exprtree_assignmentop_new(const char *operator, exprtree_variable_t *lexpr, exprtree_t *rexpr)
{
    exprtree_assignmentop_t *node = malloc_x(sizeof *node);
    node->operator = str_dup(operator);
    node->left_expr = lexpr;
    node->right_expr = rexpr;
    ((exprtree_t*)node)->eval = exprtree_assignmentop_eval;
    ((exprtree_t*)node)->del = exprtree_assignmentop_delete;
    return (exprtree_t*)node;
}

/* built-in function call */
typedef struct exprtree_function_t exprtree_function_t;
struct exprtree_function_t {
    exprtree_t base;
    bif_t fun;
    exprtree_t *param[4];
};

static float exprtree_function_eval(exprtree_t *tree)
{
    exprtree_function_t *node = (exprtree_function_t*)tree;
    exprtree_t *p[4];
 
    p[0] = node->param[0];
    p[1] = node->param[1];
    p[2] = node->param[2];
    p[3] = node->param[3];

    switch(node->fun.arity) {
    case 0: return node->fun.call.arity0();
    case 1: return node->fun.call.arity1(p[0]->eval(p[0]));
    case 2: return node->fun.call.arity2(p[0]->eval(p[0]), p[1]->eval(p[1]));
    case 3: return node->fun.call.arity3(p[0]->eval(p[0]), p[1]->eval(p[1]), p[2]->eval(p[2]));
    case 4: return node->fun.call.arity4(p[0]->eval(p[0]), p[1]->eval(p[1]), p[2]->eval(p[2]), p[3]->eval(p[3]));
    }

    return 0.0f;
}

static void exprtree_function_delete(exprtree_t *tree)
{
    exprtree_t *child;
    int i;

    for(i=0; i<4; i++) {
        child = ((exprtree_function_t*)tree)->param[i];
        if(child != NULL)
            child->del(child);
    }

    free(tree);
}

static exprtree_t* exprtree_function_arity0_new(float (*fun)())
{
    exprtree_function_t *node = malloc_x(sizeof *node);
    node->fun.arity = 0;
    node->fun.call.arity0 = fun;
    node->param[0] = NULL;
    node->param[1] = NULL;
    node->param[2] = NULL;
    node->param[3] = NULL;
    ((exprtree_t*)node)->eval = exprtree_function_eval;
    ((exprtree_t*)node)->del = exprtree_function_delete;
    return (exprtree_t*)node;
}

static exprtree_t* exprtree_function_arity1_new(float (*fun)(float), exprtree_t *param0)
{
    exprtree_function_t *node = malloc_x(sizeof *node);
    node->fun.arity = 1;
    node->fun.call.arity1 = fun;
    node->param[0] = param0;
    node->param[1] = NULL;
    node->param[2] = NULL;
    node->param[3] = NULL;
    ((exprtree_t*)node)->eval = exprtree_function_eval;
    ((exprtree_t*)node)->del = exprtree_function_delete;
    return (exprtree_t*)node;
}

static exprtree_t* exprtree_function_arity2_new(float (*fun)(float,float), exprtree_t *param0, exprtree_t *param1)
{
    exprtree_function_t *node = malloc_x(sizeof *node);
    node->fun.arity = 2;
    node->fun.call.arity2 = fun;
    node->param[0] = param0;
    node->param[1] = param1;
    node->param[2] = NULL;
    node->param[3] = NULL;
    ((exprtree_t*)node)->eval = exprtree_function_eval;
    ((exprtree_t*)node)->del = exprtree_function_delete;
    return (exprtree_t*)node;
}

static exprtree_t* exprtree_function_arity3_new(float (*fun)(float,float,float), exprtree_t *param0, exprtree_t *param1, exprtree_t *param2)
{
    exprtree_function_t *node = malloc_x(sizeof *node);
    node->fun.arity = 3;
    node->fun.call.arity3 = fun;
    node->param[0] = param0;
    node->param[1] = param1;
    node->param[2] = param2;
    node->param[3] = NULL;
    ((exprtree_t*)node)->eval = exprtree_function_eval;
    ((exprtree_t*)node)->del = exprtree_function_delete;
    return (exprtree_t*)node;
}

static exprtree_t* exprtree_function_arity4_new(float (*fun)(float,float,float,float), exprtree_t *param0, exprtree_t *param1, exprtree_t *param2, exprtree_t *param3)
{
    exprtree_function_t *node = malloc_x(sizeof *node);
    node->fun.arity = 4;
    node->fun.call.arity4 = fun;
    node->param[0] = param0;
    node->param[1] = param1;
    node->param[2] = param2;
    node->param[3] = param3;
    ((exprtree_t*)node)->eval = exprtree_function_eval;
    ((exprtree_t*)node)->del = exprtree_function_delete;
    return (exprtree_t*)node;
}





/* =============== LEXICAL ANALYSIS ============================ */
#define TOK_MAXLENGTH       80
typedef enum {
    TOK_NUMBER,             /* number */
    TOK_LPAREN,             /* left parenthesis */
    TOK_RPAREN,             /* right parenthesis */
    TOK_VARIABLE,           /* variable name */
    TOK_UNARYOP,            /* unary operator */
    TOK_BINARYOP,           /* binary operator */
    TOK_ASSIGNMENTOP,       /* assignment operator */
    TOK_FUNCTION,           /* built-in function name */
    TOK_COMMA,              /* comma */
    TOK_UNKNOWN             /* unknown token */
} tokentype_t;

typedef struct token_t token_t;
struct token_t {
    tokentype_t type;
    char value[TOK_MAXLENGTH];
};

static token_t token_new(tokentype_t type, const char *value)
{
    token_t t;
    t.type = type;
    strcpy(t.value, value);
    return t;
}

/* returns 1 if there are more tokens to read, 0 otherwise */
static int lexer_read_next_token(token_t *tok_ref, char **expr_ref)
{
    char value[TOK_MAXLENGTH];
    char *s = *expr_ref;
    int len;
    token_t previous_token = token_new(tok_ref->type, tok_ref->value);

    /* skip spaces */
    while(isspace((unsigned char)(s[0])))
        s = ++(*expr_ref);
    len = strlen(s);

    /* nothing more to read */
    if(len == 0) {
        tok_ref->type = TOK_UNKNOWN;
        strcpy(tok_ref->value, "");
        return 0;
    }

    /* lexer */
    if(len >= 2 && (
        ((s[0] == '$') && (s[1] == '_' || isalpha((unsigned char)s[1])))
    )) {
        /* variable name */
        int k = 0;
        while(k < TOK_MAXLENGTH-1) {
            value[k++] = *s;
            s = ++(*expr_ref);
            if(!(isalnum((unsigned char)*s) || (*s == '_')))
                break;
        }
        value[k] = 0;

        if(k > 1) {
            strcpy(tok_ref->value, value);
            tok_ref->type = TOK_VARIABLE;
        }
        else {
            error("Invalid variable name near expression '%s'", *expr_ref);
            return 0;
        }
    }
    else if(len >= 1 && (
        isdigit((unsigned char)s[0]) ||
        ((s[0] == '.') && (len >= 2))
    )) {
        /* number */
        int got_a_period = (*s == '.');
        int k = 0;
        while(k < TOK_MAXLENGTH-1) {
            value[k++] = *s;
            s = ++(*expr_ref);
            if(*s == '.' && !isdigit(*(s+1)))
                error("Syntax error near '%s'", s);
            if(!(isdigit((unsigned char)*s) || (*s == '.' && !got_a_period)))
                break;
            got_a_period = got_a_period || (*s == '.');
        }
        value[k] = 0;

        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_NUMBER;
    }
    else if(len >= 1 && (
        (s[0] == ',')
    )) {
        /* comma */
        value[0] = s[0];
        value[1] = 0;
        tok_ref->type = TOK_COMMA;
        strcpy(tok_ref->value, value);
        (*expr_ref) += 1;
    }
    else if(len >= 1 && (
        (s[0] == '(')
    )) {
        /* left parenthesis */
        value[0] = s[0];
        value[1] = 0;
        tok_ref->type = TOK_LPAREN;
        strcpy(tok_ref->value, value);
        (*expr_ref) += 1;
    }
    else if(len >= 1 && (
        (s[0] == ')')
    )) {
        /* right parenthesis */
        value[0] = s[0];
        value[1] = 0;
        tok_ref->type = TOK_RPAREN;
        strcpy(tok_ref->value, value);
        (*expr_ref) += 1;
    }
    else if(len >= 3 && (
        ((s[0] == 'a') && (s[1] == 'n') && (s[2] == 'd')) ||
        ((s[0] == 'm') && (s[1] == 'o') && (s[2] == 'd'))
    )) {
        /* 3-characters binary operators */
        value[0] = s[0];
        value[1] = s[1];
        value[2] = s[2];
        value[3] = 0;
        tok_ref->type = TOK_BINARYOP;
        strcpy(tok_ref->value, value);
        (*expr_ref) += 3;
    }
    else if(len >= 3 && (
        ((s[0] == 'n') && (s[1] == 'o') && (s[2] == 't'))
    )) {
        /* 3-characters unary operators */
        value[0] = s[0];
        value[1] = s[1];
        value[2] = s[2];
        value[3] = 0;
        tok_ref->type = TOK_UNARYOP;
        strcpy(tok_ref->value, value);
        (*expr_ref) += 3;
    }
    else if(len >= 2 && (
        ((s[0] == '=') && (s[1] == '=')) ||
        ((s[0] == '<') && (s[1] == '>')) ||
        ((s[0] == '>') && (s[1] == '=')) ||
        ((s[0] == '<') && (s[1] == '=')) ||
        ((s[0] == 'o') && (s[1] == 'r'))
    )) {
        /* 2-characters binary operators */
        value[0] = s[0];
        value[1] = s[1];
        value[2] = 0;
        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_BINARYOP;
        (*expr_ref) += 2;
    }
    else if(len >= 2 && (
        0
    )) {
        /* 2-characters unary operators */
        (*expr_ref) += 2;
    }
    else if(len >= 2 && (
        ((s[0] == '+') && (s[1] == '=')) ||
        ((s[0] == '-') && (s[1] == '=')) ||
        ((s[0] == '*') && (s[1] == '=')) ||
        ((s[0] == '/') && (s[1] == '=')) ||
        ((s[0] == '^') && (s[1] == '='))
    )) {
        /* 2-characters assignment operators */
        value[0] = s[0];
        value[1] = s[1];
        value[2] = 0;
        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_ASSIGNMENTOP;
        (*expr_ref) += 2;
    }
    else if(len >= 1 && (
        (s[0] == '+') ||
        ((s[0] == '-') && (previous_token.type != TOK_LPAREN && previous_token.type != TOK_UNARYOP && previous_token.type != TOK_BINARYOP && previous_token.type != TOK_ASSIGNMENTOP && previous_token.type != TOK_COMMA && previous_token.type != TOK_UNKNOWN)) ||
        (s[0] == '*') ||
        (s[0] == '/') ||
        (s[0] == '>') ||
        (s[0] == '<') ||
        (s[0] == '^')
    )) {
        /* 1-character binary operators */
        value[0] = s[0];
        value[1] = 0;
        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_BINARYOP;
        (*expr_ref) += 1;
    }
    else if(len >= 1 && (
        (s[0] == '-')
    )) {
        /* 1-character unary operators */
        value[0] = s[0];
        value[1] = 0;
        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_UNARYOP;
        (*expr_ref) += 1;
    }
    else if(len >= 1 && (
        (s[0] == '=')
    )) {
        /* 1-character assignment operator */
        value[0] = s[0];
        value[1] = 0;
        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_ASSIGNMENTOP;
        (*expr_ref) += 1;
    }
    else if(len >= 1 && (
        ((s[0] == '_') || isalpha((unsigned char)s[0]))
    )) {
        /* function name */
        int k = 0;
        while(k < TOK_MAXLENGTH-1) {
            value[k++] = *s;
            s = ++(*expr_ref);
            if(!((*s == '_') || isalnum((unsigned char)*s)))
                break;
        }
        value[k] = 0;

        strcpy(tok_ref->value, value);
        tok_ref->type = TOK_FUNCTION;
    }
    else {
        tok_ref->type = TOK_UNKNOWN;
        strcpy(tok_ref->value, "");
        error("Unexpected symbol near '%s'", s);
        return 0;
    }

    return 1;
}





/* =============== SYNTATIC ANALYSIS ============================ */
/* http://en.wikipedia.org/wiki/Recursive_descent_parser */

/* static data */
static token_t sym; /* current token */
static char* input = NULL; /* input text (changes as tokens are read) */
static char* prev_input = NULL; /* previous value of input */
static char* full_input = NULL; /* expression text (does not change) */
static symboltable_t* st; /* pointer to my symbol table */

/* internal methods */
static int parser_getsym(); /* gets the next token */
static int parser_expect(tokentype_t s); /* expects a token */
static int parser_accept(tokentype_t s); /* accepts a token */

/* ------- low precedence */
static exprtree_t* parser_read_anything(); /* reads anything */
static exprtree_t* parser_read_exprlist(); /* reads a comma-separated expression list */
static exprtree_t* parser_read_logicexpr(); /* reads a logic expression */
static exprtree_t* parser_read_condition(); /* reads a condition */
static exprtree_t* parser_read_expression(); /* reads an arithmethic expression */
static exprtree_t* parser_read_term(); /* reads a term */
static exprtree_t* parser_read_power(); /* reads a power */
static exprtree_t* parser_read_factor(); /* reads a factor */
/* ------- high precedence */

int parser_getsym()
{
    static int show_error = 0;
    int r;

    prev_input = input;
    r = lexer_read_next_token(&sym, &input);

    if(!r) {
        if(show_error++)
            error("Unexpected end of expression near '%s' in '%s'", prev_input, full_input);
    }
    else
        show_error = 0;

    return r;
}

int parser_expect(tokentype_t s)
{
    if(parser_accept(s))
        return 1;

    error("Unexpected symbol '%s' near '%s' in '%s'", sym.value, prev_input, full_input);
    return 0;
}

int parser_accept(tokentype_t s)
{
    if(sym.type == s) {
        parser_getsym();
        return 1;
    }

    return 0;
}

exprtree_t* parser_read_anything()
{
    /* start point */
    return parser_read_exprlist();
}

exprtree_t* parser_read_exprlist()
{
    exprtree_t *me, *left, *right;
    char *op;

    me = left = parser_read_logicexpr();
    if(sym.type == TOK_COMMA) {
        op = str_dup(sym.value);
        parser_getsym();
        right = parser_read_exprlist();
        me = exprtree_binaryop_new(op, left, right);
        free(op);
    }
    
    return me;
}

exprtree_t* parser_read_logicexpr()
{
    exprtree_t *me, *left, *right;
    char *op;

    me = left = parser_read_condition();
    if(sym.type == TOK_BINARYOP && (strcmp(sym.value, "and") == 0 || strcmp(sym.value, "or") == 0)) {
        op = str_dup(sym.value);
        parser_getsym();
        right = parser_read_logicexpr();
        me = exprtree_binaryop_new(op, left, right);
        free(op);
    }
    
    return me;
}

exprtree_t* parser_read_condition()
{
    exprtree_t *me, *left, *right;
    char *op;

    me = left = parser_read_expression();
    if(sym.type == TOK_BINARYOP && (strcmp(sym.value, "==") == 0 || strcmp(sym.value, "<>") == 0 || strcmp(sym.value, ">") == 0 || strcmp(sym.value, "<") == 0 || strcmp(sym.value, ">=") == 0 || strcmp(sym.value, "<=") == 0)) {
        op = str_dup(sym.value);
        parser_getsym();
        right = parser_read_expression();
        me = exprtree_binaryop_new(op, left, right);
        free(op);
    }
    
    return me;
}

exprtree_t* parser_read_expression()
{
    exprtree_t *me, *left, *right;
    char *op;

    me = left = parser_read_term();
    while(sym.type == TOK_BINARYOP && (strcmp(sym.value, "+") == 0 || strcmp(sym.value, "-") == 0)) {
        op = str_dup(sym.value);
        parser_getsym();
        right = parser_read_term();
        me = exprtree_binaryop_new(op, left, right);
        free(op);
        left = me;
    }
    
    return me;
}

exprtree_t* parser_read_term()
{
    exprtree_t *me, *left, *right;
    char *op;

    me = left = parser_read_power();
    while(sym.type == TOK_BINARYOP && (strcmp(sym.value, "*") == 0 || strcmp(sym.value, "/") == 0 || strcmp(sym.value, "mod") == 0)) {
        op = str_dup(sym.value);
        parser_getsym();
        right = parser_read_power();
        me = exprtree_binaryop_new(op, left, right);
        free(op);
        left = me;
    }
    
    return me;
}

exprtree_t* parser_read_power()
{
    exprtree_t *me, *left, *right;
    char *op;

    me = left = parser_read_factor();
    if(sym.type == TOK_BINARYOP && (strcmp(sym.value, "^") == 0)) {
        op = str_dup(sym.value);
        parser_getsym();
        right = parser_read_power();
        me = exprtree_binaryop_new(op, left, right);
        free(op);
    }
    
    return me;
}

exprtree_t* parser_read_factor()
{
    exprtree_t *me, *other;
    bif_t *fun;
    char *op;

    switch(sym.type) {
    case TOK_LPAREN:
        parser_getsym();
        me = parser_read_logicexpr();
        parser_expect(TOK_RPAREN);
        break;

    case TOK_NUMBER:
        me = exprtree_number_new(atof(sym.value));
        parser_getsym();
        break;

    case TOK_VARIABLE:
        me = exprtree_variable_new(sym.value, st);
        parser_getsym();
        if(sym.type == TOK_ASSIGNMENTOP) {
            exprtree_variable_t *variable = (exprtree_variable_t*)me;
            op = str_dup(sym.value);
            parser_getsym();
            other = parser_read_logicexpr();
            me = exprtree_assignmentop_new(op, variable, other);
            free(op);
        }
        break;

    case TOK_UNARYOP:
        op = str_dup(sym.value);
        parser_getsym();
        other = parser_read_factor();
        me = exprtree_unaryop_new(op, other);
        free(op);
        break;

    case TOK_FUNCTION:
        op = str_dup(sym.value); /* function name */
        if(NULL == (fun = bif_find(op))) /* built-in function */
            error("Can't find built-in function '%s' in '%s'", op, full_input);

        parser_getsym();
        parser_expect(TOK_LPAREN);
        if(sym.type != TOK_RPAREN) {
            exprtree_t *p[4]; /* function parameters */

            p[0] = parser_read_logicexpr();
            if(sym.type == TOK_COMMA) {
                parser_getsym();
                p[1] = parser_read_logicexpr();
                if(sym.type == TOK_COMMA) {
                    parser_getsym();
                    p[2] = parser_read_logicexpr();
                    if(sym.type == TOK_COMMA) {
                        parser_getsym();
                        p[3] = parser_read_logicexpr();
                        if(fun->arity != 4) error("Invalid arity for function %s/%d in '%s'", op, fun->arity, full_input);
                        me = exprtree_function_arity4_new(fun->call.arity4, p[0], p[1], p[2], p[3]);
                    }
                    else {
                        if(fun->arity != 3) error("Invalid arity for function %s/%d in '%s'", op, fun->arity, full_input);
                        me = exprtree_function_arity3_new(fun->call.arity3, p[0], p[1], p[2]);
                    }
                }
                else {
                    if(fun->arity != 2) error("Invalid arity for function %s/%d in '%s'", op, fun->arity, full_input);
                    me = exprtree_function_arity2_new(fun->call.arity2, p[0], p[1]);
                }
            }
            else {
                if(fun->arity != 1) error("Invalid arity for function %s/%d in '%s'", op, fun->arity, full_input);
                me = exprtree_function_arity1_new(fun->call.arity1, p[0]);
            }
        }
        else {
            if(fun->arity != 0) error("Invalid arity for function %s/%d in '%s'", op, fun->arity, full_input);
            me = exprtree_function_arity0_new(fun->call.arity0);
        }

        parser_expect(TOK_RPAREN);
        free(op);
        break;

    default:
        error("Syntax error near '%s' in '%s'", prev_input, full_input);
        me = exprtree_number_new(0.0f);
        break;
    }

    return me;
}

/* parser facade */
static exprtree_t* parse(const char *expression_string, symboltable_t *symbol_table)
{
    char *s = str_dup(expression_string);
    exprtree_t *tree;

    sym = token_new(TOK_UNKNOWN, "");
    st = symbol_table;
    prev_input = s;
    input = s;
    full_input = s;

    parser_getsym(); /* read the first token */
    if(sym.type != TOK_UNKNOWN) {
        tree = parser_read_anything();
        if(sym.type != TOK_UNKNOWN)
            error("End of expression expected near '%s' in '%s'", prev_input, full_input);
    }
    else
        tree = exprtree_number_new(0.0f); /* empty expression? */

    free(s);
    return tree;
}



/* =============== EXPRESSION EVALUATOR FACADE ============================ */

/* expression data structure */
struct expression_t {
    exprtree_t *root;
};

/* creates a new expression */
expression_t *expression_new(const char *expression_string, symboltable_t *symbol_table)
{
    expression_t *expr = malloc_x(sizeof *expr);
    symboltable_t *st = (symbol_table == NULL) ? symboltable_get_global_table() : symbol_table;
    expr->root = parse(expression_string, st);
    return expr;
}

/* destroys an existing expression object */
void expression_destroy(expression_t *expr)
{
    expr->root->del(expr->root);
    free(expr);
}

/* evaluates an expression */
float expression_evaluate(expression_t *expr)
{
    return expr->root->eval(expr->root);
}




/* ============ NANOCALC INTERFACE ============== */

/* initializes this module. Call this in the beginning of your program */
void nanocalc_init()
{
    global_st = symboltable_new();
    bif_init();
}

/* releases this module. Call this at the end of your program */
void nanocalc_release()
{
    bif_release();
    symboltable_destroy(global_st);
}

/* registers a built-in function (BIF) */
void nanocalc_register_bif_arity0(const char *name, float (*fun)())
{
    bif_register_arity0(name, fun);
}

void nanocalc_register_bif_arity1(const char *name, float (*fun)(float))
{
    bif_register_arity1(name, fun);
}

void nanocalc_register_bif_arity2(const char *name, float (*fun)(float,float))
{
    bif_register_arity2(name, fun);
}

void nanocalc_register_bif_arity3(const char *name, float (*fun)(float,float,float))
{
    bif_register_arity3(name, fun);
}

void nanocalc_register_bif_arity4(const char *name, float (*fun)(float,float,float,float))
{
    bif_register_arity4(name, fun);
}

/* you may optionally define your own error function (it will be called
   when an error arises). It must receive an error string */
void nanocalc_set_error_function(void (*fun)(const char*))
{
    error_fun = fun;
}

/* calls the function 'fun' defined above and kills the program */
void nanocalc_error(const char *fmt, ...)
{
    char buf[1024] = "nanocalc error! ";
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


#ifdef __cplusplus
}
#endif

/* ================ */

/* example program: */
/*
#include "nanocalc_addons.h"
int main()
{
    char str[80];

    nanocalc_init();
    nanocalc_addons_enable();
    while(gets(str)) {
        expression_t *expr = expression_new(str, NULL);
        printf("%f\n", expression_evaluate(expr));
        expression_destroy(expr);
    }
    nanocalc_release();

    return 0;
}
*/
