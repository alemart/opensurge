// -----------------------------------------------------------------------------
// File: give_lucky_bonus.ss
// Description: a function object that gives the player a bonus
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;
using SurgeEngine.Player;

//
// Give Lucky Bonus is a function object that gives
// the player a Lucky Bonus: a bunch of collectibles
// that run towards the player with a neat animation.
//
// Arguments:
// - bonus: number. A positive integer indicating
//                  the number of collectibles.
//
object "Give Lucky Bonus"
{
    // The default value for bonus is 50.
    fun call(bonus)
    {
        if(bonus > 0) {
            lucky = Level.spawn("Lucky Bonus");
            lucky.player = Player.active;
            lucky.bonus = bonus;
        }
    }
}