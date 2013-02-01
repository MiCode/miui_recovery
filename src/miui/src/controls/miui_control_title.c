/*
 * Copyright (C) 2012 xiaomi MIUI ( http://www.micode.net/ )
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
 * Descriptions:
 * -------------
 * MIUI UI: title Window Control
 *
 */
#include "../miui_inter.h"

/***************************[ BACK ]**************************/
static const char * image_back = "@title.back";
static const char * image_back_pushed = "@title.back.pushed";
static const char * image_back_focused = "@title.back.focused";
typedef struct{
  char titlep[64];
  int h;
  int w;
  
  int ty;
  int th;


  CANVAS    control;
  CANVAS    control_pushed;
  CANVAS    control_focused;
  byte      touchmsg;
  byte      focused;
  byte      pushed;
} ACBACKD, * ACBACKDP;

static byte actitle_touchoncontrol(void *win, int x, int y) {
    ACONTROLP ctl  = (ACONTROLP) win;
    ACBACKDP  d  = (ACBACKDP) ctl->d;
    int wx = ctl->x;
    int wx2 = ctl->x + d->w;
    int wy = ctl->y;
    int wy2 = ctl->y + d->h;
    if ((x >= wx) && (x < wx2) && (y >= wy) && (y < wy2))
        return 1;
    return 0;
}

