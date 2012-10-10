#
# Copyright (C) 2008 The Android Open Source Project
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
ifndef WPA_SUPPLICANT_VERSION
WPA_SUPPLICANT_VERSION := VER_0_5_X
endif

ifeq ($(WPA_SUPPLICANT_VERSION),VER_0_5_X)

LOCAL_PATH := $(call my-dir)

WPA_BUILD_SUPPLICANT := false
ifneq ($(BOARD_WPA_SUPPLICANT_DRIVER),)
  WPA_BUILD_SUPPLICANT := true
  CONFIG_DRIVER_$(BOARD_WPA_SUPPLICANT_DRIVER) = y
endif

include $(LOCAL_PATH)/.config

# To force sizeof(enum) = 4
ifeq ($(TARGET_ARCH),arm)
L_CFLAGS += -mabi=aapcs-linux
endif

# To ignore possible wrong network configurations
L_CFLAGS += -DWPA_IGNORE_CONFIG_ERRORS

# To allow non-ASCII characters in SSID
L_CFLAGS += -DWPA_UNICODE_SSID

# OpenSSL is configured without engines on Android
L_CFLAGS += -DOPENSSL_NO_ENGINE

INCLUDES = external/openssl/include frameworks/base/cmds/keystore
  
OBJS = config.c common.c md5.c md4.c rc4.c sha1.c des.c
OBJS_p = wpa_passphrase.c sha1.c md5.c md4.c common.c des.c
OBJS_c = wpa_cli.c wpa_ctrl.c

ifndef CONFIG_OS
ifdef CONFIG_NATIVE_WINDOWS
CONFIG_OS=win32
else
CONFIG_OS=unix
endif
endif

OBJS += os_$(CONFIG_OS).c
OBJS_p += os_$(CONFIG_OS).c
OBJS_c += os_$(CONFIG_OS).c

ifndef CONFIG_ELOOP
CONFIG_ELOOP=eloop
endif
OBJS += $(CONFIG_ELOOP).c


ifdef CONFIG_EAPOL_TEST
L_CFLAGS += -Werror -DEAPOL_TEST
endif

ifndef CONFIG_BACKEND
CONFIG_BACKEND=file
endif

ifeq ($(CONFIG_BACKEND), file)
OBJS += config_file.c base64.c
L_CFLAGS += -DCONFIG_BACKEND_FILE
endif

ifeq ($(CONFIG_BACKEND), winreg)
OBJS += config_winreg.c
endif

ifeq ($(CONFIG_BACKEND), none)
OBJS += config_none.c
endif

ifdef CONFIG_DRIVER_HOSTAP
L_CFLAGS += -DCONFIG_DRIVER_HOSTAP
OBJS_d += driver_hostap.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_WEXT
L_CFLAGS += -DCONFIG_DRIVER_WEXT
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_PRISM54
L_CFLAGS += -DCONFIG_DRIVER_PRISM54
OBJS_d += driver_prism54.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_HERMES
L_CFLAGS += -DCONFIG_DRIVER_HERMES
OBJS_d += driver_hermes.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_MADWIFI
L_CFLAGS += -DCONFIG_DRIVER_MADWIFI
OBJS_d += driver_madwifi.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_ATMEL
L_CFLAGS += -DCONFIG_DRIVER_ATMEL
OBJS_d += driver_atmel.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_NDISWRAPPER
L_CFLAGS += -DCONFIG_DRIVER_NDISWRAPPER
OBJS_d += driver_ndiswrapper.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_BROADCOM
L_CFLAGS += -DCONFIG_DRIVER_BROADCOM
OBJS_d += driver_broadcom.c
endif

ifdef CONFIG_DRIVER_IPW
L_CFLAGS += -DCONFIG_DRIVER_IPW
OBJS_d += driver_ipw.c
CONFIG_WIRELESS_EXTENSION=y
endif

