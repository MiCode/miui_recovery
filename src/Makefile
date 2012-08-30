LOCAL_SRC_FILES := \
    recovery_ui.c \
    mount.c \
    miui_intent.c \
    bootloader.c \
    install.c \
    roots.c \
    nandroid.c \
    verifier.c \
    recovery.c

RECOVERY_API_VERSION := 3
MYDEFINE_CFLAGS :=  -D_GLIBCXX_DEBUG_PEDANTIC \
                  -DFT2_BUILD_LIBRARY=1 \
                  -D_MIUI_NODEBUG=1 \
                  -DDARWIN_NO_CARBON 
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) 

LOCAL_C_INCLUDES += system/extras/ext4_utils

LOCAL_STATIC_LIBRARIES += libext4_utils libz
LOCAL_STATIC_LIBRARIES += libminzip libunz libmtdutils libmincrypt
#add static libraries
LOCAL_STATIC_LIBRARIES += libedify libcrecovery libflashutils libmmcutils libbmlutils
LOCAL_STATIC_LIBRARIES += libmkyaffs2image libunyaffs liberase_image libdump_image libflash_image
LOCAL_STATIC_LIBRARIES += libmiui libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc libm

LOCAL_LIB_DI
