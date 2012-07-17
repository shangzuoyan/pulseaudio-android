LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := audio_test

LOCAL_MODULE_TAGS:=eng debug

LOCAL_SRC_FILES := \
	audio_test.cpp

LOCAL_C_INCLUDES := \
	system/core/include \
	frameworks/base/include

LOCAL_SHARED_LIBRARIES:= \
	libmedia \
	libpulse

LOCAL_PRELINK_MODULE := false
include $(BUILD_EXECUTABLE)
