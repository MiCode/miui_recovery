
on init-recovery
	mount /system
	unmount /cache	
	exec -f "/system/bin/e2fsck -v -y <dev_node:/cache>"

	insmod -f -v /system/lib/modules/exfat_core.ko
	insmod -f -v /system/lib/modules/exfat_fs.ko
	lsmod

	mount /cache
	ls /cache/recovery/
	
	mount /data
	fcut --limited-file-size=64k -f /data/log/recovery_log.txt /tmp/recovery_backup.txt


on multi-csc
	echo 
	echo "-- Appling Multi-CSC..."
	mount /system	
	echo "Applied the CSC-code : <salse_code>"
	cp -y -f -r -v /system/csc/common /
	unmount /system
	mount /system
	cmp -r /system/csc/common /	
	cp -y -f -r -v /system/csc/<salse_code>/system /system
	rm -v /system/csc_contents
	ln -v -s /system/csc/<salse_code>/csc_contents /system/csc_contents
	unmount /system
	mount /system	
	cmp -r /system/csc/<salse_code>/system /system
	rm -v --limited-file-size=0 /system/app/*
	echo "Successfully applied multi-CSC."

on factory-out
	echo "Coping medias..."
	mount /preload
	mount /data
	mkdir media_rw media_rw 0775 /data/media
	cp -y -r -v -f --with-fmode=0664 --with-dmode=0775 --with-owner=media_rw.media_rw /preload/INTERNAL_SDCARD/ /data/media/
	unmount /data
	mount /data
	cmp -r /preload/INTERNAL_SDCARD/ /data/media/

on post-recovery
	mount /data
	mkdir system log 0775 /data/log
	cp -y -f -v /tmp/recovery_backup.txt /data/log/recovery_log.txt

