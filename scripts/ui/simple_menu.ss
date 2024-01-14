// -----------------------------------------------------------------------------
// File: simple_menu.ss
// Description: a simple menu system
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Input;
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Audio.Sound;

/*

This is a simple menu system that is easy to use.

 0. Spawn a Simple Menu like this:

    menu = spawn("Simple Menu");

 1. A Simple Menu has a set of options. The player can only choose a single option.
    You may initialize a Simple Menu with two options like this:

    menu = spawn("Simple Menu")
           .addOption("yes", "Oh yeah!", Vector2.zero)
           .addOption("no",  "No way!",  Vector2.down.scaledBy(16));
    //                  |          |                    |
    //                  id        text                offset
    // Each option has an ID, a display text and an offset position.

 2. Further customization can be done like this:

    menu = spawn("Simple Menu")
           .addOption("yes", "Oh yeah!", Vector2.zero)
           .addOption("no",  "No way!",  Vector2.down.scaledBy(16))
           .setPosition(Vector2(16, 16))     // position of the menu
           .setIcon("UI Pointer")            // the name of a sprite
           .setAlignment("center")           // "left" | "center" | "right"
           .setFontName("BoxyBold")          // the name of a font
           .setDefaultColor("ffffff")        // the color of the unhighlighted options (RGB hex)
           .setHighlightColor("ffff00")      // the color of the highlighted option (RGB hex)
           .setHighlightedOption("yes")      // the option that is initially highlighted
           .setHighlightSound("samples/pause_highlight.wav") // customize the sound
           .setChooseSound("samples/pause_confirm.wav");     // customize the sound

 3. Implement functions onChooseMenuOption() and, optionally,
    onHighlightMenuOption() like this:

    fun onChooseMenuOption(optionId)
    {
        if(optionId == "yes")
            Console.print("User picked Oh yeah!");
        else if(optionId == "no")
            Console.print("User picked No way!");
    }

    fun onHighlightMenuOption(optionId)
    {
        Console.print(optionId);
    }

 4. (Recommended) Make your object a detached entity. Example:

    using SurgeEngine.Vector2;

    object "My Example Menu" is "private", "detached", "entity"
    {
        menu = spawn("Simple Menu")
               .addOption("surge",    "Surge the Rabbit",  Vector2.zero)
               .addOption("gimacian", "Gimacian the Dark", Vector2.down.scaledBy(16))
               .setIcon("UI Pointer")
               .setFontName("GoodNeighbors")
               .setHighlightColor("ffff00")     // yellow
               .setPosition(Vector2(128, 128)); // position on the screen

        fun onChooseMenuOption(optionId)
        {
            Console.print(optionId);
        }
    }

    Note: after an option is chosen, the menu will no longer respond to user input.
    You may reactivate it by calling menu.reactivate(). Also, you may deactivate it
    at any time by calling menu.deactivate() or menu.chooseHighlightedOption().

*/

object "Simple Menu" is "private", "entity"
{
    transform = Transform();
    input = Input("default");
    highlight = Sound("samples/choose.wav");
    choose = Sound("samples/select.wav");
    options = [];
    indexOfHighlightedOption = 0;

    state "main"
    {
        n = options.length;

        if(n == 0)
            return;

        if(n > 1) {
            if(input.buttonPressed("down") || input.buttonPressed("right")) {
                highlight.play();
                indexOfHighlightedOption = (indexOfHighlightedOption + 1) % n;
            }

            if(input.buttonPressed("up") || input.buttonPressed("left")) {
                highlight.play();
                indexOfHighlightedOption = (indexOfHighlightedOption + (n-1)) % n;
            }
        }

        if(input.buttonPressed("fire1") || input.buttonPressed("fire3")) {
            choose.play();
            chooseHighlightedOption();
        }

        for(i = 0; i < n; i++) {
            if(i != indexOfHighlightedOption)
                options[i].setHighlighted(false);
        }
        options[indexOfHighlightedOption].setHighlighted(true);
    }

    state "deactivated"
    {
    }

    fun onChooseMenuOption(optionId)
    {
        if(parent.hasFunction("onChooseMenuOption"))
            parent.onChooseMenuOption(optionId);
    }

    fun onHighlightMenuOption(optionId)
    {
        if(parent.hasFunction("onHighlightMenuOption"))
            parent.onHighlightMenuOption(optionId);
    }

    fun reactivate()
    {
        state = "main";
        return this;
    }

    fun deactivate()
    {
        state = "deactivated";
        return this;
    }

