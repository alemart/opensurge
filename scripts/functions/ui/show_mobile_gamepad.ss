// -----------------------------------------------------------------------------
// File: show_mobile_gamepad.ss
// Description: a function object that shows the Mobile Gamepad
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Input.MobileGamepad;

//
// Show Mobile Gamepad is a function object that shows the Mobile Gamepad
//
object "Show Mobile Gamepad"
{
    fun call()
    {
        MobileGamepad.fadeIn();
    }
}