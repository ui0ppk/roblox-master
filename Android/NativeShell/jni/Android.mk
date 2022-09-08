LOCAL_PATH := $(call my-dir)


# Setup some paths and libraries based on the type of build we're targeting.
ifdef DEBUG
RBX_LIB_DIR := debug
FMOD_SUFFIX := L
endif
ifdef NOOPT
RBX_LIB_DIR := noopt
FMOD_SUFFIX := L
endif
ifdef RELEASE
RBX_LIB_DIR := release
endif

include $(CLEAR_VARS)
LOCAL_MODULE := roblox
LOCAL_SRC_FILES := ../../../../build/$(RBX_LIB_DIR)/libs/armeabi-v7a/libroblox.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := fmod
LOCAL_SRC_FILES := ../../../fmod/Android/armeabi-v7a/libfmod${FMOD_SUFFIX}.so
include $(PREBUILT_SHARED_LIBRARY)


all: POST_BUILD_STEP

.PHONY: POST_BUILD_STEP
POST_BUILD_STEP:
	sh "./${PATH_PREFIX}../../buildshaders.sh"
	sh "./${PATH_PREFIX}packAndroidAssets.sh"

# PATH_PREFIX is used when building with Android Studio, and comes from NativeShell/build.gradle