    fun chooseHighlightedOption()
    {
        if(state == "main") {
            deactivate();
            options[indexOfHighlightedOption].choose();
        }

        return this;
    }

    fun addOption(id, text, offset)
    {
        newOption = spawn("Simple Menu - Option")
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

    fun setLocalPosition(localPosition)
    {
        transform.localPosition = localPosition;
        return this;
    }

    fun setHighlightColor(color)
    {
        foreach(option in options)
            option.setHighlightColor(color);

        return this;
    }

    fun setDefaultColor(color)
    {
        foreach(option in options)
            option.setDefaultColor(color);

        return this;
    }

    fun setFontName(fontName)
    {
        foreach(option in options)
            option.setFontName(fontName);

        return this;
    }

    fun setAlignment(alignment)
    {
        foreach(option in options)
            option.setAlignment(alignment);

        return this;
    }

    fun setIcon(spriteName)
    {
        foreach(option in options)
            option.setIcon(spriteName);

        return this;
    }

    fun setHighlightedOption(optionId)
    {
        for(i = 0; i < options.length; i++) {
            if(options[i].id === optionId) {
                indexOfHighlightedOption = i;
                return this;
            }
        }

        Console.print(this.__name + ": unknown option " + optionId);
        return this;
    }

    fun setHighlightSound(path)
    {
        highlight = Sound(path);
        return this;
    }

    fun setChooseSound(path)
    {
        choose = Sound(path);
        return this;
    }
}

object "Simple Menu - Option" is "private", "entity"
{
    public readonly id = "";
    text = "";
    align = "left";
    defaultColor = "ffffff";
    highlightColor = "ffff00";
    transform = Transform();
    label = Text("GoodNeighbors");
    icon = spawn("Simple Menu - Null Icon");
    firstRun = true;

    fun choose()
    {
        parent.onChooseMenuOption(id);
    }

    fun labelText()
    {
        if(icon.visible)
            return "<color=" + highlightColor + ">" + text + "</color>";
        else
            return "<color=" + defaultColor + ">" + text + "</color>";
    }

    fun iconOffset()
    {
        if(align == "right")
            return Vector2.left.scaledBy(label.size.x);
        else if(align == "center")
            return Vector2.left.scaledBy(label.size.x / 2);
        else
            return Vector2.zero;
    }

    fun setId(newId)
    {
        assert(typeof newId == "string" || typeof newId == "number");

        id = newId;
        return this;
    }

    fun setText(newText)
    {
        text = newText;
        label.text = labelText();
        return this;
    }

    fun setHighlightColor(color)
    {
        highlightColor = color;
        return this;
    }

    fun setDefaultColor(color)
    {
        defaultColor = color;
        return this;
    }

    fun setFontName(fontName)
    {
        label = Text(fontName);
        label.align = align;
        label.text = labelText();

        return this;
    }

    fun setAlignment(value)
    {
        assert(value == "left" || value == "center" || value == "right");

        align = value;
        label.align = align;
        icon.setOffset(iconOffset());

        return this;
    }

    fun setIcon(spriteName)
    {
        highlighted = icon.visible;

        icon = spawn("Simple Menu - Icon")
               .setGraphic(spriteName)
               .setOffset(iconOffset());

        icon.visible = highlighted;

        return this;
    }

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun setHighlighted(highlighted)
    {
        if(highlighted == icon.visible && !firstRun)
            return this;

        firstRun = false;
        icon.visible = highlighted;
        label.text = labelText();
        label.visible = true;

        if(highlighted)
            parent.onHighlightMenuOption(id);

        return this;
    }

    fun constructor()
    {
        label.align = align;
        label.text = labelText();
        label.visible = false;
    }
}

object "Simple Menu - Icon" is "private", "entity"
{
    transform = Transform();
    offset = Vector2.zero;
    actor = Actor("UI Pointer");

    fun set_visible(value)
    {
        if(actor.visible == value)
            return;

        actor.visible = value;
        actor.animation.frame = 0;
    }

    fun get_visible()
    {
        return actor.visible;
    }

    fun setGraphic(spriteName)
    {
        visible = actor.visible;
        actor = Actor(spriteName);
        actor.visible = visible;
        refresh();

        return this;
    }

    fun setOffset(v)
    {
        offset = v;
        refresh();

        return this;
    }

    fun refresh()
    {
        transform.localPosition = offset;
    }

    fun constructor()
    {
        actor.visible = false;
        refresh();
    }
}

object "Simple Menu - Null Icon" is "private", "entity"
{
    public visible = false;

    fun setGraphic(spriteName)
    {
        return this;
    }

    fun setOffset(v)
    {
        return this;
    }
}