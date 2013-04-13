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

/*
 *
 * @author Rama, Meka(v.meka@samsung.com)
           Sangwoo, Park(sw5771.park@samsung.com)
           Jamie, Oh (jung-min.oh@samsung.com)
 * @date   2011-07-28
 *
 */

#ifndef ANDROID_SEC_HWC_UTILS_H_
#define ANDROID_SEC_HWC_UTILS_H_
#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "linux/fb.h"
#include <linux/videodev.h>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include "s5p_fimc.h"
#include "sec_lcd.h"
#include "sec_format.h"
#include "sec_utils.h"
#include "hal_public.h"

#define GRALLOC_USAGE_PHYS_CONTIG GRALLOC_USAGE_PRIVATE_1

#define NUM_OF_WIN          (1)
#define NUM_OF_WIN_BUF      (3)
#define NUM_OF_MEM_OBJ      (1)
#define MAX_NUM_PLANES      (3)

#define MAX_RESIZING_RATIO_LIMIT  (63)

struct sec_rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

struct sec_img {
    uint32_t w;
    uint32_t h;
    uint32_t format;
    uint32_t base;
    uint32_t offset;
    int      mem_id;
    int      mem_type;
};

inline int SEC_MIN(int x, int y) {
    return ((x < y) ? x : y);
}

inline int SEC_MAX(int x, int y) {
    return ((x > y) ? x : y);
}

struct hwc_win_info_t {
    int        fd;
    int        size;
    sec_rect   rect_info;
    uint32_t   addr[NUM_OF_WIN_BUF];
    int        buf_index;
    int        power_state;
    int        blending;
    int        layer_index;
    uint32_t   layer_prev_buf;
    int        set_win_flag;
    int        status;
    int        vsync;

    struct fb_fix_screeninfo fix_info;
    struct fb_var_screeninfo var_info;
    struct fb_var_screeninfo lcd_info;
};

enum {
    HWC_WIN_FREE = 0,
    HWC_WIN_RESERVED,
};

enum {
    HWC_UNKNOWN_MEM_TYPE = 0,
    HWC_PHYS_MEM_TYPE,
    HWC_VIRT_MEM_TYPE,
};

struct hwc_context_t {
    hwc_composer_device_t     device;

    /* our private state goes below here */
    struct hwc_win_info_t     win[NUM_OF_WIN];
    struct fb_var_screeninfo  lcd_info;
    s5p_fimc_t                fimc;
    unsigned int              num_of_fb_layer;
    unsigned int              num_of_hwc_layer;
    unsigned int              num_of_fb_layer_prev;
};

int window_open(struct hwc_win_info_t *win, int id);
int window_close(struct hwc_win_info_t *win);
int window_set_pos(struct hwc_win_info_t *win);
int window_get_info(struct hwc_win_info_t *win);
int window_pan_display(struct hwc_win_info_t *win);
int window_show(struct hwc_win_info_t *win);
int window_hide(struct hwc_win_info_t *win);
int window_get_global_lcd_info(struct fb_var_screeninfo *lcd_info);

int createFimc(s5p_fimc_t *fimc);
int destroyFimc(s5p_fimc_t *fimc);
int runFimc(struct hwc_context_t *ctx,
            struct sec_img *src_img, struct sec_rect *src_rect,
            struct sec_img *dst_img, struct sec_rect *dst_rect,
            unsigned int *phyAddr,
            uint32_t transform);
#endif /* ANDROID_SEC_HWC_UTILS_H_*/

