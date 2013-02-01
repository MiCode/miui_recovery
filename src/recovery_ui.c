/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <linux/input.h>

#include "recovery_ui.h"
#include "common.h"
#include "miui/src/miui.h"
//init ui
void device_ui_init() {
    main_ui_init();
}
//start ui?
int device_recovery_start() {
    return 0;
}

int device_main_ui_show()
{
    return main_ui_show();
}

int device_main_ui_release()
{
    return main_ui_release();
}

int device_toggle_display(volatile char* key_pressed, int key_code) {
    return key_code == KEY_HOME;
}

int device_reboot_now(volatile char* key_pressed, int key_code) {
    return 0;
}

int device_handle_key(int key_code, int visible) {
    if (visible) {
        switch (key_code) {
            case KEY_DOWN:
            case KEY_VOLUMEDOWN:
                return HIGHLIGHT_DOWN;

            case KEY_UP:
            case KEY_VOLUMEUP:
                return HIGHLIGHT_UP;

            case KEY_ENTER:
                return SELECT_ITEM;
        }
    }

    return NO_ACTION;
}

int device_perform_action(int which) {
    return which;
}

int device_wipe_data() {
    return 0;
}

void ui_set_background(int icon)
{
    //TODO
}

// Show a progress bar and define the scope of the next operation:
//   portion - fraction of the progress bar the next operation will use
//   seconds - expected time interval (progress bar moves at this minimum rate)
void ui_show_progress(float portion, int seconds)
{
    //TODO
    miuiInstall_show_progress(portion, seconds);
}
void ui_set_progress(float fraction) // 0.0 - 1.0 within the defined scope
{
    miuiInstall_set_progress(fraction); // 0.0 - 1.0 within the defined scope
    //TODO
}
void ui_set_text(char *str)
{
    miuiInstall_set_text(str);
}
// Show a rotating "barberpole" for ongoing operations.  Updates automatically.
void ui_show_indeterminate_progress()
{
}

// Hide and reset the progress bar.
void ui_reset_progress()
{
}
