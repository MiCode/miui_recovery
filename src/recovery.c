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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  * See the License for the specific language governing permissions and * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "cutils/android_reboot.h"
#include "install.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"
#include "miui/src/miui.h"
#include "miui_intent.h"
#include "mtdutils/mounts.h"

#include "nandroid.h"
static const struct option OPTIONS[] = {
  { "send_intent", required_argument, NULL, 's' },
  { "update_package", required_argument, NULL, 'u' },
  { "wipe_data", no_argument, NULL, 'w' },
  { "wipe_cache", no_argument, NULL, 'c' },
  { "show_text", no_argument, NULL, 't' },
  { NULL, 0, NULL, 0 },
};

static const char *COMMAND_FILE = "/cache/recovery/command";
static const char *INTENT_FILE = "/cache/recovery/intent";
static const char *LOG_FILE = "/cache/recovery/log";
static const char *LAST_LOG_FILE = "/cache/recovery/last_log";
static const char *LAST_INSTALL_FILE = "/cache/recovery/last_install";
static const char *CACHE_ROOT = "/cache";
static const char *SDCARD_ROOT = "/sdcard";
static const char *TEMPORARY_LOG_FILE = "/tmp/miui_recovery.log";
static const char *TEMPORARY_INSTALL_FILE = "/tmp/last_install";
static const char *SIDELOAD_TEMP_DIR = "/tmp/sideload";


/*
 * The recovery tool communicates with the main system through /cache files.
 *   /cache/recovery/command - INPUT - command line for tool, one arg per line
 *   /cache/recovery/log - OUTPUT - combined log file from recovery run(s)
 *   /cache/recovery/intent - OUTPUT - intent that was passed in
 *
 * The arguments which may be supplied in the recovery.command file:
 *   --send_intent=anystring - write the text out to recovery.intent
 *   --update_package=path - verify install an OTA package file
 *   --wipe_data - erase user data (and cache), then reboot
 *   --wipe_cache - wipe cache (but not user data), then reboot
 *   --set_encrypted_filesystem=on|off - enables / diasables encrypted fs
 *
 * After completing, we remove /cache/recovery/command and reboot.
 * Arguments may also be supplied in the bootloader control block (BCB).
 * These important scenarios must be safely restartable at any point:
 *
 * FACTORY RESET
 * 1. user selects "factory reset"
 * 2. main system writes "--wipe_data" to /cache/recovery/command
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--wipe_data"
 *    -- after this, rebooting will restart the erase --
 * 5. erase_volume() reformats /data
 * 6. erase_volume() reformats /cache
 * 7. finish_recovery() erases BCB
 *    -- after this, rebooting will restart the main system --
 * 8. main() calls reboot() to boot main system
 *
 * OTA INSTALL
 * 1. main system downloads OTA package to /cache/some-filename.zip
 * 2. main system writes "--update_package=/cache/some-filename.zip"
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--update_package=..."
 *    -- after this, rebooting will attempt to reinstall the update --
 * 5. install_package() attempts to install the update
 *    NOTE: the package install must itself be restartable from any point
 * 6. finish_recovery() erases BCB
 *    -- after this, rebooting will (try to) restart the main system --
 * 7. ** if install failed **
 *    7a. prompt_and_wait() shows an error icon and waits for the user
 *    7b; the user reboots (pulling the battery, etc) into the main system
 * 8. main() calls maybe_install_firmware_update()
 *    ** if the update contained radio/hboot firmware **:
 *    8a. m_i_f_u() writes BCB with "boot-recovery" and "--wipe_cache"
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8b. m_i_f_u() writes firmware image into raw cache partition
 *    8c. m_i_f_u() writes BCB with "update-radio/hboot" and "--wipe_cache"
 *        -- after this, rebooting will attempt to reinstall firmware --
 *    8d. bootloader tries to flash firmware
 *    8e. bootloader writes BCB with "boot-recovery" (keeping "--wipe_cache")
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8f. erase_volume() reformats /cache
 *    8g. finish_recovery() erases BCB
 *        -- after this, rebooting will (try to) restart the main system --
 * 9. main() calls reboot() to boot main system
 */

