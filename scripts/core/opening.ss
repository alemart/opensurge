// -----------------------------------------------------------------------------
// File: opening.ss
// Description: default Level Opening Animation
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;

object "Default Opening Animation" is "entity", "awake", "detached", "private"
{
    theVoid = spawn("DefaultOpeningAnimation.Void");
    leftHalf = spawn("DefaultOpeningAnimation.LeftHalf");
    rightHalf = spawn("DefaultOpeningAnimation.RightHalf");
    leftArrow = spawn("DefaultOpeningAnimation.Arrow").pointingTo("left");
    rightArrow = spawn("DefaultOpeningAnimation.Arrow").pointingTo("right");
    act = spawn("DefaultOpeningAnimation.Act");
    title = spawn("DefaultOpeningAnimation.Title");
    game = spawn("DefaultOpeningAnimation.Game");
    lighting = spawn("DefaultOpeningAnimation.Lighting");
    formulas = [
        spawn("DefaultOpeningAnimation.Formula").atColumn(0),
        spawn("DefaultOpeningAnimation.Formula").atColumn(1),
        spawn("DefaultOpeningAnimation.Formula").atColumn(2)
    ];

    state "main"
    {
        leftArrow.appear();
        rightArrow.appear();
        state = "appearing arrows";
    }

    state "appearing arrows"
    {
        if(timeout(0.3)) {
            leftHalf.appear();
            rightHalf.appear();
            state = "appearing halfs";
        }
    }

    state "appearing halfs"
    {
        if(timeout(0.2)) {
            game.appear();
            title.appear();
            act.appear();
            lighting.appear();
            foreach(formula in formulas)
                formula.appear();
            state = "displaying info";
        }
    }

    state "displaying info"
    {
        if(timeout(2.7)) {
            theVoid.disappear();
            leftHalf.disappear();
            rightHalf.disappear();
            foreach(formula in formulas)
                formula.disappear();
            lighting.disappear();
            act.disappear();
            title.disappear();
            game.disappear();
            state = "done";
        }
    }

    state "done"
    {
        if(timeout(1.0))
            destroy();
    }
}

object "DefaultOpeningAnimation.Void" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.Void");

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = 1001.0;
    }

    fun disappear()
    {
        destroy();
    }
}

object "DefaultOpeningAnimation.LeftHalf" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.LeftHalf");
    speed = Screen.width * 5;

    state "main"
    {
    }

    state "appearing"
    {
        transform.move(speed * Time.delta, 0);
        if(transform.position.x >= Screen.width) {
            transform.position = Vector2(Screen.width, 0);
            state = "main";
        }
    }

    state "disappearing"
    {
        transform.move(-speed * Time.delta, 0);
        if(transform.position.x < 0)
            destroy();
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = 1001.1;
    }

    fun appear()
    {
        state = "appearing";
    }

    fun disappear()
    {
        state = "disappearing";
    }
}

object "DefaultOpeningAnimation.RightHalf" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.RightHalf");
    speed = Screen.width * 5;

    state "main"
    {
    }

    state "appearing"
    {
        transform.move(-speed * Time.delta, 0);
        if(transform.position.x < 0) {
            transform.position = Vector2.zero;
            state = "main";
        }
    }

    state "disappearing"
    {
        transform.move(speed * Time.delta, 0);
        if(transform.position.x >= Screen.width)
            destroy();
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width, 0);
        actor.zindex = 1001.25;
    }

    fun appear()
    {
        state = "appearing";
    }

    fun disappear()
    {
        state = "disappearing";
    }
}

object "DefaultOpeningAnimation.Arrow" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.Arrow");
    speed = Screen.width * 3;

    state "main"
    {
    }

    state "appearing"
    {
        transform.move(speed * Time.delta, 0);
        if(timeout(Screen.width / speed + 0.7))
            destroy();
    }

    fun constructor()
    {
        transform.position = Vector2(-999, -999);
        actor.zindex = 1001.1;
    }

    fun pointingTo(direction)
    {
        if(direction == "right") {
            actor.anim = 1;
            transform.position = Vector2(0, Screen.height / 2);
            speed = Math.abs(speed);
        }
        else if(direction == "left") {
            actor.anim = 0;
            transform.position = Vector2(Screen.width, Screen.height / 2);
            speed = -Math.abs(speed);
        }
        return this;
    }

    fun appear()
    {
        state = "appearing";
    }
}

object "DefaultOpeningAnimation.Formula" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.Formula");
    speed = Screen.width * 0.7;

    state "main"
    {
    }

    state "visible"
    {
        transform.move(speed * Time.delta, 0);
        if(transform.position.x >= Screen.width)
            transform.position = Vector2(-actor.width - (Screen.width - transform.position.x), transform.position.y);
    }

    fun constructor()
    {
        transform.position = Vector2.zero;
        actor.zindex = 1001.2;
        actor.visible = false;
    }

    fun atColumn(col)
    {
        transform.position = Vector2(actor.width * (col - 1), Screen.height - actor.height);
        return this;
    }

    fun appear()
    {
        actor.visible = true;
        state = "visible";
    }

    fun disappear()
    {
        destroy();
    }
}

object "DefaultOpeningAnimation.Title" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("GoodNeighborsLarge");

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, Screen.height / 2);
        text.text = Level.name;
        text.align = "center";
        text.offset = Vector2(0, -16);
        text.zindex = 1001.51;
        text.visible = false;
    }

    fun appear()
    {
        text.visible = true;
    }

    fun disappear()
    {
        destroy();
    }
}

object "DefaultOpeningAnimation.Act" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("GoodNeighborsLarge");
    wheel = spawn("DefaultOpeningAnimation.Wheel");

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, 176);
        text.text = Level.act;
        text.align = "center";
        text.offset = Vector2(0, -16);
        text.zindex = 1001.51;
        text.visible = false;
    }

    fun appear()
    {
        text.visible = true;
        wheel.appear();
    }

    fun disappear()
    {
        destroy();
    }
}

object "DefaultOpeningAnimation.Wheel" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.Wheel");
    speed = -360;

    state "main"
    {
    }

    state "visible"
    {
        transform.rotate(speed * Time.delta);
    }

    fun constructor()
    {
        actor.zindex = 1001.5;
        actor.visible = false;
    }

    fun appear()
    {
        actor.visible = true;
        state = "visible";
    }
}

object "DefaultOpeningAnimation.Game" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    text = Text("GoodNeighbors");
    label = "opensurge2d.org";
    margin = 4;

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width - margin, margin);
        text.text = label;
        text.align = "right";
        text.zindex = 1001.51;
        text.visible = false;
    }

    fun appear()
    {
        text.visible = true;
    }

    fun disappear()
    {
        destroy();
    }
}

object "DefaultOpeningAnimation.Lighting" is "entity", "awake", "detached", "private"
{
    transform = Transform();
    actor = Actor("DefaultOpeningAnimation.Lighting");

    state "main"
    {
    }

    fun constructor()
    {
        transform.position = Vector2(Screen.width / 2, 48);
        actor.zindex = 1001.5;
        actor.visible = false;
    }

    fun appear()
    {
        actor.visible = true;
    }

    fun disappear()
    {
        destroy();
    }
}
