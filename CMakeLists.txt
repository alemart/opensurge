#
# CMakeLists.txt
# Copyright (C) 2008-2018  Alexandre Martins <alemartf(at)gmail(dot)com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Initializing...
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(opensurge C)
SET(GAME_UNIXNAME opensurge)
SET(GAME_NAME "Open Surge")
SET(GAME_VERSION "0.5.0")
MESSAGE("${GAME_NAME} version ${GAME_VERSION}")

# cool vars
MARK_AS_ADVANCED(CMAKE_BUILD_TYPE CMAKE_INSTALL_PREFIX)
SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_INSTALL_PREFIX "")
SET(RTFM "Please read the user manual (readme.html) to get help.")
SET(ALLEGRO_RECOMMENDED_VERSION "4.4.2")
SET(DEFS "") # #define'd stuff
SET(CFLAGS_EXTRA "") #SET(CFLAGS_EXTRA "-g")
SET(CFLAGS "${CFLAGS} ${CMAKE_C_FLAGS}")
MESSAGE("Using compiler: ${CMAKE_C_COMPILER}.")

# Allegro libs on *nix: it requires some tweaking...
IF(UNIX)
  EXECUTE_PROCESS(COMMAND allegro-config --libs OUTPUT_VARIABLE ALLEGRO_UNIX_LIBS)
  IF(ALLEGRO_UNIX_LIBS)
    STRING(REGEX REPLACE "[ \r\n\t]+$" "" ALLEGRO_UNIX_LIBS ${ALLEGRO_UNIX_LIBS})
  ELSE(ALLEGRO_UNIX_LIBS)
    MESSAGE(FATAL_ERROR "Couldn't run \"allegro-config --libs\"")
  ENDIF(ALLEGRO_UNIX_LIBS)
ENDIF(UNIX)

# extra options
IF(UNIX)
  # Under *nix, we prefer to use OpenAL instead of Allegro in order to handle the audio routines
  # Allegro games really suffer: PulseAudio breaks the sound ...
  OPTION(USE_OPENAL "Will use OpenAL for audio playback" ON)
ELSE(UNIX)
  OPTION(USE_OPENAL "Will use OpenAL for audio playback" OFF)
ENDIF(UNIX)

# install options
IF(NOT WIN32)
  SET(DIR_INSTALL "/usr/share/${GAME_UNIXNAME}" CACHE PATH "The folder where ${GAME_NAME} will be installed.")
  SET(DIR_EXECUTABLE "/usr/bin" CACHE PATH "The executable will be placed in this folder.")
  SET(GAME_UNIX_INSTALLDIR "${DIR_INSTALL}")
  SET(GAME_UNIX_EXECDIR "${DIR_EXECUTABLE}")
  SET(CFLAGS_EXTRA "${CFLAGS_EXTRA} -D'GAME_UNIX_INSTALLDIR=\"${GAME_UNIX_INSTALLDIR}\"' -D'GAME_UNIX_EXECDIR=\"${GAME_UNIX_EXECDIR}\"'")
ENDIF(NOT WIN32)

# prefer static libs
OPTION(USE_STATIC "Use static libs whenever possible" ON)
IF(USE_STATIC)
  IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  ENDIF(WIN32)
ENDIF(USE_STATIC)



# Finding the required libraries:

# Allegro: liballeg
FIND_LIBRARY(LALLEG NAMES alleg PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LALLEG)
  MESSAGE(FATAL_ERROR "Fatal error: couldn't find the Allegro game programming library (liballeg)! ${RTFM}")
ELSE(NOT LALLEG)
  IF(UNIX)
    EXECUTE_PROCESS(COMMAND allegro-config --version OUTPUT_VARIABLE ALLEGRO_UNIX_VERSION)
    IF(ALLEGRO_UNIX_VERSION)
      STRING(REGEX REPLACE "[ \r\n\t]+$" "" ALLEGRO_UNIX_VERSION ${ALLEGRO_UNIX_VERSION})
    ENDIF(ALLEGRO_UNIX_VERSION)
    IF(NOT ALLEGRO_UNIX_VERSION STREQUAL ALLEGRO_RECOMMENDED_VERSION)
      MESSAGE("\nWARNING: your Allegro version is ${ALLEGRO_UNIX_VERSION}. The recommended version is ${ALLEGRO_RECOMMENDED_VERSION}.\n         ${GAME_NAME} may not compile properly!.\n")
    ENDIF(NOT ALLEGRO_UNIX_VERSION STREQUAL ALLEGRO_RECOMMENDED_VERSION)
  ENDIF(UNIX)
