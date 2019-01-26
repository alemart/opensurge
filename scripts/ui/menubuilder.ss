// -----------------------------------------------------------------------------
// File: menubuilder.ss
// Description: utility object for building a menu
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Vector2;

object "MenuBuilder"
{
    menu = parent.spawn("MenuButtonList");

    // withButtons()
    // give an array of strings, one for each menu button
    fun withButtons(buttonArray)
    {
        menu.setButtons(buttonArray);
        return this;
    }

    // withTitle()
    // set an optional title
    fun withTitle(title)
    {
        menu.title = title;
        return this;
    }

    // withAxisAngle()
    // set the angle of the main axis of the menu
    fun withAxisAngle(degrees)
    {
        menu.axisAngle = degrees;
        return this;
    }

    // withSpacing()
    // set a spacing between menu buttons, in pixels
    fun withSpacing(spacing)
    {
        menu.spacing = spacing;
        return this;
    }

    // at()
    // set the position of the menu
    fun at(x, y)
    {
        menu.offset = Vector2(x, y);
        return this;
    }

    // build()
    // return the built menu
    fun build()
    {
        menu.init();
        return menu;
    }
}
