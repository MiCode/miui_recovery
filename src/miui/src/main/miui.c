/*
 * Copyright (C) 2011 xiaomi MIUI ( http://xiaomi.com/ )
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
 * Descriptions:
 * -------------
 * Main executable for MIUI Installer Binary
 *
 */
#include "../miui_inter.h"
#include "../miui.h"


struct _menuUnit *g_main_menu;//main menu
struct _menuUnit *g_root_menu;//language ui

static STATUS main_ui_clear(struct _menuUnit *p)
{
    //release tree, post order release
    if (p == NULL)
        return RET_OK;
    main_ui_clear(p->child);
    main_ui_clear(p->nextSilbing);
    free(p);
    return RET_OK;
}

static STATUS main_ui_clear_root()
{
    return main_ui_clear(g_root_menu);
}
static struct _menuUnit *tree_init()
{
    miui_debug("tree_init entery\n");
    g_root_menu = lang_ui_init();
    return_null_if_fail(g_root_menu != NULL);
    //main menu
    g_main_menu = common_ui_init();
    return_null_if_fail(g_main_menu != NULL);
    strncpy(g_main_menu->name, "<~mainmenu.name>", MENU_LEN);
    menuUnit_set_show(g_main_menu, &common_ui_show);
    g_root_menu->child = g_main_menu;
    g_main_menu->parent = g_root_menu;
    //add back operation
    g_main_menu = menuNode_init(g_main_menu);
    //inital mainmenu 
    //cancel reboot
    //assert_if_fail(menuNode_add(g_main_menu, reboot_ui_init()) == RET_OK);
    //add sd operation 
    assert_if_fail(menuNode_add(g_main_menu, sd_ui_init()) == RET_OK);
    //add wipe
    assert_if_fail(menuNode_add(g_main_menu, wipe_ui_init()) == RET_OK);
    //add mount and toggle usb storage
    assert_if_fail(menuNode_add(g_main_menu, mount_ui_init()) == RET_OK);
    //add backup
    assert_if_fail(menuNode_add(g_main_menu, backup_ui_init()) == RET_OK);
    //add power
    assert_if_fail(menuNode_add(g_main_menu, power_ui_init()) == RET_OK);
    //add tools operation
    assert_if_fail(menuNode_add(g_main_menu, tool_ui_init()) == RET_OK);
    //add info
    assert_if_fail(menuNode_add(g_main_menu, info_ui_init()) == RET_OK);

    struct stat st;
    if (stat(RECOVERY_PATH, &st) != 0)
    {
        mkdir(RECOVERY_PATH, 0755);
    }
    return g_root_menu;
}
STATUS main_ui_init()
{
#ifndef _MIUI_NODEBUG
    miui_debug("function main_ui_init enter miui debug\n");
    remove_directory("/tmp/miui-memory");
    miui_memory_debug_init();
#endif
    miui_printf("Initializing...\n");
    remove_directory(MIUI_TMP);
    unlink(MIUI_TMP_S);
    create_directory(MIUI_TMP);
    symlink(MIUI_TMP, MIUI_TMP_S);

    //miui  config init
    miui_ui_init();
    //read config file and execute it
    miui_ui_config("/res/init.conf");
    //input thread start
    ui_init();
    //graphic thread start, print background
    ag_init();

    //miui_ui start
    miui_ui_start();
	//device config after miui_ui start
    miui_ui_config("/res/device.conf");
    tree_init();


    miui_font( "0", "ttf/DroidSans.ttf;ttf/DroidSansFallback.ttf;", "12" );
    miui_font( "1", "ttf/DroidSans.ttf;ttf/DroidSansFallback.ttf;", "18" );
    return RET_OK;
}
STATUS main_ui_show()
{
    struct _menuUnit *node_show = g_root_menu;
    int index = 0;
    //show mainmenu

    while (index != MENU_QUIT)
    {
        return_val_if_fail(node_show != NULL, RET_FAIL);
        return_val_if_fail(node_show->show != NULL, RET_FAIL);
        miui_set_isbgredraw(1);
        index = node_show->show(node_show);
        if (index > 0 && index < MENU_BACK) {
            node_show = node_show->get_child_by_index(node_show, index);
        }
        else if (index == MENU_BACK || index == 0 )
        {
            if (node_show->parent != NULL)
                node_show = node_show->parent;
        }
        else {
            //TODO add MENU QUIT or some operation?
            miui_error("invalid index %d in %s\n", index, __FUNCTION__);
        }
    }
    return RET_FAIL;
}

STATUS main_ui_release()
{

#ifndef _MIUI_NODEBUG
  miui_dump_malloc();
#endif
  miui_ui_end();
  ag_close_thread();
  //clear ui tree
  main_ui_clear_root(); 
  //-- Release All
  ag_closefonts();  //-- Release Fonts
  miui_debug("Font Released\n");
  ev_exit();        //-- Release Input Engine
  miui_debug("Input Released\n");
  ag_close();       //-- Release Graph Engine
  miui_debug("Graph Released\n");

  miui_debug("Cleanup Temporary\n");
  usleep(500000);
  unlink(MIUI_TMP_S);
  remove_directory(MIUI_TMP);
  return RET_OK;
}

