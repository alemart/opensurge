![Open Surge](logo.png)

<img src="surge.png" alt="Surge" width="384" align="right">

Welcome to the **Open Surge** source code repository!

Get the game at [opensurge2d.org](http://opensurge2d.org)

## What is Open Surge?

Open Surge is a fun 2D retro platformer inspired by old-school Sonic games. Play, hack, create worlds and unleash your creativity! Currently under development.

## About Open Surge

Open Surge is written from the ground up in C language, using the [Allegro game programming library](http://liballeg.org). The project has been started by [Alexandre Martins](http://github.com/alemart), a developer from Brazil. Nowadays, Open Surge has contributors all over the world!

## How do I play?

You can play Open Surge with the keyboard or with a joystick.

| Key           | Effect          
| --------------|------------------|
| Arrows        | Move             |
| Space         | Jump             |
| Enter         | Pause            |
| Esc           | Quit             |
| Left Ctrl     | Switch character |
| Equals (=)    | Take snapshot    |
| F12           | Open the editor  |

## Advanced users

### How do I create a game?

Open Surge is a highly flexible game. It can be remixed in many ways, allowing users to unleash their creativity. Modifications (MODs) range from small hacks to whole new games.

Learn:
* How to use the level editor (press F12 during gameplay)
* How to do [basic hacking](http://opensurge2d.org) (modify sprites, sounds, characters, controls, etc.)
* How to use [SurgeScript](http://docs.opensurge2d.org) to unleash your full creativity!

Check the [video tutorials](http://youtube.com/alemart88) as well.

### Running from the command line

Advanced features are available via the command line. For more information, run:

```
opensurge --help
```

### Compiling Open Surge

To compile Open Surge from the source code, you'll need a C compiler, [CMake](http://cmake.org), and the following development libraries:

* [Allegro 5.2](http://liballeg.org) (preferably 5.2.5 or higher)
* [libsurgescript](http://github.com/alemart/surgescript)

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

You may want to run `ccmake` or `cmake-gui` to know additional build options (e.g., to change the installation directory). If you have installed the development libraries into non-standard paths, you need to configure their appropriate paths as well.

**Linux users:** game assets (images, sounds, etc.) can be stored globally or in user-space. Assets located in user-space take precedence over assets located in system directories. Open Surge uses the XDG Base Directory specification; look for the *opensurge2d* directory.