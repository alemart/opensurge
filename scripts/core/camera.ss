// -----------------------------------------------------------------------------
// File: camera.ss
// Description: the default camera for Open Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Player;
using SurgeEngine.Camera;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Video.Screen;

object "Default Camera" is "entity", "awake", "private"
{
    public offset = Vector2.zero; // an adjustable offset
    public enabled = true; // is this camera enabled?

    transform = Transform();
    actor = Actor("Default Camera");
    upDown = spawn("Default Camera - Up/Down Logic");
    player = Player.active;

    state "main"
    {
    }

    state "frozen"
    {
        // unfreeze if the player ressurrects
        if(!player.dying)
            state = "main";
    }

    fun lateUpdate()
    {
        if(state != "frozen") {

            // compute the delta
            delta = player.transform.position.minus(transform.position);

            // compute a dynamic speed cap
            // the camera will lag behind if the player is too fast
            cap = 16; // suitable when |gsp| <= 960 px/s (default cap speed)
            if(Math.abs(player.speed) > 960) {
                // don't let the player move too far away from the camera (ROI)
                if(Math.abs(delta.x - 8) >= Screen.width / 2 + 100)
                    cap = Math.abs(player.speed) / 60;
            }

            // move within a box
            if(delta.x > 8)
                transform.translateBy(Math.min(cap, delta.x - 8), 0);
            else if(delta.x < -8)
                transform.translateBy(Math.max(-cap, delta.x + 8), 0);

            if(player.midair || player.frozen) {
                if(delta.y > 32)
                    transform.translateBy(0, Math.min(cap, delta.y - 32));
                else if(delta.y < -32)
                    transform.translateBy(0, Math.max(-cap, delta.y + 32));
            }
            else {
                dy = Math.abs(player.gsp) >= 480 ? cap : 6;
                if(delta.y >= 1)
                    transform.translateBy(0, Math.min(dy, delta.y - 1));
                else if(delta.y <= -1)
                    transform.translateBy(0, Math.max(-dy, delta.y + 1));
            }

            // switched player?
            if(Player.active != player) {
                player = Player.active;
                centerCamera(player.transform.position);
            }

            // camera too far away? (e.g., player teleported)
            if(delta.length > 2 * Screen.width)
                centerCamera(player.transform.position);

            // freeze camera
            if(player.dying)
                state = "frozen";

        }

        // update the camera
        refresh();
    }

    fun centerCamera(position)
    {
        transform.position = position;
    }

    fun showGizmo()
    {
        actor.visible = true;
    }

    fun hideGizmo()
    {
        actor.visible = false;
    }

    fun translate(offset)
    {
        transform.translate(offset);
    }

    fun refresh()
    {
        if(!enabled)
            return;

        if(state == "frozen") {
            Camera.position = transform.position.plus(offset);
            return;
        }

        Camera.position = transform.position.translatedBy(offset.x, offset.y + upDown.offset);
    }

    fun constructor()
    {
        centerCamera(player.transform.position);
        actor.zindex = 9999;
        actor.visible = false;
    }
}

object "Default Camera - Up/Down Logic"
{
    public readonly offset = 0;
    player = Player.active;
    speed = 120; // move at 120 px/s

    state "main"
    {
        player = Player.active;
        if(player.lookingUp)
            state = "wait up";
        else if(player.crouchingDown)
            state = "wait down";
        else
            moveBackToZero();
    }

    state "wait up"
    {
        if(timeout(2.0))
            state = "move up";
        else if(Player.active != player || !player.lookingUp)
            state = "main";
        else
            moveBackToZero();
    }

    state "wait down"
    {
        if(timeout(2.0))
            state = "move down";
        else if(Player.active != player || !player.crouchingDown)
            state = "main";
        else
            moveBackToZero();
    }

    state "move up"
    {
        offset = Math.max(-96, offset - speed * Time.delta);
        if(Player.active != player || !player.lookingUp)
            state = "main";
    }

    state "move down"
    {
        offset = Math.min(92, offset + speed * Time.delta);
        if(Player.active != player || !player.crouchingDown)
            state = "main";
    }

    fun moveBackToZero()
    {
        if(offset > 0)
            offset = Math.max(0, offset - speed * Time.delta);
        else
            offset = Math.min(0, offset + speed * Time.delta);
    }
}
