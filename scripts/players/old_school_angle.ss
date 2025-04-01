// -----------------------------------------------------------------------------
// File: old_school_angle.ss
// Description: sets the angle of the player to a multiple of 45 degrees
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Old School Angle" is "companion"
{
    player = parent;

    fun lateUpdate()
    {
        if(player.rolling || player.dying)
            return;

        if(wantZero()) {
            player.angle = 0;
            return;
        }

        bin = Math.floor((player.slope + 22.5) / 45);
        player.angle = 45 * bin;
    }

    fun wantZero()
    {
        if(player.walking || player.running || player.braking || player.springing)
            return false;

        return player.slope < 45 || player.slope > 315;
    }
}
