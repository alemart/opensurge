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
    spawnPoint = Vector2.zero;

    state "main"
    {
        // setup
        spawnPoint = transform.position.scaledBy(1); // clone
        movement.direction = Vector2.down;
        movement.speed = 0;
        actor.anim = anim;
        actor.visible = false;
        enemy.enabled = false;
        state = "waiting";
    }

    state "waiting"
    {
        // wait for the player
        actor.visible = enemy.enabled = false;

        // activate the fish
        if(distance(Player.active) <= visibleRange) {
            if(Player.active.transform.position.y <= spawnPoint.y) {
                actor.visible = enemy.enabled = true;
                movement.speed = jumpSpeed();
                sfx.volume = jumpVolume();
                sfx.play();
                state = "jumping";
            }
        }
    }

    state "jumping"
    {
        // move the fish
        movement.speed += Level.gravity * Time.delta;

        // reposition the fish & stop its movement
        if(transform.position.y >= spawnPoint.y) {
            actor.visible = enemy.enabled = false;
            transform.move(0, spawnPoint.y - transform.position.y);
            movement.speed = 0;
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