#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"
/*
 *_sd_show_dir show file system on screen
 *return MENU_BACK when pree backmenu
 */
#define SD_MAX_PATH 256

//callback function , success return 0, non-zero fail
int file_install(char *file_name, int file_len, void *data)
{
    return_val_if_fail(file_name != NULL, RET_FAIL);
    return_val_if_fail(strlen(file_name) <= file_len, RET_INVALID_ARG);
    return_val_if_fail(data != NULL, RET_FAIL);
#ifdef DUALSYSTEM_PARTITIONS
    int choose_system_num;
    if (is_tdb_enabled()) {
        if (RET_YES == miui_confirm(5, "<~choose.system.title>", "<~choose.system.text>", "@alert", "<~choice.system0.name>", "<~choice.system1.name>")) {
            miuiIntent_send(INTENT_SETSYSTEM,1,"1");
        } else {
            miuiIntent_send(INTENT_SETSYSTEM,1,"2");
        }
    
    }
#endif
    struct _menuUnit *p = (pmenuUnit)data;
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
        miuiIntent_send(INTENT_INSTALL, 3, file_name, "0", "1");
#ifdef DUALSYSTEM_PARTITIONS
        miuiIntent_send(INTENT_SETSYSTEM,1,"0");
#endif
        return 0;
    }
    else {
#ifdef DUALSYSTEM_PARTITIONS
        miuiIntent_send(INTENT_SETSYSTEM,1,"0");
#endif
        return -1;
    }
}
//callback funtion file filter, if access ,return 0; others return -1
int file_filter(char *file, int file_len)
{
    return_val_if_fail(file != NULL, RET_FAIL);
    int len = strlen(file);
    return_val_if_fail(len <= file_len, RET_INVALID_ARG);
    if (len >= 4 && strncasecmp(file + len -4, ".zip", 4) == 0)
        return 0;
    return -1;
     
}
static STATUS sd_menu_show(menuUnit *p)
{
    //ensure_mounte sd path
    struct _intentResult* result = miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
    //whatever wether sdd is mounted, scan sdcard and go on
    //assert_if_fail(miuiIntent_result_get_int() == 0);
    int ret ;
    ret = file_scan("/sdcard", sizeof("/sdcard"), p->name, strlen(p->name), &file_install, (void *)p, &file_filter);
    if (ret == -1) return MENU_BACK;
    return ret;
}

static STATUS sdext_menu_show(menuUnit *p)
{
    //ensure_mounte sd path
    struct _intentResult* result = miuiIntent_send(INTENT_MOUNT, 1, "/external_sd");
    //whatever wether sdd is mounted, scan sdcard and go on
    //assert_if_fail(miuiIntent_result_get_int() == 0);
    int ret ;
    ret = file_scan("/external_sd", sizeof("/external_sd"), p->name, strlen(p->name), &file_install, (void *)p, &file_filter);
    if (ret == -1) return MENU_BACK;
    return ret;
}
static STATUS sd_update_show(menuUnit *p)
{
    char new_path[SD_MAX_PATH] = "/sdcard/update.zip";
    int wipe_cache = 0;
#ifdef DUALSYSTEM_PARTITIONS
    int choose_system_num;
    if (is_tdb_enabled()) {
        if (RET_YES == miui_confirm(5, "<~choose.system.title>", "<~choose.system.text>", "@alert", "<~choice.system0.name>", "<~choice.system1.name>")) {
            miuiIntent_send(INTENT_SETSYSTEM,1,"1");
        } else {
            miuiIntent_send(INTENT_SETSYSTEM,1,"2");
        }
    
    }
#endif
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
        miuiIntent_send(INTENT_INSTALL, 3, new_path, "0", "1");
    }
    miuiIntent_send(INTENT_SETSYSTEM,1,"0");
    return MENU_BACK;
}
struct _menuUnit * sd_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    strncpy(p->name, "<~sd.name>", MENU_LEN);
    strncpy(p->title_name, "<~sd.title_name>", MENU_LEN);
    strncpy(p->icon, "@sd",  MENU_LEN);
    p->result = 0;
    return_null_if_fail(menuNode_init(p) != NULL);
    //install from sd
    struct _menuUnit  *temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    menuUnit_set_icon(temp, "@sd.choose");
    strncpy(temp->name, "<~sd.install.name>", MENU_LEN);
    temp->show = &sd_menu_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    //install update.bin from sd
    temp = common_ui_init();
    menuUnit_set_icon(temp, "@sd.install");
    strncpy(temp->name,"<~sd.update.name>", MENU_LEN);
    temp->show = &sd_update_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    if (acfg()->sd_ext == 1)
    {
        //install from external_sd
        struct _menuUnit  *temp = common_ui_init();
        return_null_if_fail(temp != NULL);
        menuUnit_set_icon(temp, "@sd.choose");
        strncpy(temp->name, "<~sdext.install.name>", MENU_LEN);
        temp->show = &sdext_menu_show;
        assert_if_fail(menuNode_add(p, temp) == RET_OK);
    }
    return p;
}
