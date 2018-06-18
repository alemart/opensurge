//
// SurgeScript MOTD
// Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
// http://opensurge2d.org
//

// SurgeScriptMOTD greets the user
// with the Message Of The Day
object "SurgeScriptMOTD"
{
    state "main"
    {
        Console.print("Welcome to SurgeScript " + SurgeScript.version + "!");
        destroy();
    }
}
