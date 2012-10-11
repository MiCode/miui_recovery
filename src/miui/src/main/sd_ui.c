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
/*
 *_sd_show_dir show file system on screen
 *return MENU_BACK when pree backmenu
 */
static struct _menuUnit *sd_menu = NULL;
#define SD_MAX_PATH 256

static STATUS _sd_dir_show(struct _menuUnit *p, char *path)
{
    DIR* d;
    struct dirent* de;
    d = opendir(path);
    return_val_if_fail(d != NULL, RET_FAIL);

    int d_size = 0;
    int d_alloc = 10;
    return_val_if_fail(sd_menu != NULL, RET_FAIL);
    char** dirs = malloc(d_alloc * sizeof(char*));
    char** dirs_desc = malloc(d_alloc * sizeof(char*));
    return_val_if_fail(dirs != NULL, RET_FAIL);
    return_val_if_fail(dirs_desc != NULL, RET_FAIL);
    int z_size = 1;
    int z_alloc = 10;
    char** zips = malloc(z_alloc * sizeof(char*));
    char** zips_desc=malloc(z_alloc * sizeof(char*));
    return_val_if_fail(zips != NULL, RET_FAIL);
    return_val_if_fail(zips_desc != NULL, RET_FAIL);
    zips[0] = strdup("../");
    zips_desc[0]=strdup("../");

    while ((de = readdir(d)) != NULL) {
        int name_len = strlen(de->d_name);
        char de_path[SD_MAX_PATH];
        snprintf(de_path, SD_MAX_PATH, "%s/%s", path, de->d_name);
        struct stat st ;
        assert_if_fail(stat(de_path, &st) == 0);
        if (de->d_type == DT_DIR) {
            //skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' && 
                    de->d_name[1] == '.') continue;
            if (d_size >= d_alloc) {
                d_alloc *= 2;
                dirs = realloc(dirs, d_alloc * sizeof(char*));
                dirs_desc = realloc(dirs_desc, d_alloc * sizeof(char*));
            }
            dirs[d_size] = malloc(name_len + 2);
            dirs_desc[d_size] = malloc(64);
            strcpy(dirs[d_size], de->d_name);
            dirs[d_size][name_len] = '/';
            dirs[d_size][name_len + 1] = '\0';
            snprintf(dirs_desc[d_size], 64, "%s" ,ctime(&st.st_mtime));
            ++d_size;
        } else if (de->d_type == DT_REG && name_len >= 4 &&
                  strncasecmp(de->d_name + (name_len - 4), ".zip", 4) == 0) {
            if (z_size >= z_alloc) {
                z_alloc *= 2;
                zips = realloc(zips, z_alloc * sizeof(char*));
                zips_desc = realloc(zips_desc, z_alloc * sizeof(char*));
            }
            zips[z_size] = strdup(de->d_name);
            zips_desc[z_size] = malloc(64);
            snprintf(zips_desc[z_size], 64, "%s   %lldbytes" ,ctime(&st.st_mtime), st.st_size);
            z_size++;
        }
    }
    closedir(d);


    // append dirs to the zips list
    if (d_size + z_size + 1 > z_alloc) {
        z_alloc = d_size + z_size + 1;
        zips = realloc(zips, z_alloc * sizeof(char*));
        zips_desc = realloc(zips_desc, z_alloc * sizeof(char*));
    }
    memcpy(zips + z_size, dirs, d_size * sizeof(char *));
    memcpy(zips_desc + z_size, dirs_desc, d_size * sizeof(char*));
    free(dirs);
    z_size += d_size;
    zips[z_size] = NULL;
    zips_desc[z_size] = NULL;

   int result;
   int chosen_item = 0;
   do {
       chosen_item = miui_sdmenu(sd_menu->name, zips, zips_desc, z_size);
       return_val_if_fail(chosen_item >= 0, RET_FAIL);
       char * item = zips[chosen_item];
       return_val_if_fail(item != NULL, RET_FAIL);
       int item_len = strlen(item);
       if ( chosen_item == 0) {
           //go up but continue browsing
           result = -1;
           break;
       } else if (item[item_len - 1] == '/') {
           char new_path[SD_MAX_PATH];
           strlcpy(new_path, path, SD_MAX_PATH);
           strlcat(new_path, "/", SD_MAX_PATH);
           strlcat(new_path, item, SD_MAX_PATH);
           new_path[strlen(new_path) - 1] = '\0';
           result = _sd_dir_show(p, new_path);
           if (result > 0) break;
       } else {
           // select a zipfile
           // the status to the caller
           char new_path[SD_MAX_PATH];
           strlcpy(new_path, path, SD_MAX_PATH);
           strlcat(new_path, "/", SD_MAX_PATH);
           strlcat(new_path, item, SD_MAX_PATH);
           int wipe_cache = 0;
           //if third parameter is 1, echo sucess dialog
           if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
               miuiIntent_send(INTENT_INSTALL, 3, new_path, "0", "1"); 
           }
           result = -1;//back up layer
           break;
       }
   } while(1);

   int i;
   for (i = 0; i < z_size; ++i) 
   {
       free(zips[i]);
       free(zips_desc[i]);
   }
   free(zips);
   return result;
}
static STATUS sd_menu_show(menuUnit *p)
{
    //ensure_mounte sd path
    struct _intentResult* result = miuiIntent_send(INTENT_MOUNT, 1, "/sdcard");
    //whatever wether sdd is mounted, scan sdcard and go on
    //assert_if_fail(miuiIntent_result_get_int() == 0);
    int ret ;
    ret = _sd_dir_show(p, "/sdcard");
    if (ret == -1) return MENU_BACK;
    return ret;
}

static STATUS sd_update_show(menuUnit *p)
{
    char new_path[SD_MAX_PATH] = "/sdcard/update.zip";
    int wipe_cache = 0;
    if (RET_YES == miui_confirm(3, p->name, p->desc, p->icon)) {
        miuiIntent_send(INTENT_INSTALL, 3, new_path, "0", "1");
    }
    return MENU_BACK;
}
struct _menuUnit * sd_ui_init()
{
    struct _menuUnit *p = common_ui_init();
    return_null_if_fail(p != NULL);
    strncpy(p->name, "<~sd.name>", MENU_LEN);
    strncpy(p->title_name, "<~sd.title_name>", MENU_LEN);
    strncpy(p->icon, "@sd",  MENU_LEN);
    p->result = 0;
    sd_menu = p;
    return_null_if_fail(menuNode_init(p) != NULL);
    struct _menuUnit  *temp = common_ui_init();
    return_null_if_fail(temp != NULL);
    menuUnit_set_icon(temp, "@sd.choose");
    strncpy(temp->name, "<~sd.install.name>", MENU_LEN);
    temp->show = &sd_menu_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    temp = common_ui_init();
    menuUnit_set_icon(temp, "@sd.install");
    strncpy(temp->name,"<~sd.update.name>", MENU_LEN);
    temp->show = &sd_update_show;
    assert_if_fail(menuNode_add(p, temp) == RET_OK);
    return p;
}
