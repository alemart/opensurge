// -----------------------------------------------------------------------------
// File: none.ss
// Description: companion object for playable character 'None'
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

object "None" is "companion"
{
    player = parent;

    state "main"
    {
        // disable physics
        player.frozen = true;

        // make it non-focusable
        player.focusable = false;
    }
}