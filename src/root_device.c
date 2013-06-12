#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <sys/reboot.h>

#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <signal.h>
#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "minui/minui.h"
#include "root_device.h"

#include "roots.h"
#include "libcrecovery/common.h"
#include "miui/src/miui.h"
#include "miui_intent.h"
#include "mtdutils/mounts.h"
#include "nandroid.h"
#include "minzip/DirUtil.h"
#include "edify/expr.h"
#include <libgen.h>
#include "cutils/android_reboot.h"



/*
 * Author: sndnvaps@gmail.com
 * date: 2013/05/16
 *
 *
 */
//#define DEBUG_ROOT



//#define SIG_CHECK_FILE "/tmp/gaojiquan/stat"
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
/* Remove signature check to support ors */
/*
int check_sig() {
	FILE* f;
	char SIG_STAT[20];
	int ret = 0;
	f = fopen(SIG_CHECK_FILE,"r");
	 if (f != NULL ) {
		 printf("success open file: %s..\n",SIG_CHECK_FILE);

		 fgets(SIG_STAT,2,f);
                   printf(" %s -> %s..\n",SIG_STAT, SIG_CHECK_FILE);
		   if (SIG_STAT != NULL) {
		 if (strcmp(SIG_STAT, "E")== 0) {
				 ret = 1;
		 } else if (strcmp(SIG_STAT, "D")==0 ) {
			 ret = 0;
		 } else {
			 printf("read stat error\n"); 
		   }
	 } else {
		 printf("Cann't open %s .. \n", SIG_CHECK_FILE);
		 return -1;
	 }

     }
	 fclose(f);
	return ret;
}

*/


