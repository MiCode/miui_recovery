/*
 * Copyright (C) 2011 xiaomi MIUI ( http://xiaomi.com/ )
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
 * MIUI UI: Main MIUI UI Window
 *
 */
#include <sched.h>
#include "../miui_inter.h"

/***************************[ GLOBAL VARIABLES ]**************************/
static AC_CONFIG acfg_var;
static byte      on_dialog_window = 0;

/***************************[ CONFIG FUNCTIONS ]**************************/
AC_CONFIG * acfg(){ return &acfg_var; }
void acfg_reset_text(){
  snprintf(acfg_var.text_ok,64,"OK");
  snprintf(acfg_var.text_next,64,"Next >");
  snprintf(acfg_var.text_back,64,"< Back");
  snprintf(acfg_var.text_yes,64,"Yes");
  snprintf(acfg_var.text_no,64,"No");
  snprintf(acfg_var.text_about,64,"About");
  snprintf(acfg_var.text_calibrating,64,"Calibrating Tools");
  snprintf(acfg_var.text_quit,64,"Quit Installation");
  snprintf(acfg_var.text_quit_msg,128,"Are you sure to quit the installer?");
}
void acfg_init_ex(byte themeonly){
  acfg_var.winbg        = ag_rgb(0xf0,0xf0,0xf0);
  acfg_var.winbg_g      = ag_rgb(0xee,0xee,0xee);
  
  acfg_var.dialogbg     = acfg_var.winbg;
  acfg_var.dialogbg_g   = acfg_var.winbg_g;
  
  acfg_var.textbg       = ag_rgb(0xff,0xff,0xff);
  acfg_var.textfg       = ag_rgb(0x00,0x00,0x00);
  acfg_var.textfg_gray  = ag_rgb(0x88,0x88,0x88);
  acfg_var.winfg_gray   = acfg_var.textfg_gray;
  
  acfg_var.winfg        = acfg_var.textfg;
  acfg_var.dialogfg     = acfg_var.textfg;
  
  acfg_var.controlbg    = ag_rgb(0xcc,0xcc,0xcc);
  acfg_var.controlbg_g  = ag_rgb(0xaa,0xaa,0xaa);
  acfg_var.controlfg    = ag_rgb(0x44,0x44,0x44);
  
  acfg_var.selectbg     = ag_rgb(158,228,32);
  acfg_var.selectbg_g   = ag_rgb(76,120,14);
  acfg_var.selectfg     = ag_rgb(0xff,0xff,0xff);

  acfg_var.titlebg      = ag_rgb(0x44,0x44,0x44);
  acfg_var.titlebg_g    = ag_rgb(0x11,0x11,0x11);
  acfg_var.titlefg      = ag_rgb(0xff,0xff,0xff);

  acfg_var.dlgtitlebg   = acfg_var.titlebg;
  acfg_var.dlgtitlebg_g = acfg_var.titlebg_g;
  acfg_var.dlgtitlefg   = acfg_var.titlefg;

  acfg_var.navbg        = ag_rgb(0x66,0x66,0x66);
  acfg_var.navbg_g      = ag_rgb(0x33,0x33,0x33);

  acfg_var.scrollbar    = ag_rgb(0x66,0x66,0x66);

  acfg_var.border       = ag_rgb(0x99,0x99,0x99);
  acfg_var.border_g     = ag_rgb(0x66,0x66,0x66);

  acfg_var.progressglow = acfg_var.selectbg;

  acfg_var.winroundsz   = 4;
  acfg_var.roundsz      = 3;
  acfg_var.btnroundsz   = 2;
  acfg_var.fadeframes   = 5;

  memset(acfg_var.themename, 0x00, 64);

  acfg_var.input_filter = 0;
  acfg_var.sd_ext = 0;

  acfg_reset_text();

  snprintf(acfg_var.rom_name,128,MIUI_NAME);
  snprintf(acfg_var.rom_version,128,MIUI_VERSION);
  snprintf(acfg_var.rom_author,128,MIUI_BUILD_A);
  snprintf(acfg_var.rom_device,128,"Not Defined");
  snprintf(acfg_var.rom_date,128,MIUI_BUILD);
  snprintf(acfg_var.brightness_path, PATH_MAX, "/sys/class/leds/lcd-backlight/brightness");
  memset(acfg_var.lun_file, 0x00, PATH_MAX);


  acfg_var.ckey_up      = 0;
  acfg_var.ckey_down    = 0;
  acfg_var.ckey_select  = 0;
  acfg_var.ckey_back    = 0;
  acfg_var.ckey_menu    = 0;

  atheme_releaseall();
}
void acfg_init(){
  acfg_init_ex(0);
}

