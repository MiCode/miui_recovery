#!/system/bin/sh

# Do not allow bugreports on user builds unless USB debugging
# is enabled.
if [ "x$(getprop ro.build.type)" = "xuser" -a \
     "x$(getprop init.svc.adbd)" != "xrunning" ]; then
  exit 0
fi

timestamp=`date +'%Y-%m-%d-%H-%M-%S'`
storagePath="$EXTERNAL_STORAGE/bugreports"
bugreport=$storagePath/bugreport-$timestamp
screenshotPath="$EXTERNAL_STORAGE/Pictures/Screenshots"
screenshot=$screenshotPath/Screenshot_$timestamp.png

# check screen shot folder
if [ ! -e $screenshotPath ]; then
  mkdir $screenshotPath
fi

# take screen shot
# we run this as a bg job in case screencap is stuck
/system/bin/screencap -p $screenshot &

# run bugreport
/system/bin/dumpstate -o $bugreport $@


# make files readable
chown root.sdcard_rw $bugreport.txt
chown root.sdcard_rw $screenshot

# invoke send_bug to look up email accounts and fire intents
# make it convenient to send bugreport to oneself
/system/bin/send_bug $bugreport.txt $screenshot
