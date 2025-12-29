// -----------------------------------------------------------------------------
// File: title_card.ss
// Description: default Title Card animation
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

//
// The Default Title Card animation is divided into 5 phases:
//
// phase 0: warming up
// phase 1: appearing
// phase 2: sustaining
// phase 3: disappearing
// phase 4: finishing
//
// These phases are played sequentially. The level name and zone number (act)
// appear during phases 1, 2 and 3. After phase 4, the title card disappears.
//
// Creating a new title card can be done simply: provide a new spritesheet and
// override the sprite script. Be mindful of the above phases.
//
// The following basic events are available:
//
// - onStart: triggered when starting phase 0
// - onFinish: triggered when finishing phase 4
//
// In addition, the following events allow more fine-grained control:
//
// - onAppear: triggered when starting phase 1
// - onSustain: triggered when starting phase 2
// - onDisappear: triggered when starting phase 3
// - onComplete: triggered when starting phase 4
//

using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Input.MobileGamepad;
using SurgeEngine.Events.Event;

object "Default Title Card" is "entity", "awake", "detached", "private"
{
    public onStart = Event();
    public onAppear = Event();
    public onSustain = Event();
    public onDisappear = Event();
    public onComplete = Event();
    public onFinish = Event();

    public readonly zindex = 1001.0;
    transform = Transform();
    actor = Actor(this.__name);
    levelName = spawn(this.__name + " - Level Name");
    zoneNumber = spawn(this.__name + " - Zone Number");
    timeBlocker = spawn(this.__name + " - Time Blocker");
    playerBlocker = spawn(this.__name + " - Player Blocker");
    layers = [];

    state "main"
    {
        MobileGamepad.fadeOut();
        setAnim(0);
        timeBlocker.setTime(Level.time);
        onStart();
        state = "warming up";
    }

    state "warming up"
    {
        if(actor.animation.finished) {
            setAnim(1);
            onAppear();
            state = "appearing";
        }
    }

    state "appearing"
    {
        if(actor.animation.finished) {
            setAnim(2);
            onSustain();
            state = "sustaining";
        }
    }

    state "sustaining"
    {
        if(actor.animation.finished) {
            setAnim(3);
            onDisappear();
            state = "disappearing";
        }
    }

    state "disappearing"
    {
        MobileGamepad.fadeIn();
        if(actor.animation.finished) {
            setAnim(4);
            onComplete();
            state = "finishing";
        }
    }

    state "finishing"
    {
        if(actor.animation.finished) {

            // we're done with this Title Card!
            onFinish();
            destroy();

        }
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = zindex;

        // note: props may not exist (compatibility mode),
        //       or they may be incorrectly configured

        // read and validate the number of layers
        numberOfLayers = Math.floor(Number(actor.animation.prop("number_of_layers")));
        if(numberOfLayers < 1)
            numberOfLayers = 1;

        // create layers
        for(n = numberOfLayers; n >= 1; n--) {
            zoffset = 1 - (n-1) / numberOfLayers;
            layer = spawn(this.__name + " - Layer")
                    .setLayerNumber(n)
                    .setZIndex(zindex + zoffset);

            layers.push(layer);
        }

        // attach the level name to a layer
        titleLayerNumber = Math.floor(Number(actor.animation.prop("level_name_layer")));
        titleLayer = getLayer(titleLayerNumber);
        levelName.setLayer(titleLayer);

        // set the font of the level name
        titleFontAlignment = String(actor.animation.prop("level_name_alignment") || "center");
        titleFontName = String(actor.animation.prop("level_name_font") || "GoodNeighborsLarge");
        levelName.setFontAlignment(titleFontAlignment);
        levelName.setFontName(titleFontName);

        // attach the zone number to a layer
        zoneLayerNumber = Math.floor(Number(actor.animation.prop("level_zone_layer")));
        zoneLayer = getLayer(zoneLayerNumber);
        zoneNumber.setLayer(zoneLayer);
    }

    fun setAnim(anim)
    {
        actor.anim = anim;
        for(i = layers.length - 1; i >= 0; i--)
            layers[i].setAnim(anim);
    }

    fun getLayer(n)
    {
        if(n >= 1 && n <= layers.length)
            return layers[layers.length - n];

        return null;
    }
}

object "Default Title Card - Layer" is "entity", "awake", "detached", "private"
{
    public readonly layerNumber = 1;
    transform = Transform();
    offset = Vector2.zero;
    actor = null;

    fun setLayerNumber(n)
    {
        assert(n >= 1 && actor === null);

        layerNumber = n;
        actor = Actor(this.__name + " " + layerNumber);

        return this;
    }

    fun setAnim(anim)
    {
        assert(actor !== null);

        actor.anim = anim;
        actor.visible = actor.animation.exists;

        transform.localPosition = layerPosition();
        offset = transform.localPosition.plus(actor.actionOffset);

        return this;
    }

    fun setZIndex(zindex)
    {
        assert(actor !== null);

        actor.zindex = zindex;

        return this;
    }



    fun layerPosition()
    {
        pos = actor.animation.prop("layer_position");

        // undefined position?
        if(typeof pos !== "object")
            return Vector2.zero;

        // read position
        xpos = Number(pos[0]);
        ypos = Number(pos[1]);
        return Vector2(xpos, ypos);
    }

    fun apparentPosition()
    {
        animationTransform = actor.animation.findTransform();
        return animationTransform.localPosition.plus(offset);
    }
}

object "Default Title Card - Level Name" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    label = null;
    layer = null;
    align = "center";

    state "main"
    {
        if(layer !== null)
            transform.localPosition = layer.apparentPosition();
    }

    fun setLayer(l)
    {
        layer = l;
        return this;
    }

    fun setFontName(fontName)
    {
        label = Text(fontName);

        label.text = Level.name;
        label.align = align;
        label.zindex = parent.zindex + 1.1;

        return this;
    }

    fun setFontAlignment(alignment)
    {
        align = alignment;

        if(label !== null)
            label.align = align;

        return this;
    }
}

object "Default Title Card - Zone Number" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor(this.__name);
    layer = null;

    state "main"
    {
        if(layer !== null)
            transform.localPosition = layer.apparentPosition();
    }

    fun setLayer(l)
    {
        layer = l;
        return this;
    }

    fun constructor()
    {
        actor.anim = Level.act;
        actor.zindex = parent.zindex + 1.2;

        if(!actor.animation.exists)
            actor.visible = false;
    }
}

// Do not advance time until we are finished with the Title Card animation
object "Default Title Card - Time Blocker"
{
    time = 0.0;

    state "main"
    {
        Level.time = time;
    }

    fun setTime(t)
    {
        time = t;
        return this;
    }
}

// Block player control until we are finished with the Title Card animation
object "Default Title Card - Player Blocker"
{
    player = Player.active;

    fun constructor()
    {
        player.input.enabled = false;
    }

    fun destructor()
    {
        player.input.enabled = true;
    }
}