/***************************[ THEME ]**************************/
static char theme_name[MIUI_THEME_CNT][27]={
  "img.background",
  "img.titlebar",
  "img.navbar",
  "img.dialog",
  "img.dialog.titlebar",
  "img.progress",
  "img.prograss.fill",
  "img.selection",
  "img.selection.push",
  "img.button",
  "img.button.focus",
  "img.button.push",
  "img.checkbox",
  "img.checkbox.focus",
  "img.checkbox.push",
  "img.checkbox.on",
  "img.checkbox.on.focus",
  "img.checkbox.on.push",
  "img.radio",
  "img.radio.focus",
  "img.radio.push",
  "img.radio.on",
  "img.radio.on.focus",
  "img.radio.on.push"
};
void atheme_releaseall(){
  int i=0;
  for (i=0;i<MIUI_THEME_CNT;i++){
    if (acfg_var.theme[i]!=NULL){
      apng_close(acfg_var.theme[i]);
      free(acfg_var.theme[i]);
    }
    acfg_var.theme[i]   =NULL;
    acfg_var.theme_9p[i]=0;
  }
}
void atheme_release(char * key){
  int i=0;
  for (i=0;i<MIUI_THEME_CNT;i++){
    if (strcmp(theme_name[i],key)==0){
      if (acfg_var.theme[i]!=NULL){
        apng_close(acfg_var.theme[i]);
        free(acfg_var.theme[i]);
        acfg_var.theme[i]=NULL;
        acfg_var.theme_9p[i]=0;
      }
      return;
    }
  }
  return;
}
PNGCANVASP atheme_create(char * key, char * path){
  int id = atheme_id(key);
  if (id!=-1){
    PNGCANVAS * ap = malloc(sizeof(PNGCANVAS));
    if (apng_load(ap,path)){
      if (acfg_var.theme[id]!=NULL){
        apng_close(acfg_var.theme[id]);
        free(acfg_var.theme[id]);
        acfg_var.theme[id]=NULL;
        acfg_var.theme_9p[id]=0;
      }
      acfg_var.theme[id]  = ap;
      int ln = strlen(path)-1;
      acfg_var.theme_9p[id]=0;
      if (ln>2){
        if ((path[ln]=='9')&&(path[ln-1]=='.')){
          acfg_var.theme_9p[id]=1;
        }
      }
      return ap;
    }
    free(ap);
  }
  return NULL;
}
byte atheme_draw(char * key, CANVAS * _b, int x, int y, int w, int h){
  return atheme_id_draw(atheme_id(key),_b,x,y,w,h);
}
byte atheme_id_draw(int id, CANVAS * _b, int x, int y, int w, int h){
  if (id<0) return 0;
  if (id>=MIUI_THEME_CNT) return 0;
    
  if (acfg_var.theme[id]!=NULL){
    if (acfg_var.theme_9p[id]){
      return apng9_draw(_b,acfg_var.theme[id],x,y,w,h,NULL,1);
    }
    else{
      return apng_stretch(
        _b,
        acfg_var.theme[id],
        x,y,w,h,
        0,0,acfg_var.theme[id]->w,acfg_var.theme[id]->h);
    }
  }
  return 0;
}
PNGCANVASP atheme(char * key){
  int i=0;
  for (i=0;i<MIUI_THEME_CNT;i++){
    if (strcmp(theme_name[i],key)==0)
      return acfg_var.theme[i];
  }
  return NULL;
}
int atheme_id(char * key){
  int i=0;
  for (i=0;i<MIUI_THEME_CNT;i++){
    if (strcmp(theme_name[i],key)==0)
      return i;
  }
  return -1;
}
char * atheme_key(int id){
  if (id<0) return NULL;
  if (id>=MIUI_THEME_CNT) return NULL;
  return theme_name[id];
}


