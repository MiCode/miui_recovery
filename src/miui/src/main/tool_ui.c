#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"

static STATUS battary_menu_show(struct _menuUnit* p)
{
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
        miuiIntent_send(INTENT_MOUNT, 1, "/data");
        unlink("/data/system/batterystats.bin");
        miuiIntent_send(INTENT_UNMOUNT, 1, "/data");
        miui_printf("Battery Stats wiped.\n");
    }
    return MENU_BACK;
}
static STATUS permission_menu_show(struct _menuUnit* p)
{
    miuiIntent_send(INTENT_MOUNT, 1, "/system");
    miuiIntent_send(INTENT_MOUNT, 1, "/data");
    miuiIntent_send(INTENT_SYSTEM, 1, "fix_permissions");
    miui_alert(4, p->name, "<~global_done>", "@alert", acfg()->text_ok);
    return MENU_BACK;
}
static STATUS log_menu_show(struct _menuUnit* p)
{
    char desc[512];
    char file_name[PATH_MAX];
    struct stat st;
    time_t timep;
    struct tm *time_tm;
    time(&timep);
    time_tm = gmtime(&timep);
    if (stat(RECOVERY_PATH, &st) != 0)
    {
        mkdir(RECOVERY_PATH, 0755);
    }
    snprintf(file_name, PATH_MAX - 1, "%s/log", RECOVERY_PATH);
    if (stat(file_name, &st) != 0)
    {
        mkdir(file_name, 0755);
    }
    snprintf(file_name, PATH_MAX - 1, "%s/log/recovery-%02d%02d%02d-%02d%02d.log", RECOVERY_PATH,
           time_tm->tm_year, time_tm->tm_mon + 1, time_tm->tm_mday,
           time_tm->tm_hour, time_tm->tm_min);
    snprintf(desc, 511, "%s%s?",p->desc, file_name);
    if (RET_YES == miui_confirm(3, p->name, desc, p->icon)) {
        miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
        miuiIntent_send(INTENT_COPY, 2, MIUI_LOG_FILE, file_name);
    }
    return MENU_BACK;
}
struct _menuUnit* tool_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    menuUnit_set_name(p, "<~tool.name>");
    menuUnit_set_title(p, "<~tool.title>");
    menuUnit_set_icon(p, "@tool");
    assert_if_fail(menuNode_init(p) != NULL);
    //batarry wipe
    struct _menuUnit *temp = common_ui_init();
    menuUnit_set_name(temp, "<~tool.battary.name>"); 
    menuUnit_set_icon(temp, "@tool.battery");
    menuUnit_set_show(temp, &battary_menu_show);
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    //copy log
    temp = common_ui_init();
    menuUnit_set_name(temp, "<~tool.log.name>"); 
    menuUnit_set_show(temp, &log_menu_show);
    menuUnit_set_icon(temp, "@tool.log");
    menuUnit_set_desc(temp, "<~tool.log.desc>");
    assert_if_fail(menuNode_add(p, temp) == RET_OK);

    //fix permission
    temp = common_ui_init();
    menuUnit_set_name(temp, "<~tool.permission.name>"); 
    menuUnit_set_icon(temp, "@tool.permission");
    menuUnit_set_show(temp, &permission_menu_show);
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return p;
}
