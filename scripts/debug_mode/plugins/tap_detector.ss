// -----------------------------------------------------------------------------
// File: tap_detector.ss
// Description: tap detector plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin is used to detect taps on the screen. Use it as in this example:

object "Debug Mode - My Tapping Test" is "debug-mode-plugin"
{
    fun onLoad(debugMode)
    {
        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.subscribe(this);
    }

    fun onUnload(debugMode)
    {
        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.unsubscribe(this);
    }

    fun onTapScreen(position)
    {
        // position is a Vector2 given in screen coordinates
        Console.print("Tapped at " + position);
    }
}

*/

object "Debug Mode - Tap Detector" is "debug-mode-plugin", "debug-mode-observable"
{
    observable = spawn("Debug Mode - Observable");
    touchId = -1;
    touchMoved = false;
    touchInitialTime = 0.0;
    tapTimeThreshold = 0.25; //0.2;

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
        touchId = touch.id;
        touchInitialTime = Time.time;
        touchMoved = false;
    }

    fun onTouchEnd(touch)
    {
        if(touchId == touch.id) {
            touchId = -1;

            if(!touchMoved) {
                if(Time.time <= touchInitialTime + tapTimeThreshold) {
                    observable.notify(touch.position);
                }
            }
        }
    }

    fun onTouchMove(touch)
    {
        if(touchId == touch.id) {
            touchMoved = true;
            //touchMoved = (touch.deltaPosition.length >= 0.5);
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

    fun onNotifyObserver(observer, position)
    {
        observer.onTapScreen(position);
    }
}