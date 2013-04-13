LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := reflash-bootloader.c
LOCAL_MODULE = tuna-reflash-bootloader
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libc
include $(BUILD_EXECUTABLE)
