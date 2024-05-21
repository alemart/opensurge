#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# sign_apk.sh
# Sign an APK with a debug key
#

echo "Signing the APK with a debug key..."
source "$(dirname "$0")/set_sdk.sh" "$@"

# Usage
function usage()
{
    echo "Usage: sign_apk.sh [-K /path/to/my/debug.keystore] [-P /path/to/my.apk]"
}

# How to generate a keystore
function keystore_help()
{
    echo
    echo "Generate a debug keystore as follows:"
    echo '$ keytool -genkey -v -keystore debug.keystore -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 32768 -dname "C=US, O=Android, CN=Android Debug'
    #echo
    #echo "Generate a release keystore as follows:"
    #echo '$ keytool -genkey -v -keystore release.keystore -alias alias_name -keyalg RSA -keysize 2048 -validity 32768'
}

# Read positional arguments
unset APK
unset KEYSTORE
while getopts "K:P:c:b:S:" OPTION; do
    case "$OPTION" in
        K) KEYSTORE="$(realpath "$OPTARG")";;
        P) APK="$(realpath "$OPTARG")";;
        c|b|S) ;;
        ?) echo "Invalid argument"; usage; exit 1;;
    esac
done

# Set defaults
APK="${APK:-$(realpath build/opensurge.apk)}"
KEYSTORE="${KEYSTORE:-$HOME/.android/debug.keystore}"
echo "Will sign $APK..."

# Validate
if [[ ! -f "$APK" ]]; then
    echo "ERROR: can't find the APK at $APK"
    usage
    exit 1
elif [[ ! -f "$KEYSTORE" ]]; then
    echo "ERROR: can't find the keystore at $KEYSTORE"
    usage
    keystore_help
    exit 1
fi

# Skip if the APK is already signed
echo "Checking if the APK is already signed..."
"$SDK_BUILD_TOOLS/apksigner" verify --verbose "$APK" && {
    echo "The APK is already signed"
    echo "The signed APK can be found at $APK"
    exit 0
}

# Sign the APK
echo "Signing..."
"$SDK_BUILD_TOOLS/apksigner" sign \
    --ks "$KEYSTORE" \
    --ks-key-alias androiddebugkey \
    --ks-pass pass:android \
    --key-pass pass:android \
    "$APK"

# Done!
echo "The signed APK can be found at $APK"
