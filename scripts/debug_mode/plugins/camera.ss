// -----------------------------------------------------------------------------
// File: camera.ss
// Description: camera plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin helps the user see the level.

Properties
----------
- position: Vector2, read-only. Camera position in world space.
- mode: string. Camera mode. One of the following: "player", "free-look" or "scroll".

If you need to know the spot that is being looked at, use the public property
position of this plugin. Example:

object "Debug Mode - My Plugin" is "debug-mode-plugin"
{
    camera = null;

    state "main"
    {
        Console.print(camera.position);
    }

    fun onLoad(debugMode)
    {
        camera = debugMode.plugin("Debug Mode - Camera");
    }
}

*/
using SurgeEngine;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Camera;
using SurgeEngine.Video.Screen;

object "Debug Mode - Camera" is "debug-mode-plugin", "debug-mode-observable", "awake", "private", "entity"
{
    transform = Transform();
    mode = "";
    observable = spawn("Debug Mode - Observable");
    defaultCameraDisabler = spawn("Debug Mode - Camera - Default Camera Disabler");
    strategies = {
        "player":    spawn("Debug Mode - Camera - Player Mode"),
        "free-look": spawn("Debug Mode - Camera - Free Look Mode"),
        "scroll":    spawn("Debug Mode - Camera - Scroll Mode")
    };
    strategy = null;
    grid = null;

    state "main"
    {
        prevX = transform.position.x;
        prevY = transform.position.y;

        strategy.move(transform, grid);

        currX = transform.position.x;
        currY = transform.position.y;

        deltaPosition = Vector2(currX - prevX, currY - prevY);
        if(deltaPosition.length > 0)
            observable.notify(deltaPosition);
    }

    fun lateUpdate()
    {
        Camera.position = transform.position;
    }

    fun get_position()
    {
        return transform.position;
    }

    fun get_mode()
    {
        return mode;
    }

    fun set_mode(newMode)
    {
        // nothing to do
        if(newMode == mode)
            return;

        // change the mode
        if(newMode == "player") {
            mode = newMode;
            strategy = strategies["player"].init(transform);
        }
        else if(newMode == "free-look") {
            mode = newMode;
            strategy = strategies["free-look"];
        }
        else if(newMode == "scroll") {
            mode = newMode;
            strategy = strategies["scroll"];
        }
        else
            Console.print("Invalid camera mode: " + newMode);
    }

    fun subscribe(observer)
    {
        observable.subscribe(observer);
    }

    fun unsubscribe(observer)
    {
        observable.unsubscribe(observer);
    }

    fun onNotifyObserver(observer, deltaPosition)
    {
        observer.onCameraMove(deltaPosition);
    }

    fun onLoad(debugMode)
    {
        grid = debugMode.plugin("Debug Mode - Grid System");

        player = Player.active;
        transform.position = player.transform.position;

        defaultCameraDisabler.onLoad(debugMode);
    }

    fun onUnload(debugMode)
    {
        defaultCameraDisabler.onUnload(debugMode);
    }

    fun constructor()
    {
        if(SurgeEngine.mobile)
            this.mode = "player";
        else
            this.mode = "free-look";
    }
}

object "Debug Mode - Camera - Player Mode" is "debug-mode-camera-strategy"
{
    scrollSpeed = 400;
    gridSize = 16;
    input = spawn("Debug Mode - Input");
    turboButton = "secondary-action";

    fun move(transform, grid)
    {
        // move the camera
        ds = scrollVector();
        transform.translate(ds);

        // snap to grid
        if(ds.length == 0)
            transform.position = grid.snapToGrid(transform.position);

        // reposition the player
        player = Player.active;
        player.transform.position = transform.position;
    }

    fun init(transform)
    {
        // look at the player
        player = Player.active;
        transform.position = player.transform.position;
        return this;
    }

    fun scrollVector()
    {
        multiplier = input.buttonDown(turboButton) ? 4 : 1;
        speed = scrollSpeed * multiplier;

        return scrollDirection().scaledBy(speed * Time.delta);
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
}

object "Debug Mode - Camera - Free Look Mode" is "debug-mode-camera-strategy"
{
    scrollSpeed = 400;
    gridSize = 8; //16;
    input = spawn("Debug Mode - Input");
    turboButton = "secondary-action";

    fun move(transform, grid)
    {
        // move the camera
        ds = scrollVector();
        transform.translate(ds);

        // snap to grid
        if(ds.length == 0)
            transform.position = grid.snapToGrid(transform.position);
    }

    fun scrollVector()
    {
        multiplier = input.buttonDown(turboButton) ? 4 : 1;
        speed = scrollSpeed * multiplier;

        return scrollDirection().scaledBy(speed * Time.delta);
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
}

object "Debug Mode - Camera - Scroll Mode" is "debug-mode-camera-strategy"
{
    scroller = spawn("Debug Mode - UI Scroller");

    fun move(transform, grid)
    {
        // update the active area
        scroller.setActiveArea(0, 32, Screen.width, Screen.height - 32);

        // scroll
        dx = scroller.dx;
        dy = scroller.dy;

        // move
        transform.translateBy(-dx, -dy);
    }

    fun constructor()
    {
        scroller.smooth = false;
    }
}

object "Debug Mode - Camera - Default Camera Disabler"
{
    defaultCamera = null;
    wasEnabled = true;

    state "main"
    {
        if(defaultCamera !== null)
            defaultCamera.enabled = false;
    }

    fun onLoad(debugMode)
    {
        defaultCamera = Level.findObject("Default Camera");
        if(defaultCamera !== null)
            wasEnabled = defaultCamera.enabled;
    }

    fun onUnload(debugMode)
    {
        if(defaultCamera !== null)
            defaultCamera.enabled = wasEnabled;
    }
}