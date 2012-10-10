# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	yaffs2/utils/mkyaffs2image.c \
	yaffs2/yaffs_packedtags2.c \
	yaffs2/yaffs_ecc.c \
	yaffs2/yaffs_tagsvalidity.c

LOCAL_CFLAGS =   -O2 -Wall -DCONFIG_YAFFS_UTIL -DCONFIG_YAFFS_DOES_ECC
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline

LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2

LOCAL_MODULE := mkyaffs2image

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals, dist_files, $(LOCAL_BUILT_MODULE))


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	yaffs2/utils/mkyaffs2image.c \
	yaffs2/yaffs_packedtags2.c \
	yaffs2/yaffs_ecc.c \
	yaffs2/yaffs_tagsvalidity.c
LOCAL_CFLAGS =   -O2 -Wall -DCONFIG_YAFFS_UTIL -DCONFIG_YAFFS_DOES_ECC
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline
LOCAL_CFLAGS+=   -DS_IWRITE=0200 -DS_IREAD=0400
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2
LOCAL_MODULE := mkyaffs2image
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	yaffs2/utils/mkyaffs2image.c \
	yaffs2/yaffs_packedtags2.c \
	yaffs2/yaffs_ecc.c \
	yaffs2/yaffs_tagsvalidity.c
LOCAL_CFLAGS =   -O2 -Wall -DCONFIG_YAFFS_UTIL -DCONFIG_YAFFS_DOES_ECC
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline
LOCAL_CFLAGS+=   -DS_IWRITE=0200 -DS_IREAD=0400
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2
LOCAL_MODULE := libmkyaffs2image
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS += -Dmain=mkyaffs2image_main
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libunyaffs
LOCAL_SRC_FILES := yaffs2/utils/unyaffs.c
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS =   -O2 -Wall -DCONFIG_YAFFS_UTIL -DCONFIG_YAFFS_DOES_ECC
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline
LOCAL_CFLAGS+=   -DS_IWRITE=0200 -DS_IREAD=0400
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2
LOCAL_CFLAGS += -Dmain=unyaffs_main
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := unyaffs
LOCAL_SRC_FILES := yaffs2/utils/unyaffs.c
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS =   -O2 -Wall -DCONFIG_YAFFS_UTIL -DCONFIG_YAFFS_DOES_ECC
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline
LOCAL_CFLAGS+=   -DS_IWRITE=0200 -DS_IREAD=0400
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := unyaffs
LOCAL_SRC_FILES := yaffs2/utils/unyaffs.c
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS =   -O2 -Wall -DCONFIG_YAFFS_UTIL -DCONFIG_YAFFS_DOES_ECC
LOCAL_CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
LOCAL_CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline
LOCAL_CFLAGS+=   -DS_IWRITE=0200 -DS_IREAD=0400
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2
include $(BUILD_HOST_EXECUTABLE)


