#include <stdio.h>
#include <stdlib.h>

#ifndef ROOT_DEVICE_H
#define ROOT_DEVICE_H
#endif
//install supersu from /recovery
int intsall_supersu();


//disable restore recovery from stock ROM
int un_of_recovery();

//remove supersu functions 
int remove_supersu();

//signature_check function
int signature_check(char* cmd);
int check_sig(); // return 0, or 1 

//main func of root_device()
int root_device_main(char *cmd[]);



