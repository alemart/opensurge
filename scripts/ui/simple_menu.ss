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

 0. Build a Simple Menu like this:

    menu = spawn("Simple Menu Builder")
           .build();

 1. A Simple Menu has a set of options. The player can only choose a single option.
    You may initialize a Simple Menu with two options like this:

    menu = spawn("Simple Menu Builder")
           .addOption("yes", "Oh yeah!", Vector2.zero)
           .addOption("no",  "No way!",  Vector2.down.scaledBy(16))
           .build();
    //                  |          |                    |
    //                  |          |                    |
    //                  id        text                offset
    // Each option has an ID, a display text and an offset position.

 2. Further customization can be done like this:

    menu = spawn("Simple Menu Builder")
           .addOption("yes", "Oh yeah!", Vector2.zero)
           .addOption("no",  "No way!",  Vector2.down.scaledBy(16))
           .setPosition(Vector2(16, 16))     // position of the menu
           .setIcon("UI Pointer")            // the name of a sprite
           .setIcon({
               "yes": "UI Pointer",          // you can also set different
               "no": "Another Pointer"       // icons for different options
           })
           .setBlinking(true)                // make the highlighted option blink
           .setFontName("BoxyBold")          // the name of a font
           .setAlignment("center")           // "left" | "center" | "right"
           .setDefaultColor("ffffff")        // the color of the unhighlighted options (RGB hex)
           .setHighlightColor("ffff00")      // the color of the highlighted option (RGB hex)
           .setHighlightedOption("yes")      // the option that is initially highlighted
           .setHighlightSound("samples/pause_highlight.wav") // customize the sound
           .setChooseSound("samples/pause_confirm.wav")      // customize the sound
           .build();

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
    using SurgeEngine.Video.Screen;

    object "My Example Menu" is "private", "detached", "entity"
    {
        menu = spawn("Simple Menu Builder")
               .addOption("start",   "Start Game",  Vector2.zero)
               .addOption("battle",  "Battle Mode", Vector2.down.scaledBy(16 * 1))
               .addOption("options", "Options",     Vector2.down.scaledBy(16 * 2))
               .addOption("exit",    "Exit",        Vector2.down.scaledBy(16 * 3))
               .setIcon("UI Pointer")
               .setHighlightColor("ffee11")
               .setFontName("GoodNeighbors")
               .setAlignment("center")
               .setPosition(Vector2(Screen.width / 2, 64))
               .build();

        fun onChooseMenuOption(optionId)
        {
            Console.print(optionId);
        }
    }

    Note: after an option is chosen, the menu will no longer respond to user input.
    You may reactivate it by calling menu.reactivate(). Also, you may deactivate it
    at any time by calling menu.deactivate() or menu.chooseHighlightedOption().

*/

object "Simple Menu Builder"
{
    // default settings
    options = [];
    position = Vector2.zero;
    icon = null;
    highlightedOptionId = null;
    defaultColor = "ffffff";
    highlightColor = "ffff00";
    fontName = "GoodNeighbors";
    alignment = "left";
    blinking = false;
    highlightSoundPath = "samples/choose.wav";
    chooseSoundPath = "samples/select.wav";

    fun addOption(id, text, offset)
    {
        assert(typeof id == "string" || typeof id == "number");
        assert(typeof text == "string");
        assert(typeof offset == "object" && offset.__name == "Vector2");

        options.push({
            "id": id,
            "text": text,
            "offset": offset
        });
        return this;
    }

    fun setPosition(pos)
    {
        assert(typeof pos == "object" && pos.__name == "Vector2");

        position = pos;
        return this;
    }

    fun setDefaultColor(color)
    {
        defaultColor = color;
        return this;
    }

    fun setHighlightColor(color)
    {
        highlightColor = color;
        return this;
    }

    fun setHighlightedOptionId(optionId)
    {
        highlightedOptionId = optionId;
        return this;
    }

    fun setFontName(name)
    {
        fontName = name;
        return this;
    }

