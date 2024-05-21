#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# set_sdk.sh
# Set SDK-related variables
#

# Usage
function usage()
{
    echo "Usage: set_sdk.sh [-S /path/to/sdk] [-c <compile-sdk-version>] [-b <build-tools-version>]"
}

# Read positional arguments
unset SDK
unset SDK_PLATFORM
unset SDK_BUILD_TOOLS
unset PLATFORM_VERSION
unset BUILD_TOOLS_VERSION
while getopts "c:b:S:v:x:p:K:P:" OPTION ; do
    case $OPTION in
        c) PLATFORM_VERSION="$OPTARG";;
        b) BUILD_TOOLS_VERSION="$OPTARG";;
        S) export SDK="$([[ -z "$OPTARG" ]] && echo "" || echo "$(realpath "$OPTARG")")";;
    esac
done
OPTIND=1

# Use a default SDK path if none is given
export SDK="${SDK:-$ANDROID_HOME}"
if [[ ! -d "$SDK" || ! -d "$SDK/platforms" || ! -d "$SDK/build-tools" || ! -d "$SDK/platform-tools" ]]; then
    echo "Invalid path to the Android SDK: $SDK"
    usage
    return 1
fi

# Set the Android platform
PLATFORM_VERSION=${PLATFORM_VERSION:-$(ls "$SDK/platforms" | tail -n 1 | sed 's/android-//g')}
export SDK_PLATFORM=$SDK/platforms/android-$PLATFORM_VERSION
if [[ ! -e "$SDK_PLATFORM/android.jar" ]]; then
    echo "Invalid SDK_PLATFORM: $SDK_PLATFORM"
    usage
    return 1
fi

# Set the path to the build tools
BUILD_TOOLS_VERSION=${BUILD_TOOLS_VERSION:-$(ls "$SDK/build-tools" | tail -n 1)}
export SDK_BUILD_TOOLS=$SDK/build-tools/$BUILD_TOOLS_VERSION
if [[ ! -x "$SDK_BUILD_TOOLS/zipalign" ]]; then
    echo "Invalid SDK_BUILD_TOOLS: $SDK_BUILD_TOOLS"
    usage
    return 1
fi

# Set the path to the platform tools
export SDK_PLATFORM_TOOLS=$SDK/platform-tools
if [[ ! -x "$SDK_PLATFORM_TOOLS/adb" ]]; then
    echo "Invalid SDK_PLATFORM_TOOLS: $SDK_PLATFORM_TOOLS"
    usage
    return 1
fi

# Look for javac in $JAVA_HOME
JAVAC="$JAVA_HOME/bin/javac"
if [[ -z "$JAVA_HOME" ]]; then
    echo "JAVA_HOME is unset"
    return 1
elif [[ ! -x "$JAVAC" ]]; then
    echo "Can't find javac at $JAVAC"
    return 1
fi

# Done!
echo "----------------"
echo "ANDROID_HOME is set to $ANDROID_HOME"
echo "JAVA_HOME is set to $JAVA_HOME"
echo "Compile SDK version is set to $PLATFORM_VERSION"
echo "Build tools version is set to $BUILD_TOOLS_VERSION"
echo "The path to the Android SDK is set to $SDK"
echo "The path to the SDK platform is set to $SDK_PLATFORM"
echo "The path to the SDK build tools is set to $SDK_BUILD_TOOLS"
echo "----------------"
