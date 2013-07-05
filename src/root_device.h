#include <stdio.h>
#include <stdlib.h>

#ifndef ROOT_DEVICE_H
#define ROOT_DEVICE_H
#endif

#define SCRIPT_COMMAND_SIZE 512
#define ORS_TMP "/tmp/ors_tmp"

//install supersu from /recovery
int intsall_supersu();


//disable restore recovery from stock ROM
int un_of_recovery();

//remove supersu functions 
int remove_supersu();

//signature_check function
int signature_check(char* cmd);
int check_sig(); // return 0, or 1 

//run ors in sdcard | external_sd 

extern int check_for_script_file(const char* ors_boot_script);
extern int run_ors_script(const char* ors_script);


//main func of root_device()
int root_device_main(char *cmd[]);



