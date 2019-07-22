// -----------------------------------------------------------------------------
// File: bridge.ss
// Description: bridge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Collisions.CollisionBox;

// Bridge Controller
object "Bridge" is "entity", "gimmick"
{
    public length = 12;
    public anim = 1;

    bridgeCollider = CollisionBox(1, 1);
    elements = [];
    leftCorner = null;
    rightCorner = null;

    state "main"
    {
        setup();
        state = "idle";
    }

    state "idle"
    {
        // compute the maximum depth of all bridge elements
        // according to the position of the players
        maxDepth = 0; deepestElementIndex = -1;
        for(i = 0; i < Player.count; i++) { // for each player
            player = Player[i];
            if(bridgeCollider.collidesWith(player.collider)) { // player on the bridge
                feet = Vector2(player.collider.center.x, player.collider.bottom);
                for(j = 0; j < elements.length; j++) {
                    if(elements[j].gotCollision(feet)) { // player on bridge element j
                        depth = elements[j].depth;
                        if(depth > maxDepth) {
                            maxDepth = depth;
                            deepestElementIndex = j;
                        }
                        break;
                    }
                }
            }
        }

        // position the bridge elements
        for(j = 0; j < elements.length; j++)
            elements[j].updatePosition(deepestElementIndex, maxDepth);
    }

    fun setup()
    {
        // need to reset?
        if(elements.length > 0)
            clear();

        // create the bridge elements
        length = Math.clamp(length, 6, 24);
        for(i = 0; i < length; i++) {
            e = spawn("Bridge Element").setElement(i, length, anim);
            elements.push(e);
        }

        // create the corners
        leftCorner = spawn("Bridge Corner").setCorner(-1, anim);
        rightCorner = spawn("Bridge Corner").setCorner(length, anim);

        // setup the bridge collider
        bridgeCollider.width = elements[0].width * length;
        bridgeCollider.height = elements[0].height;
        bridgeCollider.setAnchor(0, 0.5);
    }

    fun clear()
    {
        // clear bridge
        if(leftCorner != null) {
            leftCorner.destroy();
            leftCorner = null;
        }

        if(rightCorner != null) {
            rightCorner.destroy();
            rightCorner = null;
        }

        for(i = 0; i < elements.length; i++) {
            elements[i].destroy();
            elements[i] = null;
        }
        elements = [];
    }
}

// Bridge Element
object "Bridge Element" is "entity", "private"
{
    public readonly depth = 0;
    public readonly index = 0;

    transform = Transform();
    actor = Actor("Bridge Element");
    brick = Brick("Bridge Element");
    collider = CollisionBox(actor.width, actor.height).setAnchor(0, 0.5);
    length = 1;
    offx = 0;
    offy = 0;
    timer = 0;
    ninety = Math.deg2rad(90);
    accomodationTime = 0.3;

    state "main"
    {
        //collider.visible = true;
    }

    fun setElement(idx, len, anim)
    {
        // set the animation
        actor.anim = anim;

        // set the element index
        index = idx;

        // store the length of the bridge
        length = len;

        // set the position of the element
        offx = index * actor.width;
        transform.localPosition = Vector2(offx, 0);

        // compute its depth
        d1 = index + 1;
        d2 = length - index;
        depth = Math.min(d1, d2) * 2;

        // make this call chainable
        return this;
    }

    fun updatePosition(deepestElementIndex, maxDepth)
    {
        // if the player is on the bridge...
        if(deepestElementIndex >= 0) {
            // compute the depth factor of this element
            f1 = (index + 1) / (deepestElementIndex + 1);
            f2 = (length - index) / (length - deepestElementIndex);
            factor = Math.min(f1, f2);

            // update timer
            timer += Time.delta / accomodationTime;
            if(timer > 1)
                timer = 1;

            // update graphics
            targetFrame = Math.floor((timer * factor * factor) * (actor.animation.frameCount - 1));
            actor.animation.frame = targetFrame;

            // update position
            offy = maxDepth * Math.sin(ninety * factor);
            y = Math.smoothstep(0, offy, timer);
            dy = y - transform.localPosition.y;
            transform.localPosition = Vector2(offx, y);

            // move player
            if(dy > 0.7) {
                player = Player.active;
                if(collider.collidesWith(player.collider))
                    player.transform.move(0, dy - 0.7);
            }
        }
        else if(timer > 0) {
            // update timer
            timer -= Time.delta / accomodationTime;
            if(timer < 0)
                timer = 0;

            // update graphics
            targetFrame = Math.floor((timer * timer) * (actor.animation.frameCount - 1));
            actor.animation.frame = Math.min(targetFrame, actor.animation.frame);

            // update position
            y = Math.smoothstep(0, offy, timer);
            dy = y - transform.localPosition.y;
            transform.localPosition = Vector2(offx, y);
        }
        else
            actor.animation.frame = 0;
    }

    fun gotCollision(position)
    {
        return collider.contains(position);
    }

    fun get_width()
    {
        return actor.width;
    }

    fun get_height()
    {
        return actor.height;
    }

    fun constructor()
    {
        brick.type = "cloud";
    }
}

// Bridge Corner
object "Bridge Corner" is "entity", "private"
{
    actor = null;

    state "main"
    {
    }

    fun setCorner(index, anim)
    {
        // configure the actor
        actor = Actor((index > 0) ? "Bridge Corner Right" : "Bridge Corner Left");
        actor.anim = anim;
        actor.offset = Vector2(index * actor.width, 0);

        // return the object
        return this;
    }
}