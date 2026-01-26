#
# Open Surge Engine
# CMakeLists.txt - CMake script
# Copyright 2008-2026  Alexandre Martins <alemartf@gmail.com>
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
# Initialization
# ------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(opensurge LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(GAME_UNIXNAME "opensurge")
set(GAME_NAME "Open Surge Engine")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/misc/cmake")
set(HELP "Tweak the options or get help at opensurge2d.org")
set(DEFS "") # our #defines

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug | Release | MinSizeRel | RelWithDebInfo" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(utils)
read_game_version(GAME_VERSION_STRING GAME_VERSION_MAJOR GAME_VERSION_MINOR GAME_VERSION_PATCH GAME_VERSION_BUILD)
read_game_years(GAME_YEARS)

message("${GAME_NAME} version ${GAME_VERSION_STRING}")




# ------------------------------------------
# User options
# ------------------------------------------

option(ALLEGRO_MONOLITH "Use the monolith version of Allegro" OFF)
option(SURGESCRIPT_STATIC "Link SurgeScript statically" OFF)

# Link Allegro statically
option(ALLEGRO_STATIC "Link Allegro statically (Windows only)" OFF)
if(ALLEGRO_STATIC)
  list(APPEND DEFS "ALLEGRO_STATICLINK=1")
endif()

# Run in place?
option(GAME_RUNINPLACE "Read assets only from the directory of the executable" OFF)
if(GAME_RUNINPLACE)
  list(APPEND DEFS "GAME_RUNINPLACE=1")
endif()

# Set the build type & version - this helps identify the platform and the development builds
set(GAME_BUILD_VERSION "" CACHE STRING "Build type & version (optional)")
if(NOT GAME_BUILD_VERSION STREQUAL "")
  list(APPEND DEFS "GAME_BUILD_VERSION=\"${GAME_BUILD_VERSION}\"")
endif()

# Set the build date
option(WANT_BUILD_DATE "Include the build date in the binary (disable for reproducible builds)" ON)
if(WANT_BUILD_DATE)
  list(APPEND DEFS "GAME_BUILD_DATE=__DATE__")
endif()

# Play a MOD (options screen)
option(WANT_PLAYMOD "Display the MOD loader in the options screen" ON)
if(WANT_PLAYMOD)
  list(APPEND DEFS "WANT_PLAYMOD=1")
endif()

# OpenGL ES
option(WANT_GLES "Use OpenGL ES (Android and X11)" OFF)
if(WANT_GLES)
  list(APPEND DEFS "WANT_GLES=1")
endif()

# FastDraw
option(WANT_FASTDRAW "Use the FastDraw mini-library" ON)
if(WANT_FASTDRAW)
  list(APPEND DEFS "WANT_FASTDRAW=1")
endif()

# Improved Gamepad support
option(WANT_BETTER_GAMEPAD "Autodetect gamepad layouts for consistent cross-platform behavior" ON)
if(WANT_BETTER_GAMEPAD)
  list(APPEND DEFS "WANT_BETTER_GAMEPAD=1")
endif()

# User-specified paths
set(ALLEGRO_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}" CACHE PATH "Where to look for Allegro & its dependencies")
set(ALLEGRO_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}" CACHE PATH "Where to look for the header files of Allegro")
set(SURGESCRIPT_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}" CACHE PATH "Where to look for SurgeScript")
set(SURGESCRIPT_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}" CACHE PATH "Where to look for the header files of SurgeScript")
set(PHYSFS_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}" CACHE PATH "Where to look for PhysicsFS")
set(PHYSFS_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}" CACHE PATH "Where to look for the header files of PhysicsFS")

if(UNIX)
  set(GAME_BINDIR "games" CACHE PATH "Installation directory of the executable. It will be appended to CMAKE_INSTALL_PREFIX if it's a relative path")
  set(GAME_DATADIR "share/games/${GAME_UNIXNAME}" CACHE PATH "Installation directory of the game data. It will be appended to CMAKE_INSTALL_PREFIX if it's a relative path")
  set(GAME_USERDIRNAME "${GAME_UNIXNAME}" CACHE PATH "Name of the user-modifiable asset directory")
endif()

if(UNIX AND NOT ANDROID)
  set(DESKTOP_ENTRY_PATH "/usr/share/applications" CACHE PATH "Installation directory of the .desktop file")
  set(DESKTOP_ICON_PATH "/usr/share/icons/hicolor/256x256/apps" CACHE PATH "Installation directory of the icon file")
  set(DESKTOP_METAINFO_PATH "/usr/share/metainfo" CACHE PATH "Installation directory of the metainfo file. You shouldn't change this (check the AppStream specfication)")
