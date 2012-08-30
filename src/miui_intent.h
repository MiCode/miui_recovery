#ifndef _MIUI_INTENT_H
#define _MIUI_INTENT_H

#ifndef return_val_if_fail
#define miui_printf printf
#define miui_error printf
#define return_val_if_fail(p, val) \
    if (!(p)) { \
        miui_printf("function %s(line %d) cause %s failed  return %d\n", __FUNCTION__, __LINE__, #p,  val);return val;}	
#define return_null_if_fail(p) \
    if (!(p)) { \
        miui_printf("function %s(line %d) " #p " \n", __FUNCTION__, __LINE__);return NULL;}	
#define assert_if_fail(p) \
    if (!(p)) { \
        miui_printf("function %s(line %d) " #p " \n",  __FUNCTION__, __LINE__);}
#endif

typedef enum _intentType{
    INTENT_MOUNT,
    INTENT_ISMOUNT,
    INTENT_UNMOUNT,
    INTENT_REBOOT,
    INTENT_POWEROFF,
    INTENT_INSTALL,
    INTENT_WIPE,
    INTENT_TOGGLE,
    INTENT_FORMAT,
    INTENT_RESTORE,
    INTENT_BACKUP,
    INTENT_ADVANCED_BACKUP,
    INTENT_SYSTEM,
    INTENT_COPY
}intentType;

#define INTENT_RESULT_LEN 16
typedef struct _intentResult{
    int ret;
    char result[INTENT_RESULT_LEN];
}intentResult, pintentResult;

typedef intentResult * (*intentFunction)(int argc, char *argv[]);
typedef struct _intentData{
    intentType type;
    intentFunction function;
}intentData, *pintentData;
typedef struct _miuiIntent{
    struct _intentData  *data;
    int alloc;
    int size;
}miuiIntent, *pmiuiIntent;
extern struct _intentResult intent_result;
int miuiIntent_init(int size);
int miuiIntent_register(intentType type, intentFunction function);
intentResult * miuiIntent_send(intentType type, int argc, char *args, ...);
intentResult*  miuiIntent_result_set(int ret, char *str);
char* miuiIntent_result_get_string();
int miuiIntent_result_get_int();
intentResult* intent_toggle(int argc, char *argv[]);
#endif
