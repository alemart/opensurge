#
# Here you can customize the build
#


# Version of the APK, as in major.minor.patch.build
# a.b.c.d is such that a,b,c,d >= 0; b,c < 8; d < 64
GAME_VERSION_MAJOR := 6
GAME_VERSION_MINOR := 0
GAME_VERSION_PATCH := 11
GAME_VERSION_BUILD := 0



# Target ABIs
# It is mandatory to include arm64-v8a
ABI_LIST ?= arm64-v8a armeabi-v7a
#ABI_LIST ?= arm64-v8a armeabi-v7a x86_64 x86 # uncomment if you intend to run the engine on emulators



# Package name
# Do not use "org.opensurge2d.surgeengine" - this is reserved for official builds
# Do not use "org.opensurge2d" - this points to the official domain
GAME_PACKAGE_NAME ?= org.opensurge2d.surgeengine.unofficial
GAME_VERSION_SUFFIX ?= unofficial



# Leave these blank to autodetect
COMPILE_SDK_VERSION ?= # must be greater than or equal to the targetSdkVersion declared in the manifest
BUILD_TOOLS_VERSION ?= # a directory name in $ANDROID_HOME/build-tools



# Misc
NDK_API_LEVEL := 21 # must match the minSdkVersion declared in the manifest
DEBUG_KEYSTORE := $(HOME)/.android/debug.keystore