    fun setAlignment(align)
    {
        assert(align == "left" || align == "center" || align == "right");

        alignment = align;
        return this;
    }

    fun setIcon(spriteName)
    {
        icon = spriteName;
        return this;
    }

    fun setBlinking(value)
    {
        blinking = value;
        return this;
    }

    fun setHighlightSound(path)
    {
        highlightSoundPath = path;
        return this;
    }

    fun setChooseSound(path)
    {
        chooseSoundPath = path;
        return this;
    }

    fun build()
    {
        menu = parent.spawn("Simple Menu");

        foreach(option in options)
            menu._addOption(option["id"], option["text"], option["offset"]);

        menu._setFont(fontName, alignment, highlightColor, defaultColor);
        menu._setPosition(position);
        menu._setBlinking(blinking);

        menu._setHighlightSound(highlightSoundPath);
        menu._setChooseSound(chooseSoundPath);

        if(icon !== null)
            menu._setIcon(icon);

        if(highlightedOptionId !== null) {
            if(!menu._setHighlightedOption(highlightedOptionId))
                Console.print(this.__name + ": unknown option " + highlightedOptionId);
        }

        menu._updateDirection();
        return menu;
    }
}

object "Simple Menu" is "private", "entity"
{
    transform = Transform();
    input = Input("default");
    highlight = Sound("samples/choose.wav");
    choose = Sound("samples/select.wav");
    options = [];
    indexOfHighlightedOption = 0;
    nextButton = "down";
    previousButton = "up";

    state "main"
    {
        n = options.length;

        if(n == 0)
            return;

        if(n > 1) {
            if(input.buttonPressed(nextButton)) {
                highlight.play();
                indexOfHighlightedOption = (indexOfHighlightedOption + 1) % n;
            }

            if(input.buttonPressed(previousButton)) {
                highlight.play();
                indexOfHighlightedOption = (indexOfHighlightedOption + (n-1)) % n;
            }
        }

        if(input.buttonPressed("fire1") || input.buttonPressed("fire3")) {
            choose.play();
            chooseHighlightedOption();
        }

        for(i = 0; i < n; i++)
            options[i].setHighlighted(i == indexOfHighlightedOption);
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
    }

    fun deactivate()
    {
        state = "deactivated";
    }

    fun chooseHighlightedOption()
    {
        if(state == "main" && options.length > 0) {
            deactivate();
            options[indexOfHighlightedOption].choose();
        }
    }

    fun highlight(optionId)
    {
        if(!_setHighlightedOption(optionId)) {
            Console.print(this.__name + ": unknown option " + optionId);
            return false;
        }

        return true;
    }

    fun _addOption(id, text, offset)
    {
        newOption = spawn("Simple Menu - Option")
                    .setId(id)
                    .setText(text)
                    .setOffset(offset);

        options.push(newOption);
    }

    fun _setPosition(position)
    {
        transform.position = position;
    }

    fun _setFont(fontName, alignment, highlightColor, defaultColor)
    {
        foreach(option in options)
            option.setFont(fontName, alignment, highlightColor, defaultColor);
    }

    fun _setHighlightedOption(optionId)
    {
        i = _findOptionIndex(optionId);
        if(i < 0)
            return false;

        indexOfHighlightedOption = i;
        return true;
    }

    fun _setIcon(spriteNameOrDictionary)
    {
        assert(
            typeof spriteNameOrDictionary == "string" ||
            (typeof spriteNameOrDictionary == "object" && spriteNameOrDictionary.__name == "Dictionary")
        );

        if(typeof spriteNameOrDictionary == "string") {

            spriteName = spriteNameOrDictionary;
            foreach(option in options)
                option.setIcon(spriteName);

        }
        else {

            dictionary = spriteNameOrDictionary;
            foreach(entry in dictionary) {
                optionId = entry.key;
                spriteName = entry.value;

                j = _findOptionIndex(optionId);
                if(j >= 0)
                    options[j].setIcon(spriteName);
            }

        }
    }

