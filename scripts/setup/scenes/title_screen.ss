// -----------------------------------------------------------------------------
// File: title_screen.ss
// Description: title screen script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Audio.Music;
using SurgeEngine.Camera;
using SurgeEngine.Behaviors.DirectionalMovement;

// ----------------------------------------------------------------------------
// TITLE SCREEN: SETUP OBJECT
// ----------------------------------------------------------------------------

object "Title Screen" is "setup"
{
    fader = spawn("Fader");
    logo = spawn("Title Screen - Logo");
    version = spawn("Title Screen - Version");
    website = spawn("Title Screen - Website");
    music = spawn("Title Screen - Music");
    menu = spawn("Title Screen - Menu").setFader(fader);
    scroller = spawn("Title Screen - Scroller");

    fun constructor()
    {
        fader.fadeTime = 0.5;
        fader.fadeIn();
    }
}



// ----------------------------------------------------------------------------
// GAME LOGO
// ----------------------------------------------------------------------------

object "Title Screen - Logo" is "private", "detached", "entity"
{
    transform = Transform();
    actor = Actor("Title Screen - Logo");

    fun constructor()
    {
        // the hot spot of the logo will be at the center of the screen
        transform.position = Vector2(Screen.width / 2, Screen.height / 2);
    }
}



// ----------------------------------------------------------------------------
// MUSIC
// ----------------------------------------------------------------------------

object "Title Screen - Music"
{
    jingle = Music("musics/title.ogg");

    fun constructor()
    {
        // play once
        jingle.play();
    }
}



// ----------------------------------------------------------------------------
// CAMERA SCROLLER
// ----------------------------------------------------------------------------
object "Title Screen - Scroller" is "private", "entity", "awake"
{
    transform = Transform();
    movement = DirectionalMovement();
    scrollSpeed = 800; // in pixels per second
    wrapThreshold = 1000000;

    state "main"
    {
        // wrap around
        if(transform.position.x >= wrapThreshold)
            transform.position = Level.spawnpoint;

        // update the position of the camera
        Camera.position = transform.position;
    }

    fun constructor()
    {
        // set the initial position of this entity
        transform.position = Level.spawnpoint;

        // setup the directional movement
        movement.speed = scrollSpeed;
        movement.direction = Vector2.right;
    }
}



// ----------------------------------------------------------------------------
// VERSION TEXT
// ----------------------------------------------------------------------------

object "Title Screen - Version" is "private", "detached", "entity"
{
    transform = Transform();
    text = spawn("Title Screen - Version Text");
    padding = 8;

    fun constructor()
    {
        transform.position = Vector2(Screen.width - padding, padding);
    }
}

object "Title Screen - Version Text" is "private", "detached", "entity"
{
    label = Text("Title Screen - Tiny Text");

    fun constructor()
    {
        label.text = "$TITLESCREEN_VERSION";
        label.align = "right";
    }
}



// ----------------------------------------------------------------------------
// WEBSITE TEXT
// ----------------------------------------------------------------------------

object "Title Screen - Website" is "private", "detached", "entity"
{
    transform = Transform();
    text = spawn("Title Screen - Website Text");
    padding = 8;

    fun constructor()
    {
        transform.position = Vector2(padding, padding);
    }
}

object "Title Screen - Website Text" is "private", "detached", "entity"
{
    label = Text("Title Screen - Tiny Text");

    fun constructor()
    {
        label.text = "$TITLESCREEN_WEBSITE";
        label.align = "left";
    }
}


// ----------------------------------------------------------------------------
// MENU: BASIC LOGIC
// ----------------------------------------------------------------------------

object "Title Screen - Menu" is "private", "detached", "entity"
{
    transform = Transform();
    itemGroup = spawn("Title Screen - Menu Item Group");
    fader = null;
    selectedMenuItem = null;

    state "main"
    {
    }

