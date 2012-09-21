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

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include "ext4_utils.h"
#include "ext4.h"
#include "make_ext4fs.h"
#include "allocate.h"
#include "contents.h"
#include "extent.h"
#include "indirect.h"

static u32 dentry_size(u32 entries, struct dentry *dentries)
{
	u32 len = 24;
	unsigned int i;
	unsigned int dentry_len;

	for (i = 0; i < entries; i++) {
		dentry_len = 8 + ALIGN(strlen(dentries[i].filename), 4);
		if (len % info.block_size + dentry_len > info.block_size)
			len += info.block_size - (len % info.block_size);
		len += dentry_len;
	}

	/* include size of the dentry used to pad until the end of the block */
	if (len % info.block_size + 8 > info.block_size)
		len += info.block_size - (len % info.block_size);
	len += 8;

	return len;
}

static struct ext4_dir_entry_2 *add_dentry(u8 *data, u32 *offset,
		struct ext4_dir_entry_2 *prev, u32 inode, const char *name,
		u8 file_type)
{
	u8 name_len = strlen(name);
	u16 rec_len = 8 + ALIGN(name_len, 4);
	struct ext4_dir_entry_2 *dentry;

	u32 start_block = *offset / info.block_size;
	u32 end_block = (*offset + rec_len - 1) / info.block_size;
	if (start_block != end_block) {
		/* Adding this dentry will cross a block boundary, so pad the previous
		   dentry to the block boundary */
		if (!prev)
			critical_error("no prev");
		prev->rec_len += end_block * info.block_size - *offset;
		*offset = end_block * info.block_size;
	}

	dentry = (struct ext4_dir_entry_2 *)(data + *offset);
	dentry->inode = inode;
	dentry->rec_len = rec_len;
	dentry->name_len = name_len;
	dentry->file_type = file_type;
	memcpy(dentry->name, name, name_len);

	*offset += rec_len;
	return dentry;
}

/* Creates a directory structure for an array of directory entries, dentries,
   and stores the location of the structure in an inode.  The new inode's
   .. link is set to dir_inode_num.  Stores the location of the inode number
   of each directory entry into dentries[i].inode, to be filled in later
   when the inode for the entry is allocated.  Returns the inode number of the
   new directory */
u32 make_directory(u32 dir_inode_num, u32 entries, struct dentry *dentries,
	u32 dirs)
{
	struct ext4_inode *inode;
	u32 blocks;
	u32 len;
	u32 offset = 0;
	u32 inode_num;
	u8 *data;
	unsigned int i;
	struct ext4_dir_entry_2 *dentry;

	blocks = DIV_ROUND_UP(dentry_size(entries, dentries), info.block_size);
	len = blocks * info.block_size;

	if (dir_inode_num) {
		inode_num = allocate_inode(info);
	} else {
		dir_inode_num = EXT4_ROOT_INO;
		inode_num = EXT4_ROOT_INO;
	}

	if (inode_num == EXT4_ALLOCATE_FAILED) {
		error("failed to allocate inode\n");
		return EXT4_ALLOCATE_FAILED;
	}

	add_directory(inode_num);

	inode = get_inode(inode_num);
	if (inode == NULL) {
		error("failed to get inode %u", inode_num);
		return EXT4_ALLOCATE_FAILED;
	}

	data = inode_allocate_data_extents(inode, len, len);
	if (data == NULL) {
		error("failed to allocate %u extents", len);
		return EXT4_ALLOCATE_FAILED;
	}

	inode->i_mode = S_IFDIR;
	inode->i_links_count = dirs + 2;
	inode->i_flags |= aux_info.default_i_flags;

	dentry = NULL;

	dentry = add_dentry(data, &offset, NULL, inode_num, ".", EXT4_FT_DIR);
	if (!dentry) {
		error("failed to add . directory");
		return EXT4_ALLOCATE_FAILED;
	}

	dentry = add_dentry(data, &offset, dentry, dir_inode_num, "..", EXT4_FT_DIR);
	if (!dentry) {
		error("failed to add .. directory");
		return EXT4_ALLOCATE_FAILED;
	}

	for (i = 0; i < entries; i++) {
		dentry = add_dentry(data, &offset, dentry, 0,
				dentries[i].filename, dentries[i].file_type);
		if (offset > len || (offset == len && i != entries - 1))
			critical_error("internal error: dentry for %s ends at %d, past %d\n",
				dentries[i].filename, offset, len);
		dentries[i].inode = &dentry->inode;
		if (!dentry) {
			error("failed to add directory");
			return EXT4_ALLOCATE_FAILED;
		}
	}

	dentry = (struct ext4_dir_entry_2 *)(data + offset);
	dentry->inode = 0;
	dentry->rec_len = len - offset;
	dentry->name_len = 0;
	dentry->file_type = EXT4_FT_UNKNOWN;

	return inode_num;
}

/* Creates a file on disk.  Returns the inode number of the new file */
u32 make_file(const char *filename, u64 len)
{
	struct ext4_inode *inode;
	u32 inode_num;

	inode_num = allocate_inode(info);
	if (inode_num == EXT4_ALLOCATE_FAILED) {
		error("failed to allocate inode\n");
		return EXT4_ALLOCATE_FAILED;
	}

	inode = get_inode(inode_num);
	if (inode == NULL) {
		error("failed to get inode %u", inode_num);
		return EXT4_ALLOCATE_FAILED;
	}

	if (len > 0)
		inode_allocate_file_extents(inode, len, filename);

	inode->i_mode = S_IFREG;
	inode->i_links_count = 1;
	inode->i_flags |= aux_info.default_i_flags;

	return inode_num;
}

/* Creates a file on disk.  Returns the inode number of the new file */
u32 make_link(const char *filename, const char *link)
{
	struct ext4_inode *inode;
	u32 inode_num;
	u32 len = strlen(link);

	inode_num = allocate_inode(info);
	if (inode_num == EXT4_ALLOCATE_FAILED) {
		error("failed to allocate inode\n");
		return EXT4_ALLOCATE_FAILED;
	}

	inode = get_inode(inode_num);
	if (inode == NULL) {
		error("failed to get inode %u", inode_num);
		return EXT4_ALLOCATE_FAILED;
	}

	inode->i_mode = S_IFLNK;
	inode->i_links_count = 1;
	inode->i_flags |= aux_info.default_i_flags;
	inode->i_size_lo = len;

	if (len + 1 <= sizeof(inode->i_block)) {
		/* Fast symlink */
		memcpy((char*)inode->i_block, link, len);
	} else {
		u8 *data = inode_allocate_data_indirect(inode, info.block_size, info.block_size);
		memcpy(data, link, len);
		inode->i_blocks_lo = info.block_size / 512;
	}

	return inode_num;
}

int inode_set_permissions(u32 inode_num, u16 mode, u16 uid, u16 gid, u32 mtime)
{
	struct ext4_inode *inode = get_inode(inode_num);

	if (!inode)
		return -1;

	inode->i_mode |= mode;
	inode->i_uid = uid;
	inode->i_gid = gid;
	inode->i_mtime = mtime;
	inode->i_atime = mtime;
	inode->i_ctime = mtime;

	return 0;
}
