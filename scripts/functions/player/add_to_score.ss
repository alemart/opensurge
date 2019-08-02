// -----------------------------------------------------------------------------
// File: add_to_score.ss
// Description: a function object that adds a value to the score of the player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;

//
// Add to Score is a function object that adds a value
// to the score of the player.
//
// Arguments:
// - value: number. A positive or negative integer.
//
object "Add to Score"
{
    fun call(value)
    {
        Player.active.score += value;
    }
}