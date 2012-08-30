LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	SEC_OMX_H264dec.c \
	library_register.c


LOCAL_MODULE := libOMX.SEC.AVC.Decoder

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libSEC_OMX_Vdec libsecosal libsecbasecomponent \
						libsecmfcdecapi libseccsc
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui libhardware

LOCAL_C_INCLUDES := $(SEC_OMX_INC)/khronos \
	$(SEC_OMX_INC)/sec \
	$(SEC_OMX_TOP)/sec_osal \
	$(SEC_OMX_TOP)/sec_omx_core \
	$(SEC_OMX_COMPONENT)/common \
	$(SEC_OMX_COMPONENT)/video/dec

LOCAL_C_INCLUDES += $(SEC_OMX_TOP)/sec_codecs/video/mfc_c110/include

include $(BUILD_SHARED_LIBRARY)
