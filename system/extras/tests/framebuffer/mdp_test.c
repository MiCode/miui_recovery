/*
 * Copyright (C) 2007 Google Inc.
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/msm_mdp.h>

static struct fb_var_screeninfo vi;

static int open_file(char *name, int *fd, int *len, int *fmt)
{
    struct stat stat;
    char *type, *fn;

    type = name;
    fn = strchr(name, ':');
    if (!fn)
        return -1;
    *(fn++) = '\0';

    if (!strncmp(type, "yuv420", 6))
        *fmt = MDP_Y_CBCR_H2V2;
    else if (!strncmp(type, "rgb565", 6))
        *fmt = MDP_RGB_565;
    else {
        fprintf(stderr, "Unsupported image type: %s\n", type);
        return -1;
    }

    *fd = open(fn, O_RDONLY);
    if (*fd < 0) {
        perror("cannot open file");
        return -1;
    }

    if (fstat(*fd, &stat) < 0) {
        perror("cannot fstat file");
        goto err;
    }

    *len = stat.st_size;

    printf("Successfully opened file %s (fmt=%d len=%d fd=%d)\n", fn, *fmt,
           *len, *fd);
    return 0;

err:
    close(*fd);
    return -1;
}

static int get_pmem(int *fd, void **data, int sz)
{
    *fd = open("/dev/pmem", O_RDWR | O_NONBLOCK | O_SYNC);
    if (*fd < 0) {
        perror("cannot open /dev/pmem");
        return -1;
    }

    sz = (sz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    *data = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (*data == MAP_FAILED) {
        perror("pmem mmap");
        goto err_pmem_mmap;
    }

    return 0;

err_pmem_mmap:
   close(*fd);
   return -1;
}

static int get_framebuffer(int *fd, char **fb, int *width, int *height)
{
    struct fb_fix_screeninfo fi;
    void *bits;

    *fd = open("/dev/graphics/fb0", O_RDWR);
    if(*fd < 0) {
        perror("cannot open fb0");
        return -1;
    }

    if(ioctl(*fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        return -1;
    }

    if(ioctl(*fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        return -1;
    }

    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if(bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        return -1;
    }

    *width = vi.xres;
    *height = vi.yres;
    *fb = bits;
    return 0;
}

static void set_active_framebuffer(int fd, unsigned n)
{

    if(n > 1) return;
    vi.yres_virtual = vi.yres * 2;
    vi.yoffset = n * vi.yres;
    if(ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        fprintf(stderr,"active fb swap failed!\n");
    }
}

/* geometry: WxH+X+Y */
int parse_geometry(char *geom, int *w, int *h, int *x, int *y)
{
    char *ptr;

    *w = *h = 0;

    if (!(ptr = strchr(geom, 'x')))
        return -1;
    *ptr = '\0';
    *w = atoi(geom);
    geom = ptr + 1;

    ptr = strchr(geom, '+');
    if (ptr)
        *ptr = '\0';
    *h = atoi(geom);
    if (!ptr)
        return 0;

    geom = ptr + 1;

    if (!x || !y || !(ptr = strchr(geom, '+')))
        return -1;
    *ptr = '\0';
    *x = atoi(geom);
    geom = ptr + 1;

    *y = atoi(geom);

    return 0;
}

