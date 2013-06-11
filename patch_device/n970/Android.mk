LOCAL_PATH := $(call my-dir)
include $(MIUI_CLEAR)
MIUI_PRODUCT := n970
MIUI_KERNEL := $(LOCAL_PATH)/kernel
MIUI_KERNEL_BASE := 0x80200000
MIUI_KERNEL_PAGESIZE := 2048
MIUI_KERNEL_CMDLINE := androidboot.hardware=qcom user_debug=31 loglevel=7 kgsl.mmutype=gpummu
MIUI_PRODUCT_ROOT := $(LOCAL_PATH)/root
MIUI_DEVICE_CONFIG := $(LOCAL_PATH)/*.conf
include $(MIUI_RECOVERY)
