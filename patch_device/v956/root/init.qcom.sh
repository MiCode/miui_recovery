#!/system/bin/sh
# Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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

target=`getprop ro.board.platform`

#
# Function to start sensors for DSPS enabled platforms
#
start_sensors()
{
    mkdir -p /data/system/sensors
    touch /data/system/sensors/settings
    chmod 775 /data/system/sensors
    chmod 664 /data/system/sensors/settings

    mkdir -p /data/misc/sensors
    chmod 775 /data/misc/sensors

    if [ ! -s /data/system/sensors/settings ]; then
        # If the settings file is empty, enable sensors HAL
        # Otherwise leave the file with it's current contents
        echo 1 > /data/system/sensors/settings
    fi
    start sensors
}

start_battery_monitor()
{
	chown root.system /sys/module/pm8921_bms/parameters/*
	chmod 0660 /sys/module/pm8921_bms/parameters/*
	mkdir -p /data/bms
	chown root.system /data/bms
	chmod 0770 /data/bms
	start battery_monitor
}

#
# set the ro.hw_platform property for identify this board
#
setprop ro.hw_platform "`cat /sys/devices/system/soc/soc0/hw_platform`"

#
# set the ro.hw_version property for identify the verison of this board
#
hw_version=`cat /sys/devices/system/soc/soc0/platform_version`
mmc_type=`cat /sys/class/mmc_host/mmc0/mmc0:0001/name`
hw_version_ma=$(($hw_version/65536))
hw_version_mi=$(($hw_version-hw_version_ma*65536))

# Fix hw_version_mi by mmc_type
# The platform with "XINYH" flash is version 3.1
if [ $hw_version_mi -eq 0 -a $mmc_type == XINYH ];then
hw_version_mi=1
fi

setprop ro.hw_version ${hw_version_ma}.${hw_version_mi}

baseband=`getprop ro.baseband`

#
# Suppress default route installation during RA for IPV6; user space will take
# care of this
#
for file in /proc/sys/net/ipv6/conf/*
do
  echo 0 > $file/accept_ra_defrtr
done

#
# Start gpsone_daemon for SVLTE Type I & II devices
#
case "$target" in
        "msm7630_fusion")
        start gpsone_daemon
esac
case "$baseband" in
        "svlte2a")
        start gpsone_daemon
        start bridgemgrd
esac
case "$target" in
        "msm7630_surf" | "msm8660" | "msm8960")
        start quipc_igsn
esac
case "$target" in
        "msm7630_surf" | "msm8660" | "msm8960")
        start quipc_main
esac

case "$target" in
        "msm8960")
        start location_mq
        start xtwifi_inet
        start xtwifi_client
esac

case "$target" in
    "msm7630_surf" | "msm7630_1x" | "msm7630_fusion")
        value=`cat /sys/devices/system/soc/soc0/hw_platform`
        case "$value" in
            "Fluid")
             start profiler_daemon;;
        esac
        ;;
    "msm8660" )
        platformvalue=`cat /sys/devices/system/soc/soc0/hw_platform`
        case "$platformvalue" in
            "Fluid")
                start_sensors
                start profiler_daemon;;
        esac
        ;;
    "msm8960")
        start_sensors
        case "$baseband" in
            "msm")
		start_battery_monitor;;
        esac

        platformvalue=`cat /sys/devices/system/soc/soc0/hw_platform`
        case "$platformvalue" in
             "Fluid")
                 start profiler_daemon;;
             "Liquid")
                 start profiler_daemon;;
        esac
        ;;

esac
