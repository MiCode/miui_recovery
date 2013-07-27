#include <stdlib.h>
#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"

#define POWER_REBOOT                 0
#define POWER_RECOVERY               1
#define POWER_BOOTLOADER             2
#define POWER_POWEROFF               3
#ifdef DUALSYSTEM_PARTITIONS
#define POWER_REBOOT_SYSTEM0         4
#define POWER_REBOOT_SYSTEM1         5
extern int is_tdb_enabled();
static int setbootmode(char* bootmode) {
   // open misc-partition
   FILE* misc = fopen("/dev/block/platform/msm_sdcc.1/by-name/misc", "wb");
   if (misc == NULL) {
      printf("Error opening misc partition.\n");
      return -1;
   }

   // write bootmode
   fseek(misc, 0x1000, SEEK_SET);
   if(fputs(bootmode, misc)<0) {
      printf("Error writing bootmode to misc partition.\n");
      return -1;
   }

   // close
   fclose(misc);
   return 0;
}
#endif
static STATUS power_child_show(menuUnit *p)
{
    //confirm
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
#ifdef DUALSYSTEM_PARTITIONS
        //set truedualboot script
        miuiIntent_send(INTENT_UNMOUNT, 1, "/system");
        miuiIntent_send(INTENT_UNMOUNT, 1, "/system1");
        miuiIntent_send(INTENT_SETSYSTEM,1,"0");
        miuiIntent_send(INTENT_MOUNT, 1, "/system");
        miuiIntent_send(INTENT_MOUNT, 1, "/system1");
        __system("rm /system/bin/mount_ext4.sh");
        __system("rm /system1/bin/mount_ext4.sh");
        if (is_tdb_enabled()) {
            __system("cp /res/dualsystem/mount_ext4_tdb.sh /system/bin/mount_ext4.sh");
            __system("cp /res/dualsystem/mount_ext4_tdb.sh /system1/bin/mount_ext4.sh");
        } else {
            __system("cp /res/dualsystem/mount_ext4_default.sh /system/bin/mount_ext4.sh");
            __system("cp /res/dualsystem/mount_ext4_default.sh /system1/bin/mount_ext4.sh");
        }
        __system("chmod 777 /system/bin/mount_ext4.sh");
        __system("chmod 777 /system1/bin/mount_ext4.sh");
#endif
        //disable recovery flash
        miuiIntent_send(INTENT_MOUNT, 1, "/system");
        miuiIntent_send(INTENT_MOUNT, 1, "/system1");
        __system("rm /system/etc/install_recovery.sh");
        __system("rm /system1/etc/install_recovery.sh");
        switch(p->result) {
            case POWER_REBOOT:
                miuiIntent_send(INTENT_REBOOT, 1, "reboot");
                break;
            case POWER_BOOTLOADER:
                miuiIntent_send(INTENT_REBOOT, 1, "bootloader");
                break;
            case POWER_RECOVERY:
                miuiIntent_send(INTENT_REBOOT, 1, "recovery");
                break;
            case POWER_POWEROFF:
                miuiIntent_send(INTENT_REBOOT, 1, "poweroff");
                break;
#ifdef DUALSYSTEM_PARTITIONS
            case POWER_REBOOT_SYSTEM0:
                setbootmode("boot-system0");
                miuiIntent_send(INTENT_REBOOT, 1, "reboot");
                break;
            case POWER_REBOOT_SYSTEM1:
                setbootmode("boot-system1");
                miuiIntent_send(INTENT_REBOOT, 1, "reboot");
                break;
#endif
            default:
                assert_if_fail(0);
            break;
        }
    }
    return MENU_BACK;
}

struct _menuUnit* reboot_ui_init()
{
    struct _menuUnit *temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.null>", MENU_LEN);
    menuUnit_set_icon(temp, "@reboot");
    temp->result = POWER_REBOOT;
    temp->show = &power_child_show;
    return temp;
}

struct _menuUnit * power_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    strncpy(p->name, "<~power.name>", MENU_LEN);
    menuUnit_set_title(p, "<~power.title>");
    menuUnit_set_icon(p,"@power");
    menuUnit_set_show(p, &common_menu_show);
    p->result = 0;
    assert_if_fail(menuNode_init(p) != NULL);
    //reboot
    struct _menuUnit *temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.null>", MENU_LEN);
    menuUnit_set_title(temp, "<~reboot.null.title>");
    menuUnit_set_icon(temp, "@reboot");
    temp->result = POWER_REBOOT;
    temp->show = &power_child_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    //reboot bootloader
    temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.bootloader>", MENU_LEN);
    menuUnit_set_icon(temp, "@reboot.bootloader");
    temp->result = POWER_BOOTLOADER;
    temp->show = &power_child_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);

    //reboot recovery
    temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.recovery>", MENU_LEN);
    menuUnit_set_icon(temp, "@reboot.recovery");
    temp->result = POWER_RECOVERY;
    temp->show = &power_child_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
#ifdef DUALSYSTEM_PARTITIONS
    //reboot to system0
    temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.system0>", MENU_LEN);
    menuUnit_set_title(temp, "<~reboot.system0.title>");
    menuUnit_set_icon(temp, "@reboot");
    temp->result = POWER_REBOOT_SYSTEM0;
    temp->show = &power_child_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);

    //reboot to system1
    temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.system1>", MENU_LEN);
    menuUnit_set_title(temp, "<~reboot.system1.title>");
    menuUnit_set_icon(temp, "@reboot");
    temp->result = POWER_REBOOT_SYSTEM1;
    temp->show = &power_child_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
#endif
    //poweroff
    temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    strncpy(temp->name, "<~reboot.poweroff>", MENU_LEN);
    menuUnit_set_icon(temp, "@power");
    temp->result = POWER_POWEROFF;
    temp->show = &power_child_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return p;
}

