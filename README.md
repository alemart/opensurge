<div align="center"><br>
<img src="https://opensurge2d.org/img/surge_profile.webp" height="256">
<h1>Surge Engine</h1>

A retro game engine with a fun platformer for making your dreams come true!

[![Latest release](https://img.shields.io/github/v/release/alemart/opensurge?color=blue)](https://github.com/alemart/opensurge/releases)
[![License](https://img.shields.io/github/license/alemart/opensurge?color=brightgreen)](#license)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/alemart?logo=github)](https://github.com/sponsors/alemart)
[![Discord server](https://img.shields.io/discord/493384707937927178?color=5662f6&logo=discord&logoColor=white)](https://discord.gg/w8JqM7m)

<img src="https://opensurge2d.org/surge-demo.gif" alt="Demo" height="360">
<br><br>
</div>

## It's cool!

**Make your dreams come true!** Open Surge Engine is an open-source 2D retro game engine for creating games and making your dreams come true!

**It's a ton of fun!** Surge the Rabbit is a featured jump 'n' run created with the Open Surge Engine. It's made in the spirit of classic 16-bit Sonic platformers of the 1990s. Play as Surge in fun and exciting levels filled with challenges!

**Unleash your creativity!** Create your own amazing games and play them on your PC and on your mobile device! Share your games with your friends! It's limitless fun!

**A powerful engine for retro games!** One of the core elements of the engine is [SurgeScript](https://github.com/alemart/surgescript), a scripting language for games. Use it to create new gameplay mechanics, characters with special abilities, bosses, and much more! The sky is the limit!

Open Surge Engine is an amazing tool for learning game development, programming, digital art, and the nature of free and open-source software in a playful way.

Official website: http://opensurge2d.org

## Download

[<img src="https://opensurge2d.org/img/badge_github.png" height="120" alt="Get it on GitHub">](https://github.com/alemart/opensurge/releases)
[<img src="https://opensurge2d.org/img/badge_fdroid.png" height="120" alt="Get it on F-Droid">](https://f-droid.org/packages/org.opensurge2d.surgeengine)

[<img src="https://opensurge2d.org/img/badge_flathub.png" height="120" alt="Get it on Flathub">](https://flathub.org/apps/org.opensurge2d.OpenSurge)
[<img src="https://opensurge2d.org/img/badge_snapcraft.png" height="120" alt="Get it from the Snap Store">](https://snapcraft.io/opensurge)

## Support the project

[![GitHub Sponsors](https://img.shields.io/github/sponsors/alemart?style=social&logo=github&label=Sponsors)](https://github.com/sponsors/alemart)

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J41O00K)

## Play MODs

You can play MODs created by other users.

1. Download the MOD.
    - Linux users: preferably download it to your home folder.
    - Flatpak users: download it to `~/Downloads`.
    - Snap users: download it to your home folder.
2. Load the game.
    - Windows users: extract the MOD and launch the executable.
    - Other platforms: load the game from the options screen.

## Documentation

* [Open Surge Wiki](https://wiki.opensurge2d.org)
* [Introduction to Modding](https://wiki.opensurge2d.org/Introduction_to_Modding)
* [SurgeScript documentation](https://docs.opensurge2d.org)
* [How to contribute](https://github.com/alemart/opensurge/blob/master/CONTRIBUTING.md)

## About

The project is written from the ground up in C language, using the [Allegro game programming library](http://liballeg.org). It has been started by [Alexandre Martins](http://github.com/alemart), a computer scientist from Brazil. Nowadays, it has contributors all over the world!

## License

[GPLv3](https://github.com/alemart/opensurge/blob/master/LICENSE)

## Advanced

### Command-line options

Run `opensurge --help`

### Paths

Check the *Engine information* at the options screen to see where the files are.

Content is distributed in the following locations:

- `bin`: executable file
- `share`: game assets (images, audio, levels, scripts, etc.)
- `user`: user-modifiable data (preferences, logs, screenshots, additional assets such as user-made levels)

The default paths of these locations vary according to the platform:

- Windows (.zip package):
    * `bin`: `./opensurge.exe`
    * `share`: `.`
    * `user`: `.` or `%OPENSURGE_USER_PATH%`

- Linux:
    * `bin`: `/usr/games/opensurge`
    * `share`: `/usr/share/games/opensurge/`
    * `user`: `~/.local/share/opensurge/` or `$XDG_DATA_HOME/opensurge/` or `$OPENSURGE_USER_PATH`

- Linux (Flatpak):
    * `bin`: `flatpak run org.opensurge2d.OpenSurge`
    * `share`: `/var/lib/flatpak/app/org.opensurge2d.OpenSurge/current/active/files/share/opensurge/`
    * `user`: `~/.var/app/org.opensurge2d.OpenSurge/data/opensurge/` or `$OPENSURGE_USER_PATH`

    If you use `$OPENSURGE_USER_PATH`, make sure it points to a subdirectory of `~/Downloads` (`$XDG_DOWNLOAD_DIR`).

- Linux (Snap):
    * `bin`: `snap run opensurge`
    * `share`: `/snap/opensurge/current/share/games/opensurge/`
    * `user`: `~/snap/opensurge/current/.local/share/opensurge/` or `$OPENSURGE_USER_PATH`

    If you use `$OPENSURGE_USER_PATH`, make sure it points to a subdirectory of your home folder.

- macOS:
    * `bin`: `Contents/MacOS`
    * `share`: `Contents/Resources`
    * `user`: `~/Library/Application Support/opensurge/` or `$OPENSURGE_USER_PATH`

If you intend to hack the game, it's easier to have all files in the same place (read-write), because some of the above folders are read-only. Download the sources (use the same engine version), extract them to your filesystem and use the `--game-folder` command-line option.

Tip: you can also use the command-line option `--verbose`. The directories will appear at the beginning of the output.

### Compiling the engine

Dependencies:

* [Allegro](http://liballeg.org) version 5.2.7 or later (modified 5.2.9 on Android)
* [SurgeScript](http://github.com/alemart/surgescript) version 0.6.0
* [PhysicsFS](https://github.com/icculus/physfs) version 3.2.0 (recommended) or 3.0.2

Compile as usual:

```
cd /path/to/opensurge/
mkdir build && cd build
cmake ..
make -j4
sudo make install
```

Use `cmake-gui` or `ccmake` for tweaking, like installing the engine to or finding the dependencies on non-standard paths.
