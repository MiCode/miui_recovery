//extend_command.c for miui_recovery 
//author: Gaojiquan.com Lao Yang

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/times.h>
//#include "../mmcutils/mmcutils.h"
#include "../libcrecovery/common.h" 

extern char **environ;


//define miui_printf 
#ifndef miui_printf
#define miui_printf printf
#endif 


void apply_rm_binary(char* name) { //删除 /system/bin/name /system/xbin/name /system/sbin/name 
	char tmp[128], tmp_a[128], tmp_b[128];
	sprintf(tmp, "rm /system/bin/%s", name);
         sprintf(tmp_a, "rm /system/xbin/%s", name);
           sprintf(tmp_b, "rm /system/sbin/%s", name);

	__system(tmp);
        __system(tmp_a);
          __system(tmp_b);
}





int check_file_state(char *filename) { //返回1，表示存在，返回0，表示不存在。
                    
              char tmp_a[128], tmp_b[128], tmp_c[128];
                   sprintf(tmp_a,"/system/bin/%s", filename);
                         sprintf(tmp_b, "/system/xbin/%s", filename);
                           sprintf(tmp_c, "/system/sbin/%s", filename);

         if (access(tmp_a, F_OK) == 0 || access(tmp_b, F_OK) == 0 || access(tmp_c, F_OK) == 0) {
                          return 1;
                     }
                 return 0;
}


int root_device() {
    
     miui_printf("\nInstall busybox....\n");
      
   int is_state = 0;
   int is_state_su = 0;
   int is_state_superuser = 0;
 // remove busybox mount /system 
// add mount /system method to ../src/miui/src/main/tool_ui.c 
    
   
     //__system("rm /system/xbin/busybox");
     //__system("rm /system/bin/busybox");
     //__system("rm /system/sbin/busybox");
         is_state = check_file_state("busybox");
          if ( 0 != is_state) {
           apply_rm_binary("busybox");
            }

     __system("/sbin/busybox install -m 0755  /sbin/busybox /system/xbin/");
     __system("chown 0.0 /system/xbin/busybox");

           miui_printf("Install su binary ...\n");
        is_state_su = check_file_state("su");
        if ( 0 != is_state_su) {
            apply_rm_binary("su");
             }
     //copy su binary to /sytem/xbin/su
     __system("/sbin/busybox install -m 06755 /sbin/su /system/xbin/");
     __system("chown 0.0 /system/xbin/su");
     __system("chmod 06755 /system/xbin/su");
             if (access("/system/app/Superuser.apk", F_OK) != 0 || access("/system/app/superuser.apk", F_OK) != 0 ) {
                    is_state_superuser = 1;
                  
                   } 
                      
             if ( 0 != is_state_superuser ) {
                __system("rm /system/app/Superuser.apk");
                     __system("rm /system/app/superuser.apk");
                    }

          miui_printf("Install Superuser.apk....\n");
     //copy Superuser.apk to /system/app/Superuser.apk 
      __system("/sbin/busybox install -m 0755 /sbin/Superuser.apk /system/app/");
      __system("chown 0.0 /system/app/Superuser.apk");
         //unmount /system
         //miuiIntent_send(INTENT_UNMOUNT, 1, "/system");
          // __system("/sbin/busybox umount /system");
      return 0;

}



int main(int argc, char** args) {
      root_device();
         return 0;
}


