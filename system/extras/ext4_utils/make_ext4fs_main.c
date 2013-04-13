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

#include <unistd.h>
#include <libgen.h>

#if defined(__linux__)
#include <linux/fs.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/disk.h>
#endif

#include "make_ext4fs.h"

extern struct fs_info info;


static void usage(char *path)
{
        fprintf(stderr, "%s [ -l <len> ] [ -j <journal size> ] [ -b <block_size> ]\n", basename(path));
        fprintf(stderr, "    [ -g <blocks per group> ] [ -i <inodes> ] [ -I <inode size> ]\n");
        fprintf(stderr, "    [ -L <label> ] [ -f ] [ -a <android mountpoint> ]\n");
        fprintf(stderr, "    [ -z | -s ] [ -t ] [ -w ] [ -c ] [ -J ]\n");
        fprintf(stderr, "    <filename> [<directory>]\n");
}

int main(int argc, char **argv)
{
        int opt;
        const char *filename = NULL;
        const char *directory = NULL;
        char *mountpoint = "";
        int android = 0;
        int gzip = 0;
        int sparse = 0;
        int crc = 0;
        int wipe = 0;
        int init_itabs = 0;

        while ((opt = getopt(argc, argv, "l:j:b:g:i:I:L:a:fwzJsct")) != -1) {
                switch (opt) {
                case 'l':
                        info.len = parse_num(optarg);
                        break;
                case 'j':
                        info.journal_blocks = parse_num(optarg);
                        break;
                case 'b':
                        info.block_size = parse_num(optarg);
                        break;
                case 'g':
                        info.blocks_per_group = parse_num(optarg);
                        break;
                case 'i':
                        info.inodes = parse_num(optarg);
                        break;
                case 'I':
                        info.inode_size = parse_num(optarg);
                        break;
                case 'L':
                        info.label = optarg;
                        break;
                case 'f':
                        force = 1;
                        break;
                case 'a':
                        android = 1;
                        mountpoint = optarg;
                        break;
                case 'w':
                        wipe = 1;
                        break;
                case 'z':
                        gzip = 1;
                        break;
		case 'J':
			info.no_journal = 1;
			break;
		case 'c':
			crc = 1;
			break;
                case 's':
                        sparse = 1;
                        break;
                case 't':
                        init_itabs = 1;
                        break;
                default: /* '?' */
                        usage(argv[0]);
                        exit(EXIT_FAILURE);
                }
        }

	if (gzip && sparse) {
                fprintf(stderr, "Cannot specify both gzip and sparse\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
	}

        if (wipe && sparse) {
                fprintf(stderr, "Cannot specifiy both wipe and sparse\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        if (wipe && gzip) {
                fprintf(stderr, "Cannot specifiy both wipe and gzip\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        if (optind >= argc) {
                fprintf(stderr, "Expected filename after options\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        filename = argv[optind++];

        if (optind < argc)
                directory = argv[optind++];

        if (optind < argc) {
                fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        return make_ext4fs_internal(filename, directory, mountpoint, android, gzip,
                       sparse, crc, wipe, init_itabs);
}
