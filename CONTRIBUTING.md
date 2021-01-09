# Contributing to Open Surge

Thank you for taking the time to contribute to Open Surge! :+1:

Open Surge receives contributions from volunteers all over the world. Different people have blessed this game with their talent, donating their time and effort to a project they love. Each contribution is like a little brick that is part of a beautiful cathedral. You too can help build this beautiful cathedral by contributing with your own special bricks - your creative work.

From a technical perspective, Open Surge is a [free and open-source software](https://en.wikipedia.org/wiki/Free_and_open-source_software) project divided into two parts: game engine and game data. The game engine is the software that provides basic functionality to the game, whereas the game data is the set of files composed of media / content such as: images, animations, music, sound effects, fonts, levels, translations and scripts.

## Contributing game data

Say you intend to submit some artwork to Open Surge. You may contribute as long as you are its creator (i.e., you are the owner of the work) and as long as you irrevocably release it under a suitable license.

### Pick a suitable license

Open Surge uses a few different licenses for its content. All such licenses are considered to be free by the [Free Software Foundation](http://www.fsf.org). We use the following licenses for our game data:

* Most graphics, audio files and levels are released under the [CC-BY](licenses/CC-BY-3.0-legalcode.txt) (Creative Commons Attribution) International license.
    * Generally speaking, this license gives everyone permission to freely use, share and remix your work as long as they give you credit. The Creative Commons website has a nice [human-readable summary](https://creativecommons.org/licenses/by/3.0/).
    * Works licensed under the [CC0](licenses/CC0-1.0-legalcode.txt) (Creative Commons Zero / public domain) or under the [CC-BY-SA](licenses/CC-BY-SA-3.0-legalcode.txt) (Creative Commons Attribution ShareAlike) may also be accepted.
* Scripts and configuration files in general are released under the [MIT license](licenses/MIT-license.txt).

If your contribution is an image or a text-based file, please specify your name and the license of your work (e.g., CC-BY 3.0) in the file itself. If it's an audio file, you may use a metadata field to specify this information.

### Submit it for review

The Open Surge project has been founded by and is led by [Alexandre Martins](https://github.com/alemart), a computer scientist from Brazil. Content submitted to Open Surge must be approved by him.

* The preferred way of submitting your contribution is by opening a [pull request](https://github.com/alemart/opensurge/pulls).
* Please do not open a pull request out of the blue. [Talk to Alexandre](https://wiki.opensurge2d.org/Contact_the_developers) before contributing.
* In your commit message, declare that you are the author of the work and what its license is if that information is not already clearly visible in your work.
* If you're doing multiple contributions, open multiple pull requests, so that each contribution can be reviewed separately.
* You may be asked to change things before your work is approved.

Contributors who get their work approved are mentioned in the credits screen of the game.

## Contributing as a user

You may also contribute to Open Surge as a user, without getting directly involved in its development.

If you love Open Surge, [make a donation](http://opensurge2d.org/contribute)! Open Surge is loved by its fans. Some really like the game and appreciate the fact that it is given as a gift. Others love the fact that they can [create their own games](https://wiki.opensurge2d.org/Introduction_to_Modding) with it: it teaches them how to code, how to do game design, how to have fun while expressing their creativity, and more. It provides them with the joy of seeing their own ideas come to life! By making a donation, you are showing your appreciation.

Other ways to contribute as a user include:
* Create your own games with Open Surge and tell people about the engine. It's a great and highly flexible engine. Its possibilities are endless!
* [Share Open Surge in your social media](http://opensurge2d.org/share). Write about it in your blog. Tell your friends. Spread the word!
* Create videos in which you play Open Surge and other games made with it. Tell your viewers that they too can make their own amazing games with this engine.
* Teach people how to create games with Open Surge. Help others in our [forum](http://forum.opensurge2d.org), in our Discord server and in other places. Create video tutorials, write blog posts.
* Contribute to our [wiki](http://wiki.opensurge2d.org) by adding or fixing content. Our wiki has excellent content that helps people learn how to use the engine. Its pages must be neat, instructive and accurate.

## Contributing to the engine

No contributions to the game engine are accepted at this time. While [suggestions and bug reports](http://opensurge2d.org/appdata/bugtracker.html) are welcome, any pull requests attempting to modify the game engine will be rejected.

The Open Surge game engine is written in C language and is licensed under the [GPLv3](licenses/GPL3-license.txt). It provides the ability to display graphics and animations, play musics and sound effects, accept user input, load levels, run physics simulations, parse and run user-made scripts, and much more. It uses the [Allegro game programming library](http://liballeg.org) in its core.