static const int MAX_ARG_LENGTH = 4096;
static const int MAX_ARGS = 100;

// open a given path, mounting partitions as necessary
FILE*
fopen_path(const char *path, const char *mode) {
    if (ensure_path_mounted(path) != 0) {
        LOGE("Can't mount %s\n", path);
        return NULL;
    }

    // When writing, try to create the containing directory, if necessary.
    // Use generous permissions, the system (init.rc) will reset them.
    if (strchr("wa", mode[0])) dirCreateHierarchy(path, 0777, NULL, 1);

    FILE *fp = fopen(path, mode);
    return fp;
}

// close a file, log an error if the error indicator is set
static void
check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) LOGE("Error in %s\n(%s)\n", name, strerror(errno));
    fclose(fp);
}

// command line args come from, in decreasing precedence:
//   - the actual command line
//   - the bootloader control block (one per line, after "recovery")
//   - the contents of COMMAND_FILE (one per line)
static void
get_args(int *argc, char ***argv) {
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    get_bootloader_message(&boot);  // this may fail, leaving a zeroed structure

    if (boot.command[0] != 0 && boot.command[0] != 255) {
        LOGI("Boot command: %.*s\n", sizeof(boot.command), boot.command);
    }

    if (boot.status[0] != 0 && boot.status[0] != 255) {
        LOGI("Boot status: %.*s\n", sizeof(boot.status), boot.status);
    }

    // --- if arguments weren't supplied, look in the bootloader control block
    if (*argc <= 1) {
        boot.recovery[sizeof(boot.recovery) - 1] = '\0';  // Ensure termination
        const char *arg = strtok(boot.recovery, "\n");
        if (arg != NULL && !strcmp(arg, "recovery")) {
            *argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
            (*argv)[0] = strdup(arg);
            for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
                if ((arg = strtok(NULL, "\n")) == NULL) break;
                (*argv)[*argc] = strdup(arg);
            }
            LOGI("Got arguments from boot message\n");
        } else if (boot.recovery[0] != 0 && boot.recovery[0] != 255) {
            LOGE("Bad boot message\n\"%.20s\"\n", boot.recovery);
        }
    }

    // --- if that doesn't work, try the command file
    if (*argc <= 1) {
        FILE *fp = fopen_path(COMMAND_FILE, "r");
        if (fp != NULL) {
            char *argv0 = (*argv)[0];
            *argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
            (*argv)[0] = argv0;  // use the same program name

            char buf[MAX_ARG_LENGTH];
            for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
                if (!fgets(buf, sizeof(buf), fp)) break;
                (*argv)[*argc] = strdup(strtok(buf, "\r\n"));  // Strip newline.
            }

            check_and_fclose(fp, COMMAND_FILE);
            LOGI("Got arguments from %s\n", COMMAND_FILE);
        }
    }

    // --> write the arguments we have back into the bootloader control block
    // always boot into recovery after this (until finish_recovery() is called)
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
    int i;
    for (i = 1; i < *argc; ++i) {
        strlcat(boot.recovery, (*argv)[i], sizeof(boot.recovery));
        strlcat(boot.recovery, "\n", sizeof(boot.recovery));
    }
    set_bootloader_message(&boot);
}

static void
set_sdcard_update_bootloader_message() {
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
    set_bootloader_message(&boot);
}

// How much of the temp log we have copied to the copy in cache.
static long tmplog_offset = 0;

