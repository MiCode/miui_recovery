$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

$(call inherit-product-if-exists, vendor/zte/u5/u5-vendor.mk)

DEVICE_PACKAGE_OVERLAYS += device/zte/u5/overlay

LOCAL_PATH := device/zte/u5
ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := $(LOCAL_PATH)/kernel
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

ifeq ($(TARGET_RECOVERY_INITRC),)
	LOCAL_INITRC := $(LOCAL_PATH)/init.rc
else
	LOCAL_INITRC := $(TARGET_RECOVERY_INITRC)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel \
    $(LOCAL_INITRC):init.rc



$(call inherit-product, build/target/product/full.mk)

PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
PRODUCT_NAME := full_u5
PRODUCT_DEVICE := u5
