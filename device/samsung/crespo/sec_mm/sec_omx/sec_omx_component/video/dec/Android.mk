LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SEC_OMX_Vdec.c

LOCAL_MODULE := libSEC_OMX_Vdec
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/sec_osal \
	$(SEC_OMX_TOP)/sec_omx_core \
	$(SEC_OMX_COMPONENT)/common \
	$(SEC_OMX_COMPONENT)/video/dec

LOCAL_C_INCLUDES += $(SEC_OMX_TOP)/sec_codecs/video/mfc_c110/include

include $(BUILD_STATIC_LIBRARY)
