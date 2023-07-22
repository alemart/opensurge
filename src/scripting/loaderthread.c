/*
 * Open Surge Engine
 * loaderthread.c - scripting system: SurgeScript loader thread
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

#include <surgescript.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_physfs.h>
#include <setjmp.h>
#include <stdbool.h>
#include "scripting.h"
#include "../core/logfile.h"
#include "../util/util.h"
#include "../util/stringutil.h"

typedef struct ssthreadcontext_t {
    jmp_buf env;
    int argc;
    char** argv;
    char error_message[1024];
} ssthreadcontext_t;

static void* load_surgescript(ALLEGRO_THREAD* thread, void* arg);
static void thread_crash_fn(const char* message, void* context);
static void thread_log_fn(const char* message, void* context);
static void crash_fn(const char* message, void* context);
static void log_fn(const char* message, void* context);



/*
 * surgescriptloaderthread_create()
 * Create a SurgeScript loader thread with the provided command-line arguments
 */
ALLEGRO_THREAD* surgescriptloaderthread_create(int argc, const char** argv)
{
    /* initialize a thread context */
    ssthreadcontext_t* ctx = mallocx(sizeof *ctx);
    ctx->error_message[0] = '\0';
    ctx->argc = argc;
    ctx->argv = NULL;
    if(argc > 0) {
        ctx->argv = mallocx(argc * sizeof(char*));
        for(int i = 0; i < argc; i++)
            ctx->argv[i] = str_dup(argv[i]);
    }

    /* create & start thread */
    ALLEGRO_THREAD* thread = al_create_thread(load_surgescript, ctx);
    al_start_thread(thread);
    return thread;
}

/*
 * surgescriptloaderthread_destroy()
 * Wait for the completion of a SurgeScript loader thread and destroy it
 * If it succeeds, all scripts will have been compiled, but the VM will NOT be launched
 */
ALLEGRO_THREAD* surgescriptloaderthread_destroy(ALLEGRO_THREAD* thread)
{
    ssthreadcontext_t* ctx = NULL;

    /* wait for completion */
    void* ret = NULL;
    al_join_thread(thread, &ret);
    ctx = (ssthreadcontext_t*)ret;
    assertx(ctx != NULL);

    /* release thread */
    al_destroy_thread(thread);

    /* error checking */
    if(ctx->error_message[0] != '\0') {
        /* fatal_error() must be called in the main thread
           (because of the destruction of OpenGL textures) */
        fatal_error("%s", ctx->error_message);
    }

    /* release the thread context */
    if(ctx->argv != NULL) {
        for(int i = ctx->argc - 1; i >= 0; i--)
            free(ctx->argv[i]);
        free(ctx->argv);
    }
    free(ctx);

    /* done */
    return NULL;
}




/*
 * private
 */

/* SurgeScript initialization thread */
void* load_surgescript(ALLEGRO_THREAD* thread, void* arg)
{
    volatile ssthreadcontext_t* ctx = (ssthreadcontext_t*)arg;

    /* use the physfs file interface in this thread */
    al_set_physfs_file_interface();

    /* set thread-specific error functions */
    surgescript_util_set_log_function(thread_log_fn, arg);
    surgescript_util_set_crash_function(thread_crash_fn, arg);

    /* save the calling environment */
    if(!setjmp(((ssthreadcontext_t*)arg)->env)) {
        /* load scripts */
        scripting_init(ctx->argc, (const char**)(ctx->argv));
    }
    else {
        /* we'll exit the thread with a non-blank ctx->error_message */
        ;
    }

    /* set regular error functions */
    surgescript_util_set_log_function(log_fn, NULL);
    surgescript_util_set_crash_function(crash_fn, NULL);

    /* done */
    return (void*)ctx;
}

/* crash function called in the initialization thread */
void thread_crash_fn(const char* message, void* context)
{
    ssthreadcontext_t* ctx = (ssthreadcontext_t*)context;

    /* copy the error message */
    str_cpy(ctx->error_message, message, sizeof(ctx->error_message));

    /* exit the thread */
    longjmp(ctx->env, 1);
}

/* logging function called in the initialization thread */
void thread_log_fn(const char* message, void* context)
{
    /* logfile_message() is thread-safe */
    logfile_message("%s", message);
}

/* crash function called during gameplay */
void crash_fn(const char* message, void* context)
{
    /* the crash function MUST exit the app */
    fatal_error("%s", message);
}

/* logging function called during gameplay */
void log_fn(const char* message, void* context)
{
    logfile_message("%s", message);
}