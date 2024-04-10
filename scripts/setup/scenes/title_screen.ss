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
using SurgeEngine.Input.MobileGamepad;
using SurgeTheRabbit;

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

        MobileGamepad.fadeIn();
        SurgeTheRabbit.Settings.wantNeonAsPlayer2 = false;
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

    fun destructor()
    {
        if(jingle.playing)
            jingle.stop();
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
    }

    fun lateUpdate()
    {
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
        transform.position = Vector2(Screen.width / 2, Screen.height - 25);
    }
}

object "Title Screen - Menu Item Group" is "private", "detached", "entity"
{
    transform = Transform();
    slide = Sound("samples/slide.wav");
    select = Sound("samples/select.wav");
    spacing = 80; // spacing between items
    selectedItem = 0;
    items = [];
    offsets = [];
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
        x = Math.smoothstep(transform.localPosition.x, -offsets[selectedItem], timer);
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

    fun computeOffsets()
    {
        offsets.clear();
        if(items.length == 0)
            return;

        offsets.push(0);
        items[0].setOffset(Vector2.zero);
        totalOffset = Math.floor(items[0].width / 2);

        for(i = 1; i < items.length; i++) {
            item = items[i];

            halfWidth = Math.floor(item.width / 2);
            totalOffset += spacing + halfWidth;

            offsets.push(totalOffset);
            item.setOffset(Vector2.right.scaledBy(totalOffset));

            totalOffset += halfWidth;
        }
    }

    fun constructor()
    {
        // spawn menu items
        items.push(spawn("Title Screen - Menu Item - Start Game"));
        items.push(spawn("Title Screen - Menu Item - Options"));

        if(!SurgeEngine.mobile)
            items.push(spawn("Title Screen - Menu Item - Mobile"));

        items.push(spawn("Title Screen - Menu Item - Share"));

        if(SurgeTheRabbit.canAcceptDonations())
            items.push(spawn("Title Screen - Menu Item - Donate"));

        /*if(SurgeEngine.mobile)
            items.push(spawn("Title Screen - Menu Item - Report Issue"));*/

        items.push(spawn("Title Screen - Menu Item - Submit Feedback"));
        items.push(spawn("Title Screen - Menu Item - Quit"));

        // compute offsets of the menu items
        computeOffsets();
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
        // reset player data
        player = Player.active;
        player.lives = Player.initialLives;
        player.score = 0;

        // load the default quest
        Level.load("quests/default.qst");
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Options" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_OPTIONS");

    fun onEnter()
    {
        Player.active.lives = Player.initialLives;
        Level.load("quests/options.qst");
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Mobile" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_MOBILE");

    fun onEnter()
    {
        SurgeTheRabbit.download();
        Level.restart();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Share" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_SHARE");

    fun onEnter()
    {
        SurgeTheRabbit.share();
        Level.restart();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Donate" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_DONATE");

    fun onEnter()
    {
        SurgeTheRabbit.donate();
        Level.restart();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Quit" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_QUIT");

    back = "fire4";

    state "main"
    {
        // handle the back button (required on Android)
        input = Player.active.input;
        if(input.buttonPressed(back))
            onSelect();
    }

    fun onEnter()
    {
        Level.loadNext();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Submit Feedback" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText(SurgeEngine.mobile ? "$TITLESCREEN_RATEAPP" : "$TITLESCREEN_SUBMITFEEDBACK");

    fun onEnter()
    {
        SurgeTheRabbit.submitFeedback();
        Level.restart();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item - Report Issue" is "private", "detached", "entity"
{
    delegate = spawn("Title Screen - Menu Item")
               .setText("$TITLESCREEN_REPORTISSUE");

    fun onEnter()
    {
        SurgeTheRabbit.reportIssue();
        Level.restart();
    }

    fun onSelect()
    {
        parent.onSelect(this);
    }

    fun onHighlight()
    {
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

    fun get_width()
    {
        return delegate.width;
    }
}

object "Title Screen - Menu Item" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("Title Screen - Menu Item");
    pointer = spawn("Title Screen - Pointer");
    text = "";
    action = "fire1";
    start = "fire3";
    highlighted = false;

    state "main"
    {
        if(!highlighted)
            return;

        input = Player.active.input;
        if(input.buttonPressed(action) || input.buttonPressed(start))
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
        pointer.visible = highlighted;
        pointer.offset = Vector2(-label.size.x / 2, 0);

        if(highlighted)
            parent.onHighlight();

        return this;
    }

    fun setOffset(offset)
    {
        transform.localPosition = offset;
        return this;
    }

    fun get_width()
    {
        return label.size.x;
    }

    fun constructor()
    {
        label.align = "center";
        pointer.visible = false;
    }
}


object "Title Screen - Pointer" is "private", "detached", "entity"
{
    actor = Actor("Title Screen - Pointer");
    transform = Transform();

    state "main"
    {
        if(!actor.visible)
            return;

        // swing back and forth
        x = 2.0 * Math.cos(12.6 * Time.time);
        actor.offset = Vector2(x, 0);
    }

    fun set_visible(visible)
    {
        actor.visible = visible;
    }

    fun get_visible()
    {
        return actor.visible;
    }

    fun set_offset(offset)
    {
        transform.localPosition = offset;
    }

    fun get_offset()
    {
        return transform.localPosition;
    }
}