ifdef CONFIG_DRIVER_BSD
L_CFLAGS += -DCONFIG_DRIVER_BSD
OBJS_d += driver_bsd.c
ifndef CONFIG_L2_PACKET
CONFIG_L2_PACKET=freebsd
endif
endif

ifdef CONFIG_DRIVER_NDIS
L_CFLAGS += -DCONFIG_DRIVER_NDIS
OBJS_d += driver_ndis.c driver_ndis_.c
ifndef CONFIG_L2_PACKET
CONFIG_L2_PACKET=pcap
endif
CONFIG_WINPCAP=y
ifdef CONFIG_USE_NDISUIO
L_CFLAGS += -DCONFIG_USE_NDISUIO
endif
endif

ifdef CONFIG_DRIVER_WIRED
L_CFLAGS += -DCONFIG_DRIVER_WIRED
OBJS_d += driver_wired.c
endif

ifdef CONFIG_DRIVER_TEST
L_CFLAGS += -DCONFIG_DRIVER_TEST
OBJS_d += driver_test.c
endif

ifdef CONFIG_DRIVER_CUSTOM
L_CFLAGS += -DCONFIG_DRIVER_CUSTOM
endif

ifndef CONFIG_L2_PACKET
CONFIG_L2_PACKET=linux
endif

OBJS += l2_packet_$(CONFIG_L2_PACKET).c

ifeq ($(CONFIG_L2_PACKET), pcap)
ifdef CONFIG_WINPCAP
L_CFLAGS += -DCONFIG_WINPCAP
LIBS += -lwpcap -lpacket
LIBS_w += -lwpcap
else
LIBS += -ldnet -lpcap
endif
endif

ifeq ($(CONFIG_L2_PACKET), winpcap)
LIBS += -lwpcap -lpacket
LIBS_w += -lwpcap
endif

ifeq ($(CONFIG_L2_PACKET), freebsd)
LIBS += -lpcap
endif

ifdef CONFIG_EAP_TLS
# EAP-TLS
L_CFLAGS += -DEAP_TLS
OBJS += eap_tls.c
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_PEAP
# EAP-PEAP
L_CFLAGS += -DEAP_PEAP
OBJS += eap_peap.c
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
CONFIG_EAP_TLV=y
endif

ifdef CONFIG_EAP_TTLS
# EAP-TTLS
L_CFLAGS += -DEAP_TTLS
OBJS += eap_ttls.c
MS_FUNCS=y
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_MD5
# EAP-MD5 (also used by EAP-TTLS)
L_CFLAGS += -DEAP_MD5
OBJS += eap_md5.c
CONFIG_IEEE8021X_EAPOL=y
endif

# backwards compatibility for old spelling
ifdef CONFIG_MSCHAPV2
ifndef CONFIG_EAP_MSCHAPV2
CONFIG_EAP_MSCHAPV2=y
endif
endif

ifdef CONFIG_EAP_MSCHAPV2
# EAP-MSCHAPv2 (also used by EAP-PEAP)
L_CFLAGS += -DEAP_MSCHAPv2
OBJS += eap_mschapv2.c
MS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_GTC
# EAP-GTC (also used by EAP-PEAP)
L_CFLAGS += -DEAP_GTC
OBJS += eap_gtc.c
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_OTP
# EAP-OTP
L_CFLAGS += -DEAP_OTP
OBJS += eap_otp.c
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_SIM
# EAP-SIM
L_CFLAGS += -DEAP_SIM
OBJS += eap_sim.c
CONFIG_IEEE8021X_EAPOL=y
CONFIG_EAP_SIM_COMMON=y
endif

ifdef CONFIG_EAP_LEAP
# EAP-LEAP
L_CFLAGS += -DEAP_LEAP
OBJS += eap_leap.c
MS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_PSK
# EAP-PSK
L_CFLAGS += -DEAP_PSK
OBJS += eap_psk.c eap_psk_common.c
CONFIG_IEEE8021X_EAPOL=y
NEED_AES=y
endif

