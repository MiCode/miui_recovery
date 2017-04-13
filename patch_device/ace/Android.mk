LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := ace
MIUI_KERNEL := $(LOCAL_PATH)/kernel

MIUI_KERNEL_BASE := 0x04000000
MIUI_KERNEL_PAGESIZE := 2048
MIUI_KERNEL_CMDLINE := no_console_suspend=1

MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/device.conf
include $(MIUI_RECOVERY)
