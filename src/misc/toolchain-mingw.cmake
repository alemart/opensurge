# --------------------------
# Cross-compiling with MinGW
# --------------------------
# Use the commands below to build the Windows version of Open Surge with MinGW:
#
#     mkdir build && cd build
#     cmake .. -DCMAKE_TOOLCHAIN_FILE=../src/misc/toolchain-mingw.cmake
#     make
#
# This is for cross-compiling only.

# Set the system name
set(CMAKE_SYSTEM_NAME Windows)

# Set the location of the C compiler and of the target environment (MINGDIR)
# Please adjust the paths below:
set(CMAKE_C_COMPILER /usr/bin/i686-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER /usr/bin/i686-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)

# Other adjustments
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
