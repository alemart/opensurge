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
// phase 4: finishing up
//
// These phases are played sequentially. The level name and zone number (act)
// appear during phases 1, 2 and 3. After phase 4, the title card disappears.
//
// Creating a new title card can be done simply: provide a new spritesheet and
// override the sprite script. Be mindful of the above phases.
//

using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Input.MobileGamepad;

object "Default Title Card" is "entity", "awake", "detached", "private"
{
    public readonly zindex = 1001.0;
    transform = Transform();
    actor = Actor("Default Title Card");
    levelName = spawn("Default Title Card - Level Name");
    zoneNumber = spawn("Default Title Card - Zone Number");
    timeBlocker = spawn("Default Title Card - Time Blocker");
    playerBlocker = spawn("Default Title Card - Player Blocker");
    layers = [];

    state "main"
    {
        MobileGamepad.fadeOut();
        setAnim(0);
        state = "warming up";
    }

    state "warming up"
    {
        if(actor.animation.finished) {
            setAnim(1);
            state = "appearing";
        }
    }

    state "appearing"
    {
        if(actor.animation.finished) {
            setAnim(2);
            state = "sustaining";
        }
    }

    state "sustaining"
    {
        if(actor.animation.finished) {
            setAnim(3);
            state = "disappearing";
        }
    }

    state "disappearing"
    {
        MobileGamepad.fadeIn();
        if(actor.animation.finished) {
            setAnim(4);
            state = "finishing up";
        }
    }

    state "finishing up"
    {
        if(actor.animation.finished) {

            // we're done with this Title Card!
            destroy();

        }
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = zindex;

        // create layers
        numberOfLayers = Number(actor.animation.prop("number_of_layers"));
        for(n = numberOfLayers; n >= 1; n--) {
            zoffset = 1 - (n-1) / numberOfLayers;
            layer = spawn("Default Title Card - Layer")
                    .setLayerNumber(n)
                    .setZIndex(zindex + zoffset);

            layers.push(layer);
        }

        // attach the level name to a layer
        titleLayerNumber = Number(actor.animation.prop("level_name_layer"));
        titleLayer = getLayer(titleLayerNumber);
        levelName.setLayer(titleLayer);

        // set the font of the level name
        titleLayerFont = String(actor.animation.prop("level_name_font"));
        levelName.setFontName(titleLayerFont);

        // attach the zone number to a layer
        zoneLayerNumber = Number(actor.animation.prop("level_zone_layer"));
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
    actor = null;

    fun setLayerNumber(n)
    {
        assert(n >= 1);

        layerNumber = n;
        actor = Actor("Default Title Card - Layer " + layerNumber);

        return this;
    }

    fun setAnim(anim)
    {
        assert(actor !== null);

        actor.anim = anim;
        actor.visible = actor.animation.exists;

        return this;
    }

    fun setZIndex(zindex)
    {
        assert(actor !== null)

        actor.zindex = zindex;

        return this;
    }

    fun findPosition()
    {
        interpolatedTransform = actor.animation.findInterpolatedTransform();
        return interpolatedTransform.localPosition.plus(actor.actionOffset);
    }

}

object "Default Title Card - Level Name" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    label = null;
    layer = null;

    state "main"
    {
        if(layer !== null)
            transform.localPosition = layer.findPosition();
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
        label.align = "center";
        label.zindex = parent.zindex + 1.1;

        return this;
    }
}

object "Default Title Card - Zone Number" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("Default Title Card - Zone Number");
    layer = null;

    state "main"
    {
        if(layer !== null)
            transform.localPosition = layer.findPosition();
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
    state "main"
    {
        Level.time = 0.0;
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