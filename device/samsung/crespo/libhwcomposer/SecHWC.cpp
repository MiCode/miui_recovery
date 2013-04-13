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
           Jamie Oh (jung-min.oh@samsung.com)
 * @date   2011-07-28
 *
 */

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include "SecHWCUtils.h"

static IMG_gralloc_module_public_t *gpsGrallocModule;

static int hwc_device_open(const struct hw_module_t* module, const char* name,
                           struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Samsung S5PC11X hwcomposer module",
        author: "SAMSUNG",
        methods: &hwc_module_methods,
    }
};

static void dump_layer(hwc_layer_t const* l) {
    LOGD("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform, l->blending,
            l->sourceCrop.left,
            l->sourceCrop.top,
            l->sourceCrop.right,
            l->sourceCrop.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);
}

static int set_src_dst_info(hwc_layer_t *cur,
                            struct hwc_win_info_t *win,
                            struct sec_img *src_img,
                            struct sec_img *dst_img,
                            struct sec_rect *src_rect,
                            struct sec_rect *dst_rect,
                            int win_idx)
{
    IMG_native_handle_t *prev_handle = (IMG_native_handle_t *)(cur->handle);

    // set src image
    src_img->w       = prev_handle->iWidth;
    src_img->h       = prev_handle->iHeight;
    src_img->format  = prev_handle->iFormat;
    src_img->base    = NULL;
    src_img->offset  = 0;
    src_img->mem_id  =0;

    src_img->mem_type = HWC_PHYS_MEM_TYPE;
    src_img->w = (src_img->w + 15) & (~15);
    src_img->h = (src_img->h + 1) & (~1) ;

    //set src rect
    src_rect->x = SEC_MAX(cur->sourceCrop.left, 0);
    src_rect->y = SEC_MAX(cur->sourceCrop.top, 0);
    src_rect->w = SEC_MAX(cur->sourceCrop.right - cur->sourceCrop.left, 0);
    src_rect->w = SEC_MIN(src_rect->w, src_img->w - src_rect->x);
    src_rect->h = SEC_MAX(cur->sourceCrop.bottom - cur->sourceCrop.top, 0);
    src_rect->h = SEC_MIN(src_rect->h, src_img->h - src_rect->y);

    //set dst image
    dst_img->w = win->lcd_info.xres;
    dst_img->h = win->lcd_info.yres;

    switch (win->lcd_info.bits_per_pixel) {
    case 32:
        dst_img->format = HAL_PIXEL_FORMAT_RGBX_8888;
        break;
    default:
        dst_img->format = HAL_PIXEL_FORMAT_RGB_565;
        break;
    }

    dst_img->base     = win->addr[win->buf_index];
    dst_img->offset   = 0;
    dst_img->mem_id   = 0;
    dst_img->mem_type = HWC_PHYS_MEM_TYPE;

    //set dst rect
    //fimc dst image will be stored from left top corner
    dst_rect->x = 0;
    dst_rect->y = 0;
    dst_rect->w = win->rect_info.w;
    dst_rect->h = win->rect_info.h;

    LOGV("%s::sr_x %d sr_y %d sr_w %d sr_h %d dr_x %d dr_y %d dr_w %d dr_h %d ",
            __func__, src_rect->x, src_rect->y, src_rect->w, src_rect->h,
            dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);

    return 0;
}

static int get_hwc_compos_decision(hwc_layer_t* cur)
{
    if(cur->flags & HWC_SKIP_LAYER || !cur->handle) {
        LOGV("%s::is_skip_layer %d cur->handle %x",
                __func__, cur->flags & HWC_SKIP_LAYER, (uint32_t)cur->handle);
        return HWC_FRAMEBUFFER;
    }

    IMG_native_handle_t *prev_handle = (IMG_native_handle_t *)(cur->handle);
    int compositionType = HWC_FRAMEBUFFER;

    /* check here....if we have any resolution constraints */
    if (((cur->sourceCrop.right - cur->sourceCrop.left) < 16) ||
        ((cur->sourceCrop.bottom - cur->sourceCrop.top) < 8))
        return compositionType;

    if ((cur->transform == HAL_TRANSFORM_ROT_90) ||
        (cur->transform == HAL_TRANSFORM_ROT_270)) {
        if(((cur->displayFrame.right - cur->displayFrame.left) < 4)||
           ((cur->displayFrame.bottom - cur->displayFrame.top) < 8))
            return compositionType;
        } else if (((cur->displayFrame.right - cur->displayFrame.left) < 8) ||
                   ((cur->displayFrame.bottom - cur->displayFrame.top) < 4))
         return compositionType;

    if((prev_handle->usage & GRALLOC_USAGE_PHYS_CONTIG) &&
       (cur->blending == HWC_BLENDING_NONE))
        compositionType = HWC_OVERLAY;
    else
        compositionType = HWC_FRAMEBUFFER;

    LOGV("%s::compositionType %d bpp %d format %x usage %x",
            __func__,compositionType, prev_handle->uiBpp, prev_handle->iFormat,
            prev_handle->usage & GRALLOC_USAGE_PHYS_CONTIG);

    return  compositionType;
}

