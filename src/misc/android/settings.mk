#
# Here you can customize the build
#


#
# Version of the APK, as in major.minor.patch.build-suffix
# a.b.c.d is such that a,b,c,d >= 0; b,c < 10; d < 50
#
GAME_VERSION_MAJOR := 6
GAME_VERSION_MINOR := 0
GAME_VERSION_PATCH := 9
GAME_VERSION_BUILD := 1

# suffix
GAME_VERSION_SUFFIX ?= unofficial

# override the major.minor.patch.build prefix of the version name of the APK
# This should generally be left empty. It may be used for pre-releases
FORCE_VERSION_NAME := 6.1.0.0




#
# Target ABIs
#

# It is mandatory to include arm64-v8a
ABI_LIST ?= arm64-v8a armeabi-v7a

# uncomment if you intend to run the engine on emulators
#ABI_LIST ?= arm64-v8a armeabi-v7a x86_64 x86



#
# Application ID
# Please do not use "org.opensurge2d.surgeengine" - this is reserved for official builds
# Please do not use "org.opensurge2d" - this refers to the official domain
# https://developer.android.com/build/configure-app-module#set-application-id
#
GAME_APPLICATION_ID ?= org.opensurge2d.surgeengine.unofficial



#
# Android SDK
#

# path to the SDK
SDK := $(ANDROID_HOME)

# path to the NDK
NDK := $(ANDROID_NDK_ROOT)



#
# Leave these blank to autodetect
#

# must be greater than or equal to the targetSdkVersion declared in the manifest
COMPILE_SDK_VERSION ?= 33

# a directory name in $ANDROID_HOME/build-tools
BUILD_TOOLS_VERSION ?=



#
# Misc
#

# must match the minSdkVersion declared in the manifest
NDK_API_LEVEL := 21

# path to a debug keystore
DEBUG_KEYSTORE := $(HOME)/.android/debug.keystore