ENDIF(NOT LALLEG)

# loadpng: libloadpng, libpng and zlib
FIND_LIBRARY(LLOADPNG NAMES loadpng PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LLOADPNG)
  MESSAGE(FATAL_ERROR "Fatal error: libloadpng not found! ${RTFM}")
ENDIF(NOT LLOADPNG)

FIND_LIBRARY(LPNG NAMES png PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LPNG)
  MESSAGE(FATAL_ERROR "Fatal error: libpng not found! ${RTFM}")
ENDIF(NOT LPNG)

FIND_LIBRARY(LZ NAMES z PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LZ)
  MESSAGE(FATAL_ERROR "Fatal error: zlib not found! ${RTFM}")
ENDIF(NOT LZ)

# JPGalleg: libjpgalleg
FIND_LIBRARY(LJPGALLEG NAMES jpgalleg PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LJPGALLEG)
  MESSAGE(FATAL_ERROR "Fatal error: libjpgalleg not found! ${RTFM}")
ENDIF(NOT LJPGALLEG)

# Ogg/Vorbis: libogg, libvorbis, libvorbisfile
FIND_LIBRARY(LOGG NAMES ogg PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LOGG)
  MESSAGE(FATAL_ERROR "Fatal error: libogg not found! ${RTFM}")
ENDIF(NOT LOGG)

FIND_LIBRARY(LVORBIS NAMES vorbis PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LVORBIS)
  MESSAGE(FATAL_ERROR "Fatal error: libvorbis not found! ${RTFM}")
ENDIF(NOT LVORBIS)

FIND_LIBRARY(LVORBISFILE NAMES vorbisfile PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LVORBISFILE)
  MESSAGE(FATAL_ERROR "Fatal error: libvorbisfile not found! ${RTFM}")
ENDIF(NOT LVORBISFILE)

# LOGG: liblogg
IF(NOT USE_OPENAL)
  MESSAGE(STATUS "Building using Allegro for audio playback...")
  FIND_LIBRARY(LLOGG NAMES logg PATH "${CMAKE_LIBRARY_PATH}")
  IF(NOT LLOGG)
    MESSAGE(FATAL_ERROR "Fatal error: liblogg not found! ${RTFM}")
  ENDIF(NOT LLOGG)
  SET(AUDIO_LIBS logg vorbisfile vorbis ogg)
ENDIF(NOT USE_OPENAL)

# OpenAL & ALURE
IF(USE_OPENAL)
  MESSAGE(STATUS "Building using OpenAL for audio playback...")
  FIND_LIBRARY(LOPENAL NAMES openal OpenAL OpenAL32 al PATH "${CMAKE_LIBRARY_PATH}")
  IF(NOT LOPENAL)
    MESSAGE(FATAL_ERROR "Fatal error: can't find OpenAL! ${RTFM}")
  ENDIF(NOT LOPENAL)
  FIND_LIBRARY(LALURE NAMES alure-static ALURE32-static PATH "${CMAKE_LIBRARY_PATH}") # try the static version first
  IF(NOT LALURE)
    FIND_LIBRARY(LALURE NAMES alure PATH "${CMAKE_LIBRARY_PATH}")
    IF(NOT LALURE)
      MESSAGE(FATAL_ERROR "Fatal error: can't find ALURE! ${RTFM}")
    ENDIF(NOT LALURE)
  ENDIF(NOT LALURE)

  IF(UNIX)
    FIND_LIBRARY(LPTHREAD NAMES pthread PATH "${CMAKE_LIBRARY_PATH}")
    IF(NOT LPTHREAD)
      MESSAGE(FATAL_ERROR "Fatal error: can't find libpthread! ${RTFM}")
    ENDIF(NOT LPTHREAD)
    IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES ".*BSD|DragonFly.*")
      FIND_LIBRARY(LDL NAMES dl PATH "${CMAKE_LIBRARY_PATH}")
      IF(NOT LDL)
        MESSAGE(FATAL_ERROR "Fatal error: can't find libdl! ${RTFM}")
      ENDIF(NOT LDL)
    ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES ".*BSD|DragonFly.*")
  ELSE(UNIX)
    SET(LPTHREAD "") # TODO?
    SET(LDL "")
  ENDIF(UNIX)

  SET(DEFS ${DEFS} __USE_OPENAL__ ALURE_STATIC_LIBRARY)
  SET(AUDIO_LIBS ${LALURE} ${LOPENAL} vorbisfile vorbis ogg stdc++ ${LPTHREAD} ${LDL})
