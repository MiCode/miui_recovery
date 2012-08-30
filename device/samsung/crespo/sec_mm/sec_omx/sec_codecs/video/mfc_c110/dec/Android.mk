
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	src/SsbSipMfcDecAPI.c

LOCAL_MODULE := libsecmfcdecapi



LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_C_INCLUDES := \
	$(SEC_CODECS)/video/mfc_c110/include

include $(BUILD_STATIC_LIBRARY)

