/*
*  Copyright (c) 2013 The software  Author sndnvaps. 
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of  sndnvaps nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS" AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "../miui_inter.h"
#include "../miui.h"
#include "../utils.h"
#include "../../../miui_intent.h"
#include "../libs/miui_screen.h"
#include "../libs/miui_libs.h"

#define ROOT_DEVICE 1
#define DISABLE_OFFICAL_REC 2
#define E_SIG 0x89
#define D_SIG 0X90
//#define CURRENT_SIGN_STAT_PATH "/tmp/gaojiquan"
//INTENT_RUN_ORS ,1, filename
//
#define AUTHOR_INFO "/tmp/author_info.log"

static STATUS root_device_item_show(menuUnit *p) {
	if(RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
		miui_busy_process();
		switch(p->result) {
			case ROOT_DEVICE:
				miuiIntent_send(INTENT_MOUNT, 1 , "/system");
				miuiIntent_send(INTENT_ROOT, 1, "root_device");
				miuiIntent_send(INTENT_UNMOUNT, 1 , "/system");
				break;
			case DISABLE_OFFICAL_REC:
				miuiIntent_send(INTENT_MOUNT, 1, "/system");
				miuiIntent_send(INTENT_ROOT, 1, "un_of_rec");
				miuiIntent_send(INTENT_UNMOUNT, 1, "/system");
				break;

			 default:
				assert_if_fail(0);
				break;
		}
	}
	return MENU_BACK;
}

static STATUS brightness_menu_show(struct _menuUnit* p) {
	switch (p->result) {
		case 25:
			screen_set_brightness("25");
			break;
		case 50:
			screen_set_brightness("50");
			break;
		case 75:
			screen_set_brightness("75");
			break;
		case 100:
			screen_set_brightness("100");
			break;
		default:
			//we should never get here!
			break;
	}
	return MENU_BACK;
}
/* Remove it to support ors */
/*
static STATUS signature_set_menu_show(struct _menuUnit* p) {
	struct stat st;
	char cmd[1024];
	if (stat(CURRENT_SIGN_STAT_PATH, &st) != 0) {
		mkdir(CURRENT_SIGN_STAT_PATH, 0777); 
	}
	if (stat("/tmp/gaojiquan/stat", &st) != 0) {
	snprintf(cmd, 1023, "busybox touch %s/stat", CURRENT_SIGN_STAT_PATH);
	miuiIntent_send(INTENT_SYSTEM, 1, cmd);
	}


	switch(p->result) {
		       case E_SIG:
				signature_set("Enable");
				break;
			case D_SIG:
			        signature_set("Disable");
				break;
			default:
				//we should never get here
				break;
	}
	return MENU_BACK;
}
*/


/* 
 *  _sd_show_dir show file system on screen 
 *  return MENU_BACK when press backmenu
 */

#define _SD_MAX_PATH 256
//callback function, success return 0, non-zero fail
 int ors_file_run(char *file_name, int file_len, void* data) {
	 return_val_if_fail(file_name != NULL, RET_FAIL);
	 return_val_if_fail(strlen(file_name) <= file_len, RET_INVALID_ARG);
	 return_val_if_fail(data != NULL, RET_FAIL);
	 struct _menuUnit* p = (pmenuUnit)data;
	  if(RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
		  miuiIntent_send(INTENT_RUN_ORS, 1, file_name);
		  return 0;

	  }else return -1;

	  return 0;
 }

//callback function file filter, if access, return 0, other return -1
int ors_file_filter(char* file, int file_len) {
	return_val_if_fail(file != NULL, RET_FAIL);
	int len = strlen(file);
	return_val_if_fail(len <= file_len, RET_INVALID_ARG);
	if (len >= 4 && strncasecmp(file + len -4, ".ors", 4) == 0)
		return 0;
	return -1;
}

static STATUS ors_sd_menu_show(menuUnit *p) {
	//ensure mounte sd path 
	struct _intentResult* result = miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
	//whatever wether sd is mount, scan sdcard and go on 
	int ret;
	ret = file_scan("/sdcard",sizeof("/sdcard"), p->name, strlen(p->name), &ors_file_run, (void*)p, &ors_file_filter);
	if (ret == -1) return MENU_BACK;
	return ret;
}

