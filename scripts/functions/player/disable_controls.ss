// -----------------------------------------------------------------------------
// File: disable_controls.ss
// Description: a function object that disables input controls
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Disable Controls is a function object that
// disables the input controls of the active player
//
object "Disable Controls"
{
    fun call()
    {
        Player.active.input.enabled = false;
    }
}