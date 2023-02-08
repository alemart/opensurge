// -----------------------------------------------------------------------------
// File: scroller.ss
// Description: scroller UI component (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input.Mouse;
using SurgeEngine.Video.Screen;

object "Debug Mode - UI Scroller" is "debug-mode-ui-component"
{
    delegate = spawn("Debug Mode - Basic UI Scroller");
    smoothingFactor = 1.0 * (1.0 / 60.0); // between 0 and 1
    public readonly dx = 0;
    public readonly dy = 0;

    state "main"
    {
        alpha = delegate.isScrolling() ? 1 : smoothingFactor;

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

    fun isScrolling()
    {
        return delegate.isScrolling();
    }
}

object "Debug Mode - Basic UI Scroller" is "debug-mode-ui-component"
{
    // coordinates are given in screen space
    x1 = 0; // active area
    y1 = 0;
    x2 = Screen.width;
    y2 = Screen.height;
    mx = 0; // last position
    my = 0;
    public readonly dx = 0; // offset
    public readonly dy = 0;
    mouseWheelSpeed = 12;

    state "main"
    {
        state = "waiting";
    }

    state "waiting"
    {
        if(Mouse.buttonDown("left")) {
            if(isInActiveArea(Mouse.position)) {
                mx = Mouse.position.x;
                my = Mouse.position.y;
                dx = dy = 0;
                state = "scrolling";
                return;
            }

            state = "locked";
        }
        else if(Mouse.scrollUp || Mouse.scrollDown) {
            if(isInActiveArea(Mouse.position)) {
                sign = Mouse.scrollUp ? -1 : 1;
                dx = dy = mouseWheelSpeed * sign;
                state = "mouse scrolling";
            }
        }
    }

    state "scrolling"
    {
        if(!Mouse.buttonDown("left")) {
            dx = dy = 0;
            state = "waiting";
            return;
        }

        dx = Mouse.position.x - mx;
        dy = Mouse.position.y - my;

        mx = Mouse.position.x;
        my = Mouse.position.y;
    }

    state "mouse scrolling"
    {
        if(Mouse.scrollUp || Mouse.scrollDown)
            return;

        dx = dy = 0;
        state = "waiting";
    }

    state "locked"
    {
        if(!Mouse.buttonDown("left"))
            state = "waiting";
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

    fun isScrolling()
    {
        return state == "scrolling" || state == "mouse scrolling";
    }
}