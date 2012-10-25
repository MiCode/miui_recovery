LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := finder
MIUI_KERNEL := $(LOCAL_PATH)/kernel

MIUI_KERNEL_BASE := 0x40200000 --ramdiskaddr 0x41500000
MIUI_KERNEL_PAGESIZE := 2048
MIUI_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.hardware=qcom

MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/device.conf
include $(MIUI_RECOVERY)
