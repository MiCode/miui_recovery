#!/bin/sh

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

# 155376 = IRK28C
# 156135 = IRK32
# 186921 = IRK77
# 213821 = ICL26
ZIP=yakju-ota-213821.zip
BUILD=icl26
ROOTDEVICE=maguro
DEVICE=maguro
MANUFACTURER=samsung

for COMPANY in broadcom csr imgtec invensense nxp samsung ti
do
  echo Processing files from $COMPANY
  rm -rf tmp
  FILEDIR=tmp/vendor/$COMPANY/$DEVICE/proprietary
  mkdir -p $FILEDIR
  mkdir -p tmp/vendor/$MANUFACTURER/$ROOTDEVICE
  case $COMPANY in
  broadcom)
    TO_EXTRACT="\
            system/vendor/firmware/bcm4330.hcd \
            "
    ;;
  csr)
    TO_EXTRACT="\
            system/vendor/etc/sirfgps.conf \
            system/vendor/lib/hw/gps.omap4.so \
            "
    ;;
  imgtec)
    TO_EXTRACT="\
            system/vendor/bin/pvrsrvinit \
            system/vendor/lib/egl/libEGL_POWERVR_SGX540_120.so \
            system/vendor/lib/egl/libGLESv1_CM_POWERVR_SGX540_120.so \
            system/vendor/lib/egl/libGLESv2_POWERVR_SGX540_120.so \
            system/vendor/lib/hw/gralloc.omap4.so \
            system/vendor/lib/libglslcompiler.so \
            system/vendor/lib/libIMGegl.so \
            system/vendor/lib/libpvr2d.so \
            system/vendor/lib/libpvrANDROID_WSEGL.so \
            system/vendor/lib/libPVRScopeServices.so \
            system/vendor/lib/libsrv_init.so \
            system/vendor/lib/libsrv_um.so \
            system/vendor/lib/libusc.so \
            "
    ;;
  invensense)
    TO_EXTRACT="\
            system/vendor/lib/libinvensense_mpl.so \
            "
    ;;
  nxp)
    TO_EXTRACT="\
            system/vendor/firmware/libpn544_fw.so \
            "
    ;;
  samsung)
    TO_EXTRACT="\
            system/bin/fRom \
            system/lib/libsecril-client.so \
            system/vendor/lib/libsec-ril.so \
            "
    ;;
  ti)
    TO_EXTRACT="\
            system/vendor/firmware/ducati-m3.bin \
            "
    ;;
  esac
  echo \ \ Extracting files from OTA package
  for ONE_FILE in $TO_EXTRACT
  do
    echo \ \ \ \ Extracting $ONE_FILE
    unzip -j -o $ZIP $ONE_FILE -d $FILEDIR > /dev/null || echo \ \ \ \ Error extracting $ONE_FILE
    if test $ONE_FILE = system/vendor/bin/gpsd -o $ONE_FILE = system/vendor/bin/pvrsrvinit -o $ONE_FILE = system/bin/fRom
    then
      chmod a+x $FILEDIR/$(basename $ONE_FILE) || echo \ \ \ \ Error chmoding $ONE_FILE
    fi
    if test $(echo $ONE_FILE | grep \\.apk\$ | wc -l) = 1
    then
      echo \ \ \ \ Splitting $ONE_FILE
      mkdir -p $FILEDIR/$(basename $ONE_FILE).parts || echo \ \ \ \ Error making parts dir for $ONE_FILE
      unzip $FILEDIR/$(basename $ONE_FILE) -d $FILEDIR/$(basename $ONE_FILE).parts > /dev/null || echo \ \ \ \ Error unzipping $ONE_FILE
      rm $FILEDIR/$(basename $ONE_FILE) || echo \ \ \ \ Error removing original $ONE_FILE
      rm -rf $FILEDIR/$(basename $ONE_FILE).parts/META-INF || echo \ \ \ \ Error removing META-INF for $ONE_FILE
    fi
  done
  echo \ \ Setting up $COMPANY-specific makefiles
  cp -R $COMPANY/staging/* tmp/vendor/$COMPANY/$DEVICE || echo \ \ \ \ Error copying makefiles
  echo \ \ Setting up shared makefiles
  cp -R root/* tmp/vendor/$MANUFACTURER/$ROOTDEVICE || echo \ \ \ \ Error copying makefiles
  echo \ \ Generating self-extracting script
  SCRIPT=extract-$COMPANY-$DEVICE.sh
  cat PROLOGUE > tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat $COMPANY/COPYRIGHT >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART1 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat $COMPANY/LICENSE >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART2 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  echo tail -n +$(expr 2 + $(cat PROLOGUE $COMPANY/COPYRIGHT PART1 $COMPANY/LICENSE PART2 PART3 | wc -l)) \$0 \| tar zxv >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART3 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  (cd tmp ; tar zc --owner=root --group=root vendor/ >> $SCRIPT || echo \ \ \ \ Error generating embedded tgz)
  chmod a+x tmp/$SCRIPT || echo \ \ \ \ Error generating script
  ARCHIVE=$COMPANY-$DEVICE-$BUILD-$(md5sum < tmp/$SCRIPT | cut -b -8 | tr -d \\n).tgz
  rm -f $ARCHIVE
  echo \ \ Generating final archive
  (cd tmp ; tar --owner=root --group=root -z -c -f ../$ARCHIVE $SCRIPT || echo \ \ \ \ Error archiving script)
  rm -rf tmp
done
