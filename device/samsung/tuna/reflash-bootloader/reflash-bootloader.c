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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/stat.h>

#define error(s, a...)                          \
    {                                           \
        printf("error: " s "\n", ##a);          \
        exit(-1);                               \
    }

#define error_errno(s, a...) error(s ": %s", ##a, strerror(errno))

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

enum omap_type_enum {
    OMAP4460_EMU = 0,
    OMAP4460_HS,
    OMAP4460_HS_PROD,
    OMAP4430_HS,
};

struct omap_type {
    const char *family;
    const char *type;
    unsigned long msv_val;
    const char *msv_type;
    off_t offset;
} omap_type_list[] = {
    [OMAP4460_EMU]     = {"OMAP4460", "EMU", 0x00000000, "eng",  0x1000},
    [OMAP4460_HS]      = {"OMAP4460", "HS",  0x00000000, "eng",  0x21000},
    [OMAP4460_HS_PROD] = {"OMAP4460", "HS",  0xf0000f00, "prod", 0x41000},
    [OMAP4430_HS]      = {"OMAP4430", "HS",  0x00000000, "eng",  0x61000},
};

#define IMG_PIT_OFFSET 0UL
#define IMG_SBL_OFFSET 0x81000UL

#define MMC_PIT_OFFSET 0x4400UL
#define MMC_XLOADER_OFFSET 0x20000UL
#define MMC_SBL_OFFSET 0x80000UL

#define PIT_SIZE 0x1000UL
#define XLOADER_SIZE 0x20000UL

static void drop_caches(void)
{
    int fd;
    int ret;
    char buf[] = "3\n";

    fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
    if (fd < 0)
        error_errno("failed to open /proc/sys/vm/drop_caches");

    ret = write(fd, buf, sizeof(buf));
    if (ret < 0)
        error_errno("failed to write to /proc/sys/vm/drop_caches");
}

static void read_file(const char *filename, char *buf, size_t size)
{
    int fd;
    ssize_t ret;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        error_errno("failed to open %s", filename);

    ret = read(fd, buf, size - 1);
    if (ret < 0)
        error_errno("failed to read %s", filename);
    buf[ret] = 0;
    while (buf[ret - 1] == '\n')
        buf[--ret] = 0;

    close(fd);
}

static const struct omap_type *get_omap_type(void)
{
    int fd;
    char family[10];
    char type[5];
    char msv[9];
    unsigned long msv_val;
    ssize_t ret;
    unsigned int i;

    read_file("/sys/board_properties/soc/type", type, sizeof(type));
    read_file("/sys/board_properties/soc/family", family, sizeof(family));
    read_file("/sys/board_properties/soc/msv", msv, sizeof(msv));

    msv_val = strtoul(msv, NULL, 16);

    for (i = 0; i < ARRAY_SIZE(omap_type_list); i++)
        if ((strcmp(omap_type_list[i].family, family) == 0) &&
            (strcmp(omap_type_list[i].type, type) == 0) &&
            msv_val == omap_type_list[i].msv_val)
            return &omap_type_list[i];

    error("unknown omap type %s %s %s (0x%08lx)", family, type, msv, msv_val);
}

static void zero_data(int to_fd, off_t to_offset, ssize_t size)
{
    char buf[4096];
    int ret;
    unsigned int to_write;

    memset(buf, 0, sizeof(buf));

    ret = lseek(to_fd, to_offset, SEEK_SET);
    if (ret < 0)
        error_errno("failed to seek output file to %lx", to_offset);

    while (size != 0) {
        to_write = size;
        if (to_write > sizeof(buf))
            to_write = sizeof(buf);

        ret = write(to_fd, buf, to_write);
        if (ret < 0)
            error_errno("failed to write to output file");
        size -= ret;
    }
}

static void verify_data(int to_fd, off_t to_offset,
                      int from_fd, off_t from_offset,
                      ssize_t size)
{
    char buf_to[4096];
    char buf_from[4096];
    int ret;
    int to_read;
    int c;
    char *ptr;

    ret = lseek(to_fd, to_offset, SEEK_SET);
    if (ret < 0)
        error_errno("failed to seek output file to %lx", to_offset);

    ret = lseek(from_fd, from_offset, SEEK_SET);
    if (ret < 0)
        error_errno("failed to seek input file to %lx", from_offset);

    while (size != 0) {
        to_read = sizeof(buf_to);
        if (size > 0 && to_read > size)
            to_read = size;

        ptr = buf_to;
        c = to_read;
        while (c > 0) {
            ret = read(to_fd, ptr, c);
            if (ret < 0)
                error_errno("failed to read from output file");
            if (ret == 0 && size < 0)
                return;
            if (ret == 0)
                error_errno("eof while reading output file");
            ptr += ret;
            c -= ret;
        }

        ptr = buf_from;
        c = to_read;
        while (c > 0) {
            ret = read(from_fd, ptr, c);
            if (ret < 0)
                error_errno("failed to read from input file");
            if (ret == 0 && size < 0)
                return;
            if (ret == 0)
                error_errno("eof while reading input file");
            ptr += ret;
            c -= ret;
        }

        if (memcmp(buf_from, buf_to, to_read) != 0)
            error("mismatch while verifying written data");

        size -= to_read;
    }
}

static void copy_data(int to_fd, off_t to_offset,
                      int from_fd, off_t from_offset,
                      ssize_t size)
{
    char buf[4096];
    int ret;
    int to_write;
    const char *ptr;

    ret = lseek(to_fd, to_offset, SEEK_SET);
    if (ret < 0)
        error_errno("failed to seek output file to %lx", to_offset);

    ret = lseek(from_fd, from_offset, SEEK_SET);
    if (ret < 0)
        error_errno("failed to seek input file to %lx", from_offset);

    while (size != 0) {
        ret = read(from_fd, buf, sizeof(buf));
        if (ret < 0)
            error_errno("failed to read from input file");
        if (ret == 0 && size > 0)
            error_errno("eof while reading input file");
        if (ret == 0)
            return;

        to_write = ret;
        ptr = buf;

        if (size > 0)
            size -= to_write;

        while (to_write > 0) {
            ret = write(to_fd, ptr, to_write);
            if (ret < 0)
                error_errno("failed to write to output file");
            to_write -= ret;
            ptr += ret;
        }
    }
}

static void init(void)
{
    int ret;

    umask(0);

    ret = mkdir("/dev", 0755);
    if (ret && errno != EEXIST)
        error_errno("failed to create /dev");

    ret = mkdir("/proc", 0755);
    if (ret && errno != EEXIST)
        error_errno("failed to create /proc");

    ret = mkdir("/sys", 0755);
    if (ret && errno != EEXIST)
        error_errno("failed to create /sys");

    ret = mount("proc", "/proc", "proc", 0, NULL);
    if (ret)
        error_errno("failed to mount proc");

    ret = mount("sysfs", "/sys", "sysfs", 0, NULL);
    if (ret)
        error_errno("failed to mount sys");

    ret = mkdir("/dev/block", 0755);
    if (ret && errno != EEXIST)
        error_errno("failed to create /dev/block");

    ret = mknod("/dev/block/mmcblk0", S_IFBLK | 0755, makedev(179, 0));
    if (ret)
        error_errno("failed to create mmcblk0");
}

int main(int argc, char **argv)
{
    int in_fd;
    int out_fd;
    const struct omap_type *type;

    if (getpid() == 1)
        init();

    in_fd = open("bootloader.img", O_RDONLY);
    if (in_fd < 0)
        error_errno("failed to open bootloader.img");

    out_fd = open("/dev/block/mmcblk0", O_RDWR);
    if (out_fd < 0)
        error_errno("failed to open mmcblk0");

    type = get_omap_type();

    printf("Found %s %s %s\n", type->family, type->type, type->msv_type);

    printf("Zeroing to end of sbl\n");
    zero_data(out_fd, 0, MMC_SBL_OFFSET);

    /* Don't write the partition table, let the bootloader do it on next boot */
#if 0
    printf("Writing partition-table from %lx to %lx\n",
           IMG_PIT_OFFSET, MMC_PIT_OFFSET);
    copy_data(out_fd, MMC_PIT_OFFSET, in_fd, IMG_PIT_OFFSET, PIT_SIZE);
#endif

    printf("Writing xloader from %lx to %lx\n",
           type->offset, MMC_XLOADER_OFFSET);
    copy_data(out_fd, MMC_XLOADER_OFFSET, in_fd, type->offset, XLOADER_SIZE);

    printf("Writing sbl from %lx to %lx\n",
           IMG_SBL_OFFSET, MMC_SBL_OFFSET);
    copy_data(out_fd, MMC_SBL_OFFSET, in_fd, IMG_SBL_OFFSET, -1);

#if 0
    printf("Verifying partition table\n");
    verify_data(out_fd, MMC_PIT_OFFSET, in_fd, IMG_PIT_OFFSET, PIT_SIZE);
#endif

    printf("Verifying xloader\n");
    verify_data(out_fd, MMC_XLOADER_OFFSET, in_fd, type->offset, XLOADER_SIZE);

    printf("Verifying sbl\n");
    verify_data(out_fd, MMC_SBL_OFFSET, in_fd, IMG_SBL_OFFSET, -1);

    printf("Syncing\n");
    sync();

    printf("Dropping caches\n");
    drop_caches();

    printf("Verifying xloader.img\n");
    verify_data(out_fd, MMC_XLOADER_OFFSET, in_fd, type->offset, XLOADER_SIZE);

    printf("Verifying sbl.img\n");
    verify_data(out_fd, MMC_SBL_OFFSET, in_fd, IMG_SBL_OFFSET, -1);

    printf("Done\n");

    if (getpid() == 1) {
        __reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                 LINUX_REBOOT_CMD_RESTART2, "bootloader");

        while (1) { sleep(1); }
    }

    return 0;
}
