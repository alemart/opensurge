/*
 * Open Surge Engine
 * quest.c - quest scene
 * Copyright (C) 2010, 2012-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include <string.h>
#include "quest.h"
#include "level.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/scene.h"
#include "../core/quest.h"
#include "../core/logfile.h"
#include "../core/storyboard.h"

/* private data */
#define STACK_MAX 32 /* maximum number of simultaneous quests */
static int top = -1;
static struct {
    quest_t* current_quest;
    int current_level;
    int abort_quest;
} stack[STACK_MAX]; /* one can stack quests */

static void push_appropriate_scene(const char *str);



/* public scene functions */

/*
 * quest_init()
 * Initializes the quest scene.
 */
void quest_init(void *path_to_qst_file)
{
    const char *filepath = (const char*)path_to_qst_file;

    if(++top >= STACK_MAX)
        fatal_error("The quest stack can't hold more than %d quests.", STACK_MAX);

    stack[top].current_quest = load_quest(filepath);
    stack[top].current_level = 0;
    stack[top].abort_quest = FALSE;

    logfile_message("Pushed quest '%s' (\"%s\") onto the quest stack...", stack[top].current_quest->file, stack[top].current_quest->name);
}


/*
 * quest_release()
 * Releases the quest scene
 */
void quest_release()
{
    unload_quest(stack[top--].current_quest);
    logfile_message("The quest has been released.");
}


/*
 * quest_render()
 * Actually, this function does nothing
 */
void quest_render()
{
    ;
}


/*
 * quest_update()
 * Updates the quest manager
 */
void quest_update()
{
    /* invalid quest */
    if(stack[top].current_quest->level_count == 0)
        fatal_error("Quest '%s' has no levels.", stack[top].current_quest->file);

    /* quest manager */
    if(stack[top].current_level < stack[top].current_quest->level_count && !stack[top].abort_quest) {
        /* next level... */
        const char *path_to_lev_file = stack[top].current_quest->level_path[stack[top].current_level++];
        push_appropriate_scene(path_to_lev_file);
    }
    else {
        /* the user has cleared (or exited) the quest! */
        logfile_message(
            "Quest '%s' has been %s Popping...",
            stack[top].current_quest->file,
            (stack[top].abort_quest ? "aborted." : "cleared!")
        );
        scenestack_pop();
    }
}















/*
 * quest_abort()
 * Aborts the current quest, popping it from the stack
 */
void quest_abort()
{
    if(top >= 0)
        stack[top].abort_quest = TRUE;
}


/*
 * quest_setlevel()
 * Jumps to the given level, 0 <= lev < n
 */
void quest_setlevel(int lev)
{
    if(top >= 0)
        stack[top].current_level = max(0, lev);
}

/*
 * quest_currentlevel()
 * id of the current level, 0 <= id < n
 */
int quest_currentlevel()
{
    return top >= 0 ? stack[top].current_level : 0;
}

/*
 * quest_numberoflevels()
 * number of levels of the current quest
 */
int quest_numberoflevels()
{
    return top >= 0 ? stack[top].current_quest->level_count : 0;
}

/*
 * quest_getname()
 * Returns the name of the last running quest
 */
const char *quest_getname()
{
    return top >= 0 ? stack[top].current_quest->name : "null";
}





/* ------ private --------- */
void push_appropriate_scene(const char *str)
{
    if(str[0] == '<' && str[strlen(str)-1] == '>') {
        /* not a level? TODO: at the moment, the engine can't handle those scenes repeatedly on the stack */
        if(str_icmp(str, "<options>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_OPTIONS), NULL);
        /*else if(str_icmp(str, "<language_select>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);
        else if(str_icmp(str, "<quest_select>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_QUESTSELECT), NULL);
        else if(str_icmp(str, "<stage_select>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_STAGESELECT), (void*)FALSE);
        else if(str_icmp(str, "<stage_select_debug>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_STAGESELECT), (void*)TRUE);
        else if(str_icmp(str, "<credits>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_CREDITS), NULL);*/
        else
            fatal_error("Quest error: unrecognized symbol '%s'", str);
    }
    else {
        /* just a regular level */
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)str);
    }
}
