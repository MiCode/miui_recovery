#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"
static struct _menuUnit *tdb_node = NULL;

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
static STATUS sideload_menu_show(struct _menuUnit *p) {
    miui_sideload_process();
    miuiIntent_send(INTENT_SIDELOAD, 1, NULL);
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
        miuiIntent_send(INTENT_INSTALL, 3, "/tmp/update.zip", "0", "1");
    }
#ifdef DUALSYSTEM_PARTITIONS
    miuiIntent_send(INTENT_SETSYSTEM,1,"0");
#endif
    __system("rm /tmp/update.zip");
    return MENU_BACK;
}
#ifdef DUALSYSTEM_PARTITIONS
int is_tdb_enabled()
{
    struct stat st;
    miuiIntent_send(INTENT_MOUNT, 1, "/data");
    return (lstat("/data/.truedualboot",&st)==0);
}
static STATUS enable_or_disable_tdb(struct _menuUnit* p)
{
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
        miui_busy_process();
        if(is_tdb_enabled()) {
            miuiIntent_send(INTENT_UNMOUNT, 1, "/data");
            miuiIntent_send(INTENT_FORMAT, 1, "/data");
            menuUnit_set_name(tdb_node,"<~tool.enable.tdb>");
        } else {
            miuiIntent_send(INTENT_UNMOUNT, 1, "/data");
            miuiIntent_send(INTENT_FORMAT, 1, "/data");
            miuiIntent_send(INTENT_MOUNT, 1, "/data");
            menuUnit_set_name(tdb_node,"<~tool.disable.tdb>");
            FILE * pFile = fopen("/data/.truedualboot","w");
            fclose(pFile);
        }
    }
    return MENU_BACK;
}
static STATUS tool_menu_show(struct _menuUnit* p)
{
    if (is_tdb_enabled()) {
        menuUnit_set_name(tdb_node, "<~tool.disable.tdb>");
        menuUnit_set_icon(tdb_node, "@alert");
    } else {
        menuUnit_set_name(tdb_node, "<~tool.enable.tdb>");
        menuUnit_set_icon(tdb_node, "@alert");
    }
    //show menu
    return_val_if_fail(p != NULL, RET_FAIL);
    int n = p->get_child_count(p);
    return_val_if_fail(n > 0, RET_FAIL);
    int selindex = 0;
    return_val_if_fail(n >= 1, RET_FAIL);
    return_val_if_fail(n < ITEM_COUNT, RET_FAIL);
    struct _menuUnit *temp = p->child;
    return_val_if_fail(temp != NULL, RET_FAIL);
    char **menu_item = malloc(n * sizeof(char *));
    assert_if_fail(menu_item != NULL);
    char **icon_item=malloc(n * sizeof(char *));
    assert_if_fail(icon_item != NULL);
    char **title_item= malloc(n * sizeof(char *));
    assert_if_fail(title_item != NULL);
    int i = 0;
    for (i = 0; i < n; i++)
    {
        menu_item[i] = temp->name;
        title_item[i] = temp->title_name;
        icon_item[i] = temp->icon;
        temp = temp->nextSilbing;
    }
    selindex = miui_menubox(p->name, menu_item, n);
    p->result = selindex;
    if (menu_item != NULL) free(menu_item);
    if (title_item != NULL) free(title_item);
    if (icon_item != NULL) free(icon_item);
    return p->result;
}
#endif
struct _menuUnit* tool_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    menuUnit_set_name(p, "<~tool.name>");
    menuUnit_set_title(p, "<~tool.title>");
    menuUnit_set_icon(p, "@tool");
#ifdef DUALSYSTEM_PARTITIONS
    menuUnit_set_show(p, &tool_menu_show);
#endif
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

    //adb sideload
    temp = common_ui_init();
    menuUnit_set_name(temp, "<~tool.sideload.name>"); 
    menuUnit_set_icon(temp, "@alert");
    menuUnit_set_show(temp, &sideload_menu_show);
    menuUnit_set_desc(temp, "<~tool.sideload.desc>");
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
#ifdef DUALSYSTEM_PARTITIONS
    //truedualboot
    temp = common_ui_init();
    if (is_tdb_enabled()) {
        menuUnit_set_name(temp, "<~tool.disable.tdb>");
        menuUnit_set_icon(temp, "@alert");
    } else {
        menuUnit_set_name(temp, "<~tool.enable.tdb>");
        menuUnit_set_icon(temp, "@alert");
    }
    menuUnit_set_show(temp, &enable_or_disable_tdb);
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    tdb_node = temp;
#endif
    return p;
}
