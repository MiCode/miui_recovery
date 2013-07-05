LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := dedupe.c driver.c
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE := dedupe_host
LOCAL_MODULE_TAGS := optional 
LOCAL_STATIC_LIBRARIES := libcrypto_static
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../external/openssl/include
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := dedupe.c
LOCAL_STATIC_LIBRARIES := libcrypto_static libcutils libc
LOCAL_MODULE := libdedupe
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := external/openssl/include
include $(BUILD_STATIC_LIBRARY)
## remove it 
#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := driver.c
#LOCAL_STATIC_LIBRARIES := libdedupe libcrypto_static libcutils libc
#LOCAL_MODULE := dedupe
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
#OCAL_C_INCLUDES := external/openssl/include
#LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
#LOCAL_FORCE_STATIC_EXECUTABLE := true
#include $(BUILD_EXECUTABLE)
