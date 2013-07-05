#include "../miui_inter.h"
#include "../miui.h"
static STATUS lang_menu_show(menuUnit *p)
{
    return_val_if_fail(p != NULL, RET_FAIL);
        miui_loadlang("langs/cn.lang");
        miui_font( "0", "ttf/DroidSansFallback.ttf;ttf/DroidSans.ttf", "12" );
        miui_font( "1", "ttf/DroidSansFallback.ttf;ttf/DroidSans.ttf", "18" );
        p->result = 1;
    return p->result;
}

struct _menuUnit * lang_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    strncpy(p->name, "<~lang.name>", MENU_LEN);
    strncpy(p->title_name, "<~lang.title_name>", MENU_LEN);
    menuUnit_set_icon(p, "@lang");
    p->result = 0;
    p->show = &lang_menu_show;
    return p;
}


