#include <stdlib.h>
#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"

#define POWER_REBOOT                 0
#define POWER_RECOVERY               1
#define POWER_BOOTLOADER             2
#define POWER_POWEROFF               3

static STATUS power_child_show(menuUnit *p)
{
    //confirm
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
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

