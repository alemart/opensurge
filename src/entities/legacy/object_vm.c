/*
 * Open Surge Engine
 * object_vm.c - virtual machine of the objects
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

#include "object_vm.h"
#include "../../util/util.h"
#include "../../util/stringutil.h"

/* private stuff */
typedef struct objectmachine_list_t objectmachine_list_t;
typedef struct objectmachine_stack_t objectmachine_stack_t;

/* objectvm_t class */
struct objectvm_t
{
    enemy_t* owner; /* who owns this VM? */
    objectmachine_list_t* state_list; /* list of states */
    objectmachine_t** reference_to_current_state; /* reference to the current state */
    symboltable_t* symbol_table; /* object's private symbol table (stores its variables) */
    objectmachine_stack_t* history; /* stores previous states */
};

/* linked list of object machines */
struct objectmachine_list_t {
    char *name;
    objectmachine_t *data;
    objectmachine_list_t *next;
};

static objectmachine_list_t* objectmachine_list_new(objectmachine_list_t* list, const char *name, enemy_t* owner);
static objectmachine_list_t* objectmachine_list_delete(objectmachine_list_t* list);
static objectmachine_list_t* objectmachine_list_find(objectmachine_list_t* list, const char *name);
static objectmachine_list_t* objectmachine_list_findmachine(objectmachine_list_t* list, objectmachine_t* machine);

/* stack of object machines */
#define OBJECTMACHINE_STACK_CAPACITY 5 /* return_to_previous_state history */
struct objectmachine_stack_t {
    objectmachine_list_t *data[OBJECTMACHINE_STACK_CAPACITY]; /* nodes of objectmachine_list_t */
    int top, size;
};

static objectmachine_stack_t* objectmachine_stack_new();
static objectmachine_stack_t* objectmachine_stack_delete(objectmachine_stack_t* stack);
static void objectmachine_stack_push(objectmachine_stack_t* stack, objectmachine_list_t *list_node);
static objectmachine_list_t* objectmachine_stack_pop(objectmachine_stack_t* stack);
static void objectmachine_stack_clear(objectmachine_stack_t* stack);



/* public methods */

objectvm_t* objectvm_create(enemy_t* owner)
{
    objectvm_t *vm = mallocx(sizeof *vm);
    vm->owner = owner;
    vm->state_list = NULL;
    vm->reference_to_current_state = NULL;
    vm->history = objectmachine_stack_new();
    vm->symbol_table = symboltable_new();
    return vm;
}

objectvm_t* objectvm_destroy(objectvm_t* vm)
{
    symboltable_destroy(vm->symbol_table);
    vm->history = objectmachine_stack_delete(vm->history);
    vm->state_list = objectmachine_list_delete(vm->state_list);
    vm->reference_to_current_state = NULL;
    vm->owner = NULL;
    free(vm);
    return NULL;
}

objectmachine_t** objectvm_get_reference_to_current_state(objectvm_t* vm)
{
    return vm->reference_to_current_state;
}

symboltable_t* objectvm_get_symbol_table(objectvm_t *vm)
{
    return vm->symbol_table;
}

void objectvm_create_state(objectvm_t* vm, const char *name)
{
    if(objectmachine_list_find(vm->state_list, name) == NULL)
        vm->state_list = objectmachine_list_new(vm->state_list, name, vm->owner);
    else
        fatal_error("Object script error: can't redefine state \"%s\" in object \"%s\".", name, vm->owner->name);
}


const char* objectvm_get_current_state(objectvm_t* vm)
{
    objectmachine_list_t *m = objectmachine_list_findmachine(vm->state_list, *(vm->reference_to_current_state));

    if(m == NULL) {
        fatal_error("Object script error: can't get current state name in object \"%s\". This shouldn't happen.", vm->owner->name);
        return NULL;
    }
    else
        return m->name;
}

void objectvm_set_current_state(objectvm_t* vm, const char *name)
{
    objectmachine_list_t *m = objectmachine_list_find(vm->state_list, name);

    if(m != NULL) {
        if(vm->reference_to_current_state != &(m->data)) {
            vm->reference_to_current_state = &(m->data);
            objectmachine_stack_push(vm->history, m);
        }
    }
    else
        fatal_error("Object script error: can't find state \"%s\" in object \"%s\".", name, vm->owner->name);
}

void objectvm_return_to_previous_state(objectvm_t *vm)
{
    objectmachine_list_t *m;

    objectmachine_stack_pop(vm->history); /* discard current state */
    m = objectmachine_stack_pop(vm->history); /* previous state */

    if(m != NULL) {
        vm->reference_to_current_state = &(m->data);
        objectmachine_stack_push(vm->history, m);
    }
    else
        fatal_error("Object script error: can't return to previous state in object \"%s\".", vm->owner->name);
}

void objectvm_reset_history(objectvm_t *vm)
{
    objectmachine_stack_clear(vm->history);
}

objectmachine_t* objectvm_get_state_by_name(objectvm_t* vm, const char *name)
{
    objectmachine_list_t *m = objectmachine_list_find(vm->state_list, name);

    if(m == NULL) {
        fatal_error("Object script error: can't find state \"%s\" in object \"%s\".", name, vm->owner->name);
        return NULL;
    }
    else
        return m->data;
}

/* objectmachine_list_t: private methods */

objectmachine_list_t* objectmachine_list_new(objectmachine_list_t* list, const char *name, enemy_t *owner)
{
    objectmachine_list_t *l = mallocx(sizeof *l);
    l->name = str_dup(name);
    l->data = objectbasicmachine_new(owner);
    l->next = list;
    return l;
}

objectmachine_list_t* objectmachine_list_delete(objectmachine_list_t* list)
{
    if(list != NULL) {
        objectmachine_t *machine = list->data;
        objectmachine_list_delete(list->next);
        free(list->name);
        machine->release(machine);
        free(list);
    }

    return NULL;
}

objectmachine_list_t* objectmachine_list_find(objectmachine_list_t* list, const char *name)
{
    if(list != NULL) {
        if(str_icmp(list->name, name) != 0)
            return objectmachine_list_find(list->next, name);
        else
            return list;
    }
    else
        return NULL;
}

objectmachine_list_t* objectmachine_list_findmachine(objectmachine_list_t* list, objectmachine_t* machine)
{
     if(list != NULL) {
        if(list->data != machine)
            return objectmachine_list_findmachine(list->next, machine);
        else
            return list;
    }
    else
        return NULL;   
}

/* objectmachine_stack_t: private methods */

objectmachine_stack_t* objectmachine_stack_new()
{
    objectmachine_stack_t *s = mallocx(sizeof *s);
    s->top = s->size = 0;
    return s;
}

objectmachine_stack_t* objectmachine_stack_delete(objectmachine_stack_t* stack)
{
    free(stack);
    return NULL;
}

void objectmachine_stack_push(objectmachine_stack_t* stack, objectmachine_list_t *list_node)
{
    int n = OBJECTMACHINE_STACK_CAPACITY;

    stack->size = min(n, 1 + stack->size);
    stack->data[stack->top] = list_node;
    stack->top = (stack->top+1) % n; /* circular stack */
}

objectmachine_list_t* objectmachine_stack_pop(objectmachine_stack_t* stack)
{
    int n = OBJECTMACHINE_STACK_CAPACITY;

    if(stack->size > 0) {
        --(stack->size);
        stack->top = ((stack->top-1)+n)%n;
        return stack->data[stack->top];
    }
    else
        return NULL; /* empty stack */
}

void objectmachine_stack_clear(objectmachine_stack_t* stack)
{
    stack->size = stack->top = 0;
}