endif()

# Data folder
if(DEFINED GAME_DATADIR)
  list(APPEND DEFS "GAME_USERDIRNAME=\"${GAME_USERDIRNAME}\"")
  if(IS_ABSOLUTE "${GAME_DATADIR}")
    list(APPEND DEFS "GAME_DATADIR=\"${GAME_DATADIR}\"")
  else()
    list(APPEND DEFS "GAME_DATADIR=\"${CMAKE_INSTALL_PREFIX}/${GAME_DATADIR}\"")
  endif()
endif()




# ------------------------------------------
# Templates
# ------------------------------------------

# Generate the content of the credits screen
csv_to_cstr(COPYRIGHT_DATA_CSV "src/misc/copyright_data.csv")
generate_from_template("credits.c")

# Generate desktop & metadata files
if(DEFINED DESKTOP_ENTRY_PATH)

  # Find absolute paths
  cmake_path(APPEND DESKTOP_ICON_FULLPATH "${DESKTOP_ICON_PATH}" "opensurge.png")
  if(IS_ABSOLUTE "${GAME_BINDIR}")
    cmake_path(APPEND DESKTOP_EXEC_FULLPATH "${GAME_BINDIR}" "${GAME_UNIXNAME}")
  else()
    cmake_path(APPEND DESKTOP_EXEC_FULLPATH "${CMAKE_INSTALL_PREFIX}" "${GAME_BINDIR}" "${GAME_UNIXNAME}")
  endif()

  generate_from_template("opensurge.desktop")
  generate_from_template("opensurge.appdata.xml")

endif()

# Generate the resource file
if(WIN32)
  generate_from_template("opensurge.rc")
endif()




# ------------------------------------------
# Find the dependencies
# ------------------------------------------

include(libs)

# Allegro
find_path(ALLEGRO_INCDIR NAMES "allegro5/allegro.h" PATHS ${ALLEGRO_INCLUDE_PATH})
if(NOT ALLEGRO_INCDIR)
  message(FATAL_ERROR "Can't find allegro.h! ${HELP}")
else()
  message(STATUS "Found allegro.h at ${ALLEGRO_INCDIR}/allegro5")
endif()

set(LALLEGRO_ALL "")
foreach(A5_LIB ${ALLEGRO_LIBS})
  string(TOUPPER "L${A5_LIB}" _A5_LIB)
  find_library(${_A5_LIB} NAMES "${A5_LIB}" PATHS ${ALLEGRO_LIBRARY_PATH})
  if(NOT ${_A5_LIB})
    message(FATAL_ERROR "Can't find lib${A5_LIB}. ${HELP}")
  else()
    message(STATUS "Found lib${A5_LIB} at ${${_A5_LIB}}...")
    list(APPEND LALLEGRO_ALL "${${_A5_LIB}}")
  endif()
endforeach()

# SurgeScript
find_path(SURGESCRIPT_INCDIR NAMES "surgescript.h" PATHS ${SURGESCRIPT_INCLUDE_PATH})
if(NOT SURGESCRIPT_INCDIR)
  message(FATAL_ERROR "Can't find surgescript.h! ${HELP}")
else()
  message(STATUS "Found surgescript.h at ${SURGESCRIPT_INCDIR}")
endif()

find_library(LSURGESCRIPT NAMES "${SURGESCRIPT_LIB}" PATHS ${SURGESCRIPT_LIBRARY_PATH})
if(NOT LSURGESCRIPT)
  message(FATAL_ERROR "Can't find lib${SURGESCRIPT_LIB}! ${HELP}")
else()
  message(STATUS "Found lib${SURGESCRIPT_LIB} at ${LSURGESCRIPT}...")
endif()

# PhysicsFS
find_path(PHYSFS_INCDIR NAMES "physfs.h" PATHS ${PHYSFS_INCLUDE_PATH})
if(NOT PHYSFS_INCDIR)
  message(FATAL_ERROR "Can't find physfs.h! ${HELP}")
else()
  message(STATUS "Found physfs.h at ${PHYSFS_INCDIR}")
endif()

find_library(LPHYSFS NAMES "physfs" PATHS ${PHYSFS_LIBRARY_PATH})
if(NOT LPHYSFS)
  message(FATAL_ERROR "Can't find libphysfs! ${HELP}")
