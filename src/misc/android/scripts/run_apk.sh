#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# run_apk.sh
# Run the engine
#
set -e

echo "Will run the engine..."
source "$(dirname "$0")/set_sdk.sh" "$@"

ADB="$SDK_PLATFORM_TOOLS/adb"
AAPT2="$SDK_BUILD_TOOLS/aapt2"

APK="build/opensurge.apk"
if [[ ! -f "$APK" ]]; then
    echo "Can't find $APK"
    exit 1
fi

echo "Found $APK"
PACKAGE_NAME="$("$AAPT2" dump packagename "$APK")"
echo "Package name is $PACKAGE_NAME"

echo "Installing..."
"$ADB" install -r "$APK"

echo "Launching..."
"$ADB" shell am start -n "$PACKAGE_NAME/org.opensurge2d.surgeengine.MainActivity"