static void
copy_log_file(const char* source, const char* destination, int append) {
    FILE *log = fopen_path(destination, append ? "a" : "w");
    if (log == NULL) {
        LOGE("Can't open %s\n", destination);
    } else {
        FILE *tmplog = fopen(source, "r");
        if (tmplog != NULL) {
            if (append) {
                fseek(tmplog, tmplog_offset, SEEK_SET);  // Since last write
            }
            char buf[4096];
            while (fgets(buf, sizeof(buf), tmplog)) fputs(buf, log);
            if (append) {
                tmplog_offset = ftell(tmplog);
            }
            check_and_fclose(tmplog, source);
        }
        check_and_fclose(log, destination);
    }
}


// clear the recovery command and prepare to boot a (hopefully working) system,
// copy our log file to cache as well (for the system to read), and
// record any intent we were asked to communicate back to the system.
// this function is idempotent: call it as many times as you like.
static void
finish_recovery(const char *send_intent) {
    // By this point, we're ready to return to the main system...
    if (send_intent != NULL) {
        FILE *fp = fopen_path(INTENT_FILE, "w");
        if (fp == NULL) {
            LOGE("Can't open %s\n", INTENT_FILE);
        } else {
            fputs(send_intent, fp);
            check_and_fclose(fp, INTENT_FILE);
        }
    }

    // Copy logs to cache so the system can find out what happened.
    copy_log_file(TEMPORARY_LOG_FILE, LOG_FILE, true);
    copy_log_file(TEMPORARY_LOG_FILE, LAST_LOG_FILE, false);
    copy_log_file(TEMPORARY_INSTALL_FILE, LAST_INSTALL_FILE, false);
    chmod(LOG_FILE, 0600);
    chown(LOG_FILE, 1000, 1000);   // system user
    chmod(LAST_LOG_FILE, 0640);
    chmod(LAST_INSTALL_FILE, 0644);

    // Reset to mormal system boot so recovery won't cycle indefinitely.
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    set_bootloader_message(&boot);

    // Remove the command file, so recovery won't repeat indefinitely.
    if (ensure_path_mounted(COMMAND_FILE) != 0 ||
        (unlink(COMMAND_FILE) && errno != ENOENT)) {
        LOGW("Can't unlink %s\n", COMMAND_FILE);
    }

    ensure_path_unmounted(CACHE_ROOT);
    sync();  // For good measure.
}

static int
erase_volume(const char *volume) {
    ui_set_background(BACKGROUND_ICON_INSTALLING);
    ui_show_indeterminate_progress();
    ui_print("Formatting %s...\n", volume);

    ensure_path_unmounted(volume);

    if (strcmp(volume, "/cache") == 0) {
        // Any part of the log we'd copied to cache is now gone.
        // Reset the pointer so we copy from the beginning of the temp
        // log.
        tmplog_offset = 0;
    }

    return format_volume(volume);
}

