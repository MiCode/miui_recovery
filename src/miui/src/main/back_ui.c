#include "../miui_inter.h"
#include "../miui.h"


STATUS back_ui_show(struct _menuUnit *p)
{
    return_val_if_fail(p != NULL, RET_FAIL);
    p->result = MENU_BACK;
    return p->result;
}
struct _menuUnit * back_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    strncpy(p->name, "<~back.name>", MENU_LEN);
    strncpy(p->title_name, "<~back.title_name>", MENU_LEN);
    strncpy(p->icon, "@title.back", MENU_LEN);
    p->result = 0;
    p->show = &back_ui_show;
    return p;
}

