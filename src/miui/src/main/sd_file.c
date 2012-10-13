/*
 * Copyright (C) 2012 xiaomi MIUI ( http://www.xiaomi.com/ )
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *Author:dennise 
 *Date:2012-10-12
 *Descriptions;
 *    scan file system and show in screen, invoke callback function when touch file
 *  
 *
 *
 */
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "../miui_inter.h"
#include "../miui.h"

//invoke fileFun after touch the file.
//if return value from callback funciton is not equal 0, continue, others , back to up layer
//just support single thread
static char *g_title_name = NULL;
//callback function 
static fileFun g_fun = NULL;
static fileFilterFun g_file_filter_fun= NULL;
static void *g_data = NULL;
pthread_mutex_t g_file_scan_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sd_file_dump_array(char **zips, char **zips_desc, int z_size)
{
    miui_debug("*************%s start******************\n", __FUNCTION__);
    int i = 0;
    for (i = 0; i < z_size; i++)
    {
        miui_debug("[%d]%s--%s\n", i, zips[i], zips_desc[i]);
    }
    miui_debug("*************%s end********************\n", __FUNCTION__);
}

static STATUS _file_scan(char *path, int path_len)
{
    return_val_if_fail(path != NULL, RET_FAIL);
    return_val_if_fail(strlen(path) <= path_len, RET_INVALID_ARG);
    DIR* d = NULL;
    struct dirent* de = NULL;
    int i = 0;
    int result = 0;
    d = opendir(path);
    return_val_if_fail(d != NULL, RET_FAIL);

    int d_size = 0;
    int d_alloc = 10;
    char** dirs = (char **)malloc(d_alloc * sizeof(char*));
    char** dirs_desc = (char **)malloc(d_alloc * sizeof(char*));
    return_val_if_fail(dirs != NULL, RET_FAIL);
    return_val_if_fail(dirs_desc != NULL, RET_FAIL);
    int z_size = 1;
    int z_alloc = 10;
    char** zips = (char **)malloc(z_alloc * sizeof(char*));
    char** zips_desc=(char **)malloc(z_alloc * sizeof(char*));
    return_val_if_fail(zips != NULL, RET_FAIL);
    return_val_if_fail(zips_desc != NULL, RET_FAIL);
    zips[0] = strdup("../");
    zips_desc[0]=strdup("../");

    while ((de = readdir(d)) != NULL) {
        int name_len = strlen(de->d_name);
        if (name_len <= 0) continue;
        char de_path[PATH_MAX];
        snprintf(de_path, PATH_MAX, "%s/%s", path, de->d_name);
        struct stat st ;
        assert_if_fail(stat(de_path, &st) == 0);
        if (de->d_type == DT_DIR) {
            //skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' && 
                    de->d_name[1] == '.') continue;
            if (d_size >= d_alloc) {
                d_alloc *= 2;
                dirs = (char **)realloc(dirs, d_alloc * sizeof(char*));
                assert_if_fail(dirs != NULL);
                dirs_desc = (char **)realloc(dirs_desc, d_alloc * sizeof(char*));
                assert_if_fail(dirs_desc != NULL);
            }
            dirs[d_size] = (char *)malloc(name_len + 2);
            assert_if_fail(dirs[d_size] != NULL);
            dirs_desc[d_size] = (char *)malloc(64);
            assert_if_fail(dirs_desc[d_size] != NULL);
            strncpy(dirs[d_size], de->d_name, name_len);
            dirs[d_size][name_len] = '/';
            dirs[d_size][name_len + 1] = '\0';
            snprintf(dirs_desc[d_size], 63, "%s" ,ctime(&st.st_mtime));
            dirs_desc[d_size][63] = '\0';
            ++d_size;
        } else if (de->d_type == DT_REG) {
            if (g_file_filter_fun == NULL || g_file_filter_fun(de->d_name, name_len) == 0)
            {
                if (z_size >= z_alloc) {
                    z_alloc *= 2;
                    zips = (char **) realloc(zips, z_alloc * sizeof(char*));
                    assert_if_fail(zips != NULL);
                    zips_desc = (char **) realloc(zips_desc, z_alloc * sizeof(char*));
                    assert_if_fail(zips_desc != NULL);
                }
                zips[z_size] = strdup(de->d_name);
                assert_if_fail(zips[z_size] != NULL);
                zips_desc[z_size] = (char*)malloc(64);
                assert_if_fail(zips_desc[z_size] != NULL);
                snprintf(zips_desc[z_size], 63, "%s   %lldbytes" ,ctime(&st.st_mtime), st.st_size);
                zips_desc[z_size][63] = '\0';
                z_size++;
            }
        }
    }
    closedir(d);


    // append dirs to the zips list
    if (d_size + z_size + 1 > z_alloc) {
        z_alloc = d_size + z_size + 1;
        zips = (char **)realloc(zips, z_alloc * sizeof(char*));
        assert_if_fail(zips != NULL);
        zips_desc = (char **)realloc(zips_desc, z_alloc * sizeof(char*));
        assert_if_fail(zips_desc != NULL);
    }
    for (i = 0; i < d_size; i++)
    {
        zips[z_size + i] = dirs[i];
        zips_desc[z_size + i] = dirs_desc[i];
    }
    free(dirs);
    z_size += d_size;
    zips[z_size] = NULL;
    zips_desc[z_size] = NULL;

   int chosen_item = 0;
   do {
      
       if (NULL == g_title_name) 
       {
           miui_error("g_title_name is NULL \n");
           result = -1;
           goto finish_done;
       }
#if DEBUG
       sd_file_dump_array(zips, zips_desc, z_size);
#endif
       chosen_item = miui_sdmenu(g_title_name, zips, zips_desc, z_size);
       return_val_if_fail(chosen_item >= 0, RET_FAIL);
       char * item = zips[chosen_item];
       return_val_if_fail(item != NULL, RET_FAIL);
       int item_len = strlen(item);
       if ( chosen_item == 0) {
           //go up but continue browsing
           result = -1;
           break;
       } else if (item[item_len - 1] == '/') {
           char new_path[PATH_MAX];
           strlcpy(new_path, path, PATH_MAX);
           strlcat(new_path, "/", PATH_MAX);
           strlcat(new_path, item, PATH_MAX);
           new_path[strlen(new_path) - 1] = '\0';
           result = _file_scan(new_path, PATH_MAX);
           if (result > 0) break;
       } else {
           // select a zipfile
           // the status to the caller
           char new_path[PATH_MAX];
           strlcpy(new_path, path, PATH_MAX);
           strlcat(new_path, "/", PATH_MAX);
           strlcat(new_path, item, PATH_MAX);
           int wipe_cache = 0;
           //if third parameter is 1, echo sucess dialog
           if (NULL == g_fun) 
           {
               miui_error("g_fun is NULL in fun\n");
               result = -1;
               goto finish_done;
           }
           if (0 == g_fun(new_path, PATH_MAX, (void *)g_data))//execute callback fun success
           {
               //back to up layer
               result = -1;
           }
           else { 
             miui_error("g_fun execute fail\n");
             result = 0;
           }
           break;
       }
   } while(1);

finish_done:

   for (i = 0; i < z_size; ++i)
   {
       free(zips[i]);
       free(zips_desc[i]);
   }
   free(zips);
   return result;
}

//add data obejcet, optional argument in fileFun
STATUS file_scan(char *path, int path_len, char * title, int title_len, fileFun fun, void* data, fileFilterFun filter_fun)
{
    return_val_if_fail(path != NULL, RET_FAIL);
    return_val_if_fail(strlen(path) <= path_len, RET_INVALID_ARG);
    return_val_if_fail(title != NULL, RET_FAIL);
    return_val_if_fail(strlen(title) <= title_len, RET_INVALID_ARG);
    pthread_mutex_lock(&g_file_scan_mutex);
    g_title_name = title;
    g_fun = fun;
    g_data = data;
    g_file_filter_fun = filter_fun;
    pthread_mutex_unlock(&g_file_scan_mutex);
    return _file_scan(path, path_len);
}
