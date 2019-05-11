// -----------------------------------------------------------------------------
// File: camera.ss
// Description: the default camera for Open Surge
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Player;
using SurgeEngine.Camera;
using SurgeEngine.Video.Screen;

object "DefaultCamera" is "entity", "awake", "private"
{
    transform = Transform();
    upDown = spawn("DefaultCamera.UpDownLogic");
    player = Player.active;

    state "main"
    {
        // compute the delta
        delta = player.transform.position.minus(transform.position);

        // move within a box
        if(delta.x > 8)
            transform.move(Math.min(16, delta.x - 8), 0);
        else if(delta.x < -8)
            transform.move(Math.max(-16, delta.x + 8), 0);
        if(player.midair) {
            if(delta.y > 32)
                transform.move(0, Math.min(16, delta.y - 32));
            else if(delta.y < -32)
                transform.move(0, Math.max(-16, delta.y + 32));
        }
        else {
            dy = Math.abs(player.ysp) > 360 ? 16 : 6;
            if(delta.y >= 1)
                transform.move(0, Math.min(dy, delta.y - 1));
            else if(delta.y <= -1)
                transform.move(0, Math.max(-dy, delta.y + 1));
        }

        // switched player?
        if(Player.active != player) {
            player = Player.active;
            centerCamera(player.transform.position);
        }

        // camera too far away? (e.g., player teleported)
        if(delta.length > 2 * Screen.width)
            centerCamera(player.transform.position);

        // stop camera
        if(player.activity == "dying" || player.activity == "drowning")
            return;

        // update camera
        Camera.position = transform.position.translatedBy(0, upDown.offset);
    }

    fun constructor()
    {
        centerCamera(player.transform.position);
    }

    fun centerCamera(position)
    {
        transform.position = position;
    }
}

object "DefaultCamera.UpDownLogic"
{
    player = Player.active;
    speed = 120; // move at 120 px/s
    offset = 0;

    state "main"
    {
        player = Player.active;
        if(player.activity == "lookingup")
            state = "wait up";
        else if(player.activity == "ducking")
            state = "wait down";
        else if(player.activity == "winning")
            state = "move up";
        else
            moveBackToZero();
    }

    state "wait up"
    {
        if(timeout(2.0))
            state = "move up";
        else if(Player.active != player || player.activity != "lookingup")
            state = "main";
        else
            moveBackToZero();
    }

    state "wait down"
    {
        if(timeout(2.0))
            state = "move down";
        else if(Player.active != player || player.activity != "ducking")
            state = "main";
        else
            moveBackToZero();
    }

    state "move up"
    {
        offset = Math.max(-96, offset - speed * Time.delta);
        if(Player.active != player || player.activity != "lookingup")
            state = "main";
    }

    state "move down"
    {
        offset = Math.min(92, offset + speed * Time.delta);
        if(Player.active != player || player.activity != "ducking")
            state = "main";
    }

    fun moveBackToZero()
    {
        if(offset > 0)
            offset = Math.max(0, offset - speed * Time.delta);
        else
            offset = Math.min(0, offset + speed * Time.delta);
    }

    fun get_offset()
    {
        return offset;
    }
}