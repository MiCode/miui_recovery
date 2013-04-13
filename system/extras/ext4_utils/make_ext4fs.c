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

#include "make_ext4fs.h"
#include "output_file.h"
#include "ext4_utils.h"
#include "allocate.h"
#include "contents.h"
#include "uuid.h"
#include "backed_block.h"

#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef ANDROID
#include <private/android_filesystem_config.h>
#endif

/* TODO: Not implemented:
   Allocating blocks in the same block group as the file inode
   Hash or binary tree directories
   Special files: sockets, devices, fifos
 */

static int filter_dot(const struct dirent *d)
{
	return (strcmp(d->d_name, "..") && strcmp(d->d_name, "."));
}

static u32 build_default_directory_structure()
{
	u32 inode;
	u32 root_inode;
	struct dentry dentries = {
			.filename = "lost+found",
			.file_type = EXT4_FT_DIR,
			.mode = S_IRWXU,
			.uid = 0,
			.gid = 0,
			.mtime = 0,
	};
	root_inode = make_directory(0, 1, &dentries, 1);
	inode = make_directory(root_inode, 0, NULL, 0);
	*dentries.inode = inode;
	inode_set_permissions(inode, dentries.mode,
		dentries.uid, dentries.gid, dentries.mtime);

	return root_inode;
}

/* Read a local directory and create the same tree in the generated filesystem.
   Calls itself recursively with each directory in the given directory */