static STATUS ors_sdext_menu_show(menuUnit* p) {
	//ensure_mounte sdext path
	struct _intentResult* result = miuiIntent_send(INTENT_MOUNT, 1, "/external_sd");
	//whatever wether sd-ext is mounted ,scan sd-ext and go on
	int ret;
	ret = file_scan("/external_sd", sizeof("/external_sd"), p->name, strlen(p->name), &ors_file_run, (void*)p, &ors_file_filter);
	if (ret == -1) return MENU_BACK;
	return ret;
}

static STATUS about_author_menu_show(menuUnit* p) {
	 FILE* fp;
         char author_info[4084];
	 char file_name[PATH_MAX];
	 snprintf(author_info, 4084, "\n由搞机圈老杨制作\n编译日期: %s\n微博名称: laser杨万荣\n微博地址:http://weibo.com/210124187\n",acfg()->rom_date);
	 fp = fopen(AUTHOR_INFO, "w+");
	 if (fp != NULL) {
		 fputs(author_info, fp);
		 fclose(fp);
	 } else {
		 LOGE("can not open %s!",AUTHOR_INFO);
		 fclose(fp);
	 } 
	 snprintf(file_name, PATH_MAX, "%s", AUTHOR_INFO);
	 miui_textbox(p->name, p->title_name, p->icon, miui_readfromfs(file_name));
	 return MENU_BACK;
}

static STATUS adb_sideload_menu_show(menuUnit* p) {
		miuiIntent_send(INTENT_ADB_SIDELOAD, 1, "sideload");
	
	return MENU_BACK;
}



struct _menuUnit* adb_sideload_ui_init() {
	struct _menuUnit* p = common_ui_init();
	return_null_if_fail(p != NULL);
	menuUnit_set_name(p, "ADB Sideload");
	menuUnit_set_title(p, "ADB Sideload");
	menuUnit_set_icon(p, "@root");
	assert_if_fail(menuNode_init(p) != NULL);
	//adb sideload 
	struct _menuUnit* temp = common_ui_init();
	menuUnit_set_name(temp, "ADB Sideload");
	menuUnit_set_show(temp, &adb_sideload_menu_show);
	assert_if_fail(menuNode_add(p, temp) == RET_OK);

	return p;
}




struct _menuUnit* ors_ui_init() {
	struct _menuUnit* p = common_ui_init();
	return_null_if_fail(p != NULL);
	menuUnit_set_name(p, "运行ORS脚本");
	menuUnit_set_title(p, "运行ORS脚本");
	menuUnit_set_icon(p, "@root");
	assert_if_fail(menuNode_init(p) != NULL);
	//select ORS from sd card 
	struct _menuUnit* temp = common_ui_init();
	menuUnit_set_name(temp, "从SD卡选择ORS脚本");
	menuUnit_set_show(temp, &ors_sd_menu_show);
	assert_if_fail(menuNode_add(p, temp) == RET_OK);
	
	//select ORS from external_sd
	if (acfg()->sd_ext == 1) {
	temp = common_ui_init();
	menuUnit_set_name(temp, "从内置卡选择ORS脚本");
	menuUnit_set_show(temp, &ors_sdext_menu_show);
	assert_if_fail(menuNode_add(p, temp) == RET_OK);
	}
	

	return p;
}




