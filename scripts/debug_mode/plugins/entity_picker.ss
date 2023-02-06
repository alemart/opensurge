// -----------------------------------------------------------------------------
// File: entity_picker.ss
// Description: entity picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;

object "Debug Mode - Entity Picker" is "debug-mode-plugin", "detached", "private", "entity"
{
    debugMode = parent;
    carousel = spawn("Debug Mode - Carousel").setItemSize(40, 32);
    transform = Transform();

    state "main"
    {
        // initialize
        transform.position = Vector2.zero;
        carousel
            .addItem("Collectible")
            .addItem("Checkpoint")
            .addItem("Goal")
            .addItem("Goal Capsule")
            .addItem("Bumper")
            .addItem("Layer Green")
            .addItem("Layer Yellow")
            .addItem("Powerup 1up")
            .addItem("Powerup Collectibles")
            .addItem("Powerup Lucky Bonus")
            .addItem("Powerup Invincibility")
            .addItem("Powerup Speed")
            .addItem("Powerup Shield")
            .addItem("Powerup Shield Acid")
            .addItem("Powerup Shield Fire")
            .addItem("Powerup Shield Thunder")
            .addItem("Powerup Shield Water")
            .addItem("Powerup Shield Wind")
            .addItem("Powerup Trap")
            .addItem("Spikes")
            .addItem("Spikes Down")
            .addItem("Spring Booster Left")
            .addItem("Spring Booster Right")
            .addItem("Water Bubbles")
            .addItem("Tube In")
            .addItem("Tube Out")
        ;

        // done!
        state = "enabled";
    }

    state "enabled"
    {
        // do nothing
    }

    fun onCarouselItemPick(itemName)
    {
        Console.print(itemName);
    }
}