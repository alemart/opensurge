// ---------------------------------------------------------------------------
// Open Surge Engine
// http://opensurge2d.org
//
// File:   objects/default_startup/quit.inc
// Desc:   enables the player to leave the level
// Author: Alexandre
// Date:   2010-12-22
// ---------------------------------------------------------------------------

object ".default_startup.quit"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        on_button_pressed "fire4" "quit"
    }

    state "quit"
    {
        ask_to_leave
        change_state "main"
    }
}