struct _menuUnit* brightness_ui_init() {
	struct _menuUnit* p = common_ui_init();
	return_null_if_fail(p != NULL);
	menuUnit_set_name(p, "调节亮度");
	menuUnit_set_title(p, "调节亮度");
	menuUnit_set_icon(p, "@root");
	assert_if_fail(menuNode_init(p) != NULL);
	//25% brightness
	struct _menuUnit* temp = common_ui_init();
	menuUnit_set_name(temp, "25% Brightness");
	menuUnit_set_show(temp, &brightness_menu_show);
	temp->result = 25;
	assert_if_fail(menuNode_add(p, temp) == RET_OK);
	//50% brightness
        temp = common_ui_init();
	menuUnit_set_name(temp, "50% Brightness");
	menuUnit_set_show(temp, &brightness_menu_show);
	temp->result = 50;
	assert_if_fail(menuNode_add(p, temp) == RET_OK);
	//75% brightness
	temp = common_ui_init();
	menuUnit_set_name(temp, "75% Brightness");
	menuUnit_set_show(temp, &brightness_menu_show);
	temp->result = 75;
	assert_if_fail(menuNode_add(p, temp) == RET_OK);
	//100% brightness
	temp = common_ui_init();
	menuUnit_set_name(temp, "100% Brightness");
	menuUnit_set_show(temp, &brightness_menu_show);
	temp->result = 100;
	assert_if_fail(menuNode_add(p, temp) == RET_OK);
	return p;
}

/* remove it to support ors */
/*
struct _menuUnit* SIG_SET_ui_init() {
         struct _menuUnit* p = common_ui_init();
         return_null_if_fail(p != NULL);
         menuUnit_set_name(p, "开启/关闭签名认证");
	 menuUnit_set_title(p, "开启/关闭签名认证");
	 menuUnit_set_icon(p, "@root");
	 assert_if_fail(menuNode_init(p) != NULL);

	 //Enable signature check
	 struct _menuUnit* temp = common_ui_init();
	 return_null_if_fail(temp != NULL);
	 menuUnit_set_name(temp,"开启签名认证");
	 menuUnit_set_show(temp, &signature_set_menu_show);
	 temp->result = E_SIG;
	 assert_if_fail(menuNode_add(p, temp) == RET_OK);

	 //Disable signature check
	 temp = common_ui_init();
	 return_null_if_fail(temp != NULL);
	 menuUnit_set_name(temp, "关闭签名认证");
	 menuUnit_set_show(temp, &signature_set_menu_show);
         temp->result = D_SIG;
	 assert_if_fail(menuNode_add(p, temp) == RET_OK);
	 return p;
}

*/

struct _menuUnit* root_ui_init() {

	struct _menuUnit *p = common_ui_init();
	return_null_if_fail(p != NULL);
        menuUnit_set_name(p, "<~root.name>");
	menuUnit_set_title(p, "<~root.title>");
	menuUnit_set_icon(p, "@root");
	assert_if_fail(menuNode_init(p) != NULL);

	//show info of the author 
	struct _menuUnit *tmp = common_ui_init();
	return_null_if_fail(tmp != NULL);
	strncpy(tmp->name, "关于作者",MENU_LEN);
	menuUnit_set_icon(tmp, "@info");
        menuUnit_set_show(tmp, &about_author_menu_show);
	assert_if_fail(menuNode_add(p, tmp) == RET_OK);

        //root device 
	tmp = common_ui_init();
	return_null_if_fail(tmp != NULL);
	strncpy(tmp->name, "<~root.device>", MENU_LEN);
	menuUnit_set_icon(tmp, "@root.device");
	tmp->result = ROOT_DEVICE;
	tmp->show = &root_device_item_show;
	assert_if_fail(menuNode_add(p,tmp) == RET_OK);

	//root disable_restore_official_recovery 	
	tmp = common_ui_init();
	return_null_if_fail(tmp != NULL);
	strncpy(tmp->name, "<~root.un_of_rec>", MENU_LEN);
	menuUnit_set_icon(tmp, "@root.un_of_rec");
	tmp->result = DISABLE_OFFICAL_REC;
	tmp->show = &root_device_item_show;
	assert_if_fail(menuNode_add(p, tmp) == RET_OK);

	//set Brightness
	tmp = brightness_ui_init();
	assert_if_fail(menuNode_add(p, tmp) == RET_OK);
           
	//signature set menu 
	// remove it to support ors 
	/*
	tmp = SIG_SET_ui_init();
	assert_if_fail(menuNode_add(p, tmp) == RET_OK);
         */
	//ORS function 
	tmp = ors_ui_init();
	assert_if_fail(menuNode_add(p, tmp) == RET_OK);

	//adb sideload 
	tmp = adb_sideload_ui_init();
	assert_if_fail(menuNode_add(p, tmp) == RET_OK);

	return p;
}


