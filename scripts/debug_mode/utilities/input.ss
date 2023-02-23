// -----------------------------------------------------------------------------
// File: input.ss
// Description: abstract input system (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

The following button names are available:

"up", "right", "down", "left",
"action", "secondary-action",
"back"

*/

using SurgeEngine.Input;

object "Debug Mode - Input" is "debug-mode-utility"
{
    inputA = Input("default");
    inputB = Input("Debug Mode");
    buttonMapping = {
        "up":               [ "up",     "up"    ],
        "right":            [ "right",  "right" ],
        "down":             [ "down",   "down"  ],
        "left":             [ "left",   "left"  ],
        "action":           [ "fire1",  ""      ],
        "secondary-action": [ "fire2",  "fire2" ],
        "back":             [ "fire4",  "fire4" ],
    };




    state "main"
    {
        if(timeout(0.5)) {
            inputA.enabled = true;
            inputB.enabled = true;
            state = "enabled";
        }
    }

    state "enabled"
    {
    }




    fun buttonDown(buttonName)
    {
        button = buttonMapping[buttonName];
        return inputA.buttonDown(button[0]) || inputB.buttonDown(button[1]);
    }

    fun buttonPressed(buttonName)
    {
        button = buttonMapping[buttonName];
        return inputA.buttonPressed(button[0]) || inputB.buttonPressed(button[1]);
    }

    fun buttonReleased(buttonName)
    {
        button = buttonMapping[buttonName];
        return inputA.buttonReleased(button[0]) || inputB.buttonReleased(button[1]);
    }



    fun constructor()
    {
        inputA.enabled = false;
        inputB.enabled = false;
    }
}