/***************************[ WINDOW FUNCTIONS ]**************************/
//-- CREATE WINDOW
AWINDOWP aw(CANVAS * bg){
  ag_setbusy();
  //sleep(4);
  //-- Create Window
  AWINDOWP win = (AWINDOWP) malloc(sizeof(AWINDOW));
  if (win==NULL) return NULL;
  
  //-- Create Canvas & Draw BG
  ag_canvas(&win->c,agw(),agh());
  ag_draw(&win->c,bg,0,0);
  
  //-- Initializing Variables
  win->bg           = bg;
  win->controls     = NULL;
  win->controln     = 0;
  win->threadnum    = 0;
  win->focusIndex   = -1;
  win->touchIndex   = -1;
  win->isActived    = 0;
  
  //-- RETURN
  return win;
}
void aw_set_on_dialog(byte d){
  on_dialog_window = d;
}
//-- DESTROY WINDOW
void aw_destroy(AWINDOWP win){
  ag_setbusy();
  
  //-- Set To Unactive
  win->isActived = 0;
  
  //-- Wait Thread To Closed
  int threadwait_n=0;
  while (win->threadnum>0){
    usleep(500);
    if (threadwait_n++>1000) break;
  }
  
  //-- Cleanup Controls
  if (win->controln>0){
    int i;
    ACONTROLP * controls = (ACONTROLP *) win->controls;
    for (i=win->controln-1;i>=0;i--){
      controls[i]->ondestroy((void*) controls[i]);
      free(controls[i]);
    }
    free(win->controls);
  }
  
  //-- Cleanup Window
  ag_ccanvas(&win->c);
  free(win);
}

//-- Add Control Into Window
void aw_add(AWINDOWP win,ACONTROLP ctl){
  if (win->controln>0){
    int i;
    void ** tmpctls   = win->controls;
    win->controls     = malloc( sizeof(ACONTROLP)*(win->controln+1) );
    for (i=0;i<win->controln;i++)
      win->controls[i]=tmpctls[i];
    win->controls[win->controln] = (void*) ctl;
    free(tmpctls);
  }
  else{
    win->controls    = malloc(sizeof(ACONTROLP));
    win->controls[0] = (void*) ctl;
  }
  win->controln++;
}

//-- Draw Window
void aw_draw(AWINDOWP win){
  if (!win->isActived) return;
  ag_draw(NULL,&win->c,0,0);
  ag_sync();
}

//-- Redraw Window & Controls
void aw_redraw(AWINDOWP win){
  if (!win->isActived) return;
  if (win->controln>0){
    int i;
    for (i=0;i<win->controln;i++){
      ACONTROLP ctl = (ACONTROLP) win->controls[i];
      if (ctl->ondraw!=NULL)
        ctl->ondraw(ctl);
    }
  }
  ag_draw(NULL,&win->c,0,0);
}

//-- Show Window
void aw_show(AWINDOWP win){
  win->threadnum    = 0;
  win->isActived    = 1;
  
  //-- Find First Focus
#if 1
  if (win->controln>0){
    int i;
    for (i=0;i<win->controln;i++){
      ACONTROLP ctl = (ACONTROLP) win->controls[i];
      if (ctl->onfocus!=NULL){
        if (ctl->onfocus(ctl)){
          win->focusIndex = i;
          break;
        }
      }
    }
  }  
#endif
  aw_redraw(win);
  ag_sync_fade(acfg_var.fadeframes);
}

//-- Post Message
void aw_post(dword msg){
  atouch_send_message(msg);
}

