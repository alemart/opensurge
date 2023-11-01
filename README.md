# Surge Engine

[![Latest release](https://img.shields.io/github/v/release/alemart/opensurge?color=blue)](https://github.com/alemart/opensurge/releases)
[![License](https://img.shields.io/github/license/alemart/opensurge?color=brightgreen)](#license)
[![GitHub Repo stars](https://img.shields.io/github/stars/alemart/opensurge?logo=github&color=orange)](https://github.com/alemart/opensurge/stargazers)
[![Discord server](https://img.shields.io/discord/493384707937927178?color=5662f6&logo=discord&logoColor=white)](https://discord.gg/w8JqM7m)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/alemart?label=Sponsor%20me&logo=github%20sponsors&style=social)](https://github.com/sponsors/alemart)

Make your dreams come true with **Surge Engine**, an open source game engine for retro games! Get joyful with **Surge the Rabbit**, a fun retro platformer full of playfulness!

[Download now](https://opensurge2d.org) | [Create your games](https://wiki.opensurge2d.org/Introduction_to_Modding) | [Learn scripting](https://docs.opensurge2d.org) | [Watch videos](https://youtube.com/alemart88)

<img src="https://opensurge2d.org/surge-demo.gif" alt="Open Surge demo" width="480">

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J41O00K)

---

## About

The project is written from the ground up in C language, using the [Allegro game programming library](http://liballeg.org). It has been started by [Alexandre Martins](http://github.com/alemart), a computer scientist from Brazil. Nowadays, it has contributors all over the world!

## Contribute

See [CONTRIBUTING](https://github.com/alemart/opensurge/blob/master/CONTRIBUTING.md).

## Advanced users

### Command-line options

Run `opensurge --help`

### Running MODs

Extract the MOD to your filesystem and run the game.

- Windows: launch the executable.
- Linux, macOS: run `opensurge --game /path/to/game/` on the command-line.

Use preferably the same engine version as the MOD if a native executable isn't provided (check `logfile.txt`).

Linux users should extract the MOD into their home folder, preferably into `~/Downloads`.

[Visit the wiki](https://wiki.opensurge2d.org/User-made_games) for more information on user-made MODs.

### Paths

Content is distributed in the following locations: (since version 0.6.1)

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

- Linux ([Flatpak](https://flathub.org/apps/details/org.opensurge2d.OpenSurge)):
    * `bin`: `flatpak run org.opensurge2d.OpenSurge`
    * `share`: `/var/lib/flatpak/app/org.opensurge2d.OpenSurge/current/active/files/share/opensurge/`
    * `user`: `~/.var/app/org.opensurge2d.OpenSurge/data/opensurge/` or `$OPENSURGE_USER_PATH`

    If you use `$OPENSURGE_USER_PATH`, make sure it points to a subdirectory of `~/Downloads` (`$XDG_DOWNLOAD_DIR`).

- Linux ([Snap](https://snapcraft.io/opensurge)):
    * `bin`: `snap run opensurge`
    * `share`: `/snap/opensurge/current/share/games/opensurge/`
    * `user`: `~/snap/opensurge/current/.local/share/opensurge/` or `$OPENSURGE_USER_PATH`

    If you use `$OPENSURGE_USER_PATH`, make sure it points to a subdirectory of your home folder.

- macOS:
    * `bin`: `Contents/MacOS`
    * `share`: `Contents/Resources`
    * `user`: `~/Library/Application Support/opensurge/` or `$OPENSURGE_USER_PATH`

If you intend to hack the game, it's easier to have all files in the same place (read-write), because some of the above folders are read-only. [Download the sources](https://github.com/alemart/opensurge/releases), extract them to your filesystem and use the `--game` command-line option as explained in [Running MODs](#running-mods).

Tip: since version 0.6.1 you can use the command-line option `--verbose` to see where the files are. The directories will appear at the beginning of the output.

### Compiling the engine

Dependencies:

* [Allegro](http://liballeg.org) version 5.2.7 or later
* [SurgeScript](http://github.com/alemart/surgescript) version 0.6.0
* [PhysicsFS](https://icculus.org/physfs) version 3.2.0

Compile as usual:

```
cd /path/to/opensurge/
mkdir build && cd build
cmake ..
make -j4
sudo make install
```

Use `cmake-gui` or `ccmake` for tweaking, like installing the engine to or finding the dependencies on non-standard paths.

## License

Open Surge Engine Copyright 2008-present Alexandre Martins. This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version. For more information, see [LICENSE](https://github.com/alemart/opensurge/blob/master/LICENSE).
