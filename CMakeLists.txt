#
# Open Surge Engine
# CMakeLists.txt - CMake script
# Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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
SET(GAME_NAME "Open Surge Engine")
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
SET(HELP "Tweak the CMake options or get help at opensurge2d.org")

# Default config
SET(CMAKE_C_STANDARD 99)
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug | Release | MinSizeRel | RelWithDebInfo" FORCE)
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

# Set the build type & version (optional; this helps identify the platform and the development builds)
SET(GAME_BUILD_VERSION "" CACHE STRING "Build type & version (optional)")
IF(NOT GAME_BUILD_VERSION STREQUAL "")
  LIST(APPEND DEFS "GAME_BUILD_VERSION=\"${GAME_BUILD_VERSION}\"")
ENDIF()

# Set the build date
OPTION(WANT_BUILD_DATE "Include the build date in the binary (disable for reproducible builds)" ON)
IF(WANT_BUILD_DATE)
  LIST(APPEND DEFS "GAME_BUILD_DATE=__DATE__")
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
  SET(GAME_USERDIRNAME "${GAME_UNIXNAME}" CACHE PATH "Name of the user-modifiable asset directory")
  SET(DESKTOP_ICON_PATH "/usr/share/pixmaps" CACHE PATH "Where to store the icon file when installing")
  SET(DESKTOP_ENTRY_PATH "/usr/share/applications" CACHE PATH "Where to store the .desktop file when installing")
  SET(DESKTOP_METAINFO_PATH "/usr/share/metainfo" CACHE PATH "Where to store the metainfo file. You shouldn't change this (check the AppStream spec).")
ENDIF()

# Run in place? (i.e., read assets from the directory of the executable)
OPTION(GAME_RUNINPLACE "Read assets only from the directory of the executable. For testing & development." OFF)
IF(GAME_RUNINPLACE)
  LIST(APPEND DEFS "GAME_RUNINPLACE=1")
  MESSAGE(STATUS "Detected the RUN IN PLACE flag")
ENDIF()

# data folders (game assets)
IF(UNIX)
  LIST(APPEND DEFS "GAME_USERDIRNAME=\"${GAME_USERDIRNAME}\"")
  IF(IS_ABSOLUTE "${GAME_DATADIR}")
    LIST(APPEND DEFS "GAME_DATADIR=\"${GAME_DATADIR}\"")
  ELSEIF(IS_ABSOLUTE "${CMAKE_INSTALL_PREFIX}/${GAME_DATADIR}")
    LIST(APPEND DEFS "GAME_DATADIR=\"${CMAKE_INSTALL_PREFIX}/${GAME_DATADIR}\"")
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
  allegro_main
  allegro_image
  allegro_primitives
  allegro_font
  allegro_ttf
  allegro_acodec
  allegro_audio
  allegro_dialog
  allegro_memfile
  allegro_physfs
)

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
      jpeg
      d3d9
      dinput8
      dsound
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
    MESSAGE(FATAL_ERROR "Can't find lib${A5_LIB}. ${HELP}")
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

# PhysicsFS: libphysfs
FIND_PATH(PHYSFS_INCDIR NAMES "physfs.h" PATHS ${PHYSFS_INCLUDE_PATH})
IF(NOT PHYSFS_INCDIR)
  MESSAGE(FATAL_ERROR "Can't find physfs.h! ${HELP}")
ELSE()
  MESSAGE(STATUS "Found physfs.h at ${PHYSFS_INCDIR}")
ENDIF()

FIND_LIBRARY(LPHYSFS NAMES "physfs" PATHS ${PHYSFS_LIBRARY_PATH})
IF(NOT LPHYSFS)
  MESSAGE(FATAL_ERROR "Can't find libphysfs! ${HELP}")
ELSE()
  MESSAGE(STATUS "Found libphysfs at ${LPHYSFS}...")
ENDIF()





# ------------------------------------------
# Listing the source files
# ------------------------------------------

# adding sources
SET(GAME_SRCS
  src/core/nanoparser.c
  src/core/physfs/ignorecase.c
  src/core/asset.c
  src/core/audio.c
  src/core/color.c
  src/core/commandline.c
  src/core/engine.c
  src/core/font.c
  src/core/image.c
  src/core/import.c
  src/core/input.c
  src/core/inputmap.c
  src/core/lang.c
  src/core/logfile.c
  src/core/web.c
  src/core/prefs.c
  src/core/quest.c
  src/core/resourcemanager.c
  src/core/scene.c
  src/core/screenshot.c
  src/core/fadefx.c
  src/core/sprite.c
  src/core/storyboard.c
  src/core/timer.c
  src/core/video.c

  src/util/csv.c
  src/util/fasthash.c
  src/util/iterator.c
  src/util/numeric.c
  src/util/stringutil.c
  src/util/utf8.c
  src/util/util.c
  src/util/v2d.c

  src/scenes/mobile/subscenes/info.c
  src/scenes/mobile/subscenes/credits.c
  src/scenes/mobile/subscenes/screenshot.c
  src/scenes/mobile/util/touch.c
  src/scenes/mobile/menu.c
  src/scenes/mobile/popup.c
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

  src/scripting/util/iterators.c
  src/scripting/scripting.c
  src/scripting/application.c
  src/scripting/surgeengine.c
  src/scripting/actor.c
  src/scripting/androidplatform.c
  src/scripting/animation.c
  src/scripting/brick.c
  src/scripting/brickparticle.c
  src/scripting/camera.c
  src/scripting/collisions.c
  src/scripting/console.c
  src/scripting/entitycontainer.c
  src/scripting/entitymanager.c
  src/scripting/entitytree.c
  src/scripting/events.c
  src/scripting/input.c
  src/scripting/lang.c
  src/scripting/level.c
  src/scripting/levelmanager.c
  src/scripting/levelobjectcontainer.c
  src/scripting/mobilegamepad.c
  src/scripting/mouse.c
  src/scripting/music.c
  src/scripting/obstaclemap.c
  src/scripting/platform.c
  src/scripting/player.c
  src/scripting/playermanager.c
  src/scripting/prefs.c
  src/scripting/screen.c
  src/scripting/sensor.c
  src/scripting/sound.c
  src/scripting/text.c
  src/scripting/time.c
  src/scripting/transform.c
  src/scripting/vector2.c
  src/scripting/video.c
  src/scripting/web.c
  src/physics/obstacle.c
  src/physics/obstaclemap.c
  src/physics/physicsactor.c
  src/physics/sensor.c
  src/physics/sensorstate.c
  src/physics/collisionmask.c

  src/entities/legacy/entitymanager.c
  src/entities/legacy/enemy.c
  src/entities/legacy/item.c
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
  src/entities/brickmanager.c
  src/entities/camera.c
  src/entities/character.c
  src/entities/mobilegamepad.c
  src/entities/player.c
  src/entities/renderqueue.c

  src/misc/icon.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/misc/credits.c

  src/main.c
)

