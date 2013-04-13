#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This file includes all definitions that apply to ALL tuna devices, and
# are also specific to tuna devices
#
# Everything in this directory will become public

ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := device/samsung/tuna/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

DEVICE_PACKAGE_OVERLAYS := device/samsung/tuna/overlay

# This device is xhdpi.  However the platform doesn't
# currently contain all of the bitmaps at xhdpi density so
# we do this little trick to fall back to the hdpi version
# if the xhdpi doesn't exist.
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

PRODUCT_PACKAGES := \
	lights.tuna \
	charger \
	charger_res_images

PRODUCT_PACKAGES += \
	sensors.tuna \
	libinvensense_mpl

PRODUCT_PACKAGES += \
	nfc.tuna

PRODUCT_PACKAGES += \
	audio.primary.tuna \
	audio.a2dp.default \
	libaudioutils

PRODUCT_PACKAGES += \
	tuna_hdcp_keys

PRODUCT_COPY_FILES := \
	$(LOCAL_KERNEL):kernel \
	device/samsung/tuna/init.tuna.rc:root/init.tuna.rc \
	device/samsung/tuna/init.tuna.usb.rc:root/init.tuna.usb.rc \
	device/samsung/tuna/ueventd.tuna.rc:root/ueventd.tuna.rc \
	device/samsung/tuna/media_profiles.xml:system/etc/media_profiles.xml \
	device/samsung/tuna/gps.conf:system/etc/gps.conf

# Bluetooth configuration files
PRODUCT_COPY_FILES += \
	system/bluetooth/data/main.le.conf:system/etc/bluetooth/main.conf

# Wifi
ifneq ($(TARGET_PREBUILT_WIFI_MODULE),)
PRODUCT_COPY_FILES += \
	$(TARGET_PREBUILT_WIFI_MODULE):system/lib/modules/bcmdhd.ko
endif
PRODUCT_COPY_FILES += \
	device/samsung/tuna/bcmdhd.cal:system/etc/wifi/bcmdhd.cal

PRODUCT_PROPERTY_OVERRIDES := \
	wifi.interface=wlan0 \
	wifi.supplicant_scan_interval=15

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp


# Key maps
PRODUCT_COPY_FILES += \
	device/samsung/tuna/tuna-gpio-keypad.kl:system/usr/keylayout/tuna-gpio-keypad.kl \
	device/samsung/tuna/tuna-gpio-keypad.kcm:system/usr/keychars/tuna-gpio-keypad.kcm \
	device/samsung/tuna/sec_jack.kl:system/usr/keylayout/sec_jack.kl \
	device/samsung/tuna/sec_jack.kcm:system/usr/keychars/sec_jack.kcm

# Input device calibration files
PRODUCT_COPY_FILES += \
	device/samsung/tuna/Melfas_MMSxxx_Touchscreen.idc:system/usr/idc/Melfas_MMSxxx_Touchscreen.idc


# HACK: copy panda init for now to boot on both boards
PRODUCT_COPY_FILES += \
	device/ti/panda/init.omap4pandaboard.rc:root/init.omap4pandaboard.rc

# Melfas touchscreen firmware
PRODUCT_COPY_FILES += \
    device/samsung/tuna/mms144_ts_rev31.fw:system/vendor/firmware/mms144_ts_rev31.fw \
    device/samsung/tuna/mms144_ts_rev32.fw:system/vendor/firmware/mms144_ts_rev32.fw

# Portrait dock image
PRODUCT_COPY_FILES += \
    device/samsung/tuna/dock.png:system/vendor/res/images/dock/dock.png

# Commands to migrate prefs from com.android.nfc3 to com.android.nfc
PRODUCT_COPY_FILES += \
	packages/apps/Nfc/migrate_nfc.txt:system/etc/updatecmds/migrate_nfc.txt

# file that declares the MIFARE NFC constant
PRODUCT_COPY_FILES += \
	device/sample/nxp/com.nxp.mifare.xml:system/etc/permissions/com.nxp.mifare.xml

# NFC EXTRAS add-on API
PRODUCT_PACKAGES += \
	com.android.nfc_extras

# NFCEE access control
ifeq ($(TARGET_BUILD_VARIANT),user)
    NFCEE_ACCESS_PATH := device/samsung/tuna/nfcee_access.xml
else
    NFCEE_ACCESS_PATH := device/samsung/tuna/nfcee_access_debug.xml
endif
PRODUCT_COPY_FILES += \
    $(NFCEE_ACCESS_PATH):system/etc/nfcee_access.xml

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=320

PRODUCT_CHARACTERISTICS := nosdcard

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
	librs_jni \
	com.android.future.usb.accessory

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs

# for bugmailer
PRODUCT_PACKAGES += send_bug
PRODUCT_COPY_FILES += \
	system/extras/bugmailer/bugmailer.sh:system/bin/bugmailer.sh \
	system/extras/bugmailer/send_bug:system/bin/send_bug


$(call inherit-product-if-exists, vendor/nxp/pn544/nxp-pn544-fw-vendor.mk)
$(call inherit-product, hardware/ti/omap4xxx/omap4.mk)
$(call inherit-product-if-exists, vendor/ti/proprietary/omap4/ti-omap4-vendor.mk)
$(call inherit-product-if-exists, vendor/samsung/tuna/device-vendor.mk)

BOARD_WLAN_DEVICE_REV := bcm4330_b2
WIFI_BAND             := 802_11_ABG
$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/device-bcm.mk)
