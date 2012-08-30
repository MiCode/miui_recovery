#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"

#define MOUNT_LOG "/tmp/storage.log"
STATUS info_mount_show(struct _menuUnit* p)
{
    miuiIntent_send(INTENT_MOUNT, 1,  "/data");
    miuiIntent_send(INTENT_MOUNT, 1, "/cache");
    miuiIntent_send(INTENT_MOUNT, 1, "/system");
    miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
    char command[256];
    snprintf(command, 256, "df -hP > %s", MOUNT_LOG);
    miuiIntent_send(INTENT_SYSTEM, 1, command);
    snprintf(command, 256, "%s", MOUNT_LOG);
    miui_textbox(p->name, p->title_name, p->icon, miui_readfromfs(command));
    return MENU_BACK;
}

STATUS info_log_show(struct _menuUnit* p)
{
    char file_name[PATH_MAX];
    snprintf(file_name, PATH_MAX, "%s", MIUI_LOG_FILE);
    miui_textbox(p->name, p->title_name, p->icon, miui_readfromfs(file_name));
    return MENU_BACK;
}

STATUS info_about_show(struct _menuUnit* p)
{
    char message[512];
    snprintf(message,512,
    "<b>%s %s</b>\n"
    "%s\n\n"
    "  <#selectbg_g>Build <u>%s</u></#> (<b>%s</b>)\n"
    "  %s\n"
    "  %s\n"
    "  <u>%s</u>\n\n"
    "ROM Name:\n  <b><#selectbg_g>%s</#></b>\n"
    "ROM Version:\n  <b><#selectbg_g>%s</#></b>\n"
    "ROM Author:\n  <b><#selectbg_g>%s</#></b>\n"
    "Device:\n  <b><#selectbg_g>%s</#></b>\n"
    "Update:\n  <b><#selectbg_g>%s</#></b>"
    ,
    MIUI_NAME,
    MIUI_VERSION,
    MIUI_COPY,
    
    MIUI_BUILD,
    MIUI_BUILD_CN,
    MIUI_BUILD_L,
    MIUI_BUILD_A,
    MIUI_BUILD_URL,
    
    acfg()->rom_name,
    acfg()->rom_version,
    acfg()->rom_author,
    acfg()->rom_device,
    acfg()->rom_date
  );
    miui_alert(3, MIUI_NAME " " MIUI_VERSION, message, "@default");
    return MENU_BACK;
}

struct _menuUnit* info_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    menuUnit_set_name(p, "<~info.name>");
    menuUnit_set_title(p, "<~info.title>");
    menuUnit_set_icon(p, "@info");
    assert_if_fail(menuNode_init(p) != NULL);
    //mount
    struct _menuUnit *temp = common_ui_init();
    menuUnit_set_name(temp, "<~info.mount.name>"); 
    menuUnit_set_icon(temp, "@info.mount");
    menuUnit_set_show(temp, &info_mount_show);
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    //log
    temp = common_ui_init();
    menuUnit_set_name(temp, "<~info.log.name>"); 
    menuUnit_set_icon(temp, "@info.log");
    menuUnit_set_show(temp, &info_log_show);
    assert_if_fail(menuNode_add(p, temp) == RET_OK);

    //about
    temp = common_ui_init();
    menuUnit_set_name(temp, "<~info.about.name>"); 
    menuUnit_set_icon(temp, "@info.about");
    menuUnit_set_show(temp, &info_about_show);
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return p;
}