static char*
copy_sideloaded_package(const char* original_path) {
  if (ensure_path_mounted(original_path) != 0) {
    LOGE("Can't mount %s\n", original_path);
    return NULL;
  }

  if (ensure_path_mounted(SIDELOAD_TEMP_DIR) != 0) {
    LOGE("Can't mount %s\n", SIDELOAD_TEMP_DIR);
    return NULL;
  }

  if (mkdir(SIDELOAD_TEMP_DIR, 0700) != 0) {
    if (errno != EEXIST) {
      LOGE("Can't mkdir %s (%s)\n", SIDELOAD_TEMP_DIR, strerror(errno));
      return NULL;
    }
  }

  // verify that SIDELOAD_TEMP_DIR is exactly what we expect: a
  // directory, owned by root, readable and writable only by root.
  struct stat st;
  if (stat(SIDELOAD_TEMP_DIR, &st) != 0) {
    LOGE("failed to stat %s (%s)\n", SIDELOAD_TEMP_DIR, strerror(errno));
    return NULL;
  }
  if (!S_ISDIR(st.st_mode)) {
    LOGE("%s isn't a directory\n", SIDELOAD_TEMP_DIR);
    return NULL;
  }
  if ((st.st_mode & 0777) != 0700) {
    LOGE("%s has perms %o\n", SIDELOAD_TEMP_DIR, st.st_mode);
    return NULL;
  }
  if (st.st_uid != 0) {
    LOGE("%s owned by %lu; not root\n", SIDELOAD_TEMP_DIR, st.st_uid);
    return NULL;
  }

  char copy_path[PATH_MAX];
  strcpy(copy_path, SIDELOAD_TEMP_DIR);
  strcat(copy_path, "/package.zip");

  char* buffer = malloc(BUFSIZ);
  if (buffer == NULL) {
    LOGE("Failed to allocate buffer\n");
    return NULL;
  }

  size_t read;
  FILE* fin = fopen(original_path, "rb");
  if (fin == NULL) {
    LOGE("Failed to open %s (%s)\n", original_path, strerror(errno));
    return NULL;
  }
  FILE* fout = fopen(copy_path, "wb");
  if (fout == NULL) {
    LOGE("Failed to open %s (%s)\n", copy_path, strerror(errno));
    return NULL;
  }

  while ((read = fread(buffer, 1, BUFSIZ, fin)) > 0) {
    if (fwrite(buffer, 1, read, fout) != read) {
      LOGE("Short write of %s (%s)\n", copy_path, strerror(errno));
      return NULL;
    }
  }

  free(buffer);

  if (fclose(fout) != 0) {
    LOGE("Failed to close %s (%s)\n", copy_path, strerror(errno));
    return NULL;
  }

  if (fclose(fin) != 0) {
    LOGE("Failed to close %s (%s)\n", original_path, strerror(errno));
    return NULL;
  }

  // "adb push" is happy to overwrite read-only files when it's
  // running as root, but we'll try anyway.
  if (chmod(copy_path, 0400) != 0) {
    LOGE("Failed to chmod %s (%s)\n", copy_path, strerror(errno));
    return NULL;
  }

  return strdup(copy_path);
}
/*
 * for register intent for ui send intent to some operation
 */
#define return_intent_result_if_fail(p) if (!(p)) \
    {miui_printf("function %s(line %d) " #p "\n", __FUNCTION__, __LINE__); \
    intent_result.ret = -1; return &intent_result;}
#define return_intent_ok(val, str) intent_result.ret = val; \
    if (str != NULL) \
        strncpy(intent_result.result, str, INTENT_RESULT_LEN); \
     else intent_result.result[0] = '\0'; \
     return &intent_result
//INTENT_MOUNT, mount recovery.fstab
static intentResult* intent_mount(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 1);
    return_intent_result_if_fail(argv != NULL);
    int result = ensure_path_mounted(argv[0]);
    if (result == 0)
        return miuiIntent_result_set(result, "mounted");
    return miuiIntent_result_set(result, "fail");
}

