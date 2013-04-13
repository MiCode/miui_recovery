# Copyright (C) 2010 The Android Open Source Project
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

LOCAL_PATH := vendor/imgtec/crespo

# Imgtec blobs necessary for Nexus S hardware
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/proprietary/pvrsrvinit:system/vendor/bin/pvrsrvinit \
    $(LOCAL_PATH)/proprietary/libEGL_POWERVR_SGX540_120.so:system/vendor/lib/egl/libEGL_POWERVR_SGX540_120.so \
    $(LOCAL_PATH)/proprietary/libGLESv1_CM_POWERVR_SGX540_120.so:system/vendor/lib/egl/libGLESv1_CM_POWERVR_SGX540_120.so \
    $(LOCAL_PATH)/proprietary/libGLESv2_POWERVR_SGX540_120.so:system/vendor/lib/egl/libGLESv2_POWERVR_SGX540_120.so \
    $(LOCAL_PATH)/proprietary/gralloc.s5pc110.so:system/vendor/lib/hw/gralloc.s5pc110.so \
    $(LOCAL_PATH)/proprietary/libglslcompiler.so:system/vendor/lib/libglslcompiler.so \
    $(LOCAL_PATH)/proprietary/libIMGegl.so:system/vendor/lib/libIMGegl.so \
    $(LOCAL_PATH)/proprietary/libpvr2d.so:system/vendor/lib/libpvr2d.so \
    $(LOCAL_PATH)/proprietary/libpvrANDROID_WSEGL.so:system/vendor/lib/libpvrANDROID_WSEGL.so \
    $(LOCAL_PATH)/proprietary/libPVRScopeServices.so:system/vendor/lib/libPVRScopeServices.so \
    $(LOCAL_PATH)/proprietary/libsrv_init.so:system/vendor/lib/libsrv_init.so \
    $(LOCAL_PATH)/proprietary/libsrv_um.so:system/vendor/lib/libsrv_um.so \
    $(LOCAL_PATH)/proprietary/libusc.so:system/vendor/lib/libusc.so
