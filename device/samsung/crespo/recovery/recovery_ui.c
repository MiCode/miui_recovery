/*
 * Copyright (C) 2010 The Android Open Source Project
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
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "recovery_ui.h"
#include "common.h"

char* MENU_HEADERS[] = { "Volume up/down to move highlight;",
                         "power button to select.",
                         "",
                         NULL };

char* MENU_ITEMS[] = { "reboot system now",
                       "apply update from /sdcard",
                       "wipe data/factory reset",
                       "wipe cache partition",
                       NULL };

void device_ui_init(UIParameters* ui_parameters) {
}

int device_recovery_start() {
    // recovery can get started before the kernel has created the EMMC
    // devices, which will make the wipe_data operation fail (trying
    // to open a device that doesn't exist).  Hold up the start of
    // recovery for up to 5 seconds waiting for the userdata partition
    // block device to exist.

    const char* fn = "/dev/block/platform/s3c-sdhci.0/by-name/userdata";

    int tries = 0;
    int ret;
    struct stat buf;
    do {
        ++tries;
        ret = stat(fn, &buf);
        if (ret) {
            printf("try %d: %s\n", tries, strerror(errno));
            sleep(1);
        }
    } while (ret && tries < 5);
    if (!ret) {
        printf("stat() of %s succeeded on try %d\n", fn, tries);
    } else {
        printf("failed to stat %s\n", fn);
    }

    // We let recovery attempt to carry on even if the stat never
    // succeeded.

    return 0;
}

int device_toggle_display(volatile char* key_pressed, int key_code) {
    // hold power and press volume-up
    return key_pressed[KEY_POWER] && key_code == KEY_VOLUMEUP;
}

int device_reboot_now(volatile char* key_pressed, int key_code) {
    // Reboot if the power key is pressed five times in a row, with
    // no other keys in between.
    static int presses = 0;
    if (key_code == KEY_POWER) {   // power button
        ++presses;
        return presses == 5;
    } else {
        presses = 0;
        return 0;
    }
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
            case KEY_POWER:   // crespo power
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
