# Copyright 2011 The Android Open Source Project
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_MODULE := send_bug
LOCAL_MODULE_TAGS := optional
include $(BUILD_JAVA_LIBRARY)
