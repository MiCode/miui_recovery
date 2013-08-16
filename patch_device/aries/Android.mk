LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := aries
MIUI_KERNEL := $(LOCAL_PATH)/kernel

MIUI_KERNEL_BASE := 0x80200000 --ramdiskaddr 0x82200000
MIUI_KERNEL_CMDLINE := console=null androidboot.hardware=qcom ehci-hcd.park=3 maxcpus=2
MIUI_KERNEL_PAGESIZE := 2048

MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/device.conf

BOARD_HAS_DUALSYSTEM_PARTITIONS := true
TARGET_NEEDS_VSYNC := true
include $(MIUI_RECOVERY)
