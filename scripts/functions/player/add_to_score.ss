// -----------------------------------------------------------------------------
// File: add_to_score.ss
// Description: a function object that adds a value to the score of the player
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;

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
        player = Player.active;
        score = Math.floor(value);
        if(score != 0) {
            player.score += score;
            Level.spawnEntity("Score Text", player.collider.center).setText(score);
        }
    }
}