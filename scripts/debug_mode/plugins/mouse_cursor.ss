// -----------------------------------------------------------------------------
// File: mouse_cursor.ss
// Description: mouse cursor plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input.MobileGamepad;
using SurgeEngine.Input.Mouse;
using SurgeEngine.Transform;
using SurgeEngine.Actor;

object "Debug Mode - Mouse Cursor" is "debug-mode-plugin", "detached", "private", "entity"
{
    debugMode = parent;
    actor = Actor("Mouse Cursor");
    transform = Transform();

    state "main"
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");

        // initialize
        actor.visible = !MobileGamepad.available; // display only if not on mobile
        actor.zindex = uiSettings.zindex;

        state = "active";
    }

    state "active"
    {
        // this entity is in screen space, not in world space
        transform.position = Mouse.position;
    }
}