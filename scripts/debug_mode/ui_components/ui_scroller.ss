// -----------------------------------------------------------------------------
// File: ui_scroller.ss
// Description: UI Scroller component (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This UI component helps to develop other components that rely on scrolling.

Properties:
- dx: number, read-only. Horizontal scroll.
- dy: number, read-only. Vertical scroll.
- smooth: boolean. Whether or not to use smooth scrolling. Defaults to true.

*/

using SurgeEngine.Input.Mouse;
using SurgeEngine.Video.Screen;

// The UI Scroller is the Basic UI Scroller with a smoothing filter
object "Debug Mode - UI Scroller" is "debug-mode-ui-component"
{
    public readonly dx = 0;
    public readonly dy = 0;
    public smooth = true;
    delegate = spawn("Debug Mode - Basic UI Scroller");
    smoothingFactor = 1.0 * (1.0 / 60.0); // between 0 and 1

    state "main"
    {
        wantSmoothing = smooth && !delegate.isBeingScrolled();
        alpha = wantSmoothing ? smoothingFactor : 1;

        // apply exponential smoothing
        dx += alpha * (delegate.dx - dx);
        dy += alpha * (delegate.dy - dy);
    }

    fun setActiveArea(x, y, width, height)
    {
        delegate.setActiveArea(x, y, width, height);
        return this;
    }

    fun isInActiveArea(position)
    {
        return delegate.isInActiveArea(position);
    }

    fun isBeingScrolled()
    {
        return delegate.isBeingScrolled();
    }
}

object "Debug Mode - Basic UI Scroller" is "debug-mode-ui-component"
{
    // scroll offset in pixels
    public readonly dx = 0;
    public readonly dy = 0;

    // active area: coordinates are given in screen space
    x1 = 0;
    y1 = 0;
    x2 = Screen.width;
    y2 = Screen.height;

    // misc
    mouseWheelSpeed = 12;
    trackedTouchId = -1;

    state "main"
    {
        if(Mouse.scrollUp || Mouse.scrollDown) {
            if(isInActiveArea(Mouse.position)) {
                dx = dy = Mouse.scrollUp ? -mouseWheelSpeed : mouseWheelSpeed;
                state = "mouse scrolling";
            }
        }
        Console.print([dx,dy]);
    }

    state "mouse scrolling"
    {
        dx = dy = 0;
        state = "main";
    }

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
        if(trackedTouchId < 0 && isInActiveArea(touch.position)) {
            trackedTouchId = touch.id;
            dx = dy = 0;
        }
    }

    fun onTouchMove(touch)
    {
        if(trackedTouchId == touch.id) {
            dx = touch.deltaPosition.x;
            dy = touch.deltaPosition.y;
        }
    }

    fun onTouchStationary(touch)
    {
        if(trackedTouchId == touch.id)
            dx = dy = 0;
    }

    fun onTouchEnd(touch)
    {
        if(trackedTouchId == touch.id) {
            trackedTouchId = -1;
            dx = dy = 0;
        }
    }

    fun setActiveArea(x, y, width, height)
    {
        x1 = x;
        y1 = y;
        x2 = x + width;
        y2 = y + height;

        return this;
    }

    fun isInActiveArea(position)
    {
        if(position.x >= x1 && position.x < x2) {
            if(position.y >= y1 && position.y < y2)
                return true;
        }

        return false;
    }

    fun isBeingScrolled()
    {
        return (trackedTouchId >= 0) || (dx * dx + dy * dy > 0);
    }
}