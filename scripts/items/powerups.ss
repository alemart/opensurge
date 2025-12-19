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
using SurgeEngine.Events.Event;
using SurgeEngine.Events.FunctionEvent;
using SurgeTheRabbit.GameState;

//
// The following powerups use the "Item Box" object
// as a base (see below)
//

// Collectibles
object "Powerup Collectibles" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(4);
    sfx = Sound("samples/collectible.wav");

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.collectibles += 10;
        sfx.play();
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

// Turbocharged speed
object "Powerup Speed" is "entity", "basic", "powerup"
{
    itemBox = spawn("Item Box").setAnimation(6);

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        player.turbo = true;
        Level.spawn("Powerup Speed - Music Watcher").setPlayer(player);
    }
}

// Stop the "speed" music if the turbo powerup is lost
object "Powerup Speed - Music Watcher"
{
    music = Music("musics/speed.ogg");
    player = null;

    state "main"
    {
        if(player === null)
            Application.crash(this.__name + ": unset player");

        music.play();
        state = "watch";
    }

    state "watch"
    {
        if(music.playing) {
            if(player !== null) {
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
}

// 1up
object "Powerup 1up" is "entity", "basic", "powerup"
{
    sprite = { }; // sprites are indexed by player name (not by player id because players may be transformed)
    generic1upAnim = 18;
    itemBox = spawn("Item Box").setAnimation(generic1upAnim);
    onCrush = FunctionEvent("Give Extra Life"); // function object stored in the functions/ folder
    activePlayerName = "";

    state "main"
    {
        // prevent the respawning of life powerups when restarting the level
        if(GameState.isEntityConsumed(this)) {
            itemBox.forceCrushed();
            state = "crushed";
        }
        else
            state = "watching";
    }

    state "watching"
    {
        // check if the active player has changed or transformed
        if(activePlayerName != Player.active.name) {
            activePlayerName = Player.active.name;

            // check if there are custom 1up sprites for the character
            if(!sprite.has(activePlayerName))
                sprite[activePlayerName] = findSprites(activePlayerName);

            // change the sprite of the powerup if one is available
            if(sprite[activePlayerName] !== null)
                itemBox.setAnimationEx(0, sprite[activePlayerName]["box"], sprite[activePlayerName]["icon"]);
            else
                itemBox.setAnimation(generic1upAnim);
        }
    }

    state "crushed"
    {
    }

    fun onItemBoxCrushed(player)
    {
        // this will play the 1up jingle for us
        onCrush.call();

        // update the game state
        GameState.consumeEntityUntilLevelChange(this);

        // done
        state = "crushed";
    }

    fun findSprites(playerName)
    {
        boxSprite = "Powerup 1up " + playerName;
        iconSprite = "Powerup 1up Icon " + playerName;

        box = Actor(boxSprite);
        icon = Actor(iconSprite);
        success = box.animation.exists && icon.animation.exists;
        icon.destroy();
        box.destroy();

        if(success) {
            return {
                "box": boxSprite,
                "icon": iconSprite
            };
        }

        return null;
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

// Transformation
object "Powerup Transformation" is "entity", "basic", "powerup"
{
    public character = "Surge"; // name of the character
    public duration = 20; // duration of the transformation, in seconds
    itemBox = spawn("Item Box").setAnimation(19);
    manager = Level.child("Powerup Transformation - Manager") || Level.spawn("Powerup Transformation - Manager");
    watcher = null;

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        originalName = manager.originalNameOf(player);

        // transform the player
        if(!player.transformInto(character)) {
            Console.print("Can't transform \"" + player.name + "\" into \"" + character + "\"");
            return;
        }

        Level.spawnEntity("Explosion", player.collider.center);

        // get the transformation watcher linked to this player
        watcher = manager.watcherOf(player);
        if(watcher === null) {
            // new transformation
            watcher = Level.spawn("Powerup Transformation - Player Watcher")
                           .setManager(manager);
        }
        else {
            // override existing transformation
            ;
        }

        // setup the watcher
        watcher.setPlayer(player)
               .setCharacter(originalName)
               .setDuration(duration >= 0 ? duration : Math.infinity);
    }
}

// Make the player return to its original form after a transformation
object "Powerup Transformation - Player Watcher"
{
    player = null;
    manager = null;
    endTime = 0;
    character = "";
    sfx = Sound("samples/destroy.wav");

    state "main"
    {
        assert(player !== null);
        assert(manager !== null);

        manager.setWatcherOf(player, this);
        state = "watch";
    }

    state "watch"
    {
        if(Time.time >= endTime)
            state = "detransform";
    }

    state "detransform"
    {
        assert(player !== null);
        assert(manager !== null);

        player.transformInto(character);
        Level.spawnEntity("Explosion", player.collider.center);
        sfx.play();

        manager.setWatcherOf(player, null);
        destroy();
    }

    fun setManager(m)
    {
        manager = m;
        return this;
    }

    fun setPlayer(p)
    {
        player = p;
        return this;
    }

    fun setDuration(d)
    {
        duration = Math.max(0, d);
        endTime = Time.time + duration; // this may be greater than or less than the previous endTime !!!
                                        // e.g., get multiple powerups with different durations.
                                        // we want this behavior because each powerup may transform the
                                        // player into a different character.
        return this;
    }

    fun setCharacter(s)
    {
        character = String(s);
        return this;
    }
}

// Helper object
object "Powerup Transformation - Manager"
{
    originalName = {};
    watcher = {};

    fun originalNameOf(player)
    {
        if(originalName.has(player.id))
            return originalName[player.id];

        originalName[player.id] = player.name;
        return player.name;
    }

    fun watcherOf(player)
    {
        if(watcher.has(player.id))
            return watcher[player.id];

        return null;
    }

    fun setWatcherOf(player, obj)
    {
        watcher[player.id] = obj;
    }

    fun constructor()
    {
        // store the original names of all players when the level is loaded
        for(i = 0; i < Player.count; i++) {
            player = Player[i];
            originalName[player.id] = player.name;
        }
    }
}

// Event Trigger
object "Powerup Event Trigger" is "entity", "basic", "powerup"
{
    public onTrigger = Event();
    itemBox = spawn("Item Box").setAnimation(20);

    state "main"
    {
    }

    fun onItemBoxCrushed(player)
    {
        onTrigger();
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
    brick = Brick("Item Box Mask");
    smallerCollider = CollisionBox(33, 33).setAnchor(0.5, 1.0);
    biggerCollider = CollisionBox(65, 81).setAnchor(0.5, 0.8); // 2 * player capspeed = 2 * player topyspeed = 2 * 16 px/frame
    transform = Transform();
    iconSpriteName = "Item Box Icon";
    crushed = 11;
    empty = 0;
    score = 100;

    state "main"
    {
        for(i = 0; i < Player.count; i++) {
            player = Player[i];

            if(player.collider.collidesWith(biggerCollider)) {
                if(canCrush(player))
                    brick.enabled = false;
            }
        }
    }

    state "crushed"
    {
        setAnimation(crushed); // can't change the animation if the item box is crushed
    }

    fun constructor()
    {
        setAnimation(empty);
    }

    fun lateUpdate()
    {
        if(isCrushed())
            return;

        restoreBrick = true;

        for(i = 0; i < Player.count; i++) {
            player = Player[i];

            if(player.collider.collidesWith(smallerCollider)) {
                if(canCrush(player)) {
                    player.ysp = -player.ysp; // bounce the player
                    crush(player);
                    return;
                }
            }

            if(player.collider.collidesWith(biggerCollider)) {
                if(canCrush(player))
                    restoreBrick = false;
            }
        }

        if(restoreBrick)
            brick.enabled = true;
    }

    // crushes the item box
    fun crush(player)
    {
        if(isCrushed())
            return;

        // where should we spawn the sprites?
        if(actor.actionSpot.length > 0)
            actionSpot = transform.position.plus(actor.actionOffset);
        else
            actionSpot = transform.position.translatedBy(0, -actor.height * 0.7);

        // create explosion & item box icon
        Level.spawnEntity("Explosion", actionSpot);
        Level.spawnEntity("Item Box Icon", actionSpot).setIconEx(actor.anim, iconSpriteName);

        // add to score
        if(score != 0) {
            player.score += score;
            Level.spawnEntity("Score Text", actionSpot).setText(score);
        }

        // really crush the item box
        forceCrushed();

        // notify the parent
        parent.onItemBoxCrushed(player);
    }

    // can the player crush the item box?
    fun canCrush(player)
    {
        if(player.secondary)
            return false;

        // player.attacking won't cut it (it's true when invincible)
        return player.jumping || player.rolling || player.charging || player.aggressive;
    }

    // force the crushed state without exploding and without notifying the parent
    fun forceCrushed()
    {
        if(isCrushed())
            return;

        smallerCollider.enabled = false;
        biggerCollider.enabled = false;
        brick.enabled = false;
        setAnimation(crushed);

        state = "crushed";
    }

    // is the item box crushed?
    fun isCrushed()
    {
        return state == "crushed";
    }

    // --- MODIFIERS ---

    // set the score to be added to the player when this item box gets hit
    fun setScore(value)
    {
        score = Math.max(value, 0);
        return this;
    }

    // set the animation of the "Item Box" sprite
    fun setAnimation(anim)
    {
        return setAnimationEx(anim, "Item Box", "Item Box Icon");
    }

    fun setAnimationEx(anim, spriteNameOfBox, spriteNameOfIcon)
    {
        if(spriteNameOfBox != actor.animation.sprite) {
            actor.destroy();
            actor = Actor(spriteNameOfBox);
            actor.animation.sync = true;
        }

        actor.anim = anim;
        iconSpriteName = spriteNameOfIcon;

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
    empty = 0;
    zindex = 0.51;

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

    fun constructor()
    {
        setIcon(empty);
    }

    // --- MODIFIERS ---

    // set the icon, given as an animation ID
    fun setIcon(anim)
    {
        return setIconEx(anim, "Item Box Icon");
    }

    fun setIconEx(anim, spriteName)
    {
        if(spriteName != actor.animation.sprite) {
            actor.destroy();
            actor = Actor(spriteName);
            actor.zindex = zindex;
        }

        actor.anim = anim;
        return this;
    }
}
