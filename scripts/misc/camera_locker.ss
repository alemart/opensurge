// -----------------------------------------------------------------------------
// File: camera_lock.ss
// Description: the Camera Lock utility is used to smoothly lock the camera
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Camera;
using SurgeEngine.Video.Screen;

object "Camera Lock"
{
    public readonly x1 = -Math.infinity;
    public readonly y1 = -Math.infinity;
    public readonly x2 = Math.infinity;
    public readonly y2 = Math.infinity;

    transitionTime = 1.0; // seconds
    timer = 0;
    initialX1 = 0;
    initialY1 = 0;
    initialX2 = 0;
    initialY2 = 0;
    targetX1 = 0;
    targetY1 = 0;
    targetX2 = 0;
    targetY2 = 0;
    mx = Screen.width / 2; // margins
    my = Screen.height / 2;

    state "main"
    {
    }

    state "locking"
    {
        // update timer
        timer += Time.delta / transitionTime;

        // smooth out the coordinates
        interpolate(timer);

        // lock camera
        Camera.lock(x1, y1, x2, y2);

        // end of transition?
        if(timer >= 1)
            state = "main";
    }

    state "unlocking"
    {
        // update timer
        timer += Time.delta / transitionTime;

        // smooth out the coordinates
        interpolate(timer);

        // lock camera
        Camera.lock(x1, y1, x2, y2);

        // end of transition?
        if(timer >= 1) {
            x1 = -Math.infinity;
            y1 = -Math.infinity;
            x2 = Math.infinity;
            y2 = Math.infinity;
            Camera.unlock();
            state = "main";
        }
    }

    state "expanding"
    {
        timer += Time.delta / transitionTime;
        interpolate(timer);
        Camera.lock(x1, y1, x2, y2);
        if(timer >= 1)
            state = "main";
    }

    // lock camera; all coordinates given in pixels (world space)
    fun lock(left, top, right, bottom)
    {
        // can't do it?
        if(state == "locking")
            return false;

        // sanitize
        if(left > right)
            left = right = (left + right) / 2;
        if(top > bottom)
            top = bottom = (top + bottom) / 2;

        // set up initial values
        initialX1 = left - mx;
        initialY1 = top - my;
        initialX2 = right + mx;
        initialY2 = bottom + my;

        // set up target values
        targetX1 = left;
        targetY1 = top;
        targetX2 = right;
        targetY2 = bottom;

        // start transition
        timer = 0;
        state = "locking";

        // success
        return true;
    }

    // unlock camera
    fun unlock()
    {
        // can't do it?
        if(!this.locked || state == "unlocking")
            return false;

        // set up initial values
        initialX1 = x1;
        initialY1 = y1;
        initialX2 = x2;
        initialY2 = y2;

        // set up target values
        targetX1 = x1 - mx;
        targetY1 = y1 - my;
        targetX2 = x2 + mx;
        targetY2 = y2 + my;

        // start transition
        timer = 0;
        state = "unlocking";

        // success
        return true;
    }

    // expand or shrink the locked area
    // expand if delta > 0, shrink if delta < 0
    fun expand(deltaLeft, deltaTop, deltaRight, deltaBottom)
    {
        // can't do it?
        if(!this.locked || state == "expanding")
            return false;

        // set up initial values
        initialX1 = targetX1;
        initialY1 = targetY1;
        initialX2 = targetX2;
        initialY2 = targetY2;

        // set up target values
        targetX1 = initialX1 - deltaLeft;
        targetY1 = initialY1 - deltaTop;
        targetX2 = initialX2 + deltaRight;
        targetY2 = initialY2 + deltaBottom;

        // sanitize
        if(targetX1 > targetX2) {
            targetX1 = initialX1;
            targetX2 = initialX2;
        }
        if(targetY1 > targetY2) {
            targetY1 = initialY1;
            targetY2 = initialY2;
        }

        // start transition
        timer = 0;
        state = "locking";

        // success
        return true;
    }

    // is the camera currently locked?
    fun get_locked()
    {
        return Camera.locked;
    }

    // smooths out (x1, y1, x2, y2), given t in [0,1]
    fun interpolate(t)
    {
        x1 = Math.smoothstep(initialX1, targetX1, t);
        y1 = Math.smoothstep(initialY1, targetY1, t);
        x2 = Math.smoothstep(initialX2, targetX2, t);
        y2 = Math.smoothstep(initialY2, targetY2, t);
    }
}