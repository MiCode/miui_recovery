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

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk

# Use the non-open-source parts, if they're present
-include vendor/zte/n880f/BoardConfigVendor.mk   #need to modify 

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a
TARGET_ARCH_VRAIANT_CPU := cortex-a5
TARGET_ARCH_VARIANT_FPU := neon 
ARCH_ARM_HAVE_VFP := true
ARCH_ARM_HAVE_NEON := true
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_HAVE_ARMV7A := true
ARCH_ARM_HAVE_VFP := true 
TARGET_GLOBAL_CFLAGS += -mtune=cortex-a5 -mfpu=neon -mfloat-abi=softfp
TARGET_GLOBAL_CPPFLAGS += -mtune=cortex-a5 -mfpu=neon -mfloat-abi=softfp

TARGET_BOARD_PLATFORM := msm7627a


BOARD_EGL_CFG := device/zte/n880f/egl.cfg 

BOARD_HAVE_BLUETOOTH := true 
BOARD_HAVE_BLUETOOTH_BCM := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := false

TARGET_NO_RADIOIMAGE := true 
TARGET_PROVIDES_INIT_TARGET_RC := true 


TARGET_SEC_INTERNAL_STORAGE := true

BOARD_USES_GENERIC_AUDIO := false

DEFAULT_FB_NUM := 2

BOARD_NAND_PAGE_SIZE := 4096 
BOARD_NANA_SPARE_SIZE := 128
# Kernel 
TARGET_PREBUILT_KERNEL := device/zte/n880f/kernel
BOARD_KERNEL_BASE := 0x00f45004
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.hardware=qcom

#TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
TARGET_RECOVERY_UI_LIB := librecovery_ui_n880f
TARGET_RELEASETOOLS_EXTENSIONS := device/zte/n880f

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_USES_MMCUTILS := true
BOARD_HAS_LARGE_FILESYSTEM := true 

BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
BOARD_USERDATAIMAGE_PARTITION_SIZE := 1073741824
BOARD_FLASH_BLOCK_SIZE := 262144 #(BOARD_KERNEL_PAGESIZE * 64)

# connectivity - Wi-Fi

# Vold and FS
BOARD_VOLD_MAX_PARTITIONS := 20
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
BOARD_UMS_LUNFILE := "/sys/device/platform/msm_hsusb/gadget/lun0/file"
BOARD_UMS_2ND_LUNFILE := "/sys/device/platform/msm_hsusb/gadget/lun1/file"
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/platform/msm_hsusb/gadget/lun%d/file"










