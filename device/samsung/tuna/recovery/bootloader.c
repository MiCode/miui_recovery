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
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "bootloader.h"

#define SECTOR_SIZE 512
#define NUM_SECONDARY_GPT_SECTORS 34
#define PIT_PARTITION_TABLE_SIZE 0x1000 // 4KB
#define BOOT_PART_LEN 0x20000 // 128KB
#define SBL_OFFSET (PIT_PARTITION_TABLE_SIZE + (BOOT_PART_LEN * 4))

#define SMALL_BUFFER_SIZE 0x20

// A combination of these defines the specification of the device.
#define OMAP4460  0x1
#define OMAP4430  0x2
#define CHIP_HS   0x4
#define CHIP_EMU  0x8
#define MSV_PROD 0x10

// Location of the PIT partition table in EMMC
#define PIT_PARTITION_TABLE_LOCATION 0x4400

static const char* FAMILY_LOCATION = "/sys/board_properties/soc/family";
static const char* TYPE_LOCATION = "/sys/board_properties/soc/type";
static const char* MSV_LOCATION = "/sys/board_properties/soc/msv";

static const char* MMC_LOCATION = "/dev/block/mmcblk0";

/* pit structure = header + (pit partition info * n) */
struct pit_header {
        unsigned int magic;
        int count;              /* onenand + mmc partitions */
        int dummy[5];
} __attribute__((packed));

struct pit_partinfo {
        int binary;             /* BINARY_TYPE_ */
        int device;             /* PARTITION_DEV_TYPE_ */
        int id;                 /* partition id */
        int attribute;          /* PARTITION_ATTR_ */
        int update;             /* PARTITION_UPDATE_ATTR_ - dedicated. */
        unsigned int blksize;   /* mmc start sector */
        unsigned int blklen;    /* sector count */
        unsigned int offset;    /* file offset (in TAR) */
        unsigned int filesize;  /* file size */
        char name[32];          /* partition name */
        char filename[32];      /* file name */
        char deltaname[32];     /* delta file name - dedicated. */
} __attribute__((packed));

unsigned int read_whole_file(const char* fname, char* buffer,
                             int buffer_size) {
  memset(buffer, 0, buffer_size);

  FILE* f = fopen(fname, "rb");
  if (f == NULL) {
    fprintf(stderr, "Cannot open %s!\n", fname);
    return -1;
  }

  int read_byte_count = fread(buffer, 1, buffer_size - 1, f);
  fclose(f);
  if (read_byte_count < 0) {
    fprintf(stderr, "Couldn't read %s\n", fname);
    return -1;
  }

  // Remove any newlines at the end.
  while (buffer[read_byte_count - 1] == '\n') {
    buffer[--read_byte_count] = 0;
  }

  return 0;
}

// Get the specifications for this device
int get_specification() {
  int spec = 0;

  char file_data[SMALL_BUFFER_SIZE];

  if (read_whole_file(FAMILY_LOCATION, file_data, SMALL_BUFFER_SIZE) == 0) {
    if (strcmp(file_data, "OMAP4430") == 0) {
      spec |= OMAP4430;
    } else if (strcmp(file_data, "OMAP4460") == 0) {
      spec |= OMAP4460;
    } else {
      fprintf(stderr, "Unknown family: %s\n", file_data);
      return -1;
    }
  } else {
    fprintf(stderr, "No family\n");
    return -1;
  }

  if (read_whole_file(TYPE_LOCATION, file_data, SMALL_BUFFER_SIZE) == 0) {
    if (strcmp(file_data, "HS") == 0) {
      spec |= CHIP_HS;
    } else if (strcmp(file_data, "EMU") == 0) {
      spec |= CHIP_EMU;
    } else {
      fprintf(stderr, "Unknown chip type: %s\n", file_data);
      return -1;
    }
  } else {
    fprintf(stderr, "No chip type\n");
    return -1;
  }

  // MSV is either prod (non-zero) or eng (zero). Default to eng.
  if (read_whole_file(MSV_LOCATION, file_data, SMALL_BUFFER_SIZE) == 0) {
    if (strtoul(file_data, NULL, 16) != 0) {
      spec |= MSV_PROD;
    }
  } else {
    fprintf(stderr, "No msv\n");
  }

  return spec;
}


// Four different xloaders are supported by bootloader.img:
// 4460 EMU, 4460 HS (eng), 4460 HS (prod), 4430 HS.
// The layout of the bootloader.img is:
//
// PIT Partition table (4KB)
// 4460 EMU xloader (128KB)
// 4460 HS (eng) xloader (128KB)
// 4460 HS (prod) xloader (128KB)
// 4430 HS xloader(128KB)
// sbl (the rest)
int get_xloader_offset() {
  int spec = get_specification();

  if (spec < 0) {
    return -1;
  }

  if (spec & OMAP4460 &&
      spec & CHIP_EMU) {
    return 0;
  } else if (spec & OMAP4460 &&
             spec & CHIP_HS &&
             !(spec & MSV_PROD)) {
    return BOOT_PART_LEN;
  } else if (spec & OMAP4460 &&
             spec & CHIP_HS &&
             spec & MSV_PROD) {
    return BOOT_PART_LEN * 2;
  } else if (spec & OMAP4430 &&
             spec & CHIP_HS) {
    return BOOT_PART_LEN * 3;
  }

  fprintf(stderr, "Unsupported spec for bootloader.img: %d", spec);
  return -1;
}

