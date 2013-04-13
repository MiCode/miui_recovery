LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include   $(SEC_CODECS)/video/mfc_c110/dec/Android.mk
include   $(SEC_CODECS)/video/mfc_c110/enc/Android.mk
include   $(SEC_CODECS)/video/mfc_c110/csc/Android.mk