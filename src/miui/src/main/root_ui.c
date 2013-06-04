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

#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"
#include "../libs/miui_screen.h"

#define ROOT_DEVICE 1
#define DISABLE_OFFICAL_REC 2

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

struct _menuUnit* brightness_ui_init() {
	struct _menuUnit* p = common_ui_init();
	return_null_if_fail(p != NULL);
	menuUnit_set_name(p, "Set Brightness");
	menuUnit_set_title(p, "set Brightness");
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


	


struct _menuUnit* root_ui_init() {

	struct _menuUnit *p = common_ui_init();
	return_null_if_fail(p != NULL);
        menuUnit_set_name(p, "<~root.name>");
	menuUnit_set_title(p, "<~root.title>");
	menuUnit_set_icon(p, "@root");
	assert_if_fail(menuNode_init(p) != NULL);

	//root root_device 
	struct _menuUnit *tmp = common_ui_init();
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


	return p;
}