# adding headers
SET(GAME_HEADERS
  src/core/nanoparser.h
  src/core/physfs/ignorecase.h
  src/core/asset.h
  src/core/audio.h
  src/core/color.h
  src/core/commandline.h
  src/core/engine.h
  src/core/font.h
  src/core/global.h
  src/core/image.h
  src/core/import.h
  src/core/input.h
  src/core/inputmap.h
  src/core/lang.h
  src/core/logfile.h
  src/core/web.h
  src/core/prefs.h
  src/core/quest.h
  src/core/resourcemanager.h
  src/core/scene.h
  src/core/screenshot.h
  src/core/fadefx.h
  src/core/sprite.h
  src/core/storyboard.h
  src/core/timer.h
  src/core/video.h

  src/util/csv.h
  src/util/darray.h
  src/util/fasthash.h
  src/util/hashtable.h
  src/util/iterator.h
  src/util/numeric.h
  src/util/stringutil.h
  src/util/utf8.h
  src/util/util.h
  src/util/v2d.h

  src/scenes/mobile/subscenes/info.h
  src/scenes/mobile/subscenes/credits.h
  src/scenes/mobile/subscenes/screenshot.h
  src/scenes/mobile/subscenes/subscene.h
  src/scenes/mobile/util/touch.h
  src/scenes/mobile/menu.h
  src/scenes/mobile/popup.h
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

  src/scripting/util/iterators.h
  src/scripting/scripting.h
  src/physics/obstacle.h
  src/physics/obstaclemap.h
  src/physics/physicsactor.h
  src/physics/sensor.h
  src/physics/sensorstate.h
  src/physics/collisionmask.h

  src/entities/legacy/spatialhash.h
  src/entities/legacy/entitymanager.h
  src/entities/legacy/enemy.h
  src/entities/legacy/item.h
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
  src/entities/brickmanager.h
  src/entities/camera.h
  src/entities/character.h
  src/entities/mobilegamepad.h
  src/entities/player.h
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
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO5})
    TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")
    EXECUTE_PROCESS(COMMAND ${CMAKE_RC_COMPILER} -O coff -o "${CMAKE_CURRENT_BINARY_DIR}/iconwin.res" -i "${CMAKE_SOURCE_DIR}/src/misc/iconwin.rc" -I "${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ \"${CMAKE_CURRENT_BINARY_DIR}/iconwin.res\"")
  ELSEIF(MSVC)
    ADD_EXECUTABLE(${GAME_UNIXNAME} WIN32 ${GAME_SRCS} ${GAME_HEADERS})
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO5})
    TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "/D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS ${CMAKE_C_FLAGS}")
  ELSE()
    MESSAGE("*** Unrecognized compiler ***") # e.g., clang?
    ADD_EXECUTABLE(${GAME_UNIXNAME} WIN32 ${GAME_SRCS})
    TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO5})
    TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")     
  ENDIF()

  # Bugfix
  IF(ALLEGRO_STATIC)
    SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES LINKER_LANGUAGE CXX)
  ENDIF()

ELSEIF(UNIX)

  # *nix executable
  ADD_EXECUTABLE(${GAME_UNIXNAME} ${GAME_SRCS})
  TARGET_LINK_LIBRARIES(${GAME_UNIXNAME} m ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO5})
  TARGET_INCLUDE_DIRECTORIES(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
  SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")

ENDIF()

# Use relative paths in __FILE__ (reproducible builds)
IF(NOT MSVC)
  TARGET_COMPILE_OPTIONS(${GAME_UNIXNAME} PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
ENDIF()

# Our #define's
TARGET_COMPILE_DEFINITIONS(${GAME_UNIXNAME} PUBLIC ${DEFS})

# Target properties
SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES PROJECT_LABEL "${GAME_NAME}")
SET_TARGET_PROPERTIES(${GAME_UNIXNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Installing on *nix
IF(UNIX)
  INSTALL(CODE "MESSAGE(\"Installing ${GAME_NAME} ${GAME_VERSION}... Make sure that you have the appropriate privileges.\")")
  INSTALL(FILES LICENSE README.md CHANGES.md CONTRIBUTING.md logo.png surge.png surge.rocks surge.cfg DESTINATION "${GAME_DATADIR}")
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