int main(int argc, const char *argv[])
{
    int fb_fd, width, height;
    char* fb;
    struct mdp_blit_req_list *req_list;
    struct mdp_blit_req *req;
    int opt;
    int srcw = 0, srch = 0, dstw = 0, dsth = 0;
    int srcx = 0; int srcy = 0;
    int dstx = 10; int dsty = 10;
    int src_imgw = 0, src_imgh = 0, dst_imgw = 0, dst_imgh = 0;
    int from;
    int src_fmt;
    int dst_fmt = MDP_RGB_565;
    int src_fd = -1;
    void *src_data;

    req_list = malloc(sizeof(struct mdp_blit_req_list) +
                      sizeof(struct mdp_blit_req));
    req_list->count = 1;
    req = req_list->req;


    while ((opt = getopt(argc, argv, "s:d:f:t:u:v:")) != -1) {
        switch (opt) {
        case 's':
            if (parse_geometry(optarg, &srcw, &srch, &srcx, &srcy)) {
                fprintf(stderr, "Can't parse source\n");
                exit(-1);
            }
            printf("Got source: w=%d h=%d x=%d y=%d\n", srcw, srch, srcx, srcy);
            break;

        case 'd':
            if (parse_geometry(optarg, &dstw, &dsth, &dstx, &dsty)) {
                fprintf(stderr, "Can't parse dest\n");
                exit(-1);
            }
            printf("Got dest: w=%d h=%d x=%d y=%d\n", dstw, dsth, dstx, dsty);
            break;

        case 'u':
            if (parse_geometry(optarg, &src_imgw, &src_imgh, NULL, NULL)) {
                fprintf(stderr, "Can't parse src image size\n");
                exit(-1);
            }
            printf("Got src img sz: w=%d h=%d\n", src_imgw, src_imgh);
            break;

        case 'v':
            if (parse_geometry(optarg, &dst_imgw, &dst_imgh, NULL, NULL)) {
                fprintf(stderr, "Can't parse dst image size\n");
                exit(-1);
            }
            printf("Got dst img sz: w=%d h=%d\n", dst_imgw, dst_imgh);
            break;

        case 'f':
            {
                int file_fd;
                int file_len;
                int bytes;
                void *ptr;
                if (open_file(optarg, &file_fd, &file_len, &src_fmt) < 0) {
                    fprintf(stderr, "Can't open source file\n");
                    exit(-1);
                }

                if (get_pmem(&src_fd, &src_data, file_len) < 0) {
                    close(file_fd);
                    exit(-1);
                }

                ptr = src_data;
                while (file_len) {
                    bytes = read(file_fd, ptr, file_len);
                    if (bytes < 0) {
                        perror("Could not read data from file");
                        exit(-1);
                    }
                    file_len -= bytes;
                    ptr += bytes;
                }
            }
            break;

        case 't':
            if (!strncmp(optarg, "yuv420", 6))
                dst_fmt = MDP_Y_CBCR_H2V2;
#if 0
            else if (!strncmp(optarg, "rgb565", 6))
                dst_fmt = MDP_RGB_565;
#endif
            break;

        default:
            fprintf(stderr, "Usage: %s -s source -d dest\n", argv[0]);
            exit(-1);
        }
    }

    if (get_framebuffer(&fb_fd, &fb, &width, &height)) {
        printf("couldnt' get fb\n");
        return -1;
    }

    set_active_framebuffer(fb_fd, 0);

    if (!src_imgw || !src_imgh) {
        src_imgw = width;
        src_imgh = height;
    }

    if (!dst_imgw || !dst_imgh) {
        dst_imgw = width;
        dst_imgh = height;
    }

    if (src_fd < 0) {
        src_fd = fb_fd;
        src_fmt = MDP_RGB_565;
    }

    req->src.width = src_imgw;
    req->src.height = src_imgh;
    req->src.format = src_fmt;
    req->src.offset = 0;
    req->src.memory_id = src_fd;
    req->src_rect.x = srcx;
    req->src_rect.y = srcy;
    req->src_rect.w = srcw;
    req->src_rect.h = srch;

    req->dst.width = dst_imgw;
    req->dst.height = dst_imgh;
    req->dst.format = dst_fmt;
    req->dst.offset = 0;
    req->dst.memory_id = fb_fd;
    req->dst_rect.x = dstx;
    req->dst_rect.y = dsty;
    req->dst_rect.w = dstw;
    req->dst_rect.h = dsth;
    req->alpha = MDP_ALPHA_NOP;
    req->transp_mask = MDP_TRANSP_NOP;
//    req->flags = MDP_ROT_90;
    req->flags = MDP_ROT_NOP;

    if(ioctl(fb_fd, MSMFB_BLIT, req_list))
        fprintf(stderr, "crap, failed blit\n");

    printf("Done\n");
    return 0;
}
