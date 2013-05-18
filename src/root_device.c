#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "root_device.h"

#include "roots.h"
#include "libcrecovery/common.h"



/*
 * Author: sndnvaps@gmail.com
 * date: 2013/05/16
 *
 *
 */


 const char* supersu_list[] = {
	"Superuser.apk",
	"Superuser.odex",
	"SuperUser.apk",
	"SuperUser.odex",
	"superuser.apk",
	"superuser.odex",
	"Supersu.apk",
	"Supersu.odex",
	"SuperSU.apk",
	"SuperSU.odex",
	"supersu.apk",
	"supersu.odex
};

 const char* su_dir_list[] = {
	"/system/xbin/su",
	"/system/bin/su",
	"/system/bin/.ext/.su",
	"/system/app/Superuser.apk
};

const char* super_su_list[] = {
	"/supersu/Superuser.apk",
	"/supersu/su",
	"/supersu/.su"
};


int remove_supersu() {
	struct stat st;
	int i = 0;
	char file[256];
	char cmd[1024];
	strncat(cmd,1023,"rm ");
	//remove supersu.apk from the system 
	for (i = 0 ; i < 12 ; i++ ) {
		snprintf(file, 255, "/system/app/%s",supersu_list[i]);
		if(stat(file,&st) == 0) {
			printf("remove file -> %s\n",file);
			snprintf(cmd, 1023,"%s",file);
			__system(cmd);
		}
	}
        //remove su binary from the system 
	for (i = 0; i < 3; i++) {
		if(stat(su_dir_list[i],&st) == 0) {
			printf("remove file -> %s\n",su_dir_list[i]);
			snprintf(cmd, 1023, "%s", su_dir_list[i]);
			__system(cmd);
		}
	}
	return 0;
}


int install_supersu() {

	struct stat st;
	char cmd[256];
	if(ensure_path_mounted("/system") != 0) {
		printf("/system didn't mount...\n");
		return 1;
	} else {
          //remove supersu from system
            remove_supersu();

           //check /system/bin/.ext dir and install .su file 
	   if(stat("/system/bin/.ext",&st) == 0) {
		   __system("busybox chmod 0777 /system/bin/.ext");
		   snprintf(cmd, 255, "busybox install %s -m 06755 %s",super_su_list[2],su_dir_list[2]);
		   __system(cmd);
		   printf("copy %s -> %s \n",super_su_list[2], su_dir_list[2]);
	   } else {
		   mkdir("system/bin/.ext",0777);
		   snprintf(cmd, 255, "busybox install %s -m 06755 %s",super_su_list[2],su_dir_list[2]);
		   __system(cmd);
	   }

	   printf("install su & Superuser.apk..\n");
	   //su binary
	   snprintf(cmd, 255, "busybox install %s -m 06755 %s", super_su_list[1], su_dir_list[0]);
	   printf("copy %s -> %s \n",super_su_list[1], su_dir_list[0]);
	   __system(cmd);

	   //Superuser.apk 
	   snprintf(cmd, 255, "busybox install %s -m 0644 %s", super_su_list[0], su_dir_list[3]);
	   printf("copy %s -> %s \n", super_su_list[0], su_dir_list[3]);
	   __system(cmd);
	}
          
    }
	return 0;
}

int un_of_recovery() {
	struct stat st;
	char *tmp1 = "/system/etc/install-recovery.sh";
	char *tmp2 = "/system/recovery-from-boot.p";
	char cmd[256];
	int status = 0;
	
	//get access to the file 
	if(stat(tmp1,&st) == 0 ) {
		printf("found %s \n",tmp1);
		printf("remove x perm from %s \n",tmp1);
		snprintf(cmd, 255, "busybox chmod 444 %s",tmp1);
		__system(cmd);
		status += 1;
	}
	if(stat(tmp2,&st) == 0 ) {
		printf("found %s \n",tmp2);
		printf("remove x perm from %s\n", tmp2);
		snprintf(cmd, 255, "busybox chmod 444 %s",tmp2);
		__system(cmd);
		status + 1;
	}
	if (status > 0 || status == 0) {
		printf("remove x perm from %s or %s\n",tmp1,tmp2);
		printf("or they didn't exists ....\n");
		return 1;
	}
     return 0;
}



int root_device_main(char *cmd) {
	int ret = 0;
	if(cmd != NULL) {
	if(strcmp(cmd,"root_device") == 0) {
	    	ret = install_supersu();
			return ret;
	}
	if(0 == strcmp(cmd,"un_of_rec")) {
		ret = un_of_recovery();
		return ret;
	}
    }
	return 0;
}