ifdef CONFIG_EAP_AKA
# EAP-AKA
L_CFLAGS += -DEAP_AKA
OBJS += eap_aka.c
CONFIG_IEEE8021X_EAPOL=y
CONFIG_EAP_SIM_COMMON=y
endif

ifdef CONFIG_EAP_SIM_COMMON
OBJS += eap_sim_common.c
NEED_AES=y
endif

ifdef CONFIG_EAP_TLV
# EAP-TLV
L_CFLAGS += -DEAP_TLV
OBJS += eap_tlv.c
endif

ifdef CONFIG_EAP_FAST
# EAP-FAST
L_CFLAGS += -DEAP_FAST
OBJS += eap_fast.c
TLS_FUNCS=y
endif

ifdef CONFIG_EAP_PAX
# EAP-PAX
L_CFLAGS += -DEAP_PAX
OBJS += eap_pax.c eap_pax_common.c
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_SAKE
# EAP-SAKE
L_CFLAGS += -DEAP_SAKE
OBJS += eap_sake.c eap_sake_common.c
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_GPSK
# EAP-GPSK
L_CFLAGS += -DEAP_GPSK
OBJS += eap_gpsk.c eap_gpsk_common.c
CONFIG_IEEE8021X_EAPOL=y
ifdef CONFIG_EAP_GPSK_SHA256
L_CFLAGS += -DEAP_GPSK_SHA256
NEED_SHA256=y
endif
endif
ifdef CONFIG_EAP_VENDOR_TEST
L_CFLAGS += -DEAP_VENDOR_TEST
OBJS += eap_vendor_test.c
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_IEEE8021X_EAPOL
# IEEE 802.1X/EAPOL state machines (e.g., for RADIUS authentication)
L_CFLAGS += -DIEEE8021X_EAPOL
OBJS += eapol_sm.c eap.c eap_methods.c
endif

ifdef CONFIG_PCSC
# PC/SC interface for smartcards (USIM, GSM SIM)
L_CFLAGS += -DPCSC_FUNCS -I/usr/include/PCSC
OBJS += pcsc_funcs.c
# -lpthread may not be needed depending on how pcsc-lite was configured
ifdef CONFIG_NATIVE_WINDOWS
#Once MinGW gets support for WinScard, -lwinscard could be used instead of the
#dynamic symbol loading that is now used in pcsc_funcs.c
#LIBS += -lwinscard
else
LIBS += -lpcsclite -lpthread
endif
endif

ifndef CONFIG_TLS
CONFIG_TLS=openssl
# CONFIG_TLS=internal
endif

ifeq ($(CONFIG_TLS), internal)
ifndef CONFIG_CRYPTO
CONFIG_CRYPTO=internal
endif
endif
ifeq ($(CONFIG_CRYPTO), libtomcrypt)
L_CFLAGS += -DCONFIG_INTERNAL_X509
endif
ifeq ($(CONFIG_CRYPTO), internal)
L_CFLAGS += -DCONFIG_INTERNAL_X509
endif


