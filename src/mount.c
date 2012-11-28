/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"
#include "cutils/properties.h"
#include "roots.h"
#include "miui/src/miui.h"
#include "miui_intent.h"


#ifndef BOARD_USB_CONFIG_FILE
/*
 * Available state: DISCONNECTED, CONFIGURED, CONNECTED
 */
#define BOARD_USB_CONFIG_FILE "/sys/class/android_usb/android0/state"
#endif

static int is_usb_connected() {
    char state[255];
    int fd = open(BOARD_USB_CONFIG_FILE, O_RDONLY);

    if (fd < 0) {
        LOGE("Unable to open usb_configuration state file(%s)\n",
            strerror(errno));
        return 0;
    }

    if (read(fd, state, sizeof(state)) < 0) {
        LOGE("Unable to read usb_configuration state file(%s)\n",
            strerror(errno));
        close(fd);
        return 0;
    }
    state[254] = '\0';
    LOGE("%s: state=%s\n", __func__, state);

    close(fd);
    return state[0] == 'C';
}

static int mount_usb() {
    int ret = 0;
    int fd;
    char value[PROPERTY_VALUE_MAX];
    Volume *vol = volume_for_path("/sdcard");

    property_get("sys.usb.state", value, "");
    value[PROPERTY_VALUE_MAX - 1] = '\0';
    LOGE("%s: sys.usb.state=%s\n", __func__, value);
    if (strncmp("mass_storage,adb", value, 16))
       property_set("sys.usb.config", "mass_storage,adb");

    if ((fd = open(acfg()->lun_file, O_WRONLY)) < 0) {
        LOGE("Unable to open ums lunfile (%s)", strerror(errno));
        ret = -1;
        goto out;
    }
    if ((write(fd, vol->device, strlen(vol->device)) < 0) &&
            (!vol->device2 || (write(fd, vol->device, strlen(vol->device2)) < 0))) {
        LOGE("Unable to write to ums lunfile (%s)", strerror(errno));
        ret = -1;
    }

    close(fd);
out:
    return ret;
}

static int umount_usb() {
    int ret;
    int fd;
    char ch = 0;
    char value[PROPERTY_VALUE_MAX];

    if ((fd = open(acfg()->lun_file, O_WRONLY)) < 0) {
        LOGE("Unable to open ums lunfile (%s)", strerror(errno));
        ret = -1;
    goto out;
    }

    if (write(fd, &ch, 1) < 0) {
        LOGE("Unable to write to ums lunfile (%s)", strerror(errno));
        ret = -1;
    }
    close(fd);

out:
    property_get("sys.usb.state", value, "");
    value[PROPERTY_VALUE_MAX - 1] = '\0';
    LOGE("%s: sys.usb.state=%s\n", __func__, value);
    if (strncmp("adb", value, 3))
        property_set("sys.usb.config", "adb");

    return ret;
}

//INTENT_TOGGLE toggle usb
intentResult* intent_toggle(int argc, char *argv[])
{

    assert_ui_if_fail(argc == 1);
    assert_ui_if_fail(argv[0] != NULL);
    int intent_type = atoi(argv[0]);
    int result = 0;
    if (intent_type == 0)
    {
        umount_usb();
        ensure_path_unmounted("/sdcard");
        return miuiIntent_result_set(result, "ok");
    }
    //wait for usb connected
    //while (is_usb_connected()) ;
    if (is_usb_connected())
    {
        mount_usb();
        return miuiIntent_result_set(result, "mounted");
    }
    LOGE("USB not connect\n");
    umount_usb();
    ensure_path_unmounted("/sdcard");
    return miuiIntent_result_set(result, "ok");
}


