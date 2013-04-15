#Copyright (c) 2012, The Linux Foundation. All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are
#met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
#    * Neither the name of The Linux Foundation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.

#THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
#WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
#ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
#BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

export PATH=/system/bin

# Set platform variables
soc_hwid=`cat /sys/devices/system/soc/soc0/id` 2> /dev/null
boot_serialno=`getprop ro.boot.serialno`
case "$boot_serialno" in
       "7227AAQRD3" | "MSM8225QRD7")
          setprop ro.sf.lcd_density 160
     ;;
esac

case "$soc_hwid" in
      "90" | "91" | "92" | "97" | "101" | "102" | "103" | "167" | "168" | "169" | "170")
         dpi_7x27A=`getprop ro.sf.7x27A.lcd_density`
         setprop ro.sf.lcd_density $dpi_7x27A
            ;;
      "127" | "128" | "129")
         dpi_8x25=`getprop ro.sf.8x25.lcd_density`
         setprop ro.sf.lcd_density $dpi_8x25
            ;;
      "98" | "99" | "100")
         setprop ro.sf.lcd_density 160
            ;;
esac
