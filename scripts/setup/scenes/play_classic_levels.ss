// -----------------------------------------------------------------------------
// File: play_classic_levels.ss
// Description: setup script of the Play Classic Levels scene
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;



// ----------------------------------------------------------------------------
// PLAY CLASSIC LEVELS SCENE: SETUP OBJECT
// ----------------------------------------------------------------------------

object "Play Classic Levels" is "setup"
{
    fader = spawn("Fader");

    text = spawn("Play Classic Levels - Text")
           .setText("$PLAYCLASSICLEVELS_TEXT")
           .setPosition(Vector2(Screen.width / 2, 128));

    optionGroup = spawn("Play Classic Levels - Option Group")
                  .add("yes", "$PLAYCLASSICLEVELS_YES", Vector2(0, 0))
                  .add("no",  "$PLAYCLASSICLEVELS_NO",  Vector2(0, 18))
                  .setPosition(Vector2(Screen.width / 2 - 12, 160));

    state "main"
    {
    }

    state "load next level"
    {
        if(timeout(fader.fadeTime))
            Level.loadNext();
    }

    state "abort quest"
    {
        if(timeout(fader.fadeTime))
            Level.abort();
    }

    fun onChooseOption(optionId)
    {
        fader.fadeOut();

        if(optionId == "yes")
            state = "load next level";
        else
            state = "abort quest";
    }

    fun constructor()
    {
        fader.fadeTime = 0.5;
        fader.fadeIn();
    }
}



// ----------------------------------------------------------------------------
// TEXTS
// ----------------------------------------------------------------------------

object "Play Classic Levels - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }

    fun setText(text)
    {
        label.text = text;
        return this;
    }

    fun constructor()
    {
        label.align = "center";
        label.maxWidth = Screen.width - 16;
    }
}



// ----------------------------------------------------------------------------
// OPTION
// ----------------------------------------------------------------------------

object "Play Classic Levels - Option Group" is "private", "detached", "entity"
{
    transform = Transform();
    input = Input("default");
    highlight = Sound("samples/choose.wav");
    confirm = Sound("samples/select.wav");
    options = [];
    indexOfActiveOption = 0;

    state "main"
    {
        n = options.length;
        if(n == 0)
            return;

        if(input.buttonPressed("down") || input.buttonPressed("right")) {
            indexOfActiveOption = (indexOfActiveOption + 1) % n;
            highlight.play();
        }

        if(input.buttonPressed("up") || input.buttonPressed("left")) {
            indexOfActiveOption = (indexOfActiveOption + (n-1)) % n;
            highlight.play();
        }

        if(input.buttonPressed("fire1") || input.buttonPressed("fire3")) {
            confirm.play();
            options[indexOfActiveOption].choose();
        }

        for(i = 0; i < n; i++) {
            if(i != indexOfActiveOption)
                options[i].setHighlighted(false);
        }
        options[indexOfActiveOption].setHighlighted(true);
    }

    fun onChooseOption(optionId)
    {
        parent.onChooseOption(optionId);
    }

    fun add(id, text, offset)
    {
        newOption = spawn("Play Classic Levels - Option")
                    .setId(id)
                    .setText(text)
                    .setOffset(offset);

        options.push(newOption);
        return this;
    }

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }
}

object "Play Classic Levels - Option" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");
    icon = spawn("Play Classic Levels - Option - Blinking Icon");
    id = "";

    fun choose()
    {
        parent.onChooseOption(id);
    }

    fun setId(newId)
    {
        id = newId;
        return this;
    }

    fun setText(text)
    {
        label.text = text;
        return this;
    }

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun setHighlighted(highlighted)
    {
        icon.visible = highlighted;
        return this;
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        label.align = "left";
        label.text = "";
    }
}

object "Play Classic Levels - Option - Blinking Icon" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");
    visible = false;
    seconds = 0.25;

    state "main"
    {
        label.visible = visible;
        if(timeout(seconds))
            state = "alt";
    }

    state "alt"
    {
        label.visible = false;
        if(timeout(seconds))
            state = "main";
    }

    state "changed"
    {
        label.visible = visible;
        state = "main";
    }

    fun set_visible(value)
    {
        if(visible != value) {
            visible = value;
            state = "changed";
        }
    }

    fun get_visible()
    {
        return visible;
    }

    fun constructor()
    {
        transform.localPosition = Vector2(-10, 0);
        label.align = "left";
        label.text = ">";
    }
}