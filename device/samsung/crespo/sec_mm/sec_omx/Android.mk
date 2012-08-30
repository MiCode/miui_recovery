
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

SEC_OMX_TOP := $(LOCAL_PATH)
SEC_CODECS := $(SEC_OMX_TOP)/sec_codecs/

SEC_OMX_INC := $(SEC_OMX_TOP)/sec_omx_include/
SEC_OMX_COMPONENT := $(SEC_OMX_TOP)/sec_omx_component

include $(SEC_OMX_TOP)/sec_osal/Android.mk
include $(SEC_OMX_TOP)/sec_omx_core/Android.mk

include $(SEC_CODECS)/Android.mk
include $(SEC_OMX_COMPONENT)/common/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/h264dec/Android.mk
include $(SEC_OMX_COMPONENT)/video/dec/mpeg4dec/Android.mk
include $(SEC_OMX_COMPONENT)/video/enc/Android.mk
include $(SEC_OMX_COMPONENT)/video/enc/h264enc/Android.mk
include $(SEC_OMX_COMPONENT)/video/enc/mpeg4enc/Android.mk