ENDIF(USE_OPENAL)

# Alfont: libalfont
FIND_LIBRARY(LALFONT NAMES alfont PATH "${CMAKE_LIBRARY_PATH}")
IF(NOT LALFONT)
  MESSAGE(FATAL_ERROR "Fatal error: modified alfont not found! ${RTFM}")
ENDIF(NOT LALFONT)







# RC compiler
IF(NOT CMAKE_RC_COMPILER)
  SET(CMAKE_RC_COMPILER windres)
ENDIF(NOT CMAKE_RC_COMPILER)


# pre-processor: #define's
IF(MSVC)
  FOREACH(d ${DEFS})
    SET(CFLAGS_EXTRA "${CFLAGS_EXTRA} /D${d}")
  ENDFOREACH(d)
ELSE(MSVC)
  FOREACH(d ${DEFS})
    SET(CFLAGS_EXTRA "${CFLAGS_EXTRA} -D${d}")
  ENDFOREACH(d)
ENDIF(MSVC)


# source files
SET(
  GAME_SRCS

  src/core/nanocalc/nanocalc.c
  src/core/nanocalc/nanocalc_addons.c
  src/core/nanoparser/nanoparser.c
  src/core/audio.c
  src/core/commandline.c
  src/core/engine.c
  src/core/font.c
  src/core/fontext.c
  src/core/image.c
  src/core/input.c
  src/core/inputmap.c
  src/core/lang.c
  src/core/logfile.c
  src/core/nanocalcext.c
  src/core/osspec.c
  src/core/preferences.c
  src/core/quest.c
  src/core/resourcemanager.c
  src/core/scene.c
  src/core/screenshot.c
  src/core/fadefx.c
  src/core/soundfactory.c
  src/core/sprite.c
  src/core/storyboard.c
  src/core/stringutil.c
  src/core/timer.c
  src/core/util.c
  src/core/v2d.c
  src/core/video.c
  src/core/hqx.c

  src/scenes/util/editorgrp.c
  src/scenes/util/grouptree.c
  src/scenes/confirmbox.c
  src/scenes/credits.c
  src/scenes/credits2.c
  src/scenes/editorhelp.c
  src/scenes/gameover.c
  src/scenes/intro.c
  src/scenes/langselect.c
  src/scenes/level.c
  src/scenes/options.c
  src/scenes/pause.c
  src/scenes/quest.c
  src/scenes/questselect.c
  src/scenes/stageselect.c

  src/entities/object_decorators/base/objectbasicmachine.c
  src/entities/object_decorators/base/objectdecorator.c
  src/entities/object_decorators/add_lives.c
  src/entities/object_decorators/add_collectibles.c
  src/entities/object_decorators/ask_to_leave.c
  src/entities/object_decorators/add_to_score.c
  src/entities/object_decorators/attach_to_player.c
  src/entities/object_decorators/audio.c
  src/entities/object_decorators/bounce_player.c
  src/entities/object_decorators/bullet_trajectory.c
  src/entities/object_decorators/enemy.c
  src/entities/object_decorators/change_closest_object_state.c
  src/entities/object_decorators/children.c
  src/entities/object_decorators/clear_level.c
  src/entities/object_decorators/create_item.c
  src/entities/object_decorators/destroy.c
  src/entities/object_decorators/elliptical_trajectory.c
  src/entities/object_decorators/showhide.c
  src/entities/object_decorators/gravity.c
  src/entities/object_decorators/jump.c
  src/entities/object_decorators/lock_camera.c
  src/entities/object_decorators/look.c
  src/entities/object_decorators/mosquito_movement.c
  src/entities/object_decorators/move_player.c
  src/entities/object_decorators/next_level.c
  src/entities/object_decorators/on_event.c
  src/entities/object_decorators/return_to_previous_state.c
  src/entities/object_decorators/hit_player.c
  src/entities/object_decorators/kill_player.c
  src/entities/object_decorators/pause.c
  src/entities/object_decorators/player_movement.c
  src/entities/object_decorators/player_action.c
  src/entities/object_decorators/set_absolute_position.c
  src/entities/object_decorators/set_alpha.c
  src/entities/object_decorators/set_angle.c
  src/entities/object_decorators/set_scale.c
  src/entities/object_decorators/set_animation.c
  src/entities/object_decorators/set_obstacle.c
  src/entities/object_decorators/set_player_speed.c
  src/entities/object_decorators/set_player_animation.c
  src/entities/object_decorators/set_player_position.c
  src/entities/object_decorators/set_player_inputmap.c
  src/entities/object_decorators/dialog_box.c
  src/entities/object_decorators/walk.c
  src/entities/object_decorators/observe_player.c
  src/entities/object_decorators/variables.c
  src/entities/object_decorators/set_zindex.c
  src/entities/object_decorators/textout.c
  src/entities/object_decorators/simulate_button.c
  src/entities/object_decorators/switch_character.c
  src/entities/object_decorators/load_level.c
  src/entities/object_decorators/restart_level.c
  src/entities/object_decorators/save_level.c
  src/entities/object_decorators/camera_focus.c
  src/entities/object_decorators/execute.c
  src/entities/object_decorators/launch_url.c
  src/entities/object_decorators/quest.c
  src/entities/object_decorators/reset_globals.c

  src/entities/items/util/itemutil.c
  src/entities/items/animal.c
  src/entities/items/animalprison.c
  src/entities/items/bigring.c
  src/entities/items/bouncingcollectible.c
  src/entities/items/bumper.c
  src/entities/items/checkpointorb.c
  src/entities/items/collectible.c
  src/entities/items/crushedbox.c
  src/entities/items/danger.c
  src/entities/items/dnadoor.c
  src/entities/items/door.c
  src/entities/items/endsign.c
  src/entities/items/explosion.c
  src/entities/items/flyingtext.c
  src/entities/items/goalsign.c
  src/entities/items/icon.c
  src/entities/items/itembox.c
  src/entities/items/loop.c
  src/entities/items/old_loop.c
  src/entities/items/spikes.c
  src/entities/items/spring.c
  src/entities/items/supercollectible.c
  src/entities/items/switch.c
  src/entities/items/teleporter.c

  src/entities/physics/obstacle.c
  src/entities/physics/obstaclemap.c
  src/entities/physics/physicsactor.c
  src/entities/physics/sensor.c
  src/entities/physics/sensorstate.c

  src/entities/actor.c
  src/entities/background.c
  src/entities/brick.c
  src/entities/camera.c
  src/entities/character.c
  src/entities/enemy.c
  src/entities/entitymanager.c
  src/entities/object_compiler.c
  src/entities/object_vm.c
  src/entities/item.c
  src/entities/player.c
  src/entities/particle.c
  src/entities/collisionmask.c
  src/entities/renderqueue.c

  src/main.c
)



