/*
 * Copyright (C) 2011 The Android Open Source Project
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
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cryptfs.h>

#include "edify/expr.h"
#include "bootloader.h"

Value* WriteBootloaderFn(const char* name, State* state, int argc, Expr* argv[])
{
    int result = -1;
    Value* img;
    Value* xloader_loc;
    Value* sbl_loc;

    if (argc != 3) {
        return ErrorAbort(state, "%s() expects 3 args, got %d", name, argc);
    }

    if (ReadValueArgs(state, argv, 3, &img, &xloader_loc, &sbl_loc) < 0) {
        return NULL;
    }

    if(img->type != VAL_BLOB ||
       xloader_loc->type != VAL_STRING ||
       sbl_loc->type != VAL_STRING) {
      FreeValue(img);
      FreeValue(xloader_loc);
      FreeValue(sbl_loc);
      return ErrorAbort(state, "%s(): argument types are incorrect", name);
    }

    result = update_bootloader(img->data, img->size,
                               xloader_loc->data, sbl_loc->data);
    FreeValue(img);
    FreeValue(xloader_loc);
    FreeValue(sbl_loc);
    return StringValue(strdup(result == 0 ? "t" : ""));
}

/*
 * The size of the userdata partition for HSPA Galaxy Nexus devices is incorrect
 * in the partition as it comes from the factory.  Updating the bootloader fixes
 * the partition table, and makes the size of the userdata partition 1 sector
 * smaller.  However, if the user had encrypted their device with the original
 * incorrect size of the partition table, the crypto footer has saved that
 * size, and tries to map that much data when decrypting.  However, with the
 * new partition table, that size is too big to be mapped, and the kernel
 * throws an error, and the user can't decrypt and boot the device after the
 * OTA is installed.  Oops!
 *
 * The fix here is to recognize a crypto footer that has the wrong size, and
 * update it to the new correct size.  This program should be run as part of
 * the recovery script for HSPA Galaxy Nexus devices.
 */

#define BAD_SIZE  0x01b14fdfULL
#define GOOD_SIZE 0x01b14fdeULL

#define HSPA_PRIME_KEY_PARTITION "/dev/block/platform/omap/omap_hsmmc.0/by-name/metadata"

Value* FsSizeFixFn(const char* name, State* state, int argc, Expr* argv[])
{
    struct crypt_mnt_ftr ftr;
    int fd;

    if (argc != 0) {
        return ErrorAbort(state, "%s() expects 0 args, got %d", name, argc);
    }

    if ((fd = open(HSPA_PRIME_KEY_PARTITION, O_RDWR)) == -1) {
        return ErrorAbort(state, "%s() Cannot open %s\n", name, HSPA_PRIME_KEY_PARTITION);
    }

    if (read(fd, &ftr, sizeof(ftr)) != sizeof(ftr)) {
        close(fd);
        return ErrorAbort(state, "%s() Cannot read crypto footer %s\n", name, HSPA_PRIME_KEY_PARTITION);
    }

   if ((ftr.magic == CRYPT_MNT_MAGIC) && (ftr.fs_size == BAD_SIZE)) {
        ftr.fs_size = GOOD_SIZE;
        if (lseek(fd, 0, SEEK_SET) == 0) {
            if (write(fd, &ftr, sizeof(ftr)) == sizeof(ftr)) {
                fsync(fd); /* Make sure it gets to the disk */
                fprintf(stderr, "Footer updated\n");
                close(fd);
                return StringValue(strdup("t"));
            }
        }
        close(fd);
        return ErrorAbort(state, "%s() Cannot seek or write crypto footer %s\n", name, HSPA_PRIME_KEY_PARTITION);
    }

    /* Nothing to do */
    fprintf(stderr, "Footer doesn't need updating\n");
    close(fd);
    return StringValue(strdup("t"));
}

void Register_librecovery_updater_tuna() {
    fprintf(stderr, "installing samsung updater extensions\n");

    RegisterFunction("samsung.write_bootloader", WriteBootloaderFn);
    RegisterFunction("samsung.fs_size_fix", FsSizeFixFn);
}