//INTENT_ISMOUNT, mount recovery.fstab
static intentResult* intent_ismount(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 1);
    return_intent_result_if_fail(argv != NULL);
    int result = is_path_mounted(argv[0]);
    return miuiIntent_result_set(result, NULL);
}
//INTENT_MOUNT, mount recovery.fstab
static intentResult* intent_unmount(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 1);
    return_intent_result_if_fail(argv != NULL);
    int result = ensure_path_unmounted(argv[0]);
    if (result == 0)
        return miuiIntent_result_set(result, "ok");
    return miuiIntent_result_set(result, "fail");
}
//INTENT_WIPE ,wipe "/data" | "cache" "dalvik-cache"
static intentResult* intent_wipe(int argc, char *argv[])
{
    return_intent_result_if_fail(argc == 1);
    return_intent_result_if_fail(argv != NULL);
    int result = 0;
    if (!strcmp(argv[0], "dalvik-cache"))
    {
        ensure_path_mounted("/data");
        ensure_path_mounted("/cache");
        __system("rm -r /data/dalvik-cache");
        __system("rm -r /cache/dalvik-cache");
        result = 0;
    }
    else
        result = erase_volume(argv[0]);
    assert_ui_if_fail(result == 0);
    return miuiIntent_result_set(result, "ok");
}
//INTENT_WIPE ,format "/data" | "/cache" | "/system" "/sdcard"
static intentResult* intent_format(int argc, char *argv[])
{
    return_intent_result_if_fail(argc == 1);
    return_intent_result_if_fail(argv != NULL);
    int result = format_volume(argv[0]);
    assert_ui_if_fail(result == 0);
    return miuiIntent_result_set(result, "ok");
}
//INTENT_REBOOT, reboot, 0, NULL | reboot, 1, "recovery" | bootloader |
static intentResult * intent_reboot(int argc, char *argv[])
{
    return_intent_result_if_fail(argc == 1);
    finish_recovery(NULL);
    if(strstr(argv[0], "reboot") != NULL)
        android_reboot(ANDROID_RB_RESTART, 0, 0);
    else if(strstr(argv[0], "poweroff") != NULL)
        android_reboot(ANDROID_RB_POWEROFF, 0, 0);
    else android_reboot(ANDROID_RB_RESTART2, 0, argv[0]);
    return miuiIntent_result_set(0, NULL);
}
//INTENT_INSTALL install path, wipe_cache, install_file
static intentResult* intent_install(int argc, char *argv[])
{
    return_intent_result_if_fail(argc == 3);
    return_intent_result_if_fail(argv != NULL);
    int wipe_cache = atoi(argv[1]);
    int echo = atoi(argv[2]);
    miuiInstall_init(&install_package, argv[0], wipe_cache, TEMPORARY_INSTALL_FILE);
    miui_install(echo);
    //echo install failed
    return miuiIntent_result_set(RET_OK, NULL);
}
/*package nandroid_restore(const char* backup_path, restore_boot, restore_system, restore_data,
 *                         restore_cache, restore_sdext, restore_wimax);
 *INTENT_RESTORE
 *intent_restore(argc, argv[0],...,argv[6])
 */
static intentResult* intent_restore(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 7);
    return_intent_result_if_fail(argv != NULL);
    int result = nandroid_restore(argv[0], atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                                  atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    assert_ui_if_fail(result == 0);
    return miuiIntent_result_set(result, NULL);
}
/*
 *nandroid_backup(backup_path);
 *
 *INTENT_BACKUP 
 *intent_backup(argc, NULL);
 */
static intentResult* intent_backup(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 1);
    int result ;
    result = nandroid_backup(argv[0]);
    assert_ui_if_fail(result == 0);
    return miuiIntent_result_set(result, NULL);
}
static intentResult* intent_advanced_backup(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 2);
    return_intent_result_if_fail(argv != NULL);
    int result = nandroid_advanced_backup(argv[0], argv[1]);
    assert_ui_if_fail(result == 0);
    return miuiIntent_result_set(result, NULL);
}

static intentResult* intent_system(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 1);
    return_intent_result_if_fail(argv != NULL);
    int result = __system(argv[0]);
    assert_if_fail(result == 0);
    return miuiIntent_result_set(result, NULL);
}
//copy_log_file(src_file, dst_file, append);
static intentResult* intent_copy(int argc, char* argv[])
{
    return_intent_result_if_fail(argc == 2);
    return_intent_result_if_fail(argv != NULL);
    copy_log_file(argv[0], argv[1], false);
    return miuiIntent_result_set(0, NULL);
}
static void
print_property(const char *key, const char *name, void *cookie) {
    printf("%s=%s\n", key, name);
}