ifdef TLS_FUNCS
# Shared TLS functions (needed for EAP_TLS, EAP_PEAP, EAP_TTLS, and EAP_FAST)
L_CFLAGS += -DEAP_TLS_FUNCS
OBJS += eap_tls_common.c
ifeq ($(CONFIG_TLS), openssl)
L_CFLAGS += -DEAP_TLS_OPENSSL
OBJS += tls_openssl.c
LIBS += -lssl -lcrypto
LIBS_p += -lcrypto
endif
ifeq ($(CONFIG_TLS), gnutls)
OBJS += tls_gnutls.c
LIBS += -lgnutls -lgcrypt -lgpg-error
LIBS_p += -lgcrypt
ifdef CONFIG_GNUTLS_EXTRA
L_CFLAGS += -DCONFIG_GNUTLS_EXTRA
LIBS += -lgnutls-extra
endif
endif
ifeq ($(CONFIG_TLS), schannel)
OBJS += tls_schannel.c
endif
ifeq ($(CONFIG_TLS), internal)
OBJS += tls_internal.c tlsv1_common.c tlsv1_client.c asn1.c x509v3.c
OBJS_p += asn1.c rc4.c aes_wrap.c
ifneq ($(CONFIG_BACKEND), file)
OBJS += base64.c
endif
L_CFLAGS += -DCONFIG_TLS_INTERNAL
ifeq ($(CONFIG_CRYPTO), internal)
ifdef CONFIG_INTERNAL_LIBTOMMATH
L_CFLAGS += -DCONFIG_INTERNAL_LIBTOMMATH
else
LIBS += -ltommath
LIBS_p += -ltommath
endif
endif
ifeq ($(CONFIG_CRYPTO), libtomcrypt)
LIBS += -ltomcrypt -ltfm
LIBS_p += -ltomcrypt -ltfm
endif
endif
ifeq ($(CONFIG_TLS), none)
OBJS += tls_none.c
L_CFLAGS += -DEAP_TLS_NONE
CONFIG_INTERNAL_AES=y
CONFIG_INTERNAL_SHA1=y
CONFIG_INTERNAL_MD5=y
CONFIG_INTERNAL_SHA256=y
endif
ifdef CONFIG_SMARTCARD
ifndef CONFIG_NATIVE_WINDOWS
ifneq ($(CONFIG_L2_PACKET), freebsd)
LIBS += -ldl
endif
endif
endif
NEED_CRYPTO=y
else
OBJS += tls_none.c
endif

ifdef CONFIG_PKCS12
L_CFLAGS += -DPKCS12_FUNCS
endif

ifdef CONFIG_SMARTCARD
L_CFLAGS += -DCONFIG_SMARTCARD
endif

ifdef MS_FUNCS
OBJS += ms_funcs.c
NEED_CRYPTO=y
endif

ifdef NEED_CRYPTO
ifndef TLS_FUNCS
ifeq ($(CONFIG_TLS), openssl)
LIBS += -lcrypto
LIBS_p += -lcrypto
endif
ifeq ($(CONFIG_TLS), gnutls)
LIBS += -lgcrypt
LIBS_p += -lgcrypt
endif
ifeq ($(CONFIG_TLS), schannel)
endif
ifeq ($(CONFIG_TLS), internal)
ifeq ($(CONFIG_CRYPTO), libtomcrypt)
LIBS += -ltomcrypt -ltfm
LIBS_p += -ltomcrypt -ltfm
endif
endif
endif
ifeq ($(CONFIG_TLS), openssl)
OBJS += crypto.c
OBJS_p += crypto.c
CONFIG_INTERNAL_SHA256=y
endif
ifeq ($(CONFIG_TLS), gnutls)
OBJS += crypto_gnutls.c
OBJS_p += crypto_gnutls.c
CONFIG_INTERNAL_SHA256=y
endif
ifeq ($(CONFIG_TLS), schannel)
OBJS += crypto_cryptoapi.c
OBJS_p += crypto_cryptoapi.c
CONFIG_INTERNAL_SHA256=y
endif
ifeq ($(CONFIG_TLS), internal)
ifeq ($(CONFIG_CRYPTO), libtomcrypt)
OBJS += crypto_libtomcrypt.c
OBJS_p += crypto_libtomcrypt.c
CONFIG_INTERNAL_SHA256=y
endif
ifeq ($(CONFIG_CRYPTO), internal)
OBJS += crypto_internal.c rsa.c bignum.c
OBJS_p += crypto_internal.c rsa.c bignum.c
L_CFLAGS += -DCONFIG_CRYPTO_INTERNAL
CONFIG_INTERNAL_AES=y
CONFIG_INTERNAL_DES=y
CONFIG_INTERNAL_SHA1=y
CONFIG_INTERNAL_MD4=y
CONFIG_INTERNAL_MD5=y
CONFIG_INTERNAL_SHA256=y
endif
ifeq ($(CONFIG_CRYPTO), cryptoapi)
OBJS += crypto_cryptoapi.c
OBJS_p += crypto_cryptoapi.c
L_CFLAGS += -DCONFIG_CRYPTO_CRYPTOAPI
CONFIG_INTERNAL_SHA256=y
endif
endif
ifeq ($(CONFIG_TLS), none)
OBJS += crypto_none.c
OBJS_p += crypto_none.c
CONFIG_INTERNAL_SHA256=y
endif
else
CONFIG_INTERNAL_AES=y
CONFIG_INTERNAL_SHA1=y
CONFIG_INTERNAL_MD5=y
endif

