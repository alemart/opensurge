#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# set_ndk.sh
# Set NDK-related variables for an Android ABI
#

# Usage
function usage()
{
    echo "Usage: set_ndk.sh -a <abi> [-N /path/to/ndk] [-m <minSdkVersion>]"
    echo "where <abi> = arm64-v8a | armeabi-v7a | x86_64 | x86"
}

# Read positional arguments
unset NDK
unset ABI
unset API
while getopts "a:m:N:v:" OPTION ; do
    case $OPTION in
        a) export ABI="$OPTARG";;
        m) export API=$(($OPTARG));;
        N) export NDK="$([[ -z "$OPTARG" ]] && echo "" || echo "$(realpath "$OPTARG")")";;
    esac
done
OPTIND=1

# The ABI must be specified
if [[ -z "$ABI" ]]; then
    echo "Unspecified ABI"
    usage
    return 1
fi

# Use a default NDK path if none is given
DEFAULT_NDK="${ANDROID_NDK_ROOT:-$ANDROID_HOME/ndk/$(ls "$ANDROID_HOME/ndk" | tail -n 1)}"
export NDK="${NDK:-$DEFAULT_NDK}"
if [[ ! -d "$NDK" ]]; then
    echo "Invalid path to the NDK: $NDK"
    usage
    return 1
fi

# Set the minSdkVersion for arm64-v8a
export API=${API:-21}

# Set the CMake toolchain
NDK_TOOLCHAIN_CMAKE="$NDK/build/cmake/android.toolchain.cmake"
if [[ ! -f "$NDK_TOOLCHAIN_CMAKE" ]]; then
    echo "NDK error. Can't find android.toolchain.cmake at $NDK_TOOLCHAIN_CMAKE"
    return 1
fi

# Set variables for an ABI
case $ABI in
    arm64-v8a)
        # arm64-v8a: 64-bit builds
        export ABI=arm64-v8a
        export HOST=aarch64-linux-android
        export CHOST=$HOST
        export HOST_TAG=linux-x86_64
        ;;

    armeabi-v7a)
        # armeabi-v7a: 32-bit builds
        export ABI=armeabi-v7a
        export HOST=arm-linux-androideabi
        export CHOST=armv7a-linux-androideabi
        export HOST_TAG=linux-x86_64
        ;;

    x86_64)
        # x86_64: useful for emulators
        export ABI=x86_64
        export HOST=x86_64-linux-android
        export CHOST=$HOST
        export HOST_TAG=linux-x86_64
        ;;

    x86)
        # x86: useful for emulators
        export ABI=x86
        export HOST=i686-linux-android
        export CHOST=$HOST
        export HOST_TAG=linux-x86_64
        ;;

    *)
        # Invalid ABI
        echo "Invalid ABI: $ABI"
        usage
        return 1
        ;;
esac

# Set the toolchain
export TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/$HOST_TAG"
export AR="$TOOLCHAIN/bin/llvm-ar"
export CC="$TOOLCHAIN/bin/$CHOST$API-clang"
export CXX="$TOOLCHAIN/bin/$CHOST$API-clang++"
export AS="$TOOLCHAIN/bin/$HOST-as"
export LD="$TOOLCHAIN/bin/ld"
export RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
export STRIP="$TOOLCHAIN/bin/llvm-strip"
export NM="$TOOLCHAIN/bin/llvm-nm"

if [[ ! -e "$CC" ]]; then
    echo "NDK error. Can't find CC at $CC. Is $API a correct API level for this SDK?"
    return 1
fi

# Set the install prefix
export PREFIX="$(realpath build/stage/sysroot/$ABI/usr)"

# Done!
echo "----------------"
echo "The ABI is set to $ABI"
echo "The API is set to $API (minSdkVersion)"
echo "The NDK is set to $NDK"
echo "The TOOLCHAIN is set to $TOOLCHAIN"
echo "The PREFIX is set to $PREFIX"
echo "----------------"
