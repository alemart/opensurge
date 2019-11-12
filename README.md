![Open Surge](logo.png)

Welcome to **Open Surge**!

Get the game at [opensurge2d.org](http://opensurge2d.org)

<img src="surge.png" alt="Surge" width="384" align="right">

## What is Open Surge?

Open Surge is a fun 2D retro platformer inspired by old-school Sonic games. It is also a game creation system that lets you unleash your creativity! Play, create & share!

Open Surge is currently available for Microsoft Windows and GNU/Linux. It is in active development!

## About Open Surge

Open Surge is two projects in one: a game and a game creation system (game engine). It is released as [free and open source software](https://en.wikipedia.org/wiki/Free_and_open-source_software).

Open Surge is written from the ground up in C language, using the [Allegro game programming library](http://liballeg.org). The project has been started by [Alexandre Martins](http://github.com/alemart), a developer from Brazil. Nowadays, Open Surge has contributors all over the world!

## How do I play?

You can play Open Surge using a keyboard or a joystick.

| Key           | Effect          
| --------------|------------------|
| Arrows        | Move             |
| Space         | Jump             |
| Enter         | Pause            |
| Esc           | Quit             |
| Left Ctrl     | Switch character |
| Equals (=)    | Take snapshot    |
| F12           | Open the editor  |

## How do I create a game?

Use Open Surge to create your own amazing games! Create new levels, items, bosses, gameplay mechanics, playable characters, special abilities and much more!

* First, learn how to create a level using the built-in editor (press F12 during gameplay)
* Next, learn how to do [basic hacking](http://opensurge2d.org) (modify the images/sounds, create new scenery, new characters, etc.)
* Finally, have fun with scripting! [SurgeScript](http://docs.opensurge2d.org), the scripting language featured in Open Surge, gives you ultimate power to create anything you desire and make your dreams come alive!

To learn more, watch the [video tutorials](http://youtube.com/alemart88) made by the developer of the engine.

## Advanced users

### Running from the command line

Advanced features are available via the command line. For more information, run:

```
opensurge --help
```

### Compiling Open Surge

To compile Open Surge from the source code, you'll need a C compiler, [CMake](http://cmake.org), and the following development libraries:

* [Allegro 5](http://liballeg.org) (version 5.2.5 or higher)
* [SurgeScript](http://github.com/alemart/surgescript) (version 0.5.4 or higher)

After downloading and extracting the source code, create a build directory and compile from there:

```
mkdir build && cd build
cmake ..
make
```

You can perform a system-wide installation on Linux by running:

```
sudo make install
```

You may run `ccmake` or `cmake-gui` to know additional build options (e.g., set the path of the installation directory). If you have installed the development libraries into non-standard paths, you need to configure their appropriate paths as well.

**Linux users:** game assets (images, sounds, etc.) can be stored globally or in user-space. Assets located in user-space take precedence over assets located in system directories. Open Surge uses the XDG Base Directory specification; look for the *opensurge2d* directory.

| Files         | Usual locations       |
| --------------|-----------------------|
| Game assets   | /usr/local/share/games, ~/.local/share/opensurge2d |
| Game config.  | ~/.config/opensurge2d |
| Logs, etc.    | ~/.cache/opensurge2d  |

**Portable setup:** if you haven't done a system-wide installation, Open Surge can also read files from the folder of the executable.