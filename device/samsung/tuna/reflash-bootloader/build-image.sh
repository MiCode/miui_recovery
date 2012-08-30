#!/bin/sh

DIR=/tmp/reflash_bootloader.$$
RAMDISK=/tmp/ramdisk.$$.img

mkdir ${DIR}
cp ${OUT}/system/bin/tuna-reflash-bootloader ${DIR}/init
cp ${ANDROID_BUILD_TOP}/vendor/samsung/tuna/bootloader.img ${DIR}/bootloader.img
mkbootfs ${DIR} | minigzip > ${RAMDISK}
mkbootimg --kernel ${OUT}/kernel --ramdisk ${RAMDISK} -o tuna-reflash-bootloader.img
rm ${DIR}/init
rm ${DIR}/bootloader.img
rmdir ${DIR}
rm ${RAMDISK}
