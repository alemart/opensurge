// -----------------------------------------------------------------------------
// File: lock_camera.ss
// Description: a function object that locks the camera
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Video.Screen;

//
// Lock Camera is a function object that locks
// the camera within a certain range.
//
// Arguments:
// - param: number | Vector2 | Array. Lock specification.
//
// Argument param can be:
//
// 1. a positive number (gives space to the right)
//    or a negative number (gives space to the left)
//    specifying an offset relative to the player
//
// 2. a Vector2 object (gives space in both axis)
//    specifying an offset relative to the player
//
// 3. a 4-element Array specifying the world
//    coordinates of the camera lock
//
object "Lock Camera"
{
    left = 0; right = 0; top = 0; bottom = 0;
    camlock = null;

    state "main"
    {
    }

    state "locking"
    {
        // try to lock until we're successful
        if(camlock.lock(left, top, right, bottom))
            state = "main";
    }

    state "expanding"
    {
        // will try to expand until we're successful
        if(camlock.expand(-(left - camlock.x1), -(top - camlock.y1), right - camlock.x2, bottom - camlock.y2))
            state = "main";
    }

    fun call(param)
    {
        // initializing
        camlock = Level.child("Camera Locker") || Level.spawn("Camera Locker");
        d = Level.child("Lock Camera Direction") || Level.spawn("Lock Camera Direction");
        d.direction = camlock.locked ? d.direction : 0; // reset if unlocked

        // get the offset(s)
        if(typeof(param) != "object") {
            offsetX = Number(param);
            offsetY = 0;
        }
        else if(param.__name == "Vector2") {
            offsetX = param.x;
            offsetY = param.y;
        }
        else if(param.__name == "Array" && param.length == 4) {
            left = Number(param[0]);
            top = Number(param[1]);
            right = Number(param[2]);
            bottom = Number(param[3]);
            state = "locking";
            return;
        }
        else
            offsetX = offsetY = 0;
        
        // here's the logic
        if(!camlock.locked) {
            player = Player.active;

            // compute base position
            baseX = player.collider.center.x;
            baseY = player.collider.bottom + 32;

            // compute lock coordinates
            if(offsetX >= 0) {
                // expand to the right
                left = baseX - Screen.width / 2;
                right = baseX + offsetX;
                top = baseY - Screen.height;
                bottom = baseY;
                d.direction = 1;
            }
            else {
                // expand to the left
                left = baseX + offsetX;
                right = baseX + Screen.width / 2;
                top = baseY - Screen.height;
                bottom = baseY;
                d.direction = -1;
            }

            // FIXME: y-offset only works on the first lock call
            if(offsetY > 0) {
                top += Math.min(Screen.height, offsetY);
                bottom += offsetY;
            }
            else if(offsetY < 0)
                top += offsetY;

            // lock camera
            state = "locking";
        }
        else {
            // compute lock coordinates
            if(d.direction >= 0) {
                if(offsetX >= 0) {
                    // expand to the right
                    left = camlock.x1;
                    right = camlock.x2 + offsetX;
                    top = camlock.y1;
                    bottom = camlock.y2;
                    d.direction = 1;
                }
                else {
                    if(camlock.x2 + offsetX >= camlock.x1) {
                        // shrink from the right
                        left = camlock.x1;
                        right = camlock.x2 + offsetX;
                        top = camlock.y1;
                        bottom = camlock.y2;
                        d.direction = 1;
                    }
                    else {
                        // expand to the left
                        left = camlock.x1 - (-offsetX - (camlock.x2 - camlock.x1));
                        right = camlock.x1;
                        top = camlock.y1;
                        bottom = camlock.y2;
                        d.direction = -1;
                    }
                }
            }
            else {
                if(offsetX >= 0) {
                    if(camlock.x1 + offsetX <= camlock.x2) {
                        // shrink from the left
                        left = camlock.x1 + offsetX;
                        right = camlock.x2;
                        top = camlock.y1;
                        bottom = camlock.y2;
                        d.direction = -1;
                    }
                    else {
                        // expand to the right
                        left = camlock.x2;
                        right = camlock.x2 + (offsetX - (camlock.x2 - camlock.x1));
                        top = camlock.y1;
                        bottom = camlock.y2;
                        d.direction = 1;
                    }
                }
                else {
                    // expand to the left
                    left = camlock.x1 + offsetX;
                    right = camlock.x2;
                    top = camlock.y1;
                    bottom = camlock.y2;
                    d.direction = -1;
                }
            }

            // expand camera
            state = "expanding";
        }
    }
}

// this object is used to keep a state between event calls
object "Lock Camera Direction" { public direction = 0; }