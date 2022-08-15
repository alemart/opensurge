// -----------------------------------------------------------------------------
// File: surge_gameplay.ss
// Description: a setup object that enables basic mechanics of the game
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Level;

object "Surge Gameplay"
{
    // handles character switching
    switchController = spawn("Switch Controller");

    // handles water-related stuff
    waterController = spawn("Water Controller");

    // handles the little animals that appear in the levels
    animalManager = Level.spawn("Animals");

    // give an extra life every time the player picks up 100 collectibles
    collectiblesListener = spawn("Collectibles Listener").triggers("Give Extra Life").every(100);
}