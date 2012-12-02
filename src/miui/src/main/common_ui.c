#include "../miui_inter.h"
#include "../miui.h"
static int _common_get_silbing_count(menuUnit *p)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    if (p->nextSilbing == NULL)return 0;
    return _common_get_silbing_count(p->nextSilbing) + 1;
}
int common_get_child_count(menuUnit *p)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    if (p->child == NULL)
        return 0;
    return _common_get_silbing_count(p->child) + 1;
}

struct _menuUnit * common_get_child_by_index(struct _menuUnit *p, int index)
{
    return_null_if_fail(p != NULL);
    return_null_if_fail(index > 0);
    return_null_if_fail(p->child != NULL);
    struct _menuUnit *temp = p->child;
    int i = 1;
    for (i = 1; i < index; i++)
    {
        temp = temp->nextSilbing;
        return_null_if_fail(temp != NULL);

    }
    return temp;
}

//for
//*for speed, can write following, but limit to ITEM_COUNT
static STATUS common_ui_show_node(menuUnit *p)
{
    //todo 
    return MENU_BACK;
}
STATUS common_ui_show(menuUnit *p)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    int n = p->get_child_count(p);
    if (n == 0) return common_ui_show_node(p);//show node 
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
    selindex = miui_mainmenu(p->name, menu_item,  icon_item, NULL,  n);
    p->result = selindex;
    if (menu_item != NULL) free(menu_item);
    if (title_item != NULL) free(title_item);
    if (icon_item != NULL) free(icon_item);
    return p->result;
}

STATUS common_menu_show(menuUnit *p)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    int n = p->get_child_count(p);
    if (n == 0) return common_ui_show_node(p);//show node 
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
STATUS menu_default_init(struct _menuUnit *p)
{
    strncpy(p->name, "<~default_name>", MENU_LEN);
    strncpy(p->title_name, "<~default_title>", MENU_LEN);
    strncpy(p->icon, "@default", MENU_LEN);
    strncpy(p->desc, "<~default_desc>", MENU_LEN);
    p->result = 0;
    p->data = NULL;
    p->child = NULL;
    p->nextSilbing = NULL;
    p->parent = NULL;
    p->show = &common_menu_show;
    p->get_child_count = &common_get_child_count;
    p->get_child_by_index = &common_get_child_by_index;
    return RET_OK;
}

struct _menuUnit *common_ui_init()
{
    struct _menuUnit *p = (struct _menuUnit *)malloc(sizeof(menuUnit));
    return_null_if_fail(p != NULL);
    menu_default_init(p);
    return p;
}
inline STATUS  menuUnit_set_name(struct _menuUnit*p, const char *name)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    strncpy(p->name, name, MENU_LEN);
    return RET_OK;
}
inline STATUS menuUnit_set_icon(struct _menuUnit*p, const char *name)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    strncpy(p->icon, name, MENU_LEN);
    return RET_OK;
}
inline STATUS menuUnit_set_title(struct _menuUnit*p, const char *name)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    strncpy(p->title_name, name, MENU_LEN); return RET_OK;
}
inline STATUS menuUnit_set_desc(struct _menuUnit*p, const char *name)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    strncpy(p->desc, name, MENU_LEN);
    return RET_OK;
}
inline STATUS menuUnit_set_result(struct _menuUnit*p, const int result)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    p->result = result;
    return RET_OK;
}
inline STATUS menuUnit_set_show(struct _menuUnit*p, menuUnitFunction fun)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    p->show = fun;
    return RET_OK;
}