else()
  message(STATUS "Found libphysfs at ${LPHYSFS}...")
endif()




# ------------------------------------------
# Create the executable
# ------------------------------------------

include(srcs)

# Executable
if(WIN32)

  # Windows executable
  if(NOT MSVC)

    find_library(LM m)
    add_executable(${GAME_UNIXNAME} WIN32 ${GAME_SRCS})
    target_link_libraries(${GAME_UNIXNAME} ${LM} ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO_ALL})
    target_include_directories(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
    set_target_properties(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")

    if(MINGW)
      if(NOT CMAKE_RC_COMPILER)
        set(CMAKE_RC_COMPILER windres)
      endif()
      execute_process(COMMAND ${CMAKE_RC_COMPILER} -O coff -o "${CMAKE_CURRENT_BINARY_DIR}/opensurge.res" -i "${CMAKE_CURRENT_BINARY_DIR}/src/misc/opensurge.rc" -I "${CMAKE_CURRENT_SOURCE_DIR}/src/misc")
      set_target_properties(${GAME_UNIXNAME} PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ \"${CMAKE_CURRENT_BINARY_DIR}/opensurge.res\"")
    endif()

    if(ALLEGRO_STATIC)
      set_target_properties(${GAME_UNIXNAME} PROPERTIES LINKER_LANGUAGE CXX)
    endif()

  else()

    add_executable(${GAME_UNIXNAME} WIN32 ${GAME_SRCS} ${GAME_HEADERS})
    target_link_libraries(${GAME_UNIXNAME} ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO_ALL})
    target_include_directories(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
    set_target_properties(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "/D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS ${CMAKE_C_FLAGS}")

  endif()

elseif(ANDROID)

  # Android: shared library
  find_library(LM m)
  find_library(LLOG log)
  add_library(${GAME_UNIXNAME} SHARED ${GAME_SRCS})
  target_link_libraries(${GAME_UNIXNAME} ${LM} ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO_ALL} ${LLOG})
  target_include_directories(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
  set_target_properties(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")

elseif(UNIX)

  # Unix executable
  find_library(LM m)
  add_executable(${GAME_UNIXNAME} ${GAME_SRCS})
  target_link_libraries(${GAME_UNIXNAME} ${LM} ${LSURGESCRIPT} ${LPHYSFS} ${LALLEGRO_ALL})
  target_include_directories(${GAME_UNIXNAME} PUBLIC ${SURGESCRIPT_INCDIR} ${PHYSFS_INCDIR} ${ALLEGRO_INCDIR})
  set_target_properties(${GAME_UNIXNAME} PROPERTIES COMPILE_FLAGS "-Wall")

endif()

# Our #defines
target_compile_definitions(${GAME_UNIXNAME} PUBLIC ${DEFS})

# Target properties
set_target_properties(${GAME_UNIXNAME} PROPERTIES PROJECT_LABEL "${GAME_NAME}")
if(GAME_RUNINPLACE)
  set_target_properties(${GAME_UNIXNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Use relative paths in __FILE__ (reproducible builds)
if(NOT MSVC)
  target_compile_options(${GAME_UNIXNAME} PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
endif()




# ------------------------------------------
# Install the game
# ------------------------------------------

if(UNIX AND NOT GAME_RUNINPLACE)

  # Executable
  install(TARGETS "${GAME_UNIXNAME}"
    RUNTIME DESTINATION "${GAME_BINDIR}"
    LIBRARY DESTINATION "${GAME_BINDIR}"
  )

  # Assets
  install(FILES LICENSE README.md CHANGELOG.md CONTRIBUTING.md logo.png surge.png surge.rocks surge.cfg DESTINATION "${GAME_DATADIR}")
  install(DIRECTORY characters scripts sprites images levels musics quests samples themes languages fonts inputs licenses DESTINATION "${GAME_DATADIR}" PATTERN ".git" EXCLUDE)

  # AppStream / freedesktop.org
  if(DEFINED DESKTOP_ENTRY_PATH)
    install(FILES src/misc/opensurge.png DESTINATION "${DESKTOP_ICON_PATH}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/src/misc/opensurge.appdata.xml" DESTINATION "${DESKTOP_METAINFO_PATH}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/src/misc/opensurge.desktop" DESTINATION "${DESKTOP_ENTRY_PATH}")
  endif()

endif()