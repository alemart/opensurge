// -----------------------------------------------------------------------------
// File: hide_mobile_gamepad.ss
// Description: a function object that hides the Mobile Gamepad
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input.MobileGamepad;

//
// Hide Mobile Gamepad is a function object that hides the Mobile Gamepad
//
object "Hide Mobile Gamepad"
{
    fun call()
    {
        MobileGamepad.fadeOut();
    }
}