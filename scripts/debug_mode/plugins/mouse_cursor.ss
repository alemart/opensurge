// -----------------------------------------------------------------------------
// File: mouse_cursor.ss
// Description: mouse cursor plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin displays a mouse cursor on Desktop computers.

*/

using SurgeEngine;
using SurgeEngine.Actor;
using SurgeEngine.Camera;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Input.Mouse;
using SurgeEngine.UI.Text;

object "Debug Mode - Mouse Cursor" is "debug-mode-plugin", "detached", "private", "entity"
{
    actor = Actor("Debug Mode - Mouse Cursor");
    text = Text("GoodNeighbors");
    transform = Transform();

    state "main"
    {
        if(!actor.visible)
            return;

        // this entity is in screen space, not in world space
        transform.position = Mouse.position;
        text.text = cursorText();
    }

    fun onLoad(debugMode)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");

        actor.zindex = uiSettings.zindex + 5000;
        text.zindex = actor.zindex;
    }

    fun cursorText()
    {
        worldPosition = Camera.screenToWorld(Mouse.position);

        xpos = Math.floor(worldPosition.x);
        ypos = Math.floor(worldPosition.y);

        return xpos + " , " + ypos;
    }

    fun constructor()
    {
        actor.visible = !SurgeEngine.mobile; // hide in mobile mode

        //text.visible = actor.visible;
        text.visible = false;
        text.align = "left";
        text.offset = Vector2(0, -16);
    }
}