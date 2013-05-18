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
#define DEBUG_ROOT


 const char* supersu_list[] = {
	"/system/app/Superuser.apk",
	"/system/app/Superuser.odex",
	"/system/app/SuperUser.apk",
	"/system/app/SuperUser.odex",
	"/system/app/superuser.apk",
	"/system/app/superuser.odex",
	"/system/app/Supersu.apk",
	"/system/app/Supersu.odex",
	"/system/app/SuperSU.apk",
	"/system/app/SuperSU.odex",
	"/system/app/supersu.apk",
	"/system/app/supersu.odex",
        "/system/app/LBESEC_MIUI.apk"	
 };


 const char* su_dir_list[] = {
	"/system/xbin/su",
	"/system/bin/su",
	"/system/bin/.ext/.su",
	"/system/app/Superuser.apk"
};

const char* super_su_list[] = {
	"/supersu/Superuser.apk", //0
	"/supersu/su",             //1 
	"/supersu/su.ext" // su.ext -> .su //2 
};


int remove_supersu() {
	struct stat st;
	int i = 0;
	char cmd[256];

#ifdef DEBUG_ROOT 
	int ret = 0;
#endif
	//remove supersu.apk from the system 
	for (i = 0 ; i < 13 ; i++ ) {
		if(stat(supersu_list[i],&st) == 0) {
			printf("remove file -> %s\n",supersu_list[i]);
			snprintf(cmd, 255,"rm %s",supersu_list[i]);
#ifdef DEBUG_ROOT
			ret = __system(cmd);
			if (ret != -1 ) {
				printf("success run command: %s\n",cmd);
			} else {
				printf("Failed to run command: %s\n", cmd);
				return -1;
			}
#else 
			__system(cmd);
#endif


#ifdef DEBUG_ROOT
		} else {
			printf("Can't stat '%s' ..\n", supersu_list[i]);
		}
#else

		}
#endif 
	}
        //remove su binary from the system 
	for (i = 0; i < 3; i++) {
		if(stat(su_dir_list[i],&st) == 0) {
			printf("remove file -> %s\n",su_dir_list[i]);
			snprintf(cmd, 255, "rm %s", su_dir_list[i]);
#ifdef DEBUG_ROOT 
			ret = __system(cmd);
			if (ret != -1) {
				printf("success run command: %s\n",cmd);
			} else {
				printf("Failed to run command: %s\n",cmd);
				return -1;
			}
#else 
			__system(cmd);
#endif
		}
	}
	return 0;
}


int install_supersu() {

	struct stat st;
	char cmd[256];

          //remove supersu from system
#ifdef DEBUG_ROOT 
	int ret = 0;
#endif

#ifdef DEBUG_ROOT 
	ret = remove_supersu();
	if(ret == 0) {
		printf("success run function remove_supersu()..\n");
	} else {
		printf("Failed run function remove_supersu()...\n");
	}
#else 
            remove_supersu();
#endif


	    //check the dir /supersu 
	    if(stat("/supersu",&st) != 0) {
		    printf("E: Can't stat '/supersu'..\n");
		    printf("E: Abort to install supersu to system ...\n");
	    } else {
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
		   printf("copy %s -> %s \n",super_su_list[2], su_dir_list[2]);
	   }

	   printf("install su & Superuser.apk..\n");
	   //su binary
	   snprintf(cmd, 255, "busybox install %s -m 06755 %s", super_su_list[1], su_dir_list[0]);
	   printf("copy %s -> %s \n",super_su_list[1], su_dir_list[0]);
#ifdef DEBUG_ROOT 
	   ret = __system(cmd);
	   if (ret != -1) {
		   printf("success run command: %s..\n",cmd);
	   } else {
		   printf("Failed to run command: %s...\n",cmd);
		   return -1;
	   }
#else 
	   __system(cmd);
#endif

	   //Superuser.apk 
	   snprintf(cmd, 255, "busybox install %s -m 0644 %s", super_su_list[0], su_dir_list[3]);
	   printf("copy %s -> %s \n", super_su_list[0], su_dir_list[3]);
#ifdef DEBUG_ROOT
	   ret = __system(cmd);
	   if (ret != -1) {
		   printf("success run command: %s..\n",cmd);
	   } else {
		   printf("Failed to run command: %s...\n",cmd);
		   return -1;
	   }
#else 
	   __system(cmd);
#endif
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
	if(0 == strcmp(cmd,"root_device")) {
	    	ret = install_supersu();
#ifdef DEBUG_ROOT 
		if (ret == -1) {
			printf("Failed to install supersu to system..\n");
		} else {
			printf("success to install supersu to system..\n");
		}
#else
			return ret;
#endif
	}
	if(0 == strcmp(cmd,"un_of_rec")) {
		ret = un_of_recovery();
		return ret;
	}
    }
	return 0;
}



