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
LOCAL_KERNEL := device/zte/n880f/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif



PRODUCT_COPY_FILES := \
	$(LOCAL_KERNEL):kernel \
	device/zte/n880f/init.rc:root/init.rc \
	device/zte/n880f/init.qcom.usb.rc:root/init.qcom.usb.rc \
	device/zte/n880f/init.qcom.rc:root/init.qcom.rc \
	device/zte/n880f/init.goldfish.rc:root/init.goldfish.rc \
	device/zte/n880f/init.target.rc:root/init.target.rc \
	device/zte/n880f/ueventd.rc:root/ueventd.rc \
	device/zte/n880f/ueventd.goldfish.rc:root/ueventd.goldfish.rc \
	device/zte/n880f/media_profiles.xml:system/etc/media_profiles.xml \
	device/zte/n880f/gps.conf:system/etc/gps.conf 

# Wifi 
ifneq ($(TARGET_PREBUILT_WIFI_MODULE),)
	$(TARGET_PREBUILT_WIFI_MODULE):system/lib/modules/ath6kl/ath6kl_sdio.ko 
endif 

PRODUCT_COPY_FILES += \
		      device/zte/n880f/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf 

PRODUCT_PROPERTY_OVERRIDES := \
	wifi.interface=wlan0 \
	wifi.supplicant_scan_interval=15

# Set default USB interface 
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
				      persist.sys.usb.config=mtp

# Key maps 
PRODUCT_COPY_FILES += \
		      device/zte/n880f/keychars/qwerty2.kcm:system/usr/keychars/qwerty2.kcm \
		      device/zte/n880f/keychars/qwerty.kcm:system/usr/keychars/qwerty.kcm \
		      device/zte/n880f/keychars/Virtual.kcm:system/usr/keychars/Virtual.kcm \
		      device/zte/n880f/keychars/Generic.kcm:system/usr/keychars/Generic.kcm \
		      device/zte/n880f/keychars/7x27a_kp.kcm:system/usr/keychars/7x27a_kp.kcm \
		      device/zte/n880f/keylayout/surf_keypad.kl:system/usr/keylayout/surf_keypad.kl \
		      device/zte/n880f/keylayout/qwerty.kl:system/usr/keylayout/qwerty.kl \
		      device/zte/n880f/keylayout/ft5x06_ts.kl:system/usr/keylayout/ftx5x06_ts.kl \
		      device/zte/n880f/keylayout/atmel_mxt_ts.kl:system/usr/keylayout/atmel_mxt_ts.kl \
		      device/zte/n880f/keylayout/Vendor_22b8_Product_093d.kl:system/usr/keylayout/Vendor_22b8_Product_093d.kl \
		      device/zte/n880f/keylayout/Vendor_054c_Product_0268.kl:system/usr/keylayout/Vendor_054c_Product_0268.kl \
		      device/zte/n880f/keylayout/Vendor_046d_Product_c532.kl:system/usr/keylayout/Vendor_046d_Product_c532.kl \
		      device/zte/n880f/keylayout/Vendor_046d_Product_c299.kl:system/usr/keylayout/Vendor_046d_Product_c299.kl \
		      device/zte/n880f/keylayout/Vendor_046d_Product_c294.kl:system/usr/keylayout/Vendor_046d_Product_c294.kl \
		      device/zte/n880f/keylayout/Vendor_046d_Product_c216.kl:system/usr/keylayout/Vendor_046d_Product_c216.kl \
		      device/zte/n880f/keylayout/Vendor_045e_Product_028e.kl:system/usr/keylayout/Vendor_045e_Product_028e.kl \
		      device/zte/n880f/keylayout/Generic.kl:system/usr/keylayout/Generic.kl \
		      device/zte/n880f/keylayout/AVRCP.kl:system/usr/keylayout/AVRCP.kl \
		      device/zte/n880f/keylayout/7x27a_kp.kl:system/usr/keylayot/7x27a_kp.kl \
		      device/zte/n880f/keylayout/7k_handset.kl:system/usr/keylayout/7k_handset.kl

# Input device calibration files 
PRODUCT_COPY_FILES += \
		      device/zte/n880f/qwerty.idc:system/usr/idc/qwerty.idc \
		      device/zte/n880f/qwerty2.idc:system/usr/idc/qwerty2.idc 


# Yamato touchscreen firmware 
PRODUCT_COPY_FILES += \
		      device/zte/n880f/yamato_pfp.fw:system/etc/firmware/yamato_pfp.fw \
		      device/zte/n880f/yamato_pm4.fw:system/etc/firmware/yamato_pm4.fw 

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=240 

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs

# for bugmailer / bugreport 
PRODUCT_PACKAGES += bugreport 
PRODUCT_COPY_FILES += \
		      device/zte/n880f/bugreport:system/bin/bugreport 

$(call inherit-product, hardware/msm7k/Android.mk)
$(call inherit-product-if-exists, vendor/zte/n880f/device-vendor.mk)

BOARD_WLAN_DEVICE_REC := ath6kl_sdio 
WIFI_BRAND            := wlan

WPA_SUPPLICANT_VERSION := VER_0_8_X

$(call inherit-product-if-exists, hardware/atheros/wlan/ath6kl/Android.mk)