static u32 build_directory_structure(const char *full_path, const char *dir_path,
		u32 dir_inode, int android)
{
	int entries = 0;
	struct dentry *dentries;
	struct dirent **namelist;
	struct stat stat;
	int ret;
	int i;
	u32 inode;
	u32 entry_inode;
	u32 dirs = 0;

	entries = scandir(full_path, &namelist, filter_dot, (void*)alphasort);
	if (entries < 0) {
		error_errno("scandir");
		return EXT4_ALLOCATE_FAILED;
	}

	dentries = calloc(entries, sizeof(struct dentry));
	if (dentries == NULL)
		critical_error_errno("malloc");

	for (i = 0; i < entries; i++) {
		dentries[i].filename = strdup(namelist[i]->d_name);
		if (dentries[i].filename == NULL)
			critical_error_errno("strdup");

		asprintf(&dentries[i].path, "%s/%s", dir_path, namelist[i]->d_name);
		asprintf(&dentries[i].full_path, "%s/%s", full_path, namelist[i]->d_name);

		free(namelist[i]);

		ret = lstat(dentries[i].full_path, &stat);
		if (ret < 0) {
			error_errno("lstat");
			i--;
			entries--;
			continue;
		}

		dentries[i].size = stat.st_size;
		dentries[i].mode = stat.st_mode & (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
		dentries[i].mtime = stat.st_mtime;
		if (android) {
#ifdef ANDROID
			unsigned int mode = 0;
			unsigned int uid = 0;
			unsigned int gid = 0;
			int dir = S_ISDIR(stat.st_mode);
			fs_config(dentries[i].path, dir, &uid, &gid, &mode);
			dentries[i].mode = mode;
			dentries[i].uid = uid;
			dentries[i].gid = gid;
#else
			error("can't set android permissions - built without android support");
#endif
		}

		if (S_ISREG(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_REG_FILE;
		} else if (S_ISDIR(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_DIR;
			dirs++;
		} else if (S_ISCHR(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_CHRDEV;
		} else if (S_ISBLK(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_BLKDEV;
		} else if (S_ISFIFO(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_FIFO;
		} else if (S_ISSOCK(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_SOCK;
		} else if (S_ISLNK(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_SYMLINK;
			dentries[i].link = calloc(info.block_size, 1);
			readlink(dentries[i].full_path, dentries[i].link, info.block_size - 1);
		} else {
			error("unknown file type on %s", dentries[i].path);
			i--;
			entries--;
		}
	}
	free(namelist);

	inode = make_directory(dir_inode, entries, dentries, dirs);

	for (i = 0; i < entries; i++) {
		if (dentries[i].file_type == EXT4_FT_REG_FILE) {
			entry_inode = make_file(dentries[i].full_path, dentries[i].size);
		} else if (dentries[i].file_type == EXT4_FT_DIR) {
			entry_inode = build_directory_structure(dentries[i].full_path,
					dentries[i].path, inode, android);
		} else if (dentries[i].file_type == EXT4_FT_SYMLINK) {
			entry_inode = make_link(dentries[i].full_path, dentries[i].link);
		} else {
			error("unknown file type on %s", dentries[i].path);
			entry_inode = 0;
		}
		*dentries[i].inode = entry_inode;

		ret = inode_set_permissions(entry_inode, dentries[i].mode,
			dentries[i].uid, dentries[i].gid,
			dentries[i].mtime);
		if (ret)
			error("failed to set permissions on %s\n", dentries[i].path);

		free(dentries[i].path);
		free(dentries[i].full_path);
		free(dentries[i].link);
		free((void *)dentries[i].filename);
	}

	free(dentries);
	return inode;
}

static u32 compute_block_size()
{
	return 4096;
}

static u32 compute_journal_blocks()
{
	u32 journal_blocks = DIV_ROUND_UP(info.len, info.block_size) / 64;
	if (journal_blocks < 1024)
		journal_blocks = 1024;
	if (journal_blocks > 32768)
		journal_blocks = 32768;
	return journal_blocks;
}

static u32 compute_blocks_per_group()
{
	return info.block_size * 8;
}

static u32 compute_inodes()
{
	return DIV_ROUND_UP(info.len, info.block_size) / 4;
}

static u32 compute_inodes_per_group()
{
	u32 blocks = DIV_ROUND_UP(info.len, info.block_size);
	u32 block_groups = DIV_ROUND_UP(blocks, info.blocks_per_group);
	u32 inodes = DIV_ROUND_UP(info.inodes, block_groups);
	inodes = ALIGN(inodes, (info.block_size / info.inode_size));

	/* After properly rounding up the number of inodes/group,
	 * make sure to update the total inodes field in the info struct.
	 */
	info.inodes = inodes * block_groups;

	return inodes;
}

static u32 compute_bg_desc_reserve_blocks()
{
	u32 blocks = DIV_ROUND_UP(info.len, info.block_size);
	u32 block_groups = DIV_ROUND_UP(blocks, info.blocks_per_group);
	u32 bg_desc_blocks = DIV_ROUND_UP(block_groups * sizeof(struct ext2_group_desc),
			info.block_size);

	u32 bg_desc_reserve_blocks =
			DIV_ROUND_UP(block_groups * 1024 * sizeof(struct ext2_group_desc),
					info.block_size) - bg_desc_blocks;

	if (bg_desc_reserve_blocks > info.block_size / sizeof(u32))
		bg_desc_reserve_blocks = info.block_size / sizeof(u32);

	return bg_desc_reserve_blocks;
}

void reset_ext4fs_info() {
    // Reset all the global data structures used by make_ext4fs so it
    // can be called again.
    memset(&info, 0, sizeof(info));
    memset(&aux_info, 0, sizeof(aux_info));
    free_data_blocks();
}

int make_ext4fs(const char *filename, s64 len)
{
    reset_ext4fs_info();
    info.len = len;
    return make_ext4fs_internal(filename, NULL, NULL, 0, 0, 0, 0, 1, 0);
}

int make_ext4fs_internal(const char *filename, const char *directory,
                         char *mountpoint, int android, int gzip, int sparse,
                         int crc, int wipe, int init_itabs)
{
        u32 root_inode_num;
        u16 root_mode;

	if (setjmp(setjmp_env))
		return EXIT_FAILURE; /* Handle a call to longjmp() */

	if (info.len <= 0)
		info.len = get_file_size(filename);

	if (info.len <= 0) {
		fprintf(stderr, "Need size of filesystem\n");
                return EXIT_FAILURE;
	}

	if (info.block_size <= 0)
		info.block_size = compute_block_size();

	/* Round down the filesystem length to be a multiple of the block size */
	info.len &= ~((u64)info.block_size - 1);

	if (info.journal_blocks == 0)
		info.journal_blocks = compute_journal_blocks();

	if (info.no_journal == 0)
		info.feat_compat = EXT4_FEATURE_COMPAT_HAS_JOURNAL;
	else
		info.journal_blocks = 0;

	if (info.blocks_per_group <= 0)
		info.blocks_per_group = compute_blocks_per_group();

	if (info.inodes <= 0)
		info.inodes = compute_inodes();

	if (info.inode_size <= 0)
		info.inode_size = 256;

	if (info.label == NULL)
		info.label = "";

	info.inodes_per_group = compute_inodes_per_group();

	info.feat_compat |=
			EXT4_FEATURE_COMPAT_RESIZE_INODE;

	info.feat_ro_compat |=
			EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER |
			EXT4_FEATURE_RO_COMPAT_LARGE_FILE;

	info.feat_incompat |=
			EXT4_FEATURE_INCOMPAT_EXTENTS |
			EXT4_FEATURE_INCOMPAT_FILETYPE;


	info.bg_desc_reserve_blocks = compute_bg_desc_reserve_blocks();

	printf("Creating filesystem with parameters:\n");
	printf("    Size: %llu\n", info.len);
	printf("    Block size: %d\n", info.block_size);
	printf("    Blocks per group: %d\n", info.blocks_per_group);
	printf("    Inodes per group: %d\n", info.inodes_per_group);
	printf("    Inode size: %d\n", info.inode_size);
	printf("    Journal blocks: %d\n", info.journal_blocks);
	printf("    Label: %s\n", info.label);

	ext4_create_fs_aux_info();

	printf("    Blocks: %llu\n", aux_info.len_blocks);
	printf("    Block groups: %d\n", aux_info.groups);
	printf("    Reserved block group size: %d\n", info.bg_desc_reserve_blocks);

	block_allocator_init();

	ext4_fill_in_sb();

	if (reserve_inodes(0, 10) == EXT4_ALLOCATE_FAILED)
		error("failed to reserve first 10 inodes");

	if (info.feat_compat & EXT4_FEATURE_COMPAT_HAS_JOURNAL)
		ext4_create_journal_inode();

	if (info.feat_compat & EXT4_FEATURE_COMPAT_RESIZE_INODE)
		ext4_create_resize_inode();

	if (directory)
		root_inode_num = build_directory_structure(directory, mountpoint, 0, android);
	else
		root_inode_num = build_default_directory_structure();

	root_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	inode_set_permissions(root_inode_num, root_mode, 0, 0, 0);

	ext4_update_free();

	if (init_itabs)
		init_unused_inode_tables();

	ext4_queue_sb();

	printf("Created filesystem with %d/%d inodes and %d/%d blocks\n",
			aux_info.sb->s_inodes_count - aux_info.sb->s_free_inodes_count,
			aux_info.sb->s_inodes_count,
			aux_info.sb->s_blocks_count_lo - aux_info.sb->s_free_blocks_count_lo,
			aux_info.sb->s_blocks_count_lo);

	write_ext4_image(filename, gzip, sparse, crc, wipe);

	return 0;
}
