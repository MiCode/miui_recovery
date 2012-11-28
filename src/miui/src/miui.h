#ifndef MIUI_H_
#define MIUI_H_
//add in ui operation
//
//
#include "utils.h"
typedef int STATUS;
#define RET_OK 0
#define RET_FAIL -1
#define RET_INVALID_ARG -1
#define RET_YES 1
#define RET_NO 0
#define RET_NULL 0

#define ICON_ENABLE   "@enable"
#define ICON_DISABLE  "@disable"
//MAX_MENU_
#define ITEM_COUNT 128
#define MENU_BACK ITEM_COUNT 
#define MENU_QUIT ITEM_COUNT + 1

#define MIUI_LOG_FILE "/tmp/miui_recovery.log"
#define RECOVERY_PATH "/sdcard/miui_recovery"

#ifndef miui_printf
#define miui_printf printf
#endif

#ifndef miui_error
#define miui_error(fmt...) printf("(%d)[%s]%s:%d::", getpid(), __FILE__, __FUNCTION__, __LINE__);printf(fmt)
#endif

#ifndef return_val_if_fail
#define return_val_if_fail(p, val) \
    if (!(p)) { \
       miui_printf("(pid:%d)function %s(line %d) cause %s failed  return %d\n", getpid(), __FUNCTION__, __LINE__, #p,  val);return val;}	
#endif

#ifndef return_null_if_fail
#define return_null_if_fail(p) \
    if (!(p)) { \
       miui_printf("(pid:%d)[%s]function %s(line %d) " #p " \n",getpid(), __FILE__, __FUNCTION__, __LINE__);return NULL;}
#endif

#ifndef assert_if_fail
#define assert_if_fail(p) \
    if (!(p)) { \
       miui_printf("(pid:%d)[%s]function %s(line %d) " #p " \n",getpid(), __FILE__,  __FUNCTION__, __LINE__);}
#endif


#define MENU_LEN 32 //international, ~ or direct print string;
typedef struct _menuUnit {
    char name[MENU_LEN];
    char title_name[MENU_LEN];
    char icon[MENU_LEN];
    char desc[MENU_LEN];
    int  result;

    void *data;

    struct _menuUnit *child;
    struct _menuUnit *nextSilbing;
    struct _menuUnit *parent;
    //

    //method
    STATUS (*show)(struct _menuUnit *p);

    int (*get_child_count)(struct _menuUnit *p);
    struct _menuUnit * (*get_child_by_index)(struct _menuUnit *p, int index);

}menuUnit, *pmenuUnit;
typedef STATUS (*menuUnitFunction)(struct _menuUnit *p);
//miui_stack.c

#ifdef MIUI_STACK
#define MAX_STACK_SIZE 64
typedef pmenuUnit dataType;
typedef struct _miuiStack{
    dataType data[MAX_STACK_SIZE];
    int alloc;
    int top;
}miuiStack, pmiuiStack;
STATUS miuiStack_init();
//judge empty
STATUS miuiStack_isEmpty();
STATUS miuiStack_isFull();
STATUS miuiStack_push(dataType d);
dataType miuiStack_pop();
dataType miuiStack_getTop();
STATUS miuiStack_disp();
#endif
//back_ui.c
//
struct _menuUnit * back_ui_init();
//miui_ui.c

void miui_redraw();
int miui_setbg(char * titlev);
char * miui_readfromfs(char * name);
void miui_writetofs(char * name, char * value);
char * miui_readfromtmp(char * name);
void miui_writetotmp(char * name, char * value);
char * miui_readfromzip(char * name);
char * miui_parseprop(char * filename,char *key);
char * miui_parsepropzip(char * filename,char *key);
char * miui_getvar(char * name);
void miui_setvar(char * name, char * value);
void miui_appendvar(char * name, char * value);
void miui_delvar(char * name);
void miui_prependvar(char * name, char * value);
STATUS miui_font(char *ttf_type, char *ttf_file, char *ttf_size);
char * miui_getprop(char *file, char *key);
char * miui_gettmpprop(char *file, char *key);
char * miui_resread(char *file);
STATUS miui_pleasewait(char *message);
STATUS miui_setcolor(char *item, char *val);
//* 
//* ini_get
//*
char * miui_ini_get(char *item);
//* 
//* ini_set
//*
STATUS miui_ini_set(char *item, char *value);
//* 
//* viewbox
//*
STATUS miui_viewbox(int argc, char *format, ...);
//* 
//* textbox, agreebox
//*
STATUS miui_textbox(char *title, char *desc, char *icon, char *message); 
//* 
//* checkbox
//*
STATUS miui_checkbox(int argc, char *format, ...); 
//* 
//* selectbox
//*
STATUS miui_selectbox(int argc, char *format, ...);
//* 
STATUS miui_langmenu(char *title_name, char *title_icon) ;
//* menubox
//*
STATUS miui_mainmenu(char *title_name, char **item, char **item_icon, char **item_icon_append, int item_cnt); 
STATUS miui_menubox(char *title_name, char **item, int item_cnt);
STATUS miui_sdmenu(char *title_name, char **item, char **item_sub, int item_cnt); 
STATUS miui_aboutmenu(char *tite, char *icon, char *content);
STATUS miui_busy_process();
//* 
//* alert
//*
STATUS miui_alert(int argc, char *format, ...);
//* 
//* confirm
//*
STATUS miui_confirm(int argc, char *format, ...);
//* 
//* textdialog
//*
STATUS miui_textdialog(int argc, char *format, ...);
//* 
//* loadlang
//*
STATUS miui_loadlang(char * name);
//* 
//* lang
//*
char * miui_lang(char *name);
STATUS miui_install(int echo);
int miui_ui_init();
int miui_ui_config(const char * file);
STATUS miui_ui_start();
STATUS miui_ui_end();


#define assert_ui_if_fail(p) if(!(p)) { \
    miui_alert(2, "<~alert.result>", "<~alert.desc>");}

//common_ui.c 
int common_get_child_count(menuUnit *p);
struct _menuUnit * common_get_child_by_index(struct _menuUnit *p, int index);
STATUS common_ui_show(menuUnit *p);
STATUS common_menu_show(menuUnit *p);
STATUS menu_default_init(struct _menuUnit *p);
struct _menuUnit *common_ui_init();
STATUS menuUnit_set_name(struct _menuUnit *p, const char* name);
STATUS menuUnit_set_icon(struct _menuUnit *p, const char* name);
STATUS menuUnit_set_title(struct _menuUnit *p, const char* name);
STATUS menuUnit_set_desc(struct _menuUnit *p, const char* name);
STATUS menuUnit_set_result(struct _menuUnit *p, const int result);
STATUS menuUnit_set_show(struct _menuUnit *p, menuUnitFunction fun);
//lang_ui.c 
struct _menuUnit* lang_ui_init();

//sd_ui.c
struct _menuUnit* sd_ui_init();
//power_ui.c
struct _menuUnit* power_ui_init();
struct _menuUnit* reboot_ui_init();

//mount_ui.c
struct _menuUnit* mount_ui_init();
//power_ui.c
struct _menuUnit* power_ui_init();
//back_ui.c
struct _menuUnit* back_ui_init();
//wipe_ui.c
struct _menuUnit* wipe_ui_init();
//backup_ui.c
struct _menuUnit* backup_ui_init();
//tool_ui.c
struct _menuUnit* tool_ui_init();
//info_ui.c
struct _menuUnit* info_ui_init();
//miui.c
extern struct _menuUnit* g_main_menu;
extern struct _menuUnit* g_root_menu;
STATUS main_ui_init();
STATUS main_ui_show();
STATUS main_ui_release();
//for re draw screen
STATUS miui_set_isbgredraw(int value);

/*
 *miui_install.c
 *@DESCRIPTION:make install pacage, be invoked by installer
 */

typedef int (*miuiInstall_fun)(char* path, int *wipe_cache, char* install_file);
typedef struct _miuiInstall {
    miuiInstall_fun pfun;
    char *path;
    char *install_file;
    int wipe_cache;
}miuiInstall, *pmiuiInstall;
//for install init 
STATUS miuiInstall_init(miuiInstall_fun fun, char *path, int wipe_cache, char* install_file);

void miuiInstall_show_progress(float portion, int seconds);
void miuiInstall_set_progress(float fraction);
void miuiInstall_set_text(char *str);
void miuiInstall_set_info(char *file_name);
//menuNode operation
struct _menuUnit * menuNode_init(struct _menuUnit *node);
STATUS menuNode_add(struct _menuUnit *parent, struct _menuUnit *child);
STATUS menuNode_delete(struct _menuUnit *parent, struct _menuUnit *child);

#endif // __MIUI_H__
