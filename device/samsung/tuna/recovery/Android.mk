ifneq (,$(findstring $(TARGET_DEVICE),tuna toro torospr maguro))

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES += bootable/recovery
LOCAL_SRC_FILES := recovery_ui.c

# should match TARGET_RECOVERY_UI_LIB set in BoardConfig.mk
LOCAL_MODULE := librecovery_ui_tuna

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

# Edify extension functions for doing bootloader updates on Tuna devices.

LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += bootable/recovery system/vold
LOCAL_SRC_FILES := recovery_updater.c bootloader.c

# should match TARGET_RECOVERY_UPDATER_LIBS set in BoardConfig.mk
LOCAL_MODULE := librecovery_updater_tuna

include $(BUILD_STATIC_LIBRARY)

endif
