// -----------------------------------------------------------------------------
// File: player_position.ss
// Description: player position plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin displays the position of the active player in world space.

*/

using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;

object "Debug Mode - Player Position" is "debug-mode-plugin"
{
    text = null;

    fun onLoad(debugMode)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex + 1000;

        player = Player.active;
        text = player.spawn("Debug Mode - Player Position - Text").setZIndex(zindex);
    }

    fun onUnload(debugMode)
    {
        text.destroy();
        text = null;
    }
}

object "Debug Mode - Player Position - Text" is "awake", "private", "entity"
{
    text = Text("GoodNeighbors");
    transform = Transform();

    state "main"
    {
        xpos = Math.floor(transform.position.x);
        ypos = Math.floor(transform.position.y);

        text.text = xpos + " , " + ypos;
    }

    fun setZIndex(zindex)
    {
        text.zindex = zindex;
        return this;
    }

    fun constructor()
    {
        text.align = "center";
        text.offset = Vector2(0, 16);
    }
}