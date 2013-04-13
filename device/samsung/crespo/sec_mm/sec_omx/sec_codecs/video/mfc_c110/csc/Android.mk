
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

ifeq ($(ARCH_ARM_HAVE_NEON),true)
LOCAL_SRC_FILES := \
	csc_yuv420_nv12t_y_neon.s \
	csc_yuv420_nv12t_uv_neon.s \
	csc_nv12t_yuv420_y_neon.s \
	csc_nv12t_yuv420_uv_neon.s \
	csc_interleave_memcpy.s \
	csc_deinterleave_memcpy.s

else
LOCAL_SRC_FILES := \
	color_space_convertor.c

endif

LOCAL_MODULE := libseccsc

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_C_INCLUDES := \
	$(SEC_CODECS)/video/mfc_c110/include

include $(BUILD_STATIC_LIBRARY)

