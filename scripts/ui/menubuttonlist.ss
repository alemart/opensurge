// -----------------------------------------------------------------------------
// File: menubuttonlist.ss
// Description: a beautiful animated menu that can be customized
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;

// To create your own Menu,
// see the MenuBuilder object
object "MenuButtonList"
{
    buttons = [];
    currentButtonIndex = 0;
    oldButtonIndex = 0;
    totalMoveTime = 0.3; // transition time, in seconds
    currentMoveTime = 0.0;
    buttonSpacing = Vector2(0, 64);
    basePosition = Vector2(Screen.width / 2, Screen.height / 2);
    transform = Transform();
    slide = Sound("samples/slide.wav");
    title = spawn("MenuTitle");

    state "main"
    {
    }

    state "moving"
    {
        if(currentMoveTime < totalMoveTime) {
            currentMoveTime += Time.delta;
            x = Math.smoothstep(buttonSpacing.x * oldButtonIndex, buttonSpacing.x * currentButtonIndex, currentMoveTime / totalMoveTime);
            y = Math.smoothstep(buttonSpacing.y * oldButtonIndex, buttonSpacing.y * currentButtonIndex, currentMoveTime / totalMoveTime);
            transform.localPosition = Vector2(-x, -y).plus(basePosition);
        }
        else {
            currentMoveTime = 0.0;
            transform.localPosition = buttonSpacing.scaledBy(-currentButtonIndex).plus(basePosition);
            oldButtonIndex = currentButtonIndex;
            state = "main";
        }
    }

    state "pressing"
    {
        if(buttons[currentButtonIndex].pressed) {
            if(parent.hasFunction("onMenuSelected"))
                parent.onMenuSelected(currentButtonIndex);
            state = "disappearing";
        }
    }

    state "disappearing"
    {
        transform.translateBy(2.5 * Screen.width * Time.delta, 0);
    }

    //
    // select()
    // Confirms the current selection (presses the button)
    //
    fun select()
    {
        if(state == "main") {
            buttons[currentButtonIndex].press();
            state = "pressing";
        }
    }

    //
    // moveNext()
    // Slides the menu to the next option
    //
    fun moveNext()
    {
        if(state == "main") {
            if(currentButtonIndex < buttons.length - 1) {
                // update indexes
                oldButtonIndex = currentButtonIndex;
                currentButtonIndex++;

                // focus on the proper button
                if(buttons.length > 0) {
                    for(j = 0; j < buttons.length; j++)
                        buttons[j].blur();
                    buttons[currentButtonIndex].focus();
                }

                // move
                slide.play();
                state = "moving";
            }
        }
    }

    //
    // movePrevious()
    // Slides the menu to the previous option
    //
    fun movePrevious()
    {
        if(state == "main") {
            if(currentButtonIndex > 0) {
                // update indexes
                oldButtonIndex = currentButtonIndex;
                currentButtonIndex--;

                // focus on the proper button
                if(buttons.length > 0) {
                    for(j = 0; j < buttons.length; j++)
                        buttons[j].blur();
                    buttons[currentButtonIndex].focus();
                }

                // move
                slide.play();
                state = "moving";
            }
        }
    }

    //
    // setButtons()
    // Set the buttons of the MenuList
    //
    fun setButtons(buttonArray)
    {
        // clear previous buttons
        for(j = 0; j < buttons.length; j++)
            buttons[j].destroy();
        buttons.clear();

        // set new buttons
        for(j = 0; j < buttonArray.length; j++)
            buttons.push(spawnButton(buttonArray[j]));
    }

    //
    // button()
    // Get an individual button, given its index
    //
    fun button(index)
    {
        if(index >= 0 && index < buttons.length)
            return buttons[index];
        else
            return null;
    }

    //
    // buttonCount, read-only
    // The number of buttons
    //
    fun get_buttonCount()
    {
        return buttons.length;
    }

    //
    // title
    // Optional title property (string)
    //
    fun set_title(text)
    {
        title.text = text;
        title.visible = !String.isNullOrEmpty(text);
    }

    fun get_title()
    {
        return title.text;
    }

    //
    // selectedIndex
    // Current button index (an integer between 0 and buttons.length - 1, inclusive)
    //
    fun get_selectedIndex()
    {
        return currentButtonIndex;
    }

    fun set_selectedIndex(index)
    {
        index = Math.clamp(index, 0, buttons.length - 1);
        if(currentButtonIndex != index) {
            currentButtonIndex = index;
            state = "moving";
        }
    }

    //
    // offset
    // The offset of the menu in relation to the parent object
    // This is a Vector2
    //
    fun get_offset()
    {
        return basePosition;
    }

    fun set_offset(offset)
    {
        if(offset.__name == "Vector2")
            basePosition = offset.scaledBy(1);
    }

    //
    // axisAngle
    // The angle of the main axis, in degrees
    // e.g., 0 for a horizontal menu; 90 for a vertical menu
    //
    fun get_axisAngle()
    {
        return buttonSpacing.angle;
    }

    fun set_axisAngle(angle)
    {
        buttonSpacing = Vector2.right.scaledBy(buttonSpacing.length).rotatedBy(angle);
    }

    //
    // spacing
    // The distance between two menu buttons, in pixels
    //
    fun get_spacing()
    {
        return buttonSpacing.length;
    }

    fun set_spacing(spacing)
    {
        buttonSpacing = Vector2.right.scaledBy(spacing).rotatedBy(buttonSpacing.angle);
    }

    // ---------------------------------------------
    // internal stuff
    // ---------------------------------------------

    fun spawnButton(label)
    {
        btn = spawn("MenuButton");
        btn.text = label;
        return btn;
    }

    fun init()
    {
        title.transform.localPosition = buttonSpacing.scaledBy(-1);//-1.15
        title.visible = !String.isNullOrEmpty(title.text);
        if(buttons.length > 0) {
            buttons[currentButtonIndex].focus();
            for(j = 0; j < buttons.length; j++)
                buttons[j].transform.localPosition = buttonSpacing.scaledBy(j);
        }
        state = "moving";
    }
}