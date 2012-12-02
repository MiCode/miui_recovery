LOCAL_PATH := $(call my-dir)


#libc
$(add-prebuilt-file libstdc++.a, STATIC_LIBRARIES)

$(add-prebuilt-file libc.so, SHARED_LIBRARIES)

include $(CLEAR_VARS)
LOCAL_MODULE := libc
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/obj/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := libc.so
include $(BUILD_PREBUILT)
