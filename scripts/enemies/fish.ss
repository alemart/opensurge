// -----------------------------------------------------------------------------
// File: fish.ss
// Description: Fish enemy script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;

// Fish is a baddie that attacks when the player
// is within a visible range
object "Fish" is "entity", "enemy"
{
    public anim = 0;
    public jumpHeight = 128; // in pixels (y-axis)
    public visibleRange = 128; // in pixels (x-axis)

    actor = Actor("Fish");
    enemy = Enemy();
    movement = DirectionalMovement();
    transform = Transform();
    sfx = Sound("samples/fish.wav");
    spawnPoint = 0;

    state "main"
    {
        // setup
        spawnPoint = transform.position.y;
        movement.direction = Vector2.down;
        movement.speed = 0;
        actor.anim = anim;
        actor.visible = false;
        state = "waiting";
    }

    state "waiting"
    {
        // wait for the player
        actor.visible = false;
        if(distance(Player.active) <= visibleRange) {
            movement.speed = jumpSpeed();
            sfx.volume = jumpVolume();
            sfx.play();
            state = "jumping";
        }
    }

    state "jumping"
    {
        // move the fish
        movement.speed += Level.gravity * Time.delta;
        actor.visible = true;

        // reposition the fish & stop its movement
        if(transform.position.y >= spawnPoint) {
            transform.move(0, spawnPoint - transform.position.y);
            movement.speed = 0;
            actor.visible = false;
            state = "cooldown";
        }
    }

    state "cooldown"
    {
        if(timeout(2.0))
            state = "waiting";
    }

    fun jumpSpeed()
    {
        // Torricelli's
        return -Math.sqrt(2 * Level.gravity * Math.abs(jumpHeight));
    }

    fun jumpVolume()
    {
        vol = 1 - distance(Player.active) / Screen.width;
        return Math.max(0, vol);
    }

    fun distance(player)
    {
        return Math.abs(player.transform.position.x - transform.position.x);
    }
}