static int assign_overlay_window(struct hwc_context_t *ctx,
                                 hwc_layer_t *cur,
                                 int win_idx,
                                 int layer_idx)
{
    struct hwc_win_info_t *win;
    sec_rect rect;
    int ret = 0;

    if(NUM_OF_WIN <= win_idx)
        return -1;

    win = &ctx->win[win_idx];

    rect.x = SEC_MAX(cur->displayFrame.left, 0);
    rect.y = SEC_MAX(cur->displayFrame.top, 0);
    rect.w = SEC_MIN(cur->displayFrame.right - rect.x, win->lcd_info.xres - rect.x);
    rect.h = SEC_MIN(cur->displayFrame.bottom - rect.y, win->lcd_info.yres - rect.y);
    win->set_win_flag = 0;

    if((rect.x != win->rect_info.x) || (rect.y != win->rect_info.y) ||
       (rect.w != win->rect_info.w) || (rect.h != win->rect_info.h)){
            win->rect_info.x = rect.x;
            win->rect_info.y = rect.y;
            win->rect_info.w = rect.w;
            win->rect_info.h = rect.h;
            win->set_win_flag = 1;
            win->layer_prev_buf = 0;
    }

    win->layer_index = layer_idx;
    win->status = HWC_WIN_RESERVED;

    LOGV("%s:: win_x %d win_y %d win_w %d win_h %d lay_idx %d win_idx %d",
            __func__, win->rect_info.x, win->rect_info.y, win->rect_info.w,
            win->rect_info.h, win->layer_index, win_idx );

    return 0;
}

static void reset_win_rect_info(hwc_win_info_t *win)
{
    win->rect_info.x = 0;
    win->rect_info.y = 0;
    win->rect_info.w = 0;
    win->rect_info.h = 0;
    return;
}

static int hwc_prepare(hwc_composer_device_t *dev, hwc_layer_list_t* list)
{

    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int overlay_win_cnt = 0;
    int compositionType = 0;
    int ret;

    //if geometry is not changed, there is no need to do any work here
    if( !list || (!(list->flags & HWC_GEOMETRY_CHANGED)))
        return 0;

    //all the windows are free here....
    for (int i = 0; i < NUM_OF_WIN; i++) {
        ctx->win[i].status = HWC_WIN_FREE;
        ctx->win[i].buf_index = 0;
    }
    ctx->num_of_hwc_layer = 0;
    ctx->num_of_fb_layer = 0;
    LOGV("%s:: hwc_prepare list->numHwLayers %d", __func__, list->numHwLayers);

    for (int i = 0; i < list->numHwLayers ; i++) {
        hwc_layer_t* cur = &list->hwLayers[i];

        if (overlay_win_cnt < NUM_OF_WIN) {
            compositionType = get_hwc_compos_decision(cur);

            if (compositionType == HWC_FRAMEBUFFER) {
                cur->compositionType = HWC_FRAMEBUFFER;
                ctx->num_of_fb_layer++;
            } else {
                ret = assign_overlay_window(ctx, cur, overlay_win_cnt, i);
                if (ret != 0) {
                    cur->compositionType = HWC_FRAMEBUFFER;
                    ctx->num_of_fb_layer++;
                    continue;
                }

                cur->compositionType = HWC_OVERLAY;
                cur->hints = HWC_HINT_CLEAR_FB;
                overlay_win_cnt++;
                ctx->num_of_hwc_layer++;
            }
        } else {
            cur->compositionType = HWC_FRAMEBUFFER;
            ctx->num_of_fb_layer++;
        }
    }

    if(list->numHwLayers != (ctx->num_of_fb_layer + ctx->num_of_hwc_layer))
        LOGV("%s:: numHwLayers %d num_of_fb_layer %d num_of_hwc_layer %d ",
                __func__, list->numHwLayers, ctx->num_of_fb_layer,
                ctx->num_of_hwc_layer);

    if (overlay_win_cnt < NUM_OF_WIN) {
        //turn off the free windows
        for (int i = overlay_win_cnt; i < NUM_OF_WIN; i++) {
            window_hide(&ctx->win[i]);
            reset_win_rect_info(&ctx->win[i]);
        }
    }
    return 0;
}

