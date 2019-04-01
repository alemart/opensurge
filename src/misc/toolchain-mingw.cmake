# ------------------------------------------------------------------------------
# Cross-compiling with MinGW
# ------------------------------------------------------------------------------
# Use the commands below to build the Windows version of Open Surge with MinGW:
#
#   $ mkdir build && cd build
#   $ cmake .. \
#         -DCMAKE_TOOLCHAIN_FILE=../src/misc/toolchain-mingw.cmake
#   $ make && sudo make install
#
# This is for cross-compiling only.
# ------------------------------------------------------------------------------

# Set the system name
set(CMAKE_SYSTEM_NAME Windows)

# Set the location of the C compiler and of the target environment (MINGDIR)
# The paths below are taken from the Allegro 5 build system; if MinGW isn't
# found automatically in your system, please adjust the paths manually:
if(EXISTS /usr/i586-mingw32msvc)
    set(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
    set(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)
    set(CMAKE_RC_COMPILER i586-mingw32msvc-windres)
    set(CMAKE_FIND_ROOT_PATH /usr/i586-mingw32msvc)
elseif(EXISTS /usr/i686-w64-mingw32)
    set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
    set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
    set(CMAKE_AR:FILEPATH /usr/bin/i686-w64-mingw32-ar)
elseif(EXISTS /opt/mingw)
    set(CMAKE_C_COMPILER /opt/mingw/usr/bin/i686-pc-mingw32-gcc)
    set(CMAKE_CXX_COMPILER /opt/mingw/usr/bin/i686-pc-mingw32-g++)
    set(CMAKE_RC_COMPILER /opt/mingw/usr/bin/i686-pc-mingw32-windres)
    set(CMAKE_FIND_ROOT_PATH /opt/mingw/usr/i686-pc-mingw32)
else()
    set(CMAKE_C_COMPILER /usr/local/cross-tools/bin/i386-mingw32-gcc)
    set(CMAKE_CXX_COMPILER /usr/local/cross-tools/bin/i386-mingw32-g++)
    set(CMAKE_RC_COMPILER /usr/local/cross-tools/bin/i386-mingw32-windres)
    set(CMAKE_FIND_ROOT_PATH /usr/local/cross-tools)
endif()

# Other settings
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
