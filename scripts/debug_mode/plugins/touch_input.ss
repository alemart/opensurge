// -----------------------------------------------------------------------------
// File: touch_input.ss
// Description: touch input plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin handles Touch Input in the Debug Mode. You should use this plugin
whenever you need to accept touch input. Use it as in this example:

object "Debug Mode - My Touch Input Test" is "debug-mode-plugin"
{
    fun onLoad(debugMode)
    {
        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.subscribe(this);
    }

    fun onUnload(debugMode)
    {
        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.unsubscribe(this);
    }

    fun onTouchBegin(touch)
    {
        Console.print("TOUCH BEGIN");
        print(touch);
    }

    fun onTouchEnd(touch)
    {
        Console.print("TOUCH END");
        print(touch);
    }

    fun onTouchMove(touch)
    {
        Console.print("TOUCH MOVE");
        print(touch);
    }

    fun onTouchStationary(touch)
    {
        Console.print("TOUCH STATIONARY");
        print(touch);
    }

    fun print(touch)
    {
        Console.print("");

        Console.print("id: " + touch.id); // a number that identifies the touch / finger
        Console.print("phase: " + touch.phase); // began | moved | stationary | ended

        Console.print("position: " + touch.position); // given in screen space
        Console.print("initialPosition: " + touch.initialPosition); // given in screen space

        Console.print("deltaPosition: " + touch.deltaPosition); // given in screen space
        Console.print("deltaTime: " + touch.deltaTime); // given in seconds
    }
}

You can also query the touch input directly, without subscribing to it:

object "Debug Mode - My Touch Input Test 2" is "debug-mode-plugin"
{
    touchInput = null;

    state "main"
    {
        for(i = 0; i < touchInput.count; i++) {
            touch = touchInput.get(i);
            print(touch);
        }

        // you may optionally enable/disable touch input as follows:
        // touchInput.enabled = true;
        // touchInput.enabled = false;
    }

    fun onLoad(debugMode)
    {
        touchInput = debugMode.plugin("Debug Mode - Touch Input");
    }
    
    fun onUnload(debugMode)
    {
    }

    fun print(touch)
    {
        Console.print("");

        Console.print("id: " + touch.id); // a number that identifies the touch / finger
        Console.print("phase: " + touch.phase); // "began" | "moved" | "stationary" | "ended"

        Console.print("position: " + touch.position); // Vector2 given in screen space
        Console.print("initialPosition: " + touch.initialPosition); // Vector2 given in screen space

        Console.print("deltaPosition: " + touch.deltaPosition); // Vector2 given in screen space
        Console.print("deltaTime: " + touch.deltaTime); // number (seconds)
    }
}

Note: this is a temporary implementation that uses the Mouse, because actual
touch input is yet to be implemented in the SurgeScript API. This plugin will
be adapted to actual touch input in the future.

*/

using SurgeEngine.Input.Mouse;
using SurgeEngine.Vector2;

object "Debug Mode - Touch Input" is "debug-mode-plugin", "debug-mode-observable"
{
    touchArray = [];
    observable = spawn("Debug Mode - Observable");

    state "main"
    {
        if(Mouse.buttonDown("left")) {
            touch = Touch("began", Mouse.position, Mouse.position, Mouse.position);
            touchArray.push(touch);

            observable.notify(touch);
            state = "touching";
        }
    }

    state "touching"
    {
        if(Mouse.buttonDown("left")) {
            moved = (Mouse.position.distanceTo(touchArray[0].position) > 0.0);
            phase = moved ? "moved" : "stationary";
            touch = Touch(phase, Mouse.position, touchArray[0].position, touchArray[0].initialPosition);
            touchArray[0] = touch;

            observable.notify(touch);
        }
        else {
            touch = Touch("ended", Mouse.position, touchArray[0].position, touchArray[0].initialPosition);
            touchArray[0] = touch;

            observable.notify(touch);
            state = "cleared";
        }
    }

    state "cleared"
    {
        touchArray.clear();
        state = "main";
    }

    state "disabled"
    {
        touchArray.clear();
    }

    fun get(touchId)
    {
        if(touchId >= 0 && touchId < touchArray.length)
            return touchArray[touchId];

        return null;
    }

    fun get_count()
    {
        return touchArray.length;
    }

    fun get_enabled()
    {
        return state != "disabled";
    }

    fun set_enabled(enabled)
    {
        if(enabled) {
            if(state == "disabled")
                state = "main";
        }
        else {
            state = "disabled";
        }
    }

    fun subscribe(observer)
    {
        observable.subscribe(observer);
    }

    fun unsubscribe(observer)
    {
        observable.unsubscribe(observer);
    }

    fun onNotifyObserver(observer, touch)
    {
        if(touch.phase == "moved") {
            if(observer.hasFunction("onTouchMove"))
                observer.onTouchMove(touch);
        }
        else if(touch.phase == "began") {
            if(observer.hasFunction("onTouchBegin"))
                observer.onTouchBegin(touch);
        }
        else if(touch.phase == "ended") {
            if(observer.hasFunction("onTouchEnd"))
                observer.onTouchEnd(touch);
        }
        else if(touch.phase == "stationary") {
            if(observer.hasFunction("onTouchStationary"))
                observer.onTouchStationary(touch);
        }
    }

    fun Touch(phase, position, previousPosition, initialPosition)
    {
        return spawn("Debug Mode - Touch").init(phase, position, previousPosition, initialPosition);
    }
}

object "Debug Mode - Touch"
{
    id = 0;
    phase = "began"; // "began" | "moved" | "stationary" | "ended"
    initialPosition = Vector2.zero;
    previousPosition = Vector2.zero;
    currentPosition = Vector2.zero;
    deltaPosition = null;
    deltaTime = 0.0;

    fun get_id()
    {
        return id;
    }

    fun get_phase()
    {
        return phase;
    }

    fun get_position()
    {
        return currentPosition;
    }

    fun get_initialPosition()
    {
        return initialPosition;
    }

    fun get_deltaPosition()
    {
        return deltaPosition || (deltaPosition = currentPosition.minus(previousPosition));
    }

    fun get_deltaTime()
    {
        return deltaTime;
    }

    fun init(currPhase, currPos, prevPos, initialPos)
    {
        id = 0;
        deltaPosition = null;
        deltaTime = Time.delta;

        phase = currPhase;
        currentPosition = currPos.scaledBy(1); // immutable
        previousPosition = prevPos.scaledBy(1);
        initialPosition = initialPos.scaledBy(1);

        return this;
    }
}