static int hwc_set(hwc_composer_device_t *dev,
                   hwc_display_t dpy,
                   hwc_surface_t sur,
                   hwc_layer_list_t* list)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    unsigned int phyAddr[MAX_NUM_PLANES];
    int skipped_window_mask = 0;
    hwc_layer_t* cur;
    struct hwc_win_info_t *win;
    int ret;
    struct sec_img src_img;
    struct sec_img dst_img;
    struct sec_rect src_rect;
    struct sec_rect dst_rect;


    if (dpy == NULL && sur == NULL && list == NULL) {
        // release our resources, the screen is turning off
        // in our case, there is nothing to do.
        ctx->num_of_fb_layer_prev = 0;
        return 0;
    }

    bool need_swap_buffers = ctx->num_of_fb_layer > 0;

    /*
     * H/W composer documentation states:
     * There is an implicit layer containing opaque black
     * pixels behind all the layers in the list.
     * It is the responsibility of the hwcomposer module to make
     * sure black pixels are output (or blended from).
     *
     * Since we're using a blitter, we need to erase the frame-buffer when
     * switching to all-overlay mode.
     *
     */
    if (ctx->num_of_hwc_layer &&
        ctx->num_of_fb_layer==0 && ctx->num_of_fb_layer_prev) {
        /* we're clearing the screen using GLES here, this is very
         * hack-ish, ideal we would use the fimc (if it can do it) */
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        need_swap_buffers = true;
    }

    ctx->num_of_fb_layer_prev = ctx->num_of_fb_layer;

    if (need_swap_buffers || !list) {
        EGLBoolean sucess = eglSwapBuffers((EGLDisplay)dpy, (EGLSurface)sur);
        if (!sucess) {
            return HWC_EGL_ERROR;
        }
    }

    if (!list) {
        /* turn off the all windows */
        for (int i = 0; i < NUM_OF_WIN; i++) {
            window_hide(&ctx->win[i]);
            reset_win_rect_info(&ctx->win[i]);
            ctx->win[i].status = HWC_WIN_FREE;
        }
        ctx->num_of_hwc_layer = 0;
        return 0;
    }

    if(ctx->num_of_hwc_layer > NUM_OF_WIN)
        ctx->num_of_hwc_layer = NUM_OF_WIN;

    /* compose hardware layers here */
    for (uint32_t i = 0; i < ctx->num_of_hwc_layer; i++) {
        win = &ctx->win[i];
        if (win->status == HWC_WIN_RESERVED) {
            cur = &list->hwLayers[win->layer_index];

            if (cur->compositionType == HWC_OVERLAY) {

                ret = gpsGrallocModule->GetPhyAddrs(gpsGrallocModule,
                        cur->handle, phyAddr);
                if (ret) {
                    LOGE("%s::GetPhyAddrs fail : ret=%d\n", __func__, ret);
                    skipped_window_mask |= (1 << i);
                    continue;
                }

                /* initialize the src & dist context for fimc */
                set_src_dst_info (cur, win, &src_img, &dst_img, &src_rect,
                        &dst_rect, i);

                ret = runFimc(ctx, &src_img, &src_rect, &dst_img, &dst_rect,
                        phyAddr, cur->transform);
                if (ret < 0){
                   LOGE("%s::runFimc fail : ret=%d\n", __func__, ret);
                   skipped_window_mask |= (1 << i);
                   continue;
                }

                if (win->set_win_flag == 1) {
                    /* turnoff the window and set the window position with new conf... */
                    if (window_set_pos(win) < 0) {
                        LOGE("%s::window_set_pos is failed : %s", __func__,
                                strerror(errno));
                        skipped_window_mask |= (1 << i);
                        continue;
                    }
                    win->set_win_flag = 0;
                }

                /* is the frame didn't change, it needs to be composited
                 * because something else below it could have changed, however
                 * it doesn't need to be swapped.
                 */
                if (win->layer_prev_buf != (uint32_t)cur->handle) {
                    win->layer_prev_buf = (uint32_t)cur->handle;
                    window_pan_display(win);
                    win->buf_index = (win->buf_index + 1) % NUM_OF_WIN_BUF;
                }

                if(win->power_state == 0)
                    window_show(win);

            } else {
                LOGE("%s:: error : layer %d compositionType should have been \
                        HWC_OVERLAY", __func__, win->layer_index);
                skipped_window_mask |= (1 << i);
                continue;
            }
         } else {
             LOGE("%s:: error : window status should have been HWC_WIN_RESERVED \
                     by now... ", __func__);
             skipped_window_mask |= (1 << i);
             continue;
         }
    }

    if (skipped_window_mask) {
        //turn off the free windows
        for (int i = 0; i < NUM_OF_WIN; i++) {
            if (skipped_window_mask & (1 << i))
                window_hide(&ctx->win[i]);
        }
    }

    return 0;
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int ret = 0;
    int i;

    if (ctx) {
        if (destroyFimc(&ctx->fimc) < 0) {
            LOGE("%s::destroyFimc fail", __func__);
            ret = -1;
        }

        for (i = 0; i < NUM_OF_WIN; i++) {
            if (window_close(&ctx->win[i]) < 0) {
                LOGE("%s::window_close() fail", __func__);
                ret = -1;
            }
        }

        free(ctx);
    }
    return ret;
}

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = 0;
    struct hwc_win_info_t *win;

    if(hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
                (const hw_module_t**)&gpsGrallocModule))
        return -EINVAL;

    if(strcmp(gpsGrallocModule->base.common.author, "Imagination Technologies"))
        return -EINVAL;

    if (strcmp(name, HWC_HARDWARE_COMPOSER))
        return -EINVAL;

    struct hwc_context_t *dev;
    dev = (hwc_context_t*)malloc(sizeof(*dev));

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = 0;
    dev->device.common.module = const_cast<hw_module_t*>(module);
    dev->device.common.close = hwc_device_close;

    dev->device.prepare = hwc_prepare;
    dev->device.set = hwc_set;

    *device = &dev->device.common;

    /* initializing */
    memset(&(dev->fimc), 0, sizeof(s5p_fimc_t));
	dev->fimc.dev_fd = -1;

    /* open WIN0 & WIN1 here */
    for (int i = 0; i < NUM_OF_WIN; i++) {
        if (window_open(&(dev->win[i]), i) < 0) {
             LOGE("%s:: Failed to open window %d device ", __func__, i);
             status = -EINVAL;
             goto err;
        }
    }

    /* get default window config */
    if (window_get_global_lcd_info(&dev->lcd_info) < 0) {
        LOGE("%s::window_get_global_lcd_info is failed : %s",
				__func__, strerror(errno));
        status = -EINVAL;
        goto err;
    }

    dev->lcd_info.yres_virtual = dev->lcd_info.yres * NUM_OF_WIN_BUF;

    /* initialize the window context */
    for (int i = 0; i < NUM_OF_WIN; i++) {
        win = &dev->win[i];
        memcpy(&win->lcd_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));
        memcpy(&win->var_info, &dev->lcd_info, sizeof(struct fb_var_screeninfo));

        win->rect_info.x = 0;
        win->rect_info.y = 0;
        win->rect_info.w = win->var_info.xres;
        win->rect_info.h = win->var_info.yres;

        if (window_set_pos(win) < 0) {
            LOGE("%s::window_set_pos is failed : %s",
					__func__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        if (window_get_info(win) < 0) {
            LOGE("%s::window_get_info is failed : %s",
					__func__, strerror(errno));
            status = -EINVAL;
            goto err;
        }

        win->size = win->fix_info.line_length * win->var_info.yres;

        if (!win->fix_info.smem_start){
            LOGE("%s:: win-%d failed to get the reserved memory", __func__, i);
            status = -EINVAL;
            goto err;
        }

        for (int j = 0; j < NUM_OF_WIN_BUF; j++) {
            win->addr[j] = win->fix_info.smem_start + (win->size * j);
            LOGI("%s::win-%d add[%d] %x ", __func__, i, j, win->addr[j]);
        }
    }

    /* open pp */
    if (createFimc(&dev->fimc) < 0) {
        LOGE("%s::creatFimc() fail", __func__);
        status = -EINVAL;
        goto err;
    }

    LOGD("%s:: success\n", __func__);

    return 0;

err:
    if (destroyFimc(&dev->fimc) < 0)
        LOGE("%s::destroyFimc() fail", __func__);

    for (int i = 0; i < NUM_OF_WIN; i++) {
        if (window_close(&dev->win[i]) < 0)
            LOGE("%s::window_close() fail", __func__);
    }

    return status;
}