int root_device_main(char *cmd[]) {
	int ret = 0;
	if(cmd != NULL) {
	if(0 == strcmp(cmd[0],"root_device")) {
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
	if(0 == strcmp(cmd[0],"un_of_rec")) {
		ret = un_of_recovery();
		return ret;
	}
	
   }
	return 0;
}

// # *** Thanks to PhilZ for all of this! *** #
// He has been such a HUGE part of where this recovery has ended up.
//
// ** start open recovery script support ** //
// ** adapted code from philZ ** //
#define SCRIPT_COMMAND_SIZE 512
//check ors script at boot (called from recovery.c)
int check_for_script_file(const char* ors_boot_script)
{
    //ensure_path_mounted("/sdcard");
    miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
   
   if (acfg()->sd_ext == 1) {
	   miuiIntent_send(INTENT_MOUNT, 1, "/external_sd");
   } 


    int ret_val = -1;
    char exec[512];
    FILE *fp = fopen(ors_boot_script, "r");
    if (fp != NULL) {
        ret_val = 0;
        LOGI("Script file found: '%s'\n", ors_boot_script);
        fclose(fp);
#ifdef BOARD_HAS_REMOVABLE_STORAGE
	miuiIntent_send(INTENT_SYSTEM, 1, "ors-mount.sh");
	//__system("ors-mount.sh");
#endif
        // Copy script file to /tmp
        strcpy(exec, "cp ");
        strcat(exec, ors_boot_script);
        strcat(exec, " ");
        strcat(exec, "/tmp/openrecoveryscript");
        __system(exec);
        // Delete the file from /cache
        strcpy(exec, "rm ");
        strcat(exec, ors_boot_script);
        // __system(exec);
    }
    return ret_val;
}
//run ors script code
//this can start on boot or manually for custom ors
int run_ors_script(const char* ors_script) {
    FILE *fp = fopen(ors_script, "r");
    int ret_val = 0, cindex, line_len, i, remove_nl;
    char script_line[SCRIPT_COMMAND_SIZE], command[SCRIPT_COMMAND_SIZE],
         value[SCRIPT_COMMAND_SIZE], mount[SCRIPT_COMMAND_SIZE],
         value1[SCRIPT_COMMAND_SIZE], value2[SCRIPT_COMMAND_SIZE];
    char *val_start, *tok;
    int ors_system = 0;
    int ors_data = 0;
    int ors_cache = 0;
    int ors_recovery = 0;
    int ors_boot = 0;
    int ors_andsec = 0;
    int ors_sdext = 0;
    int ors_wimax = 0;
    if (fp != NULL) {
        while (fgets(script_line, SCRIPT_COMMAND_SIZE, fp) != NULL && ret_val == 0) {
            cindex = 0;
            line_len = strlen(script_line);
            //if (line_len > 2)
                //continue; // there's a blank line at the end of the file, we're done!
            ui_print("script line: '%s'\n", script_line);
            for (i=0; i<line_len; i++) {
                if ((int)script_line[i] == 32) {
                    cindex = i;
                    i = line_len;
                }
            }
            memset(command, 0, sizeof(command));
            memset(value, 0, sizeof(value));
            if ((int)script_line[line_len - 1] == 10)
                remove_nl = 2;
            else
                remove_nl = 1;
            if (cindex != 0) {
                strncpy(command, script_line, cindex);
                ui_print("command is: '%s' and ", command);
                val_start = script_line;
                val_start += cindex + 1;
                strncpy(value, val_start, line_len - cindex - remove_nl);
                ui_print("value is: '%s'\n", value);
            } else {
                strncpy(command, script_line, line_len - remove_nl + 1);
                ui_print("command is: '%s' and there is no value\n", command);
            }
            if (strcmp(command, "install") == 0) {
                // Install zip

                miuiIntent_send(INTENT_INSTALL, 3, value, "0", "1");

            } else if (strcmp(command, "wipe") == 0) {
                // Wipe
                if (strcmp(value, "cache") == 0 || strcmp(value, "/cache") == 0) {
                   miuiIntent_send(INTENT_WIPE, 1, "/cache");
                } else if (strcmp(value, "dalvik") == 0 || strcmp(value, "dalvick") == 0 || strcmp(value, "dalvikcache") == 0 || strcmp(value, "dalvickcache") == 0) {
                    miuiIntent_send(INTENT_WIPE, 1, "dalvik-cache");
                } else if (strcmp(value, "data") == 0 || strcmp(value, "/data") == 0 || strcmp(value, "factory") == 0 || strcmp(value, "factoryreset") == 0) {
                    miuiIntent_send(INTENT_WIPE, 1, "/data");
                } else {
                    LOGE("Error with wipe command value: '%s'\n", value);
                    ret_val = 1;
                }
            } else if (strcmp(command, "backup") == 0) {
                // Backup: always use external sd if possible
                miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
                char path_name[PATH_MAX];
		static time_t timep;
		static struct tm *time_tm;
		time(&timep);
		time_tm = gmtime(&timep);
		snprintf(path_name,PATH_MAX, "%s/backup/backup/%02d%02d%02d-%02d%02d", 
				RECOVERY_PATH, time_tm->tm_year,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min);
		miuiIntent_send(INTENT_BACKUP, 1, path_name);

            } else if (strcmp(command, "restore") == 0) {
                // Restore
		/* Reopen the Restore function by sndnvaps */
                tok = strtok(value, " ");
                strcpy(value1, tok);
                ui_print("Restoring '%s'\n", value1);
                tok = strtok(NULL, " ");
                if (tok != NULL) {
                    ors_system = 0;
                    ors_data = 0;
                    ors_cache = 0;
                    ors_boot = 0;
                    ors_sdext = 0;
		    ors_wimax = 0;
                    memset(value2, 0, sizeof(value2));
                    strcpy(value2, tok);
                    ui_print("Setting restore options:\n");
                    line_len = strlen(value2);
                    for (i=0; i<line_len; i++) {
                        if (value2[i] == 'S' || value2[i] == 's') {
                            ors_system = 1;
                            ui_print("System\n");
                        } else if (value2[i] == 'D' || value2[i] == 'd') {
                            ors_data = 1;
                            ui_print("Data\n");
                        } else if (value2[i] == 'C' || value2[i] == 'c') {
                            ors_cache = 1;
                            ui_print("Cache\n");
                        } else if (value2[i] == 'R' || value2[i] == 'r') {
                            ui_print("Option for recovery ignored in CWMR\n");
                        } else if (value2[i] == '1') {
                            ui_print("%s\n", "Option for special1 ignored in CWMR");
                        } else if (value2[i] == '2') {
                            ui_print("%s\n", "Option for special1 ignored in CWMR");
                        } else if (value2[i] == '3') {
                            ui_print("%s\n", "Option for special1 ignored in CWMR");
                        } else if (value2[i] == 'B' || value2[i] == 'b') {
                            ors_boot = 1;
                            ui_print("Boot\n");
                        } else if (value2[i] == 'A' || value2[i] == 'a') {
                            ui_print("Option for android secure ignored in CWMR\n");
                        } else if (value2[i] == 'E' || value2[i] == 'e') {
                            ors_sdext = 1;
                            ui_print("SD-Ext\n");
			} else if (value2[i] == 'W' || value2[i] == 'w') {
				ors_wimax = 1;
				ui_print("WIMAX\n");
			} else if (value2[i] == 'M' || value2[i] == 'm') {
                            ui_print("MD5 check skip option ignored in CWMR\n");
                        }
                    }
                } else
                    LOGI("No restore options set\n");
               // nandroid_restore(value1, ors_boot, ors_system, ors_data, ors_cache, ors_sdext, ors_wimax);
	       miuiIntent_send(INTENT_RESTORE,7, value1, ors_boot, ors_system, ors_data, ors_cache, ors_sdext, ors_wimax);
                ui_print("Restore complete!\n");
		
	    } else if (strcmp(command, "mount") == 0) {
                // Mount
                if (value[0] != '/') {
                    strcpy(mount, "/");
                    strcat(mount, value);
                } else
                    strcpy(mount, value);
                //ensure_path_mounted(mount);
		miuiIntent_send(INTENT_MOUNT, 1, mount);
                ui_print("Mounted '%s'\n", mount);
            } else if (strcmp(command, "unmount") == 0 || strcmp(command, "umount") == 0) {
                // Unmount
                if (value[0] != '/') {
                    strcpy(mount, "/");
                    strcat(mount, value);
                } else
                    strcpy(mount, value);
                //ensure_path_unmounted(mount);
		miuiIntent_send(INTENT_UNMOUNT, 1,mount);
                ui_print("Unmounted '%s'\n", mount);
            } else if (strcmp(command, "set") == 0) {
                // Set value
                tok = strtok(value, " ");
                strcpy(value1, tok);
                tok = strtok(NULL, " ");
                strcpy(value2, tok);
                ui_print("Setting function disabled in CWMR: '%s' to '%s'\n", value1, value2);
            } else if (strcmp(command, "mkdir") == 0) {
                // Make directory (recursive)
                ui_print("Recursive mkdir disabled in CWMR: '%s'\n", value);
            } else if (strcmp(command, "reboot") == 0) {
                // Reboot
		miuiIntent_send(INTENT_REBOOT, 1, "reboot");
            } else if (strcmp(command, "cmd") == 0) {
                if (cindex != 0) {
                    //__system(value);
		    miuiIntent_send(INTENT_SYSTEM, 1, value);
                } else {
                    LOGE("No value given for cmd\n");
                }
            } else {
                LOGE("Unrecognized script command: '%s'\n", command);
                ret_val = 1;
            }
        }
        fclose(fp);
        ui_print("Done processing script file\n");
    } else {
        LOGE("Error opening script file '%s'\n", ors_script);
        return 1;
    }
    return ret_val;
}
//end of open recovery script file code
