#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# assemble_package.sh
# Assemble an APK
#
set -e

echo "Assembling the APK..."
source "$(dirname "$0")/set_sdk.sh" "$@"

# Usage
function usage()
{
    echo "Usage: assemble_package.sh [-v <version-code>] [-x <version-name>] [-p <package-name>]"
}

# Read positional arguments
unset VERSION_CODE
unset VERSION_NAME
unset PACKAGE_NAME
while getopts "v:x:p:c:b:S:" OPTION; do
    case $OPTION in
        v) VERSION_CODE=$(($OPTARG));;
        x) VERSION_NAME="$OPTARG";;
        p) PACKAGE_NAME="$OPTARG";;
        c|b|S) ;;
        ?) echo "Invalid argument"; usage; exit 1;;
    esac
done

# Set defaults
OUTPUT_FILENAME=opensurge.apk
PACKAGE_NAME="${PACKAGE_NAME:-org.opensurge2d.surgeengine.unofficial}"
VERSION_CODE="${VERSION_CODE:-1}"
VERSION_NAME="${VERSION_NAME:-0.0.0.1}"

# Helpers
UNALIGNED_FILENAME="${OUTPUT_FILENAME%.apk}.unaligned.apk"
UNALIGNED_APK="$(realpath "build/$UNALIGNED_FILENAME")"
ALIGNED_APK="$(realpath "build/$OUTPUT_FILENAME")"
rm -rf "$UNALIGNED_APK" "$ALIGNED_APK"

# -----

# Assemble the APK
echo "Assembling the APK..."
pushd build/apk

# Create the APK
echo "Creating the APK..."
"$SDK_BUILD_TOOLS/aapt" package -f \
    -I "$SDK_PLATFORM/android.jar" \
    -M AndroidManifest.xml \
    -S res \
    --rename-manifest-package "$PACKAGE_NAME" \
    --replace-version --version-code "$VERSION_CODE" --version-name "$VERSION_NAME" \
    -F "$UNALIGNED_APK"

# Add classes.dex
echo "Adding DEX bytecode..."
"$SDK_BUILD_TOOLS/aapt" add "$UNALIGNED_APK" classes.dex

# Add native libraries
echo "Adding native libraries..."
for lib in $(find lib/ -type f); do
    "$SDK_BUILD_TOOLS/aapt" add "$UNALIGNED_APK" "$lib"
done

# Add assets while taking care of spaces in the filepaths
echo "Adding assets..."
[[ -z "$(ls -A assets)" ]] && { echo "ERROR: can't find the assets of the APK"; exit 1; }
readarray -t assets <<< $(find assets -type f)
for asset in "${assets[@]}"; do
     "$SDK_BUILD_TOOLS/aapt" add "$UNALIGNED_APK" "$asset"
done

# zipalign
#
# ``Caution: You must use zipalign at a specific point in the build process. That point depends on which app-signing tool you use: 
#   * If you use apksigner, zipalign must be used before the APK file has been signed. If you sign your APK using apksigner and make further changes to the APK, its signature is invalidated.
#   * If you use jarsigner (not recommended), zipalign must be used after the APK file has been signed.''
#
# Source: https://developer.android.com/tools/zipalign
echo "Zipaligning..."
"$SDK_BUILD_TOOLS/zipalign" -P 16 -f -v 4 "$UNALIGNED_APK" "$ALIGNED_APK"
rm -f "$UNALIGNED_APK"

# Inspect the APK
echo "Inspecting the APK..."
"$SDK_BUILD_TOOLS/aapt" list "$ALIGNED_APK"

echo "Results:"
"$SDK_BUILD_TOOLS/aapt" dump badging "$ALIGNED_APK"

# Done!
echo "The unsigned APK can be found at $ALIGNED_APK"
echo "Version: $VERSION_NAME (version code = $VERSION_CODE)"
popd
