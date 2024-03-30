# Surge Engine

[![Latest release](https://img.shields.io/github/v/release/alemart/opensurge?color=blue)](https://github.com/alemart/opensurge/releases)
[![License](https://img.shields.io/github/license/alemart/opensurge?color=brightgreen)](#license)
[![GitHub Repo stars](https://img.shields.io/github/stars/alemart/opensurge?logo=github&color=orange)](https://github.com/alemart/opensurge/stargazers)
[![Discord server](https://img.shields.io/discord/493384707937927178?color=5662f6&logo=discord&logoColor=white)](https://discord.gg/w8JqM7m)
[![YouTube Channel Subscribers](https://img.shields.io/youtube/channel/subscribers/UCqy8swP261RPePNdBdJUdHg?style=flat&logo=youtube&label=YouTube&color=ff0000)](https://youtube.com/alemart88)

**Surge the Rabbit** is a fun and open source jump 'n run retro game made in the spirit of classic 16-bit Sonic platformers of the 90s. The levels are filled with challenges, gimmicks and hazards! Exciting adventures are waiting for you! You can also use it as a base to create your own amazing games!

**Make your dreams come true!** Surge Engine is the very flexible open source 2D game engine that powers Surge the Rabbit. Its capabilities include: level editing, artwork modding, full blown scripting, and much more! It's an open-source, cross-platform retro game engine that runs on Desktop computers and on mobile devices.

**The fun is multiplied!** Several MODs created by the fans in the Open Surge Community can be played on the engine. For those who get involved, this project is also a valuable tool for learning game development, programming, artistic skills, and the nature of free and open-source software in a playful way.

<img src="https://opensurge2d.org/surge-demo.gif" alt="Open Surge demo" width="480">

## Support the project

Please support the development of the project:

[![GitHub Sponsors](https://img.shields.io/github/sponsors/alemart?style=social&logo=github%20sponsors&label=Support%20me%20on%20GitHub%20Sponsors)](https://github.com/sponsors/alemart)

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J41O00K)

## Documentation

* [Open Surge Wiki](https://wiki.opensurge2d.org)
* [Introduction to modding](https://wiki.opensurge2d.org/Introduction_to_Modding)
* [SurgeScript documentation](https://docs.opensurge2d.org)

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
- Linux, macOS: run `opensurge --game /path/to/game/` on the command-line or load the game from the options screen.

Linux users should extract MODs preferably to their home folder. Flatpak users may use `~/Downloads`.

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

If you intend to hack the game, it's easier to have all files in the same place (read-write), because some of the above folders are read-only. [Download the sources](https://github.com/alemart/opensurge/releases) (use the same engine version), extract them to your filesystem and use the `--game-folder` command-line option.

Tip: since version 0.6.1, check the *Engine information* at the options screen to see where the files are. Alternatively, you can use the command-line option `--verbose`: the directories will appear at the beginning of the output.

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

## License

Open Surge Engine Copyright 2008-present Alexandre Martins. This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version. For more information, see [LICENSE](https://github.com/alemart/opensurge/blob/master/LICENSE).
