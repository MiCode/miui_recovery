#include "../miui_inter.h"
#include "../miui.h"
#include "../../../miui_intent.h"

#define MOUNT_CACHE     1
#define MOUNT_DATA      2
#define MOUNT_SYSTEM    3
#define MOUNT_SDCARD    4
#define MOUNT_TOGGLE    5
#define MOUNT_SDEXT    6
#define MOUNT_DESC_MOUNT       "1"
#define MOUNT_DESC_UNMOUNT     "0"
static struct _menuUnit *mount_node;
static struct _menuUnit *mount_sd_node;
static struct _menuUnit *mount_sd_ext_node = NULL;

static struct _menuUnit *mount_cache_node = NULL;
static struct _menuUnit *mount_data_node = NULL;
static struct _menuUnit *mount_system_node = NULL;
static STATUS mount_menu_show(menuUnit *p)
{
    //traverse all mount files
    //ensure cache
    miuiIntent_send(INTENT_ISMOUNT, 1, "/cache");
    if (miuiIntent_result_get_int() == 1)
    {
        menuUnit_set_icon(mount_cache_node, ICON_ENABLE);
        menuUnit_set_desc(mount_cache_node, MOUNT_DESC_MOUNT);
    }
    else
    {
        menuUnit_set_icon(mount_cache_node, ICON_DISABLE);
        menuUnit_set_desc(mount_cache_node, MOUNT_DESC_UNMOUNT);
    }
    //ensure data
    miuiIntent_send(INTENT_ISMOUNT, 1, "/data");
    if (miuiIntent_result_get_int() == 1)
    {
        menuUnit_set_icon(mount_data_node, ICON_ENABLE);
        menuUnit_set_desc(mount_data_node, MOUNT_DESC_MOUNT);
    }
    else
    {
        menuUnit_set_icon(mount_data_node, ICON_DISABLE);
        menuUnit_set_desc(mount_data_node, MOUNT_DESC_UNMOUNT);
    }
    //ensure system
    miuiIntent_send(INTENT_ISMOUNT, 1, "/system");
    if (miuiIntent_result_get_int() == 1)
    {
        menuUnit_set_icon(mount_system_node, ICON_ENABLE);
        menuUnit_set_desc(mount_system_node, MOUNT_DESC_MOUNT);
    }
    else
    {
        menuUnit_set_icon(mount_system_node, ICON_DISABLE);
        menuUnit_set_desc(mount_system_node, MOUNT_DESC_UNMOUNT);
    }
    //ensure sdcard 
    miuiIntent_send(INTENT_ISMOUNT, 1, "/sdcard");
    if (miuiIntent_result_get_int() == 1)
    {
        menuUnit_set_icon(mount_sd_node, ICON_ENABLE);
        menuUnit_set_desc(mount_sd_node, MOUNT_DESC_MOUNT);
    }
    else
    {
        menuUnit_set_icon(mount_sd_node, ICON_DISABLE);
        menuUnit_set_desc(mount_sd_node, MOUNT_DESC_UNMOUNT);
    }
    if (acfg()->sd_ext == 1)
    {
        //ensure sd-ext 
        miuiIntent_send(INTENT_ISMOUNT, 1, "/external_sd");
        if (miuiIntent_result_get_int() == 1)
        {
            menuUnit_set_icon(mount_sd_ext_node, ICON_ENABLE);
            menuUnit_set_desc(mount_sd_ext_node, MOUNT_DESC_MOUNT);
        }
        else
        {
            menuUnit_set_icon(mount_sd_ext_node, ICON_DISABLE);
            menuUnit_set_desc(mount_sd_ext_node, MOUNT_DESC_UNMOUNT);
        }

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
    selindex = miui_mainmenu(p->name, menu_item, NULL, icon_item, n);
    p->result = selindex;
    if (menu_item != NULL) free(menu_item);
    if (title_item != NULL) free(title_item);
    if (icon_item != NULL) free(icon_item);
    return p->result;
}
static STATUS mount_child_show(menuUnit *p)
{
    return_val_if_fail(p != NULL, MENU_BACK);
    intentType intent_type = (p->desc[0] == '0')?INTENT_MOUNT : INTENT_UNMOUNT;
    switch(p->result)
    {
        case MOUNT_CACHE:
            miuiIntent_send(intent_type, 1, "/cache");
            break;
        case MOUNT_DATA:
            miuiIntent_send(intent_type, 1,  "/data");
            break;
        case MOUNT_SYSTEM:
            miuiIntent_send(intent_type, 1, "/system");
            break;
        case MOUNT_SDCARD:
            miuiIntent_send(intent_type, 1, "/sdcard");
            break;
        case MOUNT_SDEXT:
            miuiIntent_send(intent_type, 1, "/external_sd");
            break;
        case MOUNT_TOGGLE:
        {
            if (intent_type == INTENT_MOUNT)
                //mount 
                miuiIntent_send(INTENT_TOGGLE, 1, "1");
            else 
                //untoggle
                miuiIntent_send(INTENT_TOGGLE, 1, "0");
            break;
        }
        default:
            break;
    }
    if(strstr(miuiIntent_result_get_string(), "mounted") != NULL)
    {
        menuUnit_set_icon(p, ICON_ENABLE);
        menuUnit_set_desc(p, MOUNT_DESC_MOUNT);
    }
    else if(strstr(miuiIntent_result_get_string(), "ok") != NULL)
    {
        menuUnit_set_icon(p, ICON_DISABLE);
        menuUnit_set_desc(p, MOUNT_DESC_UNMOUNT);
    }
    else 
    {
        assert_ui_if_fail(0);
    }
    return MENU_BACK;
}

struct _menuUnit *mount_ui_init()
{
    struct _menuUnit* p = common_ui_init();
    return_null_if_fail(p != NULL);
    strncpy(p->name, "<~mount.name>", MENU_LEN);
    menuUnit_set_title(p, "<~mount.title>");
    menuUnit_set_icon(p, "@mount");
    menuUnit_set_show(p, &mount_menu_show);
    return_null_if_fail(menuNode_init(p) != NULL);
    //mount cache?
    struct _menuUnit* temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return_null_if_fail(menuUnit_set_name(temp, "<~mount.cache.name>") == RET_OK);
    return_null_if_fail(menuUnit_set_icon(temp, ICON_DISABLE) == RET_OK);
    return_null_if_fail(menuUnit_set_result(temp, MOUNT_CACHE) == RET_OK);
    return_null_if_fail(menuUnit_set_desc(temp, MOUNT_DESC_UNMOUNT) == RET_OK);
    return_null_if_fail(RET_OK == menuUnit_set_show(temp, &mount_child_show));
    mount_cache_node = temp;
    //mount data
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return_null_if_fail(menuUnit_set_name(temp, "<~mount.data.name>") == RET_OK);
    return_null_if_fail(menuUnit_set_result(temp, MOUNT_DATA) == RET_OK);
    return_null_if_fail(menuUnit_set_icon(temp, ICON_DISABLE) == RET_OK);
    return_null_if_fail(menuUnit_set_desc(temp, MOUNT_DESC_UNMOUNT) == RET_OK);
    return_null_if_fail(RET_OK == menuUnit_set_show(temp, &mount_child_show));
    mount_data_node = temp;

    //mount system
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return_null_if_fail(menuUnit_set_name(temp, "<~mount.system.name>") == RET_OK);
    return_null_if_fail(menuUnit_set_result(temp, MOUNT_SYSTEM) == RET_OK);
    return_null_if_fail(menuUnit_set_icon(temp, ICON_DISABLE) == RET_OK);
    return_null_if_fail(menuUnit_set_desc(temp, MOUNT_DESC_UNMOUNT) == RET_OK);
    return_null_if_fail(RET_OK == menuUnit_set_show(temp, &mount_child_show));
    mount_system_node = temp;
    //mount sdcard
    temp = common_ui_init();
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return_null_if_fail(menuUnit_set_name(temp, "<~mount.sdcard.name>") == RET_OK);
    return_null_if_fail(menuUnit_set_result(temp, MOUNT_SDCARD) == RET_OK);
    return_null_if_fail(menuUnit_set_icon(temp, ICON_DISABLE) == RET_OK);
    return_null_if_fail(menuUnit_set_desc(temp, MOUNT_DESC_UNMOUNT) == RET_OK);
    return_null_if_fail(RET_OK == menuUnit_set_show(temp, &mount_child_show));
    mount_sd_node = temp;

    if (acfg()->sd_ext == 1)
    {
        //mount external_sd
        temp = common_ui_init();
        assert_if_fail(menuNode_add(p, temp) == RET_OK);
        return_null_if_fail(menuUnit_set_name(temp, "<~mount.sdext.name>") == RET_OK);
        return_null_if_fail(menuUnit_set_result(temp, MOUNT_SDEXT) == RET_OK);
        return_null_if_fail(menuUnit_set_icon(temp, ICON_DISABLE) == RET_OK);
        return_null_if_fail(menuUnit_set_desc(temp, MOUNT_DESC_UNMOUNT) == RET_OK);
        return_null_if_fail(RET_OK == menuUnit_set_show(temp, &mount_child_show));
        mount_sd_ext_node = temp;
    }

   
    //toggle usb stroage
    if (acfg()->lun_file[0] != 0)
    {
        temp = common_ui_init();
        assert_if_fail(menuNode_add(p, temp) == RET_OK);
        return_null_if_fail(menuUnit_set_name(temp, "<~mount.toggle.name>") == RET_OK);
        return_null_if_fail(menuUnit_set_result(temp, MOUNT_TOGGLE) == RET_OK);
        return_null_if_fail(menuUnit_set_icon(temp, ICON_DISABLE) == RET_OK);
        return_null_if_fail(menuUnit_set_desc(temp, MOUNT_DESC_UNMOUNT) == RET_OK);
        return_null_if_fail(RET_OK == menuUnit_set_show(temp, &mount_child_show));
    }
    mount_node = p;
    return p;

}


