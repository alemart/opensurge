// -----------------------------------------------------------------------------
// File: world_scroller.ss
// Description: world scroller plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Camera;

object "Debug Mode - World Scroller" is "debug-mode-plugin"
{
    public scrollSpeed = 400;
    public gridSize = 16;
    debugMode = parent;
    input = Input("default");

    state "main"
    {
        player = Player.active;
        transform = player.transform;
        ds = scrollVector();

        transform.translate(ds);
        if(ds.length == 0)
            transform.position = snapToGrid(transform.position);

        Camera.position = transform.position;
    }

    fun get_position()
    {
        return Player.active.transform.position;
    }

    fun set_position(position)
    {
        Player.active.transform.position = position;
    }



    fun scrollVector()
    {
        return scrollDirection().scaledBy(scrollSpeed * Time.delta);
    }

    fun scrollDirection()
    {
        dx = 0;
        dy = 0;

        if(input.buttonDown("right"))
            dx += 1;
        if(input.buttonDown("left"))
            dx -= 1;
        if(input.buttonDown("down"))
            dy += 1;
        if(input.buttonDown("up"))
            dy -= 1;

        return Vector2(dx, dy).normalized();
    }

    fun snapToGrid(position)
    {
        halfGridSize = gridSize / 2;

        mx = position.x % gridSize;
        my = position.y % gridSize;

        dx = mx < halfGridSize ? 0 : gridSize;
        dy = my < halfGridSize ? 0 : gridSize;

        return Vector2(
            position.x - mx + dx,
            position.y - my + dy
        );
    }
}