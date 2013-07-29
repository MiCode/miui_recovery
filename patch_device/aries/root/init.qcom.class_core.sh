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

# Set platform variables
target=`getprop ro.board.platform`
soc_hwplatform=`cat /sys/devices/system/soc/soc0/hw_platform` 2> /dev/null
soc_hwid=`cat /sys/devices/system/soc/soc0/id` 2> /dev/null
soc_hwver=`cat /sys/devices/system/soc/soc0/platform_version` 2> /dev/null


# Dynamic Memory Managment (DMM) provides a sys file system to the userspace
# that can be used to plug in/out memory that has been configured as unstable.
# This unstable memory can be in Active or In-Active State.
# Each of which the userspace can request by writing to a sys file.
#
# ro.dev.dmm = 1; Indicates that DMM is enabled in the Android User Space. This
# property is set in the Android system properties file.
#
# If ro.dev.dmm.dpd.start_address is set here then the target has a memory
# configuration that supports DynamicMemoryManagement.
init_DMM()
{
    block=-1

    case "$target" in
    "msm7630_surf" | "msm7630_1x" | "msm7630_fusion" | "msm8960")
        ;;
    *)
        return
        ;;
    esac

    mem="/sys/devices/system/memory"
    op=`cat $mem/movable_start_bytes`
    case "$op" in
    "0")
        log -p i -t DMM DMM Disabled. movable_start_bytes not set: $op
        ;;

    "$mem/movable_start_bytes: No such file or directory ")
        log -p i -t DMM DMM Disabled. movable_start_bytes does not exist: $op
        ;;

    *)
        log -p i -t DMM DMM available. movable_start_bytes at $op
        movable_start_bytes=0x`cat $mem/movable_start_bytes`
        block_size_bytes=0x`cat $mem/block_size_bytes`
        block=$((#${movable_start_bytes}/${block_size_bytes}))

        chown system.system $mem/memory$block/state
        chown system.system $mem/probe
        chown system.system $mem/active
        chown system.system $mem/remove

        case "$target" in
        "msm7630_surf" | "msm7630_1x" | "msm7630_fusion")
            echo $movable_start_bytes > $mem/probe
            case "$?" in
            "0")
                log -p i -t DMM $movable_start_bytes to physical hotplug succeeded.
                ;;
            *)
                log -p e -t DMM $movable_start_bytes to physical hotplug failed.
                return
                ;;
            esac

            echo online > $mem/memory$block/state
            case "$?" in
            "0")
                log -p i -t DMM \'echo online\' to logical hotplug succeeded.
                ;;
            *)
                log -p e -t DMM \'echo online\' to logical hotplug failed.
                return
                ;;
            esac
            ;;
        esac

        setprop ro.dev.dmm.dpd.start_address $movable_start_bytes
        setprop ro.dev.dmm.dpd.block $block
        ;;
    esac

    case "$target" in
    "msm8960")
        return
        ;;
    esac

    # For 7X30 targets:
    # ro.dev.dmm.dpd.start_address is set when the target has a 2x256Mb memory
    # configuration. This is also used to indicate that the target is capable of
    # setting EBI-1 to Deep Power Down or Self Refresh.
    op=`cat $mem/low_power_memory_start_bytes`
    case "$op" in
    "0")
        log -p i -t DMM Self-Refresh-Only Disabled. low_power_memory_start_bytes not set:$op
        ;;
    "$mem/low_power_memory_start_bytes No such file or directory ")
        log -p i -t DMM Self-Refresh-Only Disabled. low_power_memory_start_bytes does not exist:$op
        ;;
    *)
        log -p i -t DMM Self-Refresh-Only available. low_power_memory_start_bytes at $op
        ;;
    esac
}

#
# For controlling console and shell on console on 8960 - perist.serial.enable 8960
# On other target use default ro.debuggable property.
#
serial=`getprop persist.serial.enable`
dserial=`getprop ro.debuggable`
case "$target" in
    "msm8960")
        case "$serial" in
            "0")
                echo 0 > /sys/devices/platform/msm_serial_hsl.0/console
                ;;
            "1")
                echo 1 > /sys/devices/platform/msm_serial_hsl.0/console
                start console
                ;;
            *)
                case "$dserial" in
                     "1")
                         start console
                         ;;
                esac
                ;;
        esac
        ;;
    *)
        case "$dserial" in
            "1")
                start console
                ;;
        esac
        ;;
esac

#
# Allow persistent faking of bms
# User needs to set fake bms charge in persist.bms.fake_batt_capacity
#
fake_batt_capacity=`getprop persist.bms.fake_batt_capacity`
case "$fake_batt_capacity" in
    "") ;; #Do nothing here
    * )
    case $target in
        "msm8960")
        echo "$fake_batt_capacity" > /sys/module/pm8921_bms/parameters/bms_fake_battery
        ;;
    esac
esac

case "$target" in
    "msm7630_surf" | "msm7630_1x" | "msm7630_fusion")
        case "$soc_hwplatform" in
            "FFA" | "SVLTE_FFA")
                # linking to surf_keypad_qwerty.kcm.bin instead of surf_keypad_numeric.kcm.bin so that
                # the UI keyboard works fine.
                ln -s  /system/usr/keychars/surf_keypad_qwerty.kcm.bin /system/usr/keychars/surf_keypad.kcm.bin
                ;;
            "Fluid")
                setprop ro.sf.lcd_density 240
                setprop qcom.bt.dev_power_class 2
                ;;
            *)
                ln -s  /system/usr/keychars/surf_keypad_qwerty.kcm.bin /system/usr/keychars/surf_keypad.kcm.bin
                ;;
        esac

        insmod /system/lib/modules/ss_mfcinit.ko
        insmod /system/lib/modules/ss_vencoder.ko
        insmod /system/lib/modules/ss_vdecoder.ko
        chmod 0666 /dev/ss_mfc_reg
        chmod 0666 /dev/ss_vdec
        chmod 0666 /dev/ss_venc

        init_DMM
        ;;

    "msm8660")
        case "$soc_hwplatform" in
            "Fluid")
                setprop ro.sf.lcd_density 240
                ;;
            "Dragon")
                setprop ro.sound.alsa "WM8903"
                ;;
        esac
        ;;

    "msm8960")
        # lcd density is write-once. Hence the separate switch case
        case "$soc_hwplatform" in
            "Liquid")
                if [ "$soc_hwver" == "196608" ]; then # version 0x30000 is 3D sku
                    setprop ro.sf.hwrotation 90
                fi

                setprop ro.sf.lcd_density 160
                ;;
            "MTP")
                setprop ro.sf.lcd_density 240
                ;;
            *)
                case "$soc_hwid" in
                    "109")
                        setprop ro.sf.lcd_density 160
                        ;;
                    *)
                        setprop ro.sf.lcd_density 240
                        ;;
                esac
            ;;
        esac

        #Set up composition type based on the target
        case "$soc_hwid" in
            109| 116 | 117 | 118 | 120 | 121| 130)
                #APQ8064, MSM8930, MSM8630, MSM8230,
                # MSM8627, MSM8227, MPQ8064
                setprop debug.composition.type gpu
                ;;
            *)
        esac

        init_DMM
        ;;
esac