# *nix executable
IF(UNIX)
  SET(GAME_SRCS ${GAME_SRCS} src/misc/iconlin.c)
  ADD_EXECUTABLE(${GAME_UNIXNAME} ${GAME_SRCS})
  SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES LINK_FLAGS ${ALLEGRO_UNIX_LIBS})
  TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${AUDIO_LIBS} jpgalleg loadpng png z alfont alleg)
  SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall -O2 ${CFLAGS} ${CFLAGS_EXTRA}")
ENDIF(UNIX)



# Windows executable
IF(WIN32)

  # MSVC fix
  IF(MSVC)
    SET(
      GAME_SRCS
      ${GAME_SRCS}

      src/core/nanocalc/nanocalc.h
      src/core/nanocalc/nanocalc_addons.h
      src/core/nanoparser/nanoparser.h
      src/core/audio.h
      src/core/commandline.h
      src/core/engine.h
      src/core/font.h
      src/core/fontext.h
      src/core/global.h
      src/core/hashtable.h
      src/core/image.h
      src/core/input.h
      src/core/inputmap.h
      src/core/lang.h
      src/core/logfile.h
      src/core/nanocalcext.h
      src/core/osspec.h
      src/core/preferences.h
      src/core/quest.h
      src/core/resourcemanager.h
      src/core/scene.h
      src/core/screenshot.h
      src/core/fadefx.h
      src/core/soundfactory.h
      src/core/spatialhash.h
      src/core/sprite.h
      src/core/storyboard.h
      src/core/stringutil.h
      src/core/timer.h
      src/core/util.h
      src/core/video.h
      src/core/v2d.h
      src/core/hqx.h

      src/scenes/util/editorgrp.h
      src/scenes/util/grouptree.h
      src/scenes/confirmbox.h
      src/scenes/editorhelp.h
      src/scenes/credits.h
      src/scenes/credits2.h
      src/scenes/gameover.h
      src/scenes/intro.h
      src/scenes/langselect.h
      src/scenes/level.h
      src/scenes/options.h
      src/scenes/pause.h
      src/scenes/quest.h
      src/scenes/questselect.h
      src/scenes/stageselect.h

      src/entities/object_decorators/base/objectbasicmachine.h
      src/entities/object_decorators/base/objectdecorator.h
      src/entities/object_decorators/base/objectmachine.h
      src/entities/object_decorators/add_lives.h
      src/entities/object_decorators/add_collectibles.h
      src/entities/object_decorators/ask_to_leave.h
      src/entities/object_decorators/add_to_score.h
      src/entities/object_decorators/attach_to_player.h
      src/entities/object_decorators/audio.h
      src/entities/object_decorators/bounce_player.h
      src/entities/object_decorators/bullet_trajectory.h
      src/entities/object_decorators/enemy.h
      src/entities/object_decorators/change_closest_object_state.h
      src/entities/object_decorators/children.h
      src/entities/object_decorators/clear_level.h
      src/entities/object_decorators/create_item.h
      src/entities/object_decorators/destroy.h
      src/entities/object_decorators/elliptical_trajectory.h
      src/entities/object_decorators/showhide.h
      src/entities/object_decorators/gravity.h
      src/entities/object_decorators/jump.h
      src/entities/object_decorators/lock_camera.h
      src/entities/object_decorators/look.h
      src/entities/object_decorators/mosquito_movement.h
      src/entities/object_decorators/move_player.h
      src/entities/object_decorators/next_level.h
      src/entities/object_decorators/on_event.h
      src/entities/object_decorators/return_to_previous_state.h
      src/entities/object_decorators/hit_player.h
      src/entities/object_decorators/kill_player.h
      src/entities/object_decorators/pause.h
      src/entities/object_decorators/player_movement.h
      src/entities/object_decorators/player_action.h
      src/entities/object_decorators/set_absolute_position.h
      src/entities/object_decorators/set_alpha.h
      src/entities/object_decorators/set_angle.h
      src/entities/object_decorators/set_scale.h
      src/entities/object_decorators/set_animation.h
      src/entities/object_decorators/set_obstacle.h
      src/entities/object_decorators/set_player_speed.h
      src/entities/object_decorators/set_player_animation.h
      src/entities/object_decorators/set_player_position.h
      src/entities/object_decorators/set_player_inputmap.h
      src/entities/object_decorators/dialog_box.h
      src/entities/object_decorators/walk.h
      src/entities/object_decorators/observe_player.h
      src/entities/object_decorators/variables.h
      src/entities/object_decorators/set_zindex.h
      src/entities/object_decorators/textout.h
      src/entities/object_decorators/simulate_button.h
      src/entities/object_decorators/switch_character.h
      src/entities/object_decorators/load_level.h
      src/entities/object_decorators/restart_level.h
      src/entities/object_decorators/save_level.h
      src/entities/object_decorators/camera_focus.h
      src/entities/object_decorators/execute.h
      src/entities/object_decorators/launch_url.h
      src/entities/object_decorators/quest.h
      src/entities/object_decorators/reset_globals.h

      src/entities/items/util/itemutil.h
      src/entities/items/animal.h
      src/entities/items/animalprison.h
      src/entities/items/bigring.h
      src/entities/items/bouncingcollectible.h
      src/entities/items/bumper.h
      src/entities/items/checkpointorb.h
      src/entities/items/collectible.h
      src/entities/items/crushedbox.h
      src/entities/items/danger.h
      src/entities/items/dnadoor.h
      src/entities/items/door.h
      src/entities/items/endsign.h
      src/entities/items/explosion.h
      src/entities/items/flyingtext.h
      src/entities/items/goalsign.h
      src/entities/items/icon.h
      src/entities/items/itembox.h
      src/entities/items/loop.h
      src/entities/items/old_loop.h
      src/entities/items/spikes.h
      src/entities/items/spring.h
      src/entities/items/supercollectible.h
      src/entities/items/switch.h
      src/entities/items/teleporter.h

      src/entities/physics/obstacle.h
      src/entities/physics/obstaclemap.h
      src/entities/physics/physicsactor.h
      src/entities/physics/sensor.h
      src/entities/physics/sensorstate.h

      src/entities/actor.h
      src/entities/background.h
      src/entities/brick.h
      src/entities/camera.h
      src/entities/character.h
      src/entities/enemy.h
      src/entities/entitymanager.h
      src/entities/object_compiler.h
      src/entities/object_vm.h
      src/entities/item.h
      src/entities/player.h
      src/entities/particle.h
      src/entities/collisionmask.h
      src/entities/renderqueue.h

      src/misc/iconwin.rc
    )
  ENDIF(MSVC)

  # Executables
  ADD_EXECUTABLE(${GAME_UNIXNAME} WIN32 ${GAME_SRCS})

  # Other properties
  IF(MSVC)
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "/D_CRT_SECURE_NO_DEPRECATE /D__WIN32__ /D__MSVC__ ${CFLAGS} ${CFLAGS_EXTRA}")
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} ${AUDIO_LIBS} jpgalleg loadpng alfont alleg png z)
  ELSEIF(MINGW)
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall -O2 -D__WIN32__ ${CFLAGS} ${CFLAGS_EXTRA}")
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} ${AUDIO_LIBS} jpgalleg loadpng alfont alleg png z m)
    EXECUTE_PROCESS(COMMAND ${CMAKE_RC_COMPILER} -O coff -o "${CMAKE_SOURCE_DIR}/src/misc/iconwin.res" -i "${CMAKE_SOURCE_DIR}/src/misc/iconwin.rc" -I "${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES LINK_FLAGS "-static-libgcc \"${CMAKE_SOURCE_DIR}/src/misc/iconwin.res\"")
  ELSE(MSVC)
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-D__WIN32__ ${CFLAGS} ${CFLAGS_EXTRA} ${CMAKE_C_FLAGS}")
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} ${AUDIO_LIBS} jpgalleg loadpng alfont alleg png z m)
  ENDIF(MSVC)
ENDIF(WIN32)



# Misc
SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES PROJECT_NAME "${GAME_NAME}")
SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")



# Installing on *nix
IF(UNIX)
  INSTALL(CODE "MESSAGE(\"Installing ${GAME_NAME} ${GAME_VERSION} at ${GAME_UNIX_INSTALLDIR}... Please make sure you have enough privileges.\")")
  INSTALL(TARGETS ${GAME_UNIXNAME} RUNTIME DESTINATION ${GAME_UNIX_INSTALLDIR})
  INSTALL(FILES license.txt readme.html logo.png surge.png DESTINATION ${GAME_UNIX_INSTALLDIR})
  INSTALL(DIRECTORY characters objects sprites config images levels licenses musics quests samples screenshots themes languages fonts ttf DESTINATION ${GAME_UNIX_INSTALLDIR} PATTERN ".svn" EXCLUDE)

  INSTALL(CODE "MESSAGE(\"Copying executable to ${GAME_UNIX_EXECDIR}...\")")
  INSTALL(CODE "EXECUTE_PROCESS(COMMAND \"cmake\" \"-E\" \"copy\" \"${GAME_UNIX_INSTALLDIR}/${GAME_UNIXNAME}\" \"${GAME_UNIX_EXECDIR}/${GAME_UNIXNAME}\")")

  INSTALL(CODE "MESSAGE(\"Done! Please run ${GAME_UNIXNAME} to start ${GAME_NAME}.\")")
ENDIF(UNIX)
