// -----------------------------------------------------------------------------
// File: enable_controls.ss
// Description: a function object that enables input controls
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Enable Controls is a function object that
// enables the input controls of the active player
//
object "Enable Controls"
{
    fun call()
    {
        Player.active.input.enabled = true;
    }
}