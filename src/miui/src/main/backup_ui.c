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


#define BACKUP_ALL            1
#define BACKUP_CACHE          2
#define BACKUP_DATA           3
#define BACKUP_SYSTEM         4
#define BACKUP_BOOT           5
#define BACKUP_RECOVERY       6

#define RESTORE_ALL           11
#define RESTORE_CACHE         12
#define RESTORE_DATA          13
#define RESTORE_SYSTEM        14
#define RESTORE_BOOT          15
#define RESTORE_RECOVERY      16

static struct _menuUnit* p_current = NULL;
static struct _menuUnit* backup_menu = NULL;
static STATUS backup_restore(char* path)
{
    return_val_if_fail(p_current != NULL, RET_FAIL);
    miui_busy_process();
    switch(p_current->result) {
        case RESTORE_ALL:
            miuiIntent_send(INTENT_RESTORE, 7, path, "1", "1", "1", "1", "0", "0");
            break;
        case RESTORE_CACHE:
            miuiIntent_send(INTENT_RESTORE, 7, path, "0", "0", "0", "1", "0", "0");
            break;
        case RESTORE_DATA:
            miuiIntent_send(INTENT_RESTORE, 7, path, "0", "0", "1", "0", "0", "0");
            break;
        case RESTORE_SYSTEM:
            miuiIntent_send(INTENT_RESTORE, 7, path, "0", "1", "0", "0", "0", "0");
           break;
        case RESTORE_BOOT:
            miuiIntent_send(INTENT_RESTORE, 7, path, "1", "0", "0", "0", "0", "0");
            break;
        default:
            miui_error("p->resulte %d should not be the value\n", p_current->result);
            break;
    }
    return RET_OK;
}

int file_backup(char *file_name, int file_len, void *data)
{
    return_val_if_fail(file_name != NULL, RET_FAIL);
    return_val_if_fail(strlen(file_name) <= file_len, RET_INVALID_ARG);
    /*
     *nandroid_restore(backup_path, restore_boot, system, data, chache , sdext, wimax)
     */
    backup_restore(file_name);
    //to up layer
    return -1;
}

static STATUS restore_child_show(menuUnit* p)
{
    p_current = p;
    miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
    return_val_if_fail(miuiIntent_result_get_int() == 0, MENU_BACK);
    char path_name[PATH_MAX];
    switch(p->result) {
        case RESTORE_ALL:
            snprintf(path_name,PATH_MAX, "%s/backup/backup", RECOVERY_PATH);
            break;
        case RESTORE_CACHE:
            snprintf(path_name,PATH_MAX, "%s/backup/cache", RECOVERY_PATH);
            break;
        case RESTORE_DATA:
            snprintf(path_name,PATH_MAX, "%s/backup/data", RECOVERY_PATH);
            break;
        case RESTORE_SYSTEM:
            snprintf(path_name,PATH_MAX, "%s/backup/system", RECOVERY_PATH);
            break;
        case RESTORE_BOOT:
            snprintf(path_name,PATH_MAX, "%s/backup/boot", RECOVERY_PATH);
            break;
        default:
            miui_error("p->resulte %d should not be the value\n", p->result);
            return MENU_BACK;
    }
    file_scan(path_name, PATH_MAX, p->name, strlen(p->name), &file_backup, NULL, NULL);
    return MENU_BACK;
}

