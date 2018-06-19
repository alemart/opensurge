// ---------------------------------------------------------------------------
// Open Surge Engine
// http://opensurge2d.org
//
// File:   objects/default_startup/pause.inc
// Desc:   enables the player to pause the game
// Author: Alexandre
// Date:   2010-12-22
// ---------------------------------------------------------------------------

object ".default_startup.pause"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        on_button_pressed "fire3" "paused"
    }

    state "paused"
    {
        pause
        change_state "main"
    }
}
