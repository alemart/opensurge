// -----------------------------------------------------------------------------
// File: grid_system.ss
// Description: grid system plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin establishes a spatial grid.

Properties
- gridSize: number. Should be 16, 8 or 1 (1 means no grid)

*/

using SurgeEngine.Vector2;

object "Debug Mode - Grid System" is "debug-mode-plugin", "debug-mode-observable"
{
    gridSize = 8; // given in pixels
    observable = spawn("Debug Mode - Observable");

    fun get_size()
    {
        return gridSize;
    }

    fun set_size(newSize)
    {
        if(gridSize == newSize)
            return;

        if(!(newSize == 1 || (newSize > 1 && newSize % 8 == 0)))
            return;

        gridSize = newSize;
        observable.notify(newSize);
    }

    fun snapToGrid(position)
    {
        if(gridSize == 1)
            return position;

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

    fun onNotifyObserver(observer, newSize)
    {
        observer.onChangeGridSize(newSize);
    }
}