#
# Open Surge Engine
# CMakeLists.txt - CMake script
# Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
# http://opensurge2d.org
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

# ------------------------------------------
# Prelude
# ------------------------------------------

# Initializing...
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(opensurge)
SET(GAME_UNIXNAME "${CMAKE_PROJECT_NAME}")
SET(GAME_NAME "Open Surge")
FUNCTION(READ_VERSION VERSION_STRING)
  SET(VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/core/global.h")
  SET(VERSION_MAIN_REGEX "#define[ \t]+GAME_VERSION_SUP[ \t]+(.+)")
  SET(VERSION_MAJOR_REGEX "#define[ \t]+GAME_VERSION_SUB[ \t]+(.+)")
  SET(VERSION_MINOR_REGEX "#define[ \t]+GAME_VERSION_WIP[ \t]+(.+)")
  SET(VERSION_PATCH_REGEX "#define[ \t]+GAME_VERSION_FIX[ \t]+(.+)")
  FILE(STRINGS "${VERSION_FILE}" GAME_VERSION_SUP REGEX "${VERSION_MAIN_REGEX}")
  FILE(STRINGS "${VERSION_FILE}" GAME_VERSION_SUB REGEX "${VERSION_MAJOR_REGEX}")
  FILE(STRINGS "${VERSION_FILE}" GAME_VERSION_WIP REGEX "${VERSION_MINOR_REGEX}")
  FILE(STRINGS "${VERSION_FILE}" GAME_VERSION_FIX REGEX "${VERSION_PATCH_REGEX}")
  STRING(REGEX REPLACE ${VERSION_MAIN_REGEX} "\\1" GAME_VERSION_SUP "${GAME_VERSION_SUP}")
  STRING(REGEX REPLACE ${VERSION_MAJOR_REGEX} "\\1" GAME_VERSION_SUB "${GAME_VERSION_SUB}")
  STRING(REGEX REPLACE ${VERSION_MINOR_REGEX} "\\1" GAME_VERSION_WIP "${GAME_VERSION_WIP}")
  STRING(REGEX REPLACE ${VERSION_PATCH_REGEX} "\\1" GAME_VERSION_FIX "${GAME_VERSION_FIX}")
  SET(${VERSION_STRING} "${GAME_VERSION_SUP}.${GAME_VERSION_SUB}.${GAME_VERSION_WIP}.${GAME_VERSION_FIX}" PARENT_SCOPE)
ENDFUNCTION()
READ_VERSION(GAME_VERSION)
MESSAGE("${GAME_NAME} version ${GAME_VERSION}")
SET(HELP "You might get some luck by tweaking CMake options with ccmake | cmake-gui, or you can get help at opensurge2d.org")

# Default config
SET(CMAKE_C_STANDARD 99)
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug | Release | MinSizeRel | RelWithDebInfo" FORCE)
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()
SET(DEFS "") # #define'd stuff

# User options
OPTION(SURGESCRIPT_STATIC "Link SurgeScript statically" OFF)
OPTION(ALLEGRO_STATIC "Use the static version of Allegro 5 (Windows only)" OFF)
OPTION(ALLEGRO_MONOLITH "Use the monolith version of Allegro 5" OFF)
IF(UNIX)
  OPTION(DESKTOP_INSTALL "Add Open Surge to the Desktop menu when installing" ON)
ENDIF()

# User-specified paths
SET(ALLEGRO_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}" CACHE PATH "Where to look for Allegro & its dependencies")
SET(ALLEGRO_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}" CACHE PATH "Where to look for the header files of Allegro")
SET(SURGESCRIPT_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}" CACHE PATH "Where to look for SurgeScript")
SET(SURGESCRIPT_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}" CACHE PATH "Where to look for the header files of SurgeScript")
IF(UNIX AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install path prefix (prepended onto install directories)" FORCE)
ENDIF()
IF(UNIX)
  SET(GAME_BINDIR "games" CACHE PATH "Directory in which the game executable will be stored when installing")
  SET(GAME_DATADIR "share/games/${GAME_UNIXNAME}" CACHE PATH "Directory in which the game data will be stored when installing")
  SET(DESKTOP_ICON_PATH "/usr/share/pixmaps" CACHE PATH "Where to store the icon file when installing")
  SET(DESKTOP_ENTRY_PATH "/usr/share/applications" CACHE PATH "Where to store the .desktop file when installing")
  SET(DESKTOP_METAINFO_PATH "/usr/share/metainfo" CACHE PATH "Where to store the metainfo file. You shouldn't change this (check the AppStream spec).")
ENDIF()

# Compiling the development version?
OPTION(BLEEDING_EDGE "Enable if using the development version" OFF)
SET(BLEEDING_EDGE_VERSION "custom" CACHE STRING "Build version (bleeding edge)")
IF(BLEEDING_EDGE)
  SET(DEFS "${DEFS}" "GAME_BUILD_VERSION=${BLEEDING_EDGE_VERSION}-dev")
ENDIF()

# data folder (game assets)
IF(UNIX)
  IF(IS_ABSOLUTE "${GAME_DATADIR}")
    SET(DEFS "${DEFS}" "GAME_DATADIR=\"${GAME_DATADIR}\"")
  ELSEIF(IS_ABSOLUTE "${CMAKE_INSTALL_PREFIX}/${GAME_DATADIR}")
    SET(DEFS "${DEFS}" "GAME_DATADIR=\"${CMAKE_INSTALL_PREFIX}/${GAME_DATADIR}\"")
  ELSE()
    MESSAGE(FATAL_ERROR "GAME_DATADIR is not an absolute path: ${CMAKE_INSTALL_PREFIX}/${GAME_DATADIR}")
  ENDIF()
ENDIF()

# Generate files from templates
FUNCTION(generate_file TEMPLATE)
  CONFIGURE_FILE("src/misc/${TEMPLATE}.in" "src/misc/${TEMPLATE}" @ONLY)
ENDFUNCTION()

# Generate the content of the credits screen
SET(COPYRIGHT_DATA_CSV "")
FILE(STRINGS "src/misc/copyright_data.csv" COPYRIGHT_DATA_CSV_LIST ENCODING UTF-8)
FOREACH(line IN LISTS COPYRIGHT_DATA_CSV_LIST)
  SET(COPYRIGHT_DATA_CSV "${COPYRIGHT_DATA_CSV}\n\"${line}\\n\"")
ENDFOREACH()
generate_file("credits.c")

# Generate desktop & metadata files
# They require the absolute path of the icon
IF(DEFINED DESKTOP_ICON_PATH)
  SET(DESKTOP_ICON_FULLPATH "${DESKTOP_ICON_PATH}/opensurge.png")
  generate_file("opensurge.desktop")
  generate_file("opensurge.appdata.xml")
ENDIF()

# ------------------------------------------
# Finding the required libraries
# ------------------------------------------

# Find Allegro 5 & addons
SET(REQUIRED_A5_LIBS
allegro
allegro_image
allegro_primitives
allegro_font
allegro_ttf
allegro_acodec
allegro_audio
allegro_dialog
allegro_memfile
)

# OSX requires allegro_main
IF(APPLE)
    LIST(APPEND REQUIRED_A5_LIBS allegro_main)
ENDIF()

# Use the monolith version
IF(ALLEGRO_MONOLITH)
    SET(REQUIRED_A5_LIBS allegro_monolith)
ENDIF()

# Use static libs
IF(ALLEGRO_STATIC)

    # add -static suffix
    SET(A5_LIBS "")
    FOREACH(A5_LIB ${REQUIRED_A5_LIBS})
        LIST(APPEND A5_LIBS "${A5_LIB}-static")
    ENDFOREACH()
    SET(REQUIRED_A5_LIBS ${A5_LIBS})

    # define ALLEGRO_STATICLINK
    LIST(APPEND DEFS "ALLEGRO_STATICLINK")

    # require Win32 libs
    IF(WIN32)
        LIST(APPEND REQUIRED_A5_LIBS
        mingw32
        #dumb
        #FLAC
        vorbisfile
        vorbis
        freetype
        ogg
        #physfs
        png16
        z
        #zlibstatic
        gdiplus
        uuid
        kernel32
        winmm
        psapi
        opengl32
        glu32
        user32
        comdlg32
        gdi32
        shell32
        ole32
        advapi32
        ws2_32
        shlwapi
        dsound
        #d3dx9 # needed?
        jpeg
        #opusfile
        #opus
        #webp
        )
    ELSE()
        MESSAGE(FATAL_ERROR "Static linking is currently unavailable for this platform.")
    ENDIF()

ENDIF()

# Find the header files of Allegro
FIND_PATH(ALLEGRO_INCDIR NAMES "allegro5/allegro.h" PATHS ${ALLEGRO_INCLUDE_PATH})
IF(NOT ALLEGRO_INCDIR)
  MESSAGE(FATAL_ERROR "Can't find allegro.h. ${HELP}")
ELSE()
  MESSAGE(STATUS "Found allegro.h at ${ALLEGRO_INCDIR}/allegro5")
ENDIF()

# Write to LALLEGRO5
SET(LALLEGRO5 "")
FOREACH(A5_LIB ${REQUIRED_A5_LIBS})
STRING(TOUPPER "L${A5_LIB}" _A5_LIB)
FIND_LIBRARY(${_A5_LIB} NAMES "${A5_LIB}" PATHS ${ALLEGRO_LIBRARY_PATH})
IF(NOT ${_A5_LIB})
    MESSAGE(FATAL_ERROR "Can't find Allegro 5 lib (lib${A5_LIB}). ${HELP}")
ELSE()
    MESSAGE(STATUS "Found lib${A5_LIB} at ${${_A5_LIB}}...")
    SET(LALLEGRO5 "${LALLEGRO5}" "${${_A5_LIB}}")
ENDIF()
ENDFOREACH()

# SurgeScript: libsurgescript
FIND_PATH(SURGESCRIPT_INCDIR NAMES "surgescript.h" PATHS ${SURGESCRIPT_INCLUDE_PATH})
IF(NOT SURGESCRIPT_INCDIR)
  MESSAGE(FATAL_ERROR "Can't find surgescript.h! ${HELP}")
ELSE()
  MESSAGE(STATUS "Found surgescript.h at ${SURGESCRIPT_INCDIR}")
ENDIF()

SET(LSURGESCRIPT_NAME surgescript)
IF(SURGESCRIPT_STATIC)
  SET(LSURGESCRIPT_NAME "${LSURGESCRIPT_NAME}-static")
ENDIF()

FIND_LIBRARY(LSURGESCRIPT NAMES "${LSURGESCRIPT_NAME}" PATHS ${SURGESCRIPT_LIBRARY_PATH})
IF(NOT LSURGESCRIPT)
  MESSAGE(FATAL_ERROR "Can't find lib${LSURGESCRIPT_NAME}! ${HELP}")
ELSE()
  MESSAGE(STATUS "Found lib${LSURGESCRIPT_NAME} at ${LSURGESCRIPT}...")
ENDIF()

# setup preprocessor #define's
IF(MSVC)
  FOREACH(d ${DEFS})
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D${d}")
  ENDFOREACH()
ELSE()
  FOREACH(d ${DEFS})
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D'${d}'")
  ENDFOREACH()
ENDIF()

# ------------------------------------------
# Listing the source files
# ------------------------------------------

# adding sources
SET(GAME_SRCS
  src/core/nanoparser/nanoparser.c
  src/core/whereami/whereami.c
  src/core/utf8/utf8.c
  src/core/zip/zip.c
  src/core/assetfs.c
  src/core/audio.c
  src/core/color.c
  src/core/commandline.c
  src/core/csv.c
  src/core/engine.c
  src/core/font.c
  src/core/image.c
  src/core/input.c
  src/core/inputmap.c
  src/core/install.c
  src/core/lang.c
  src/core/logfile.c
  src/core/modmanager.c
  src/core/web.c
  src/core/prefs.c
  src/core/quest.c
  src/core/resourcemanager.c
  src/core/scene.c
  src/core/screenshot.c
  src/core/fadefx.c
  src/core/sprite.c
  src/core/storyboard.c
  src/core/stringutil.c
  src/core/timer.c
  src/core/util.c
  src/core/v2d.c
  src/core/video.c

  src/scenes/util/editorcmd.c
  src/scenes/util/editorgrp.c
  src/scenes/util/grouptree.c
  src/scenes/util/levparser.c
  src/scenes/confirmbox.c
  src/scenes/credits.c
  src/scenes/editorhelp.c
  src/scenes/editorpal.c
  src/scenes/gameover.c
  src/scenes/intro.c
  src/scenes/langselect.c
  src/scenes/level.c
  src/scenes/options.c
  src/scenes/pause.c
  src/scenes/quest.c
  src/scenes/stageselect.c

  src/scripting/scripting.c
  src/scripting/application.c
  src/scripting/surgeengine.c
  src/scripting/actor.c
  src/scripting/animation.c
  src/scripting/brick.c
  src/scripting/camera.c
  src/scripting/collisions.c
  src/scripting/console.c
  src/scripting/events.c
  src/scripting/input.c
  src/scripting/lang.c
  src/scripting/level.c
  src/scripting/levelmanager.c
  src/scripting/mouse.c
  src/scripting/music.c
  src/scripting/obstaclemap.c
  src/scripting/player.c
  src/scripting/prefs.c
  src/scripting/screen.c
  src/scripting/sensor.c
  src/scripting/sound.c
  src/scripting/text.c
  src/scripting/time.c
  src/scripting/transform.c
  src/scripting/vector2.c
  src/scripting/web.c
  src/physics/obstacle.c
  src/physics/obstaclemap.c
  src/physics/physicsactor.c
  src/physics/sensor.c
  src/physics/sensorstate.c
  src/physics/collisionmask.c

  src/entities/legacy/item.c
  src/entities/legacy/enemy.c
  src/entities/legacy/object_compiler.c
  src/entities/legacy/object_decorators.c
  src/entities/legacy/object_machine.c
  src/entities/legacy/object_vm.c
  src/entities/legacy/nanocalc/nanocalc.c
  src/entities/legacy/nanocalc/nanocalc_addons.c
  src/entities/legacy/nanocalc/nanocalcext.c

  src/entities/actor.c
  src/entities/background.c
  src/entities/brick.c
  src/entities/camera.c
  src/entities/character.c
  src/entities/entitymanager.c
  src/entities/player.c
  src/entities/particle.c
  src/entities/renderqueue.c

  src/misc/icon.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/misc/credits.c

  src/main.c
)

# adding headers
SET(GAME_HEADERS
  src/core/nanoparser/nanoparser.h
  src/core/whereami/whereami.h
  src/core/utf8/utf8.h
  src/core/zip/zip.c
  src/core/assetfs.h
  src/core/audio.h
  src/core/color.h
  src/core/commandline.h
  src/core/csv.h
  src/core/engine.h
  src/core/fasthash.h
  src/core/font.h
  src/core/global.h
  src/core/hashtable.h
  src/core/image.h
  src/core/input.h
  src/core/inputmap.h
  src/core/install.h
  src/core/lang.h
  src/core/logfile.h
  src/core/modmanager.h
  src/core/web.h
  src/core/prefs.h
  src/core/quest.h
  src/core/resourcemanager.h
  src/core/scene.h
  src/core/screenshot.h
  src/core/fadefx.h
  src/core/spatialhash.h
  src/core/sprite.h
  src/core/storyboard.h
  src/core/stringutil.h
  src/core/timer.h
  src/core/util.h
  src/core/video.h
  src/core/v2d.h

  src/scenes/util/editorcmd.h
  src/scenes/util/editorgrp.h
  src/scenes/util/grouptree.h
  src/scenes/util/levparser.h
  src/scenes/confirmbox.h
  src/scenes/editorhelp.h
  src/scenes/editorpal.h
  src/scenes/credits.h
  src/scenes/gameover.h
  src/scenes/intro.h
  src/scenes/langselect.h
  src/scenes/level.h
  src/scenes/options.h
  src/scenes/pause.h
  src/scenes/quest.h
  src/scenes/stageselect.h

  src/scripting/scripting.h
  src/physics/obstacle.h
  src/physics/obstaclemap.h
  src/physics/physicsactor.h
  src/physics/sensor.h
  src/physics/sensorstate.h
  src/physics/collisionmask.h

  src/entities/legacy/item.h
  src/entities/legacy/enemy.h
  src/entities/legacy/object_compiler.h
  src/entities/legacy/object_decorators.h
  src/entities/legacy/object_machine.h
  src/entities/legacy/object_vm.h
  src/entities/legacy/nanocalc/nanocalc.h
  src/entities/legacy/nanocalc/nanocalc_addons.h
  src/entities/legacy/nanocalc/nanocalcext.h

  src/entities/actor.h
  src/entities/background.h
  src/entities/brick.h
  src/entities/camera.h
  src/entities/character.h
  src/entities/entitymanager.h
  src/entities/player.h
  src/entities/particle.h
  src/entities/renderqueue.h
  src/entities/sfx.h

  src/misc/iconwin.rc
)

# ------------------------------------------
# Creating the executable
# ------------------------------------------

IF(WIN32)

    # Windows executable
    IF(MINGW)
      IF(NOT CMAKE_RC_COMPILER)
        SET(CMAKE_RC_COMPILER windres)
      ENDIF(NOT CMAKE_RC_COMPILER)
      ADD_EXECUTABLE(${GAME_UNIXNAME} WIN32 ${GAME_SRCS})
      TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${LSURGESCRIPT} ${LALLEGRO5})
      TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${ALLEGRO_INCDIR})
      SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")
      EXECUTE_PROCESS(COMMAND ${CMAKE_RC_COMPILER} -O coff -o "${CMAKE_CURRENT_BINARY_DIR}/iconwin.res" -i "${CMAKE_SOURCE_DIR}/src/misc/iconwin.rc" -I "${CMAKE_SOURCE_DIR}")
      SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ \"${CMAKE_CURRENT_BINARY_DIR}/iconwin.res\"")
    ELSEIF(MSVC)
      ADD_EXECUTABLE(${GAME_UNIXNAME} WIN32 ${GAME_SRCS} ${GAME_HEADERS})
      TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} ${LSURGESCRIPT} ${LALLEGRO5})
      TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${ALLEGRO_INCDIR})
      SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "/D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS ${CMAKE_C_FLAGS}")
    ELSE()
      MESSAGE("*** Unrecognized compiler ***") # e.g., clang?
      ADD_EXECUTABLE(${GAME_UNIXNAME} WIN32 ${GAME_SRCS})
      TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${LSURGESCRIPT} ${LALLEGRO5})
      TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${ALLEGRO_INCDIR})
      SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")     
    ENDIF()

    # Bugfix
    IF(ALLEGRO_STATIC)
      SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES LINKER_LANGUAGE CXX)
    ENDIF()