ifdef CONFIG_INTERNAL_AES
L_CFLAGS += -DINTERNAL_AES
endif
ifdef CONFIG_INTERNAL_SHA1
L_CFLAGS += -DINTERNAL_SHA1
endif
ifdef CONFIG_INTERNAL_SHA256
L_CFLAGS += -DINTERNAL_SHA256
endif
ifdef CONFIG_INTERNAL_MD5
L_CFLAGS += -DINTERNAL_MD5
endif
ifdef CONFIG_INTERNAL_MD4
L_CFLAGS += -DINTERNAL_MD4
endif
ifdef CONFIG_INTERNAL_DES
L_CFLAGS += -DINTERNAL_DES
endif

ifdef NEED_SHA256
OBJS += sha256.c
endif

ifdef CONFIG_WIRELESS_EXTENSION
L_CFLAGS += -DCONFIG_WIRELESS_EXTENSION
OBJS_d += driver_wext.c
endif

ifdef CONFIG_CTRL_IFACE
ifeq ($(CONFIG_CTRL_IFACE), y)
ifdef CONFIG_NATIVE_WINDOWS
CONFIG_CTRL_IFACE=udp
else
CONFIG_CTRL_IFACE=unix
endif
endif
L_CFLAGS += -DCONFIG_CTRL_IFACE
ifeq ($(CONFIG_CTRL_IFACE), udp)
L_CFLAGS += -DCONFIG_CTRL_IFACE_UDP
else
L_CFLAGS += -DCONFIG_CTRL_IFACE_UNIX
endif
OBJS += ctrl_iface.c ctrl_iface_$(CONFIG_CTRL_IFACE).c
endif

ifdef CONFIG_READLINE
L_CFLAGS += -DCONFIG_READLINE
LIBS_c += -lncurses -lreadline
endif

ifdef CONFIG_NATIVE_WINDOWS
L_CFLAGS += -DCONFIG_NATIVE_WINDOWS
LIBS += -lws2_32 -lgdi32 -lcrypt32
LIBS_c += -lws2_32
LIBS_p += -lws2_32
ifeq ($(CONFIG_CRYPTO), cryptoapi)
LIBS_p += -lcrypt32
endif
endif

ifdef CONFIG_NO_STDOUT_DEBUG
L_CFLAGS += -DCONFIG_NO_STDOUT_DEBUG
ifndef CONFIG_CTRL_IFACE
CFLAGS += -DCONFIG_NO_WPA_MSG
endif
endif

ifdef CONFIG_IPV6
# for eapol_test only
L_CFLAGS += -DCONFIG_IPV6
endif

ifdef CONFIG_PEERKEY
L_CFLAGS += -DCONFIG_PEERKEY
endif

ifdef CONFIG_IEEE80211W
L_CFLAGS += -DCONFIG_IEEE80211W
NEED_SHA256=y
endif

ifndef CONFIG_NO_WPA
OBJS += wpa.c preauth.c pmksa_cache.c
NEED_AES=y
else
L_CFLAGS += -DCONFIG_NO_WPA -DCONFIG_NO_WPA2
endif

