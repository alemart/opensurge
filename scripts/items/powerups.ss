// -----------------------------------------------------------------------------
// File: powerups.ss
// Description: powerups script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Audio.Music;
using SurgeEngine.Collisions.CollisionBox;

//
// The following powerups use the "Item Box" object
// as a base (see below)
//

// Collectibles
object "Powerup Collectibles" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(4);

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.collectibles += 10;
    }
}

// Invincibility stars
object "Powerup Invincibility" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(5);
    music = Music("musics/invincible.ogg");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.invincible = true;
        music.play();
    }
}

// Enhanced speed (turbo)
object "Powerup Speed" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(6);
    music = Music("musics/speed.ogg");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.turbo = true;
        music.play();
        Level.spawn("Powerup Speed - Music Watcher").setPlayer(player).setMusic(music);
    }
}

// Stop the "speed" music if the turbo powerup is lost
object "Powerup Speed - Music Watcher"
{
    player = null;
    music = null;

    state "main"
    {
        if(player !== null && music !== null) {
            if(music.playing) {
                if(player.turbo)
                    return; // keep playing the music
                else if(player.underwater)
                    music.stop();
            }
        }

        // discard this music watcher
        destroy();
    }

    fun setPlayer(p)
    {
        player = p;
        return this;
    }

    fun setMusic(m)
    {
        music = m;
        return this;
    }
}

// 1up
object "Powerup 1up" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(1);
    giveExtraLife = spawn("Give Extra Life"); // function object stored in the functions/ folder

    state "main"
    {
        // the animation changes according
        // to the active player
        anim = animId(Player.active.name);
        itemBox.setAnimation(anim);
    }

    fun onItemBoxCrushed(player)
    {
        // this will play the 1up jingle for us
        giveExtraLife.call();
    }

    // given a player name, get the corresponding
    // animation ID of the "Item Box" sprite
    fun animId(playerName)
    {
        if(playerName == "Surge")
            return 1;
        else if(playerName == "Neon")
            return 2;
        else if(playerName == "Charge")
            return 3;
        else
            return 18; // generic "1up" icon
    }
}

// Regular shield
object "Powerup Shield" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(8);
    sfx = Sound("samples/shield.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.shield = "shield";
        sfx.play();
    }
}

// Fire shield
object "Powerup Shield Fire" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(12);
    sfx = Sound("samples/fireshield.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.shield = "fire";
        sfx.play();
    }
}

// Thunder shield
object "Powerup Shield Thunder" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(13);
    sfx = Sound("samples/thundershield.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.shield = "thunder";
        sfx.play();
    }
}

// Water shield
object "Powerup Shield Water" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(14);
    sfx = Sound("samples/watershield.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.shield = "water";
        sfx.play();
    }
}

// Acid shield
object "Powerup Shield Acid" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(15);
    sfx = Sound("samples/acidshield.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.shield = "acid";
        sfx.play();
    }
}

// Wind shield
object "Powerup Shield Wind" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(16);
    sfx = Sound("samples/windshield.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.shield = "wind";
        sfx.play();
    }
}

// Trap
object "Powerup Trap" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setScore(0).setAnimation(9);

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.getHit(null);
    }
}

// Lucky Bonus
object "Powerup Lucky Bonus" is "entity", "basic", "powerup"
{
    public bonus = 50;
    itemBox = spawn("Item Box").setAnimation(17);

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        lucky = Level.spawn("Lucky Bonus");
        lucky.bonus = Math.max(bonus, 0);
        lucky.player = player;
    }
}




//
// The code below is related to the base
// "Item Box" object
//

// Item Box
// Whenever the item box is crushed by the player,
// it will call function onItemBoxCrushed(player)
// on the parent object.
object "Item Box" is "entity", "private"
{
    actor = Actor("Item Box");
    brick = Brick("Item Box");
    collider = CollisionBox(24, 30).setAnchor(0.5, 1.2);
    transform = Transform();
    score = 100;

    state "main"
    {
        brick.enabled = !canCrush(Player.active);
    }

    state "crushed"
    {
        actor.anim = 11; // can't change the animation if the item box is crushed
    }

    fun constructor()
    {
        actor.animation.sync = true;
    }

    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(canCrush(player)) {
                player.bounceBack(actor);
                crush(player);
            }
        }
    }

    // crushes the item box
    fun crush(player)
    {
        if(state != "crushed") {
            // where should we spawn the sprites?
            actionSpot = transform.position.translatedBy(0, -actor.height * 0.7);

            // create explosion & item box icon
            Level.spawnEntity("Explosion", actionSpot);
            Level.spawnEntity("Item Box Icon", actionSpot).setIcon(actor.anim);

            // add to score
            if(score != 0) {
                player.score += score;
                Level.spawnEntity("Score Text", actionSpot).setText(score);
            }

            // crush the item box
            collider.enabled = false;
            brick.enabled = false;
            actor.anim = 11;
            state = "crushed";

            // notify the parent
            parent.onItemBoxCrushed(player);
        }
    }

    // can the player crush the item box?
    fun canCrush(player)
    {
        /* player.attacking won't cut it (it's true when invincible) */
        return player.jumping || player.rolling || player.charging || player.aggressive;
    }

    // --- MODIFIERS ---

    // set the animation of the "Item Box" sprite
    fun setAnimation(anim)
    {
        actor.anim = anim;
        return this;
    }

    // set the score to be added to the player when this item box gets hit
    fun setScore(value)
    {
        score = Math.max(value, 0);
        return this;
    }
}

// Item Box Icon
object "Item Box Icon" is "entity", "private", "disposable"
{
    actor = Actor("Item Box Icon");
    transform = Transform();
    speed = 50;
    hscale = 1;
    timeToLive = 2.0;

    state "main"
    {
        transform.translateBy(0, -speed * Time.delta);
        if(timeout(timeToLive * 0.25))
            state = "wait";
    }

    state "wait"
    {
        if(timeout(timeToLive * 0.75))
            state = "disappear";
    }

    state "disappear"
    {
        hscale -= 5 * Time.delta;
        if(hscale > 0)
            transform.localScale = Vector2(1, hscale);
        else
            destroy();
    }

    // --- MODIFIERS ---

    // set the icon, given as an animation ID
    fun setIcon(anim)
    {
        actor.anim = anim;
        return this;
    }
}