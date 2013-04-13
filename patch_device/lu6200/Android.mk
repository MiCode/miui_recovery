LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := lu6200
MIUI_KERNEL := $(LOCAL_PATH)/kernel

MIUI_KERNEL_BASE := 0x40200000 --ramdiskaddr 0x41a00000
MIUI_KERNEL_PAGESIZE := 2048
MIUI_KERNEL_CMDLINE := console=ttyDCC0,115200,n8 androidboot.hardware=i_lgu loglevel=1

MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/device.conf
include $(MIUI_RECOVERY)
