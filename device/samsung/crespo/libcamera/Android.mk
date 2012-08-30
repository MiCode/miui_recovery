ifneq ($(filter crespo crespo4g,$(TARGET_DEVICE)),)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libs3cjpeg

LOCAL_SRC_FILES:= \
	SecCamera.cpp SecCameraHWInterface.cpp

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware
LOCAL_SHARED_LIBRARIES+= libs3cjpeg

LOCAL_MODULE := camera.herring

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