    state "fading"
    {
        if(timeout(fader.fadeTime + 0.5)) {
            selectedMenuItem.onEnter();
            state = "done";
        }
    }

    state "done"
    {
    }

    fun onSelect(menuItem)
    {
        //Console.print(menuItem.__name);
        selectedMenuItem = menuItem;
        fader.fadeOut();
        state = "fading";
    }

    fun setFader(obj)
    {
        fader = obj;
        return this;
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, Screen.height - 20);
    }
}

object "Title Screen - Menu Item Group" is "private", "detached", "entity"
{
    transform = Transform();
    slide = Sound("samples/slide.wav");
    select = Sound("samples/select.wav");
    spacing = 128; // spacing between menu items
    selectedItem = 0;
    items = [
        spawn("Title Screen - Menu Item - Start Game").setOffset(Vector2.right.scaledBy(0 * spacing)),
        spawn("Title Screen - Menu Item - Options").setOffset(Vector2.right.scaledBy(1 * spacing)),
        spawn("Title Screen - Menu Item - Quit").setOffset(Vector2.right.scaledBy(2 * spacing)),
    ];
    swipeTime = 0.5; // in seconds
    timer = 1.0;

    state "main"
    {
        // item select
        input = Player.active.input;
        if(input.buttonPressed("right"))
            selectItem(selectedItem + 1);
        else if(input.buttonPressed("left"))
            selectItem(selectedItem - 1);

        // swipe logic
        timer += Time.delta / swipeTime;
        x = Math.smoothstep(transform.localPosition.x, selectedItem * -spacing, timer);
        transform.localPosition = Vector2(Math.round(x), 0);
    }

    fun selectItem(itemNumber)
    {
        if(itemNumber < 0 || itemNumber >= items.length)
            return;

        items[selectedItem].setHighlighted(false);
        items[itemNumber].setHighlighted(true);

        timer = 0.0;
        selectedItem = itemNumber;
        slide.play();
    }

    fun onSelect(menuItem)
    {
        Player.active.input.enabled = false;
        select.play();
        parent.onSelect(menuItem);
    }
}


// ----------------------------------------------------------------------------
// MENU ITEMS
// ----------------------------------------------------------------------------

object "Title Screen - Menu Item - Start Game" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_PLAY")
               .setHighlighted(true);

    fun onEnter()
    {
        Level.load("quests/default.qst");
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun setHighlighted(highlighted)
    {
        delegate.setHighlighted(highlighted);
        return this;
    }

    fun setOffset(offset)
    {
        delegate.setOffset(offset);
        return this;
    }
}

object "Title Screen - Menu Item - Options" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_OPTIONS");

    fun onEnter()
    {
        Level.load("quests/options.qst");
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun setHighlighted(highlighted)
    {
        delegate.setHighlighted(highlighted);
        return this;
    }

    fun setOffset(offset)
    {
        delegate.setOffset(offset);
        return this;
    }
}

object "Title Screen - Menu Item - Quit" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_QUIT");

    fun onEnter()
    {
        Level.abort();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun setHighlighted(highlighted)
    {
        delegate.setHighlighted(highlighted);
        return this;
    }

    fun setOffset(offset)
    {
        delegate.setOffset(offset);
        return this;
    }
}

object "Title Screen - Menu Item" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("Title Screen - Menu Item");
    text = "";
    highlighted = false;

    state "main"
    {
        if(!highlighted)
            return;

        input = Player.active.input;
        if(input.buttonPressed("fire1") || input.buttonPressed("fire3"))
            parent.onSelect();
    }

    fun setText(txt)
    {
        text = txt;
        label.text = highlighted ? "<color=$COLOR_HIGHLIGHT>" + text + "</color>" : text;
        return this;
    }

    fun setHighlighted(h)
    {
        highlighted = h;
        label.text = highlighted ? "<color=$COLOR_HIGHLIGHT>" + text + "</color>" : text;
        return this;
    }

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun constructor()
    {
        label.align = "center";
    }
}