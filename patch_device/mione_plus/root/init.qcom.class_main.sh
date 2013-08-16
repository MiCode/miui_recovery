#!/system/bin/sh
# Copyright (c) 2012, Code Aurora Forum. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Code Aurora nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# start ril-daemon only for targets on which radio is present
#
baseband=`getprop ro.baseband`
multirild=`getprop ro.multi.rild`
dsds=`getprop persist.dsds.enabled`
netmgr=`getprop ro.use_data_netmgrd`

case "$baseband" in
    "apq")
    setprop ro.radio.noril yes
    stop ril-daemon
esac

case "$baseband" in
    "msm" | "csfb" | "svlte2a" | "mdm" | "sglte" | "unknown")
    start qmuxd
    case "$baseband" in
        "svlte2a" | "csfb" | "sglte")
        start qmiproxy
    esac
    case "$multirild" in
        "true")
         case "$dsds" in
             "true")
             start ril-daemon1
         esac
    esac
    case "$netmgr" in
        "true")
        start netmgrd
    esac
esac

#
# enable bluetooth profiles dynamically
#
case $baseband in
  "apq")
      setprop ro.qualcomm.bluetooth.opp true
      setprop ro.qualcomm.bluetooth.hfp false
      setprop ro.qualcomm.bluetooth.hsp false
      setprop ro.qualcomm.bluetooth.pbap true
      setprop ro.qualcomm.bluetooth.ftp true
      setprop ro.qualcomm.bluetooth.map true
      setprop ro.qualcomm.bluetooth.nap false
      setprop ro.qualcomm.bluetooth.sap false
      setprop ro.qualcomm.bluetooth.dun false
      ;;
  "mdm" | "svlte2a" | "svlte1" | "csfb")
      setprop ro.qualcomm.bluetooth.opp true
      setprop ro.qualcomm.bluetooth.hfp true
      setprop ro.qualcomm.bluetooth.hsp true
      setprop ro.qualcomm.bluetooth.pbap true
      setprop ro.qualcomm.bluetooth.ftp true
      setprop ro.qualcomm.bluetooth.map true
      setprop ro.qualcomm.bluetooth.nap true
      setprop ro.qualcomm.bluetooth.sap true
      setprop ro.qualcomm.bluetooth.dun false
      ;;
  "msm")
      setprop ro.qualcomm.bluetooth.opp true
      setprop ro.qualcomm.bluetooth.hfp true
      setprop ro.qualcomm.bluetooth.hsp true
      setprop ro.qualcomm.bluetooth.pbap true
      setprop ro.qualcomm.bluetooth.ftp true
      setprop ro.qualcomm.bluetooth.map true
      setprop ro.qualcomm.bluetooth.nap true
      setprop ro.qualcomm.bluetooth.sap true
      setprop ro.qualcomm.bluetooth.dun true
      ;;
  "mpq")
      setprop ro.qualcomm.bluetooth.opp false
      setprop ro.qualcomm.bluetooth.hfp false
      setprop ro.qualcomm.bluetooth.hsp false
      setprop ro.qualcomm.bluetooth.pbap false
      setprop ro.qualcomm.bluetooth.ftp false
      setprop ro.qualcomm.bluetooth.map false
      setprop ro.qualcomm.bluetooth.nap false
      setprop ro.qualcomm.bluetooth.sap false
      setprop ro.qualcomm.bluetooth.dun false
      ;;
  *)
      setprop ro.qualcomm.bluetooth.opp true
      setprop ro.qualcomm.bluetooth.hfp true
      setprop ro.qualcomm.bluetooth.hsp true
      setprop ro.qualcomm.bluetooth.pbap true
      setprop ro.qualcomm.bluetooth.ftp true
      setprop ro.qualcomm.bluetooth.map true
      setprop ro.qualcomm.bluetooth.nap true
      setprop ro.qualcomm.bluetooth.sap true
      setprop ro.qualcomm.bluetooth.dun true
      ;;
esac