    fun _setBlinking(blinking)
    {
        foreach(option in options)
            option.setBlinking(blinking);
    }

    fun _setHighlightSound(path)
    {
        highlight = Sound(path);
    }

    fun _setChooseSound(path)
    {
        choose = Sound(path);
    }

    fun _updateDirection()
    {
        n = options.length;
        if(n < 2)
            return;

        p = options[0].offset;
        q = options[n-1].offset;

        dx = q.x - p.x;
        dy = q.y - p.y;

        if(Math.abs(dy) > Math.abs(dx)) {
            if(dy > 0)
                nextButton = "down";
            else
                nextButton = "up";
        }
        else {
            if(dx > 0)
                nextButton = "right";
            else
                nextButton = "left";
        }

        oppositeDirection = {
            "up": "down",
            "left": "right",
            "down": "up",
            "right": "left"
        };

        previousButton = oppositeDirection[nextButton];
    }

    fun _findOptionIndex(optionId)
    {
         for(i = 0; i < options.length; i++) {
            if(options[i].id === optionId)
                return i;
        }

        return -1;
    }
}

object "Simple Menu - Option" is "private", "entity"
{
    public readonly id = "";
    text = "";
    defaultColor = "ffffff";
    highlightColor = "ffff00";
    transform = Transform();
    label = null;
    icon = spawn("Simple Menu - Null Icon");
    isHighlighted = false;
    wantBlinking = false;

    state "main"
    {
        assert(label !== null);

        if(wantBlinking && isHighlighted) {
            // blink at a rate of 2 cycles per second
            label.visible = (Math.floor(4 * Time.time) % 2 == 0);
        }
        else
            label.visible = true;
    }

    fun choose()
    {
        parent.onChooseMenuOption(id);
    }

    fun setId(newId)
    {
        id = newId;
        return this;
    }

    fun setText(newText)
    {
        assert(label === null);

        text = newText;
        return this;
    }

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun get_offset()
    {
        return transform.localPosition;
    }

    fun setFont(fontName, alignment, highColor, defColor)
    {
        highlightColor = highColor;
        defaultColor = defColor;

        label = Text(fontName);
        label.align = alignment;
        label.text = labelText();

        icon.setOffset(iconOffset(label.align, label.size.x));
        return this;
    }

    fun setIcon(spriteName)
    {
        assert(label !== null);

        icon = spawn("Simple Menu - Icon")
               .setSprite(spriteName)
               .setOffset(iconOffset(label.align, label.size.x));

        return this;
    }

    fun setBlinking(blinking)
    {
        wantBlinking = blinking;
        return this;
    }

    fun setHighlighted(highlighted)
    {
        if(highlighted == isHighlighted)
            return this;

        isHighlighted = highlighted;
        label.text = labelText();
        icon.visible = highlighted;

        if(highlighted)
            parent.onHighlightMenuOption(id);

        return this;
    }

    fun labelText()
    {
        if(isHighlighted)
            return "<color=" + highlightColor + ">" + text + "</color>";
        else
            return "<color=" + defaultColor + ">" + text + "</color>";
    }

    fun iconOffset(align, width)
    {
        if(align == "right")
            return Vector2.left.scaledBy(width);
        else if(align == "center")
            return Vector2.left.scaledBy(width / 2);
        else
            return Vector2.zero;
    }
}

object "Simple Menu - Icon" is "private", "entity"
{
    public visible = false;
    transform = Transform();
    actor = Actor("UI Pointer");

    state "main"
    {
        actor.visible = visible;
        actor.animation.sync = true;
    }

    fun setSprite(spriteName)
    {
        actor = Actor(spriteName);
        return this;
    }

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun constructor()
    {
        actor.visible = false;
    }
}

object "Simple Menu - Null Icon" is "private", "entity"
{
    public visible = false;

    fun setSprite(spriteName)
    {
        return this;
    }

    fun setOffset(offset)
    {
        return this;
    }
}