dword actitle_oninput(void * x,int action,ATEV * atev){
  ACONTROLP ctl  = (ACONTROLP) x;
  ACBACKDP  d  = (ACBACKDP) ctl->d;
  
#ifdef DEBUG
  miui_debug("*****%s entry******\n", __FUNCTION__);
#endif
  dword msg = 0;
  if (atev->k != 888 || actitle_touchoncontrol(x, atev->x, atev->y)){
      switch (action){
        case ATEV_MOUSEDN:
          {
            vibrate(30);
            d->pushed=1;
            msg=aw_msg(0,1,0,0);
            ctl->ondraw(ctl);
          }
          break;
        case ATEV_MOUSEUP:
          {
            d->pushed=0;
#ifdef DEBUG
            miui_debug("touch MOUSEUP in %s\n", __FUNCTION__);
#endif
            msg=aw_msg(d->touchmsg,1,0,0);
            ctl->ondraw(ctl);
          }
          break;
        case ATEV_SELECT:
          {
            if (atev->d){
              vibrate(30);
              d->pushed=1;
              msg=aw_msg(0,1,0,0);
              ctl->ondraw(ctl);
            }
            else{
              d->pushed=0;
              msg=aw_msg(d->touchmsg,1,0,0);
              ctl->ondraw(ctl);
            }
          }
          break;
      }
  } else 
      msg = aw_msg(0, 1, 0, 0);
  return msg;
}
void actitle_ondraw(void * x){
  ACONTROLP   ctl= (ACONTROLP) x;
  ACBACKDP  d  = (ACBACKDP) ctl->d;
  CANVAS *  pc = &ctl->win->c;
  
  if (d->pushed){
    ag_draw(pc,&d->control_pushed,ctl->x,ctl->y);
  }
  else if(d->focused) {
    ag_draw(pc,&d->control_focused,ctl->x,ctl->y);
  }
  else
    ag_draw(pc,&d->control,ctl->x,ctl->y);
}
void actitle_ondestroy(void * x){
  ACONTROLP   ctl= (ACONTROLP) x;
  ACBACKDP  d  = (ACBACKDP) ctl->d;
  ag_ccanvas(&d->control);
  ag_ccanvas(&d->control_pushed);
  ag_ccanvas(&d->control_focused);
  free(ctl->d);
}
byte actitle_onfocus(void * x){
  ACONTROLP   ctl= (ACONTROLP) x;
  ACBACKDP  d  = (ACBACKDP) ctl->d;
  
  d->focused=1;
  ctl->ondraw(ctl);
  return 1;
}
void actitle_onblur(void * x){
  ACONTROLP   ctl= (ACONTROLP) x;
  ACBACKDP  d  = (ACBACKDP) ctl->d;
  
  d->focused=0;
  ctl->ondraw(ctl);
}
ACONTROLP actitle(
  AWINDOWP win,
  int x,
  int y,
  int w,
  int *ph,
  char * text,
  byte isbig,
  byte touchmsg
){
  //-- Validate Minimum Size
  if (*ph<agdp()*16) *ph=agdp()*16;
  if (w<agdp()*16) w=agdp()*16;
  
  //-- Initializing Text Metrics
  int txtw     = ag_txtwidth(text,isbig);
  int txth     = ag_fontheight(isbig);
  
  //-- Initializing Button Data
  ACBACKDP d = (ACBACKDP) malloc(sizeof(ACBACKD));
  memset(d,0,sizeof(ACBACKD));
  
  
  PNGCANVAS * image = (PNGCANVAS*)malloc(sizeof(PNGCANVAS));
  int imgW = 0;
  int imgH = 0;
  //normal
  memset(image, 0, sizeof(PNGCANVAS));
  if (!apng_load(image, image_back)){
      miui_printf("load %s failed\n", image_back);
      goto apng_load_fail;
  } 
  imgW = image->w;
  imgH = image->h;
  *ph = (txth > imgH)?txth:imgH;
  int h = *ph;
  int txtx     = imgW + agdp() *8;
  int txty     = round(h/2) - round(txth/2);
  //-- Initializing Canvas
  ag_canvas(&d->control,w,h);
  ag_canvas(&d->control_pushed,w,h);
  ag_canvas(&d->control_focused,w,h);
  //normal 
  ag_rect(&d->control, 0, 0, w, h, acfg()->textbg);
  ag_rect(&d->control_pushed, 0, 0, w, h, acfg()->textbg);
  ag_rect(&d->control_focused, 0, 0, w, h, acfg()->textbg);
  apng_draw_ex(&d->control, image, 0, 0, 0, 0, imgW, imgH);
  //focused
  memset(image, 0, sizeof(PNGCANVAS));
  if (!apng_load(image, image_back_focused)){
      miui_debug("load %s failed\n", image_back_focused);
      goto apng_load_fail;
  } 
  imgW = image->w;
  imgH = image->h;
  apng_draw_ex(&d->control_focused, image,  0, 0, 0, 0, imgW, imgH); 
  //pushed 
  memset(image, 0, sizeof(PNGCANVAS));
  if (!apng_load(image, image_back_pushed)){
      miui_debug("load %s failed\n", image_back_pushed);
      goto apng_load_fail;
  } 
  imgW = image->w;
  imgH = image->h;
  apng_draw_ex(&d->control_pushed, image, 0, 0, 0, 0, imgW, imgH); 
apng_load_fail:
  free(image);
  image = NULL;
  //normal
  //
  //-- Save Touch Message & Set Stats
  d->touchmsg  = touchmsg;
  d->focused   = 0;
  d->pushed    = 0;
  //init th , ty id h,w
  d->th = h;
  d->ty = y;
  d->h = imgH;
  d->w = imgW;
  ag_textf(&d->control, txtw, txtx+1, txty+1, text, acfg()->winbg, isbig);
  ag_text(&d->control, txtw, txtx, txty, text, acfg()->winfg, isbig);
  //focused
  ag_textf(&d->control_focused, txtw, txtx+1, txty+1, text, acfg()->winbg, isbig);
  ag_text(&d->control_focused, txtw, txtx, txty, text, acfg()->winfg, isbig);
  //pushed
  ag_textf(&d->control_pushed, txtw, txtx+1, txty+1, text, acfg()->winbg, isbig);
  ag_text(&d->control_pushed, txtw, txtx, txty, text, acfg()->winfg, isbig);
  //-- Initializing Control
  ACONTROLP ctl  = malloc(sizeof(ACONTROL));
  ctl->ondestroy= &actitle_ondestroy;
  ctl->oninput  = &actitle_oninput;
  ctl->ondraw   = &actitle_ondraw;
  ctl->onblur   = &actitle_onblur;
  ctl->onfocus  = &actitle_onfocus;
  ctl->win      = win;
  ctl->x        = x;
  ctl->y        = y;
  ctl->w        = w;
  ctl->h        = h;
  ctl->forceNS  = 0;
  ctl->d        = (void *) d;
  aw_add(win,ctl);
  return ctl;
}
