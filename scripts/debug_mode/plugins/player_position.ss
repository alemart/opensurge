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

object "Debug Mode - Player Position" is "debug-mode-plugin", "awake", "private", "entity"
{
    text = Text("GoodNeighbors");
    transform = Transform();
    debugMode = parent;



    state "main"
    {
        player = Player.active;

        xpos = Math.floor(player.transform.position.x);
        ypos = Math.floor(player.transform.position.y);

        text.text = xpos + " , " + ypos;
    }



    fun init()
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");

        text.zindex = uiSettings.zindex + 1000;
        text.align = "center";
        text.offset = Vector2(0, 16);

        worldScroller = debugMode.plugin("Debug Mode - World Scroller");
        worldScroller.subscribe(this);
    }

    fun release()
    {
        worldScroller = debugMode.plugin("Debug Mode - World Scroller");
        worldScroller.unsubscribe(this);
    }

    fun onWorldScroll(position)
    {
        transform.position = position;
    }
}