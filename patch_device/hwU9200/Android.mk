LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := p1
MIUI_KERNEL := $(LOCAL_PATH)/kernel

MIUI_KERNEL_BASE := 0x80000000
MIUI_KERNEL_PAGESIZE := 2048
MIUI_KERNEL_CMDLINE := console=ttyGS2,115200n8 mem=1G vmalloc=768M omap_wdt.timer_margin=30 mmcparts=mmcblk0:p15(splash)

MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/*.conf
include $(MIUI_RECOVERY)