int
main(int argc, char **argv) {
    time_t start = time(NULL);

    // If these fail, there's not really anywhere to complain...
#ifndef DEBUG
    unlink(TEMPORARY_LOG_FILE);
#endif

    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);
    printf("Starting recovery on %s", ctime(&start));
    //miuiIntent init
    miuiIntent_init(10);
    miuiIntent_register(INTENT_MOUNT, &intent_mount);
    miuiIntent_register(INTENT_ISMOUNT, &intent_ismount);
    miuiIntent_register(INTENT_UNMOUNT, &intent_unmount);
    miuiIntent_register(INTENT_REBOOT, &intent_reboot);
    miuiIntent_register(INTENT_INSTALL, &intent_install);
    miuiIntent_register(INTENT_WIPE, &intent_wipe);
    miuiIntent_register(INTENT_TOGGLE, &intent_toggle);
    miuiIntent_register(INTENT_FORMAT, &intent_format);
    miuiIntent_register(INTENT_RESTORE, &intent_restore);
    miuiIntent_register(INTENT_BACKUP, &intent_backup);
    miuiIntent_register(INTENT_ADVANCED_BACKUP, &intent_advanced_backup);
    miuiIntent_register(INTENT_SYSTEM, &intent_system);
    miuiIntent_register(INTENT_COPY, &intent_copy);
    device_ui_init();
    load_volume_table();
    get_args(&argc, &argv);

    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    set_bootloader_message(&boot);

    int previous_runs = 0;
    const char *send_intent = NULL;
    const char *update_package = NULL;
    int wipe_data = 0, wipe_cache = 0;

    int arg;
    while ((arg = getopt_long(argc, argv, "", OPTIONS, NULL)) != -1) {
        switch (arg) {
        case 'p': previous_runs = atoi(optarg); break;
        case 's': send_intent = optarg; break;
        case 'u': update_package = optarg; break;
        case 'w': wipe_data = wipe_cache = 1; break;
        case 'c': wipe_cache = 1; break;
        //case 't': ui_show_text(1); break;
        case '?':
            LOGE("Invalid command argument\n");
            continue;
        }
    }

    device_recovery_start();

    printf("Command:");
    for (arg = 0; arg < argc; arg++) {
        printf(" \"%s\"", argv[arg]);
    }
    printf("\n");

    if (update_package) {
        // For backwards compatibility on the cache partition only, if
        // we're given an old 'root' path "CACHE:foo", change it to
        // "/cache/foo".
        if (strncmp(update_package, "CACHE:", 6) == 0) {
            int len = strlen(update_package) + 10;
            char* modified_path = malloc(len);
            strlcpy(modified_path, "/cache/", len);
            strlcat(modified_path, update_package+6, len);
            printf("(replacing path \"%s\" with \"%s\")\n",
                   update_package, modified_path);
            update_package = modified_path;
        }
    }
    printf("\n");

    property_list(print_property, NULL);
    printf("\n");

    int status = INSTALL_SUCCESS;

    if (update_package != NULL) {
        if (wipe_cache) erase_volume("/cache");
        miuiIntent_send(INTENT_INSTALL, 3, update_package,"0", "0");
        //if echo 0 ,don't print success dialog 
        status = miuiIntent_result_get_int();
        if (status != INSTALL_SUCCESS) ui_print("Installation aborted.\n");
    } else if (wipe_data) {
        if (device_wipe_data()) status = INSTALL_ERROR;
        if (erase_volume("/data")) status = INSTALL_ERROR;
        if (wipe_cache && erase_volume("/cache")) status = INSTALL_ERROR;
        if (status != INSTALL_SUCCESS) ui_print("Data wipe failed.\n");
    } else if (wipe_cache) {
        if (wipe_cache && erase_volume("/cache")) status = INSTALL_ERROR;
        if (status != INSTALL_SUCCESS) ui_print("Cache wipe failed.\n");
    } else {
        status = INSTALL_ERROR;  // No command specified
    }
    if (status != INSTALL_SUCCESS) device_main_ui_show();//show menu
    device_main_ui_release();
    // Otherwise, get ready to boot the main system...
    finish_recovery(send_intent);
    ui_print("Rebooting...\n");
    android_reboot(ANDROID_RB_RESTART, 0, 0);
    return EXIT_SUCCESS;
}