//-- Check Mouse Event
byte aw_touchoncontrol(ACONTROLP ctl, int x, int y){
  int wx  = ctl->x;
  int wx2 = wx+ctl->w;
  int wy  = ctl->y;
  int wy2 = wy+ctl->h;
  
  if ((x>=wx)&&(x<wx2)&&(y>=wy)&&(y<wy2))
    return 1;
  return 0;
}

//-- Set Focus
byte aw_setfocus(AWINDOWP win,ACONTROLP ctl){
  if (!win->isActived) return 0;
  int i;
  for (i=0;i<win->controln;i++){
    ACONTROLP fctl = (ACONTROLP) win->controls[i];
    if (fctl==ctl){
      if (fctl->onfocus!=NULL){
        if (fctl->onfocus(fctl)){
          int pf = win->focusIndex;
          win->focusIndex = i;
          if ((pf!=-1)&&(pf!=i)){
            ACONTROLP pctl = (ACONTROLP) win->controls[pf];
            pctl->onblur(pctl);
          }
          aw_draw(win);
          return 1;
        }
      }
    }
  }
  return 0;
}

//-- Dispatch Messages
dword aw_dispatch(AWINDOWP win){
  dword msg;
  int i;
  
  ui_clear_key_queue();
  while(1){
    //-- Wait For Event
    ATEV        atev;
    int action  =atouch_wait(&atev);
    
    //-- Reset Message Value
    msg         = aw_msg(0,0,0,0);
    
    //-- Check an Action Value
    switch (action){
      case ATEV_MESSAGE:{
        msg = atev.msg;
      }
      break;
      case ATEV_BACK:{
        if (!atev.d){
          msg = aw_msg(5,0,0,0);
        }
      }
      break;
      case ATEV_DOWN: case ATEV_RIGHT:
        if (!atev.d){
          if (win->focusIndex!=-1){
            ACONTROLP ctl = (ACONTROLP) win->controls[win->focusIndex];
            if (ctl->oninput!=NULL){
              msg = ctl->oninput((void*)ctl,action,&atev);
            }
            if (aw_gl(msg)==0){
              for (i=win->focusIndex+1;i<win->controln;i++){
                ACONTROLP fctl = (ACONTROLP) win->controls[i];
                if (fctl->onfocus!=NULL){
                  if (fctl->onfocus(fctl)){
                    win->focusIndex = i;
                    ctl->onblur(ctl);
                    aw_draw(win);
                    break;
                  }
                }
              }
            }
          }
        }
      break;
      case ATEV_UP: case ATEV_LEFT:
        if (!atev.d){
          if (win->focusIndex!=-1){
            ACONTROLP ctl = (ACONTROLP) win->controls[win->focusIndex];
            if (ctl->oninput!=NULL){
              msg = ctl->oninput((void*)ctl,action,&atev);
            }
            if (aw_gl(msg)==0){
              for (i=win->focusIndex-1;i>=0;i--){
                ACONTROLP fctl = (ACONTROLP) win->controls[i];
                if (fctl->onfocus!=NULL){
                  if (fctl->onfocus(fctl)){
                    win->focusIndex = i;
                    ctl->onblur(ctl);
                    aw_draw(win);
                    break;
                  }
                }
              }
            }
          }
        }
      break;
      case ATEV_MENU:
      case ATEV_SEARCH:
      case ATEV_HOME:
      case ATEV_SELECT:{
        if (win->focusIndex!=-1){
          ACONTROLP ctl = (ACONTROLP) win->controls[win->focusIndex];
          if (ctl->oninput!=NULL){
            msg = ctl->oninput((void*)ctl,action,&atev);
          }
        }
      }
      break;
      case ATEV_MOUSEDN:
      {
        if (win->controln>0){
          int i;
          for (i=win->controln-1;i>=0;i--){
            ACONTROLP ctl = (ACONTROLP) win->controls[i];
            if (aw_touchoncontrol(ctl,atev.x,atev.y)){
              if (ctl->oninput!=NULL){
                msg             = ctl->oninput((void*)ctl,action,&atev);
                win->touchIndex = i;
                break;
              }
            }
          }
        }
      }
      break;
      case ATEV_MOUSEUP:{
        if (win->touchIndex!=-1){
          ACONTROLP ctl = (ACONTROLP) win->controls[win->touchIndex];
          if (ctl->oninput!=NULL)
            msg             = ctl->oninput((void*)ctl,action,&atev);
          win->touchIndex   = -1;
        }
      }
      break;
      case ATEV_MOUSEMV:{
        if (win->touchIndex!=-1){
          ACONTROLP ctl = (ACONTROLP) win->controls[win->touchIndex];
          if (ctl->oninput!=NULL)
            msg             = ctl->oninput((void*)ctl,action,&atev);
        }
      }
      break;
    }
    
    if (aw_gd(msg)==1) aw_draw(win);
    if (aw_gm(msg)!=0) return msg;
  }
  return msg;
}
CANVAS * aw_muteparent(AWINDOWP win){
  if (win==NULL){
    //-- Set Temporary
    CANVAS * tmpbg = (CANVAS *) malloc(sizeof(CANVAS));
    ag_canvas(tmpbg,agw(),agh());
    ag_draw(tmpbg,agc(),0,0);
    return tmpbg;
  }
  else{
    win->isActived = 0;
    return NULL;
  }
}
void aw_unmuteparent(AWINDOWP win,CANVAS * p){
  if (win==NULL){
    if (p!=NULL){
      ag_draw(NULL,p,0,0);
      ag_sync_fade(acfg_var.fadeframes);
      ag_ccanvas(p);
      free(p);
    }
  }
  else{
    win->isActived = 1;
    ag_draw(NULL,&win->c,0,0);
    ag_sync_fade(acfg_var.fadeframes);
  }
}
void aw_textdialog(AWINDOWP parent,char * titlev,char * text,char * ok_text){
  
  // actext(hWin,txtX,txtY,txtW,txtH,text,0);
  CANVAS * tmpc = aw_muteparent(parent);
  //-- Set Mask
  on_dialog_window = 1;
  ag_rectopa(agc(),0,0,agw(),agh(),0x0000,180);
  ag_sync();
  
  char title[64];
  snprintf(title,64,"%s",titlev);
  
  int pad   = agdp()*4;
  int winW  = agw()-(pad*2);
  int txtW  = winW-(pad*2);
  int txtX  = pad*2;
  int btnH  = agdp()*20;
  int titW  = ag_txtwidth(title,1);
  int titH  = ag_fontheight(1) + (pad*2);
  
  PNGCANVASP winp = atheme("img.dialog");
  PNGCANVASP titp = atheme("img.dialog.titlebar");
  APNG9      winv;
  APNG9      titv;
  int vtitY = -1;
  int vpadB = pad;
  int vimgX = pad*2;
  if (titp!=NULL){
    if (apng9_calc(titp,&titv,1)){
      int tmptitH = titH - (pad*2);
      titH        = tmptitH + (titv.t+titv.b);
      vtitY       = titv.t;
    }
  }
  if (winp!=NULL){
    if (apng9_calc(winp,&winv,1)){
      txtW = winW - (winv.l+winv.r);
      txtX = pad  + (winv.l);
      vimgX= pad  + (winv.l);
      vpadB= winv.b;
    }
  }
  
  byte imgE = 0; int imgW = 0; int imgH = 0;
  int txtH    = agh()/2;
  int infH    = txtH;
  
  //-- Calculate Window Size & Position
  int winH    = titH + infH + btnH + (pad*2) + vpadB;
  
  int winX    = pad;
  int winY    = (agh()/2) - (winH/2);
  
  //-- Calculate Title Size & Position
  int titX    = (agw()/2) - (titW/2);
  int titY    = winY + pad;
  if (vtitY!=-1) titY = winY+vtitY;
  
  //-- Calculate Text Size & Position
  int infY    = winY + titH + pad;
  int txtY    = infY;
  
  //-- Calculate Button Size & Position
  int btnW    = winW / 2;
  int btnY    = infY+infH+pad;
  int btnX    = (agw()/2) - (btnW/2);
  
  //-- Initializing Canvas
  CANVAS alertbg;
  ag_canvas(&alertbg,agw(),agh());
  ag_draw(&alertbg,agc(),0,0);
  
  //-- Draw Window
  if (!atheme_draw("img.dialog", &alertbg, winX,winY,winW,winH)){
    ag_roundgrad(&alertbg,winX-1,winY-1,winW+2,winH+2,acfg_var.border,acfg_var.border_g,(acfg_var.roundsz*agdp())+1);
    ag_roundgrad(&alertbg,winX,winY,winW,winH,acfg_var.dialogbg,acfg_var.dialogbg_g,acfg_var.roundsz*agdp());
  }
  
  //-- Draw Title
  if (!atheme_draw("img.dialog.titlebar", &alertbg, winX,winY,winW,titH)){
    ag_roundgrad_ex(&alertbg,winX,winY,winW,titH,acfg_var.dlgtitlebg,acfg_var.dlgtitlebg_g,acfg_var.roundsz*agdp(),1,1,0,0);
  }
  
  ag_textf(&alertbg,titW,titX+1,titY+1,title,acfg_var.dlgtitlebg_g,1);
  ag_text(&alertbg,titW,titX,titY,title,acfg_var.dlgtitlefg,1);
  
  AWINDOWP hWin   = aw(&alertbg);
  actext(hWin,txtX,txtY,txtW,txtH,text,0);
  ACONTROLP okbtn=acbutton(hWin,btnX,btnY,btnW,btnH,(ok_text==NULL?acfg_var.text_ok:ok_text),0,5);
    
  aw_show(hWin);
  aw_setfocus(hWin,okbtn);
  byte ondispatch = 1;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 5: ondispatch = 0; break;
    }
  }
  aw_destroy(hWin);
  ag_ccanvas(&alertbg);
  on_dialog_window = 0;
  aw_unmuteparent(parent,tmpc);
}
void aw_alert(AWINDOWP parent,char * titlev,char * textv,char * img,char * ok_text){
  CANVAS * tmpc = aw_muteparent(parent);
  //-- Set Mask
  on_dialog_window = 1;
  ag_rectopa(agc(),0,0,agw(),agh(),0x0000,180);
  ag_sync();
  
  char title[32];
  char text[512];
  snprintf(title,32,"%s",titlev);
  snprintf(text,512,"%s",textv);
  
  int pad   = agdp()*4;
  int winW  = agw()-(pad*2);
  int txtW  = winW-(pad*2);
  int txtX  = pad*2;
  int btnH  = agdp()*20;
  int titW  = ag_txtwidth(title,1);
  int titH  = ag_fontheight(1) + (pad*2);
  
  PNGCANVASP winp = atheme("img.dialog");
  PNGCANVASP titp = atheme("img.dialog.titlebar");
  APNG9      winv;
  APNG9      titv;
  int vtitY = -1;
  int vpadB = -1;
  int vimgX = pad*2;
  if (titp!=NULL){
    if (apng9_calc(titp,&titv,1)){
      int tmptitH = titH - (pad*2);
      titH        = tmptitH + (titv.t+titv.b);
      vtitY       = titv.t;
    }
  }
  if (winp!=NULL){
    if (apng9_calc(winp,&winv,1)){
      txtW = winW - (winv.l+winv.r);
      txtX = pad  + (winv.l);
      vimgX= pad  + (winv.l);
      vpadB= winv.b;
    }
  }
  
  //-- Load Icon
  PNGCANVAS ap;
  byte imgE = 0; int imgW = 0; int imgH = 0;
  if (apng_load(&ap,img)){
    imgE      = 1;
    imgW      = min(ap.w,agdp()*30);
    imgH      = min(ap.h,agdp()*30);
    int imgA  = pad + imgW;
    txtX     += imgA;
    txtW     -= imgA;
  }
  
  int txtH    = ag_txtheight(txtW,text,0);
  int infH    = ((imgE)&&(txtH<imgH))?imgH:txtH;
    
  //-- Calculate Window Size & Position
  int winH    = titH + infH + btnH + (pad*3);
  if (vpadB!=-1){
    winH    = titH + infH + btnH + (pad*2) + vpadB;
  }
  
  int winX    = pad;
  int winY    = (agh()/2) - (winH/2);
  
  //-- Calculate Title Size & Position
  int titX    = (agw()/2) - (titW/2);
  int titY    = winY + pad;
  if (vtitY!=-1) titY = winY+vtitY;
  
  //-- Calculate Text Size & Position
  int infY    = winY + titH + pad;
  int txtY    = infY + ((infH - txtH) / 2);
  int imgY    = infY;
  
  //-- Calculate Button Size & Position
  int btnW    = winW / 2;
  int btnY    = infY+infH+pad;
  int btnX    = (agw()/2) - (btnW/2);
  
  //-- Initializing Canvas
  CANVAS alertbg;
  ag_canvas(&alertbg,agw(),agh());
  ag_draw(&alertbg,agc(),0,0);
  
  //-- Draw Window
  if (!atheme_draw("img.dialog", &alertbg, winX,winY,winW,winH)){
    ag_roundgrad(&alertbg,winX-1,winY-1,winW+2,winH+2,acfg_var.border,acfg_var.border_g,(acfg_var.roundsz*agdp())+1);
    ag_roundgrad(&alertbg,winX,winY,winW,winH,acfg_var.dialogbg,acfg_var.dialogbg_g,acfg_var.roundsz*agdp());
  }
  
  //-- Draw Title
  if (!atheme_draw("img.dialog.titlebar", &alertbg, winX,winY,winW,titH)){
    ag_roundgrad_ex(&alertbg,winX,winY,winW,titH,acfg_var.dlgtitlebg,acfg_var.dlgtitlebg_g,acfg_var.roundsz*agdp(),1,1,0,0);
  }
  
  ag_textf(&alertbg,titW,titX+1,titY+1,title,acfg_var.dlgtitlebg_g,1);
  ag_text(&alertbg,titW,titX,titY,title,acfg_var.dlgtitlefg,1);
  
  //-- Draw Image
  if (imgE){
    apng_draw_ex(&alertbg,&ap,vimgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }
  
  //-- Draw Text
  ag_textf(&alertbg,txtW,txtX+1,txtY+1,text,acfg_var.dialogbg,0);
  ag_text(&alertbg,txtW,txtX,txtY,text,acfg_var.dialogfg,0);
  
  AWINDOWP hWin   = aw(&alertbg);
  acbutton(hWin,btnX,btnY,btnW,btnH,(ok_text==NULL?acfg_var.text_ok:ok_text),0,5);
  aw_show(hWin);
  byte ondispatch = 1;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 5: ondispatch = 0; break;
    }
  }
  aw_destroy(hWin);
  ag_ccanvas(&alertbg);
  on_dialog_window = 0;
  aw_unmuteparent(parent,tmpc);
}
byte aw_confirm(AWINDOWP parent, char * titlev,char * textv,char * img,char * yes_text,char * no_text){
  CANVAS * tmpc = aw_muteparent(parent);
  //-- Set Mask
  on_dialog_window = 1;
  ag_rectopa(agc(),0,0,agw(),agh(),0x0000,180);
  ag_sync();
  
  char title[64];
  char text[512];
  snprintf(title,64,"%s",titlev);
  snprintf(text,512,"%s",textv);
  
  int pad   = agdp()*4;
  int winW  = agw()-(pad*2);
  int txtW  = winW-(pad*2);
  int txtX  = pad*2;
  int btnH  = agdp()*20;
  int titW  = ag_txtwidth(title,1);
  int titH  = ag_fontheight(1) + (pad*2);
  
  PNGCANVASP winp = atheme("img.dialog");
  PNGCANVASP titp = atheme("img.dialog.titlebar");
  APNG9      winv;
  APNG9      titv;
  int vtitY = -1;
  int vpadB = -1;
  int vimgX = pad*2;
  if (titp!=NULL){
    if (apng9_calc(titp,&titv,1)){
      int tmptitH = titH - (pad*2);
      titH        = tmptitH + (titv.t+titv.b);
      vtitY       = titv.t;
    }
  }
  if (winp!=NULL){
    if (apng9_calc(winp,&winv,1)){
      txtW = winW - (winv.l+winv.r);
      txtX = pad  + (winv.l);
      vimgX= pad  + (winv.l);
      vpadB= winv.b;
    }
  }
  
  //-- Load Icon
  PNGCANVAS ap;
  byte imgE = 0; int imgW = 0; int imgH = 0;
  if (apng_load(&ap,img)){
    imgE      = 1;
    imgW      = min(ap.w,agdp()*30);
    imgH      = min(ap.h,agdp()*30);
    int imgA  = pad + imgW;
    txtX     += imgA;
    txtW     -= imgA;
  }
  
  int txtH    = ag_txtheight(txtW,text,0);
  int infH    = ((imgE)&&(txtH<imgH))?imgH:txtH;
    
  //-- Calculate Window Size & Position
  int winH    = titH + infH + btnH + (pad*3);
  if (vpadB!=-1){
    winH    = titH + infH + btnH + (pad*2) + vpadB;
  }
  int winX    = pad;
  int winY    = (agh()/2) - (winH/2);
  
  //-- Calculate Title Size & Position
  int titX    = (agw()/2) - (titW/2);
  int titY    = winY + pad;
  if (vtitY!=-1) titY = winY+vtitY;

  //-- Calculate Text Size & Position
  int infY    = winY + titH + pad;
  int txtY    = infY + ((infH - txtH) / 2);
  int imgY    = infY;
  
  //-- Calculate Button Size & Position
  int btnW    = (txtW / 2) - (pad/2);
  int btnY    = infY+infH+pad;
  int btnX    = txtX;
  int btnX2   = txtX+(txtW/2)+(pad/2);
  
  //-- Initializing Canvas
  CANVAS alertbg;
  ag_canvas(&alertbg,agw(),agh());
  ag_draw(&alertbg,agc(),0,0);
  
  //-- Draw Window
  if (!atheme_draw("img.dialog", &alertbg, winX-1,winY-1,winW+2,winH+2)){
    ag_roundgrad(&alertbg,winX-1,winY-1,winW+2,winH+2,acfg_var.border,acfg_var.border_g,(acfg_var.roundsz*agdp())+1);
    ag_roundgrad(&alertbg,winX,winY,winW,winH,acfg_var.dialogbg,acfg_var.dialogbg_g,acfg_var.roundsz*agdp());
  }
  
  //-- Draw Title
  if (!atheme_draw("img.dialog.titlebar", &alertbg, winX,winY,winW,titH)){
    ag_roundgrad_ex(&alertbg,winX,winY,winW,titH,acfg_var.dlgtitlebg,acfg_var.dlgtitlebg_g,acfg_var.roundsz*agdp(),1,1,0,0);
  }
  ag_textf(&alertbg,titW,titX+1,titY+1,title,acfg_var.dlgtitlebg_g,1);
  ag_text(&alertbg,titW,titX,titY,title,acfg_var.dlgtitlefg,1);
  
  //-- Draw Image
  if (imgE){
    apng_draw_ex(&alertbg,&ap,vimgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }
  
  //-- Draw Text
  ag_textf(&alertbg,txtW,txtX+1,txtY+1,text,acfg_var.dialogbg,0);
  ag_text(&alertbg,txtW,txtX,txtY,text,acfg_var.dialogfg,0);
  
  AWINDOWP hWin   = aw(&alertbg);
  
  acbutton(hWin,btnX,btnY,btnW,btnH,(yes_text==NULL?acfg_var.text_yes:yes_text),0,6);
  acbutton(hWin,btnX2,btnY,btnW,btnH,(no_text==NULL?acfg_var.text_no:no_text),0,5);
      
  aw_show(hWin);
  byte ondispatch = 1;
  byte res = 0;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6: res=1; ondispatch = 0; break;
      case 5: ondispatch = 0; break;
    }
  }
  aw_destroy(hWin);
  ag_ccanvas(&alertbg);
  on_dialog_window = 0;
  aw_unmuteparent(parent,tmpc);
  return res;
}