ifdef CONFIG_NO_WPA2
L_CFLAGS += -DCONFIG_NO_WPA2
endif

ifdef CONFIG_NO_AES_EXTRAS
L_CFLAGS += -DCONFIG_NO_AES_WRAP
L_CFLAGS += -DCONFIG_NO_AES_CTR -DCONFIG_NO_AES_OMAC1
L_CFLAGS += -DCONFIG_NO_AES_EAX -DCONFIG_NO_AES_CBC
endif

ifdef NEED_AES
OBJS += aes_wrap.c
endif

ifdef CONFIG_CLIENT_MLME
OBJS += mlme.c
L_CFLAGS += -DCONFIG_CLIENT_MLME
endif

ifndef CONFIG_MAIN
CONFIG_MAIN=main
endif

ifdef CONFIG_DEBUG_FILE
L_CFLAGS += -DCONFIG_DEBUG_FILE
endif

OBJS += wpa_supplicant.c events.c
OBJS_t := $(OBJS) eapol_test.c radius.c radius_client.c
OBJS_t2 := $(OBJS) preauth_test.c
OBJS += $(CONFIG_MAIN).c drivers.c $(OBJS_d)

ifdef CONFIG_NDIS_EVENTS_INTEGRATED
L_CFLAGS += -DCONFIG_NDIS_EVENTS_INTEGRATED
OBJS += ndis_events.c
EXTRALIBS += -loleaut32 -lole32 -luuid
ifdef PLATFORMSDKLIB
EXTRALIBS += $(PLATFORMSDKLIB)/WbemUuid.Lib
else
EXTRALIBS += WbemUuid.Lib
endif
endif

ifeq ($(WPA_BUILD_SUPPLICANT),true)

########################

include $(CLEAR_VARS)
LOCAL_MODULE := wpa_cli
LOCAL_MODULE_TAGS := debug
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(OBJS_c)
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_EXECUTABLE)

########################
include $(CLEAR_VARS)
LOCAL_MODULE := wpa_supplicant
ifdef CONFIG_DRIVER_CUSTOM
LOCAL_STATIC_LIBRARIES := libCustomWifi
endif
ifneq ($(BOARD_WPA_SUPPLICANT_PRIVATE_LIB),)
LOCAL_STATIC_LIBRARIES += $(BOARD_WPA_SUPPLICANT_PRIVATE_LIB)
endif
LOCAL_SHARED_LIBRARIES := libc libcutils libcrypto libssl
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(OBJS)
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_EXECUTABLE)

########################
#
#include $(CLEAR_VARS)
#LOCAL_MODULE := eapol_test
#ifdef CONFIG_DRIVER_CUSTOM
#LOCAL_STATIC_LIBRARIES := libCustomWifi
#endif
#LOCAL_SHARED_LIBRARIES := libc libcrypto libssl
#LOCAL_CFLAGS := $(L_CFLAGS)
#LOCAL_SRC_FILES := $(OBJS_t)
#LOCAL_C_INCLUDES := $(INCLUDES)
#include $(BUILD_EXECUTABLE)
#
########################
#
#local_target_dir := $(TARGET_OUT)/etc/wifi
#
#include $(CLEAR_VARS)
#LOCAL_MODULE := wpa_supplicant.conf
#LOCAL_MODULE_TAGS := user
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(local_target_dir)
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#include $(BUILD_PREBUILT)
#
########################

endif # ifeq ($(WPA_BUILD_SUPPLICANT),true)

include $(CLEAR_VARS)
LOCAL_MODULE = libwpa_client
LOCAL_CFLAGS = $(L_CFLAGS)
LOCAL_SRC_FILES = wpa_ctrl.c os_unix.c
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_COPY_HEADERS_TO := libwpa_client
LOCAL_COPY_HEADERS := wpa_ctrl.h
include $(BUILD_SHARED_LIBRARY)

endif # VER_0_5_X