int write_pit_partition_table(const char* image_data,
                              size_t image_size) {
  int written = 0;
  int close_status = 0;
  int to_write;
  const char* curr;


  int mmcfd = open(MMC_LOCATION, O_RDWR);
  if (mmcfd < 0) {
    fprintf(stderr, "Could not open %s\n", MMC_LOCATION);
    return -1;
  }

  // zero out gpt magic field
  if (lseek(mmcfd, SECTOR_SIZE, SEEK_SET) < 0) {
    fprintf(stderr, "Couldn't seek to the start of sector 1\n");
    close(mmcfd);
    return -1;
  }

  char buf[SECTOR_SIZE];
  if (read(mmcfd, buf, SECTOR_SIZE) != SECTOR_SIZE) {
    fprintf(stderr, "Failed to read sector 1\n");
    close(mmcfd);
    return -1;
  }

  memset(buf, 0, 8);

  if (lseek(mmcfd, SECTOR_SIZE, SEEK_SET) < 0) {
    fprintf(stderr, "Couldn't seek to the start of sector 1, part 2\n");
    close(mmcfd);
    return -1;
  }

  to_write = SECTOR_SIZE;
  curr = buf;
  while (to_write > 0) {
    written = write(mmcfd, curr, to_write);
    if (written < 0 && errno != EINTR) {
      fprintf(stderr, "Couldn't overwrite sector 1\n");
      close(mmcfd);
      return -1;
    }
    if (written > 0) {
      to_write -= written;
      curr += written;
    }
  }

  // modify the pit partition info to reflect userdata size
  // before writing the pit partition table
  char pit_partition_copy[PIT_PARTITION_TABLE_SIZE];
  memcpy(pit_partition_copy, image_data, PIT_PARTITION_TABLE_SIZE);

  struct pit_header* hd = (struct pit_header*) pit_partition_copy;
  int i;
  for (i = 0; i < hd->count; i++) {
    struct pit_partinfo* pi = (struct pit_partinfo*)
        (pit_partition_copy + sizeof(*hd) + sizeof(*pi) * i);
    if (strcmp(pi->name, "userdata") == 0) {
      unsigned int num_sectors;
      if (ioctl(mmcfd, BLKGETSIZE, &num_sectors) < 0) {
        fprintf(stderr, "Couldn't get sector count\n");
        close(mmcfd);
        return -1;
      }

      // There are NUM_SECONDARY_GPT_SECTORS sectors reserved at the end of the
      // device to hold a backup copy of the GPT, so we subtract that number.
      pi->blklen = num_sectors - pi->blksize - NUM_SECONDARY_GPT_SECTORS;
      break;
    }
  }

  if (i == hd->count) {
    fprintf(stderr, "No userdata partition found\n");
    close(mmcfd);
    return -1;
  }

  // copy the modified pit partition table data to the correct location
  if (lseek(mmcfd, PIT_PARTITION_TABLE_LOCATION, SEEK_SET) < 0) {
    fprintf(stderr, "Couldn't seek to the pit partition table location\n");
    close(mmcfd);
    return -1;
  }

  to_write = PIT_PARTITION_TABLE_SIZE;
  curr = pit_partition_copy;
  while (to_write > 0) {
    written = write(mmcfd, curr, to_write);
    if (written < 0 && errno != EINTR) {
      fprintf(stderr, "Failed writing pit partition table\n");
      close(mmcfd);
      return -1;
    }
    if (written > 0) {
      to_write -= written;
      curr += written;
    }
  }

  if (close(mmcfd) != 0) {
    fprintf(stderr, "Failed to close file\n");
    return -1;
  }

  return 0;
}

int write_xloader(const char* image_data,
                  size_t image_size,
                  const char* xloader_loc) {
  int xloader_offset = get_xloader_offset();

  if (xloader_offset < 0) {
    return -1;
  }

  // The offsets into xloader part of the bootloader image
  xloader_offset += PIT_PARTITION_TABLE_SIZE;

  FILE* xloader = fopen(xloader_loc, "r+b");
  if (xloader == NULL) {
    fprintf(stderr, "Could not open %s\n", xloader_loc);
    return -1;
  }

  // index into the correct xloader offset
  int written = fwrite(image_data+xloader_offset, 1, BOOT_PART_LEN, xloader);
  int close_status = fclose(xloader);
  if (written != BOOT_PART_LEN || close_status != 0) {
    fprintf(stderr, "Failed writing to /xloader\n");
    return -1;
  }

  return 0;
}

int write_sbl(const char* image_data,
              size_t image_size,
              const char* sbl_loc) {
  unsigned int sbl_size = image_size - SBL_OFFSET;
  FILE* sbl = fopen(sbl_loc, "r+b");
  if (sbl == NULL) {
    fprintf(stderr, "Could not open %s\n", sbl_loc);
    return -1;
  }

  int written = fwrite(image_data+SBL_OFFSET, 1, sbl_size, sbl);
  int close_status = fclose(sbl);
  if (written != sbl_size || close_status != 0) {
    fprintf(stderr, "Failed writing to /sbl\n");
    return -1;
  }

  return 0;
}

int update_bootloader(const char* image_data,
                      size_t image_size,
                      const char* xloader_loc,
                      const char* sbl_loc) {
  if (image_size < SBL_OFFSET) {
    fprintf(stderr, "image size %d is too small\n", image_size);
    return -1;
  }

  if (write_pit_partition_table(image_data, image_size) < 0) {
    return -1;
  }

  if (write_xloader(image_data, image_size, xloader_loc) < 0) {
    return -1;
  }

  if (write_sbl(image_data, image_size, sbl_loc) < 0) {
    return -1;
  }

  return 0;
}
