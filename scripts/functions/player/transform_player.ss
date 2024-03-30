// -----------------------------------------------------------------------------
// File: transform_player.ss
// Description: transforms the active player into a different character
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Audio.Sound;

//
// Transform Player is a function object that transforms
// the active player into a different character.
//
// Arguments:
// - character: string. The name of a character, as
//   defined in a .chr file.
//
object "Transform Player"
{
    sfx = Sound("samples/destroy.wav");

    fun call(character)
    {
        player = Player.active;
        originalCharacter = player.name;

        if(!player.transformInto(character)) {
            Console.print("Can't transform \"" + originalCharacter + "\" into \"" + character + "\"");
            return;
        }

        Level.spawnEntity("Explosion", player.collider.center);
        sfx.play();
    }
}