static STATUS backup_child_show(menuUnit* p)
{
    p_current = p;
    miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
    char path_name[PATH_MAX];
    static time_t timep;
    static struct tm *time_tm = NULL;
    time(&timep);
    time_tm = gmtime(&timep);
    return_val_if_fail(miuiIntent_result_get_int() == 0, MENU_BACK);
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
        miui_busy_process();
        switch(p->result) {
            case BACKUP_ALL:
                snprintf(path_name,PATH_MAX, "%s/backup/backup/%02d%02d%02d-%02d%02d",
                        RECOVERY_PATH, time_tm->tm_year,
                        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min);
                miuiIntent_send(INTENT_BACKUP, 1, path_name);
                break;
            case BACKUP_CACHE:
                snprintf(path_name,PATH_MAX, "%s/backup/cache/%02d%02d%02d-%02d%02d",
                        RECOVERY_PATH, time_tm->tm_year,
                        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min);
                miuiIntent_send(INTENT_ADVANCED_BACKUP, 2 , path_name, "/cache");
                break;
            case BACKUP_DATA:
                snprintf(path_name,PATH_MAX, "%s/backup/data/%02d%02d%02d-%02d%02d",
                        RECOVERY_PATH, time_tm->tm_year,
                        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min);
                miuiIntent_send(INTENT_ADVANCED_BACKUP, 2 , path_name, "/data");
                break;
            case BACKUP_SYSTEM:
                snprintf(path_name,PATH_MAX, "%s/backup/system/%02d%02d%02d-%02d%02d",
                        RECOVERY_PATH, time_tm->tm_year,
                        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min);
                miuiIntent_send(INTENT_ADVANCED_BACKUP, 2 , path_name, "/system");
                break;
            case BACKUP_BOOT:
                snprintf(path_name,PATH_MAX, "%s/backup/boot/%02d%02d%02d-%02d%02d",
                        RECOVERY_PATH, time_tm->tm_year,
                        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour, time_tm->tm_min);
                miuiIntent_send(INTENT_ADVANCED_BACKUP, 2 , path_name, "/boot");
                break;
            default:
                miui_error("p->resulte %d should not be the value\n", p->result);
                break;
        }
    }
    return MENU_BACK;
}
struct _menuUnit* advanced_backup_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    menuUnit_set_name(p, "<~advanced_backup.name>");
    menuUnit_set_show(p, &common_menu_show);
    return_null_if_fail(menuNode_init(p) != NULL);
    //backup boot
    struct _menuUnit* temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_backup.boot.name>");
    menuUnit_set_result(temp, BACKUP_BOOT);
    menuUnit_set_show(temp, &backup_child_show);
    //backup system
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_backup.system.name>");
    menuUnit_set_result(temp, BACKUP_SYSTEM);
    menuUnit_set_show(temp, &backup_child_show);
    //backup data
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_backup.data.name>");
    menuUnit_set_result(temp, BACKUP_DATA);
    menuUnit_set_show(temp, &backup_child_show);
    //backup cache
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_backup.cache.name>");
    menuUnit_set_result(temp, BACKUP_CACHE);
    menuUnit_set_show(temp, &backup_child_show);
    return p;
}
struct _menuUnit* advanced_restore_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    menuUnit_set_name(p, "<~advanced_restore.name>");
    menuUnit_set_show(p, &common_menu_show);
    return_null_if_fail(menuNode_init(p) != NULL);
    //restore boot
    struct _menuUnit* temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_restore.boot.name>");
    menuUnit_set_result(temp, RESTORE_BOOT);
    menuUnit_set_show(temp, &restore_child_show);
    //restore system
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_restore.system.name>");
    menuUnit_set_result(temp, RESTORE_SYSTEM);
    menuUnit_set_show(temp, &restore_child_show);
    //restore data
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_restore.data.name>");
    menuUnit_set_result(temp, RESTORE_DATA);
    menuUnit_set_show(temp, &restore_child_show);
    //restore cache
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~advanced_restore.cache.name>");
    menuUnit_set_result(temp, RESTORE_CACHE);
    menuUnit_set_show(temp, &restore_child_show);
    return p;
}
struct _menuUnit* backup_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    menuUnit_set_name(p, "<~backup.name>");
    menuUnit_set_title(p, "<~backup.title>");
    menuUnit_set_icon(p, "@backup");
    menuUnit_set_show(p, &common_menu_show);
    return_null_if_fail(menuNode_init(p) != NULL);
    backup_menu = p;
    //backup
    struct _menuUnit* temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~backup.backup.name>");
    menuUnit_set_result(temp, BACKUP_ALL);
    menuUnit_set_show(temp, &backup_child_show);
    //restore
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    menuUnit_set_name(temp, "<~backup.restore.name>");
    menuUnit_set_result(temp, RESTORE_ALL);
    menuUnit_set_show(temp, &restore_child_show);
    //advanced backup
    temp = advanced_backup_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    //advanced restore
    temp = advanced_restore_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return p;
}
