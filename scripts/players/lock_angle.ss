// -----------------------------------------------------------------------------
// File: lock_angle.ss
// Description: locks the angle of the player, so it doesn't rotate on slopes
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// Lock Angle is a companion object that locks the player angle
// to zero, so it doesn't rotate on slopes. This is useful for
// regular platformers.
//
object "Lock Angle" is "companion"
{
    player = parent;

    state "main"
    {
    }

    fun lateUpdate()
    {
        player.angle = 0;
    }
}