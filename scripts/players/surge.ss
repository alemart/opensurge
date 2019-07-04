// -----------------------------------------------------------------------------
// File: surge.ss
// Description: companion objects for Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Surge's sneakers won't be lit while he's midair
//
object "Surge's Light Sneakers"
{
    player = Player("Surge");

    state "main"
    {
        if(player.midair) {
            if(player.walking)
                setAnim(20, player.animation.speedFactor);
            else if(player.running)
                setAnim(21, player.animation.speedFactor);
            else if(player.braking)
                setAnim(22, player.animation.speedFactor);
        }
    }

    fun setAnim(id, f)
    {
        player.anim = id;
        player.animation.speedFactor = f;
    }
}