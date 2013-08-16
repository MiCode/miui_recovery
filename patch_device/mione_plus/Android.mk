LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := mione_plus
MIUI_KERNEL := $(LOCAL_PATH)/kernel

MIUI_KERNEL_BASE := 0x40200000 --ramdiskaddr 0x41400000
MIUI_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.hardware=qcom kgsl.mmutype=gpummu vmalloc=400M
MIUI_KERNEL_PAGESIZE := 2048

MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/device.conf

#TARGET_USE_PIXEL_FORMAT_BGR565 := true
BOARD_HAS_DUALSYSTEM_PARTITIONS := true
TARGET_NEEDS_VSYNC := true
include $(MIUI_RECOVERY)
