// -----------------------------------------------------------------------------
// File: world_scroller.ss
// Description: world scroller plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin helps the user scroll through the level.

If you need to know the spot that is being looked at, subscribe to this plugin
and implement method onWorldScroll(position) as in the example below:

object "Debug Mode - My Plugin" is "debug-mode-plugin"
{
    debugMode = parent;

    fun init()
    {
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
        Console.print(position);
    }
}

*/
using SurgeEngine.Input;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Camera;

object "Debug Mode - World Scroller" is "debug-mode-plugin", "debug-mode-observable"
{
    public scrollSpeed = 400;
    public gridSize = 16;
    input = Input("default");
    observable = spawn("Debug Mode - Observable");
    debugMode = parent;

    state "main"
    {
        player = Player.active;
        ds = scrollVector();

        player.transform.translate(ds);
        if(ds.length == 0)
            player.transform.position = snapToGrid(player.transform.position);

        Camera.position = player.transform.position;
        observable.notify(player.transform.position);
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

    fun subscribe(observer)
    {
        observable.subscribe(observer);
    }

    fun unsubscribe(observer)
    {
        observable.unsubscribe(observer);
    }

    fun onNotifyObserver(observer, position)
    {
        observer.onWorldScroll(position);
    }
}