ELSEIF(UNIX)

    # *nix executable
    ADD_EXECUTABLE(${GAME_UNIXNAME} ${GAME_SRCS})
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${LSURGESCRIPT} ${LALLEGRO5})
    TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${ALLEGRO_INCDIR})
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")

ENDIF()

# Target properties
SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES PROJECT_NAME "${GAME_NAME}")
SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Installing on *nix
IF(UNIX)
  INSTALL(CODE "MESSAGE(\"Installing ${GAME_NAME} ${GAME_VERSION}... Make sure that you have the appropriate privileges.\")")
  INSTALL(FILES LICENSE README.md CHANGES.md CONTRIBUTING.md logo.png surge.png surge.rocks DESTINATION "${GAME_DATADIR}")
  INSTALL(DIRECTORY characters scripts sprites images levels musics quests samples scripts themes languages fonts inputs licenses DESTINATION "${GAME_DATADIR}" PATTERN ".git" EXCLUDE)
  INSTALL(FILES src/misc/opensurge.png DESTINATION "${DESKTOP_ICON_PATH}")
  INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/src/misc/opensurge.appdata.xml" DESTINATION "${DESKTOP_METAINFO_PATH}")
  IF(DESKTOP_INSTALL)
    INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/src/misc/opensurge.desktop" DESTINATION "${DESKTOP_ENTRY_PATH}")
  ENDIF()
  INSTALL(TARGETS "${GAME_UNIXNAME}" RUNTIME DESTINATION "${GAME_BINDIR}")

  # Success!
  IF(IS_ABSOLUTE "${GAME_BINDIR}")
    INSTALL(CODE "MESSAGE(\"Installed ${GAME_UNIXNAME} to ${GAME_BINDIR}.\")")
  ELSE()
    INSTALL(CODE "MESSAGE(\"Installed ${GAME_UNIXNAME} to ${CMAKE_INSTALL_PREFIX}/${GAME_BINDIR}.\")")
  ENDIF()
ENDIF()
