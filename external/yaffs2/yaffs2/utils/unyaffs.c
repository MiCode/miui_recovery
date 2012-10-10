/*
 * unyaffs: extract files from yaffs2 file system image to current directory
 *
 * Created by Kai Wei <kai.wei.cn@gmail.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <limits.h>

#include "yaffs_ecc.h"
#include "yaffs_guts.h"

#include "yaffs_tagsvalidity.h"
#include "yaffs_packedtags2.h"

#include "unyaffs.h"

#define CHUNK_SIZE 2048
#define SPARE_SIZE 64
#define MAX_OBJECTS 50000
#define YAFFS_OBJECTID_ROOT     1

static int read_chunk(void);

static unsigned char data[CHUNK_SIZE + SPARE_SIZE];
static unsigned char *chunk_data = data;
static unsigned char *spare_data = data + CHUNK_SIZE;
static int img_file;

static char *obj_list[MAX_OBJECTS];
static void process_chunk(unyaffs_callback callback)
{
	int out_file;
    unsigned remain, s;
	char *full_path_name;

	yaffs_PackedTags2 *pt = (yaffs_PackedTags2 *)spare_data;
	if (pt->t.byteCount == 0xffff)  {	//a new object 

		yaffs_ObjectHeader oh;
		memcpy(&oh, chunk_data, sizeof(yaffs_ObjectHeader));

		full_path_name = (char *)malloc(strlen(oh.name) + strlen(obj_list[oh.parentObjectId]) + 2);
		if (full_path_name == NULL) {
			perror("malloc full path name\n");
		}
		strcpy(full_path_name, obj_list[oh.parentObjectId]);
		strcat(full_path_name, "/");
		strcat(full_path_name, oh.name);
		obj_list[pt->t.objectId] = full_path_name;

		switch(oh.type) {
			case YAFFS_OBJECT_TYPE_FILE:
				remain = oh.fileSize;
				out_file = creat(full_path_name, oh.yst_mode);
				while(remain > 0) {
					if (read_chunk())
						return;
					s = (remain < pt->t.byteCount) ? remain : pt->t.byteCount;	
					if (write(out_file, chunk_data, s) == -1)
						return;
					remain -= s;
				}
				close(out_file);
				break;
			case YAFFS_OBJECT_TYPE_SYMLINK:
				symlink(oh.alias, full_path_name);
				break;
			case YAFFS_OBJECT_TYPE_DIRECTORY:
				mkdir(full_path_name, 0777);
				break;
			case YAFFS_OBJECT_TYPE_HARDLINK:
				link(obj_list[oh.equivalentObjectId], full_path_name);
				break;
		}
        if (callback != NULL)
            callback(full_path_name);
		lchown(full_path_name, oh.yst_uid, oh.yst_gid);
		if (oh.type != YAFFS_OBJECT_TYPE_SYMLINK)
			chmod(full_path_name, oh.yst_mode);
		struct timeval times[2];
		times[0].tv_sec = oh.yst_atime;
		times[1].tv_sec = oh.yst_mtime;
		utimes(full_path_name, times);
	}
}


static int read_chunk(void)
{
	ssize_t s;
	int ret = -1;
	memset(chunk_data, 0xff, sizeof(chunk_data));
	s = read(img_file, data, CHUNK_SIZE + SPARE_SIZE);
	if (s == -1) {
		perror("read image file\n");
	} else if (s == 0) {
		printf("end of image\n");
	} else if ((s == (CHUNK_SIZE + SPARE_SIZE))) {
		ret = 0;
	} else {
		fprintf(stderr, "broken image file\n");
	}
	return ret;
}

int unyaffs(char* filename, char* directory, unyaffs_callback callback)
{
	img_file = open(filename, O_RDONLY);
	if (img_file == -1) {
		printf("open image file failed\n");
        return 1;
	}

	memset(obj_list, sizeof(char*) * MAX_OBJECTS, 0);
	obj_list[YAFFS_OBJECTID_ROOT] = ".";
    char pwd[PATH_MAX];
    if (directory != NULL) 
    {
        getcwd(pwd, PATH_MAX);
        chdir(directory);
    }
	while(1) {
		if (read_chunk() == -1)
			break;
		process_chunk(callback);
	}
    if (directory != NULL) 
        chdir(pwd);
	close(img_file);
	int i;
	for (i = 0; i < MAX_OBJECTS; i++)
	{
		if (i == YAFFS_OBJECTID_ROOT)
			continue;
		if (obj_list[i] != NULL) {
			free(obj_list[i]);
			obj_list[i] = NULL;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: unyaffs image_file_name\n");
		exit(1);
	}
    return unyaffs(argv[1], NULL, NULL);
}
