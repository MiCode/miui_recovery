/*
 * Copyright (C) 2012 xiaomi MIUI ( http://www.xiaomi.com/ )
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
 * Source code for parsing and processing edify script (miui-config)
 *
 */

#include <sys/stat.h>       //-- Filesystem Stats
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "../edify/expr.h"

#include "../miui_inter.h"
#include "../miui.h"

#define APARSE_MAXHISTORY 256

//* 
//* GLOBAL UI VARIABLES
//* 
static  byte        miui_isbgredraw   = 0;
static  int         miui_minY                = 0;
static  CANVAS  miui_bg;                 //-- Saved CANVAS for background
static  CANVAS  miui_win_bg;             //-- Current drawed CANVAS for windows background


//* 
//* MACROS
//* 
#define MAX_FILE_GETPROP_SIZE    65536

/************************************[ MIUI INSTALLER UI - LIBRARIES ]************************************/


#define _INITARGS() \
    int args_i =0; \
    va_list arg_ptr; \
    char **args = (char**)malloc(argc * sizeof(char *)); \
    va_start(arg_ptr, format); \
    args[0] = format; \
    for (args_i = 1; args_i < argc; args_i++) \
        args[args_i] = va_arg(arg_ptr, char*);

#define _FREEARGS() \
    va_end(arg_ptr); \
    free(args);

static pthread_mutex_t title_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t redraw_mutex = PTHREAD_MUTEX_INITIALIZER;
STATUS miui_set_isbgredraw(int value)
{
    pthread_mutex_lock(&redraw_mutex);
    miui_isbgredraw = value;
    pthread_mutex_unlock(&redraw_mutex);
    return RET_OK;
}


//* 
//* Redraw Window Background
//* 
void miui_redraw(){
  if (!miui_isbgredraw) return;
  ag_blank(&miui_bg);
  int elmP  = agdp()*4;
  int capH  = ag_fontheight(0) + (elmP*2);
  miui_minY  = capH;

  ag_rect(&miui_bg,0,0,agw(),agh(),0x0000);
  //-- Background
  if (!atheme_id_draw(0, &miui_bg, 0, 0, agw(),agh())){
    ag_roundgrad(&miui_bg,0,0,agw(),agh(),acfg()->winbg,acfg()->winbg_g,acfg()->winroundsz*agdp()+2);
  }

  //-- Titlebar
  if (!atheme_id_draw(1, &miui_bg, 0, 0, agw(),capH)){
    ag_roundgrad_ex(&miui_bg,0,0,agw(),capH,acfg()->titlebg,acfg()->titlebg_g,(acfg()->winroundsz*agdp())-2,1,1,0,0);
  }
  
  miui_isbgredraw = 0;
}

//* 
//* Init Window Background With New Title
//* 
int miui_setbg(char * titlev){
  char title[64];
  snprintf(title,64,"%s",titlev);
  miui_redraw();
  int elmP  = agdp()*4;
  int titW  = ag_txtwidth(title,1);
  pthread_mutex_lock(&title_mutex);
  ag_draw(&miui_win_bg,&miui_bg,0,0);
  ag_textf(&miui_win_bg,titW,((agw()/2)-(titW/2))+1,elmP+1,title,acfg()->titlebg_g,1);
  ag_text(&miui_win_bg,titW,(agw()/2)-(titW/2),elmP,title,acfg()->titlefg,1);
  pthread_mutex_unlock(&title_mutex);
  return 2*elmP + ag_fontheight(1);
}
#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/battery/capacity"
#define BATTERY_CAPACITY_PATH_1 "/sys/class/power_supply/Battery/capacity"

static int read_from_file(const char* path, char* buf, size_t size) {
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        LOGE("Could not open '%s'", path);
        return -1;
    }

    size_t count = read(fd, buf, size);
    if (count > 0) {
        count = (count < size) ? count : size - 1;
        while (count > 0 && buf[count-1] == '\n') count--;
        buf[count] = '\0';
    } else {
        buf[0] = '\0';
    }

    close(fd);
    return count;
}

static int read_int(const char* path) {
    const int SIZE = 128;
    char buf[SIZE];
    int value = 0;

    if (read_from_file(path, buf, SIZE) > 0) {
        value = atoi(buf);
    }
    return value;
}
/*
static int _miui_setbg_title(CANVAS *win, CANVAS *bg) {
  pthread_mutex_lock(&title_mutex);
  static char bg_title[64];
  static time_t timep;
  static struct tm *p;
  time(&timep);
  p = gmtime(&timep);
  miui_redraw();
  int elmP  = agdp()*4;
  char *batt = alang_ams(BATT_NAME); 
  char *time_name = alang_ams(TIME_NAME);
  snprintf(bg_title, 64, "%s %s  %s:%d%% %s:%02d:%02d", MIUI_NAME, MIUI_VERSION, 
          batt, read_int(BATTERY_CAPACITY_PATH),
          time_name, (p->tm_hour + 8)%24, p->tm_min); 
  miui_debug("bg_title is %s\n", bg_title);
  int titW  = ag_txtwidth(bg_title,0);
  ag_draw(win, bg,0,0);
  ag_textf(win,titW,((agw()/2)-(titW/2))+1,elmP+1,bg_title,acfg()->titlebg_g,0);
  ag_text(win,titW,(agw()/2)-(titW/2),elmP,bg_title,acfg()->titlefg,0);
  pthread_mutex_unlock(&title_mutex);
  return 2*elmP + ag_fontheight(0);
}
*/

static int _miui_draw_battery(CANVAS *win, int x, int y, color fg, color bg)
{
    char batt_name[8];
    struct stat st;
    if (stat(BATTERY_CAPACITY_PATH, &st) >= 0)
        snprintf(batt_name, 8, "%2d", read_int(BATTERY_CAPACITY_PATH));
    else if (stat(BATTERY_CAPACITY_PATH_1, &st) >= 0)
        snprintf(batt_name, 8, "%2d", read_int(BATTERY_CAPACITY_PATH_1));
    else {
        miui_error("BATTERY_CAPACITY_PATH error\n"); 
		snprintf(batt_name, 8, "%2d", 0);
    }

    int txtX = x + 4;
    int txtY = y;
    int txtH = ag_fontheight(0);
    int txtW = ag_fontheight(0) * 2;
    int battW = 10*agdp();
    ag_rect(win, txtX - 2, y+1, battW, txtH-3, fg);
    ag_rect(win, txtX - 1, y+2, battW - 2, txtH-5, bg);
    txtX += agdp();
    ag_textf(win, txtW, txtX+1, txtY+1, batt_name, bg, 0);
    ag_textf(win, txtW, txtX, txtY, batt_name, fg, 0);
    int rectH = agdp() * 3;
    int rectW = agdp();
    txtY += (txtH - rectH)/2;
    ag_rect(win, txtX - 3* agdp(), txtY, rectW, rectH, fg);
    return txtH + 2*agdp();
}

static int _miui_setbg_title(CANVAS *win, CANVAS *bg) {
  pthread_mutex_lock(&title_mutex);
  static char bg_title[64];
  static time_t timep;
  static struct tm *p;
  time(&timep);
  p = gmtime(&timep);
  miui_redraw();
  int elmP  = agdp()*4;
  snprintf(bg_title, 64, "%s", MIUI_NAME); 
  miui_debug("bg_title is %s\n", bg_title);
  int titW  = ag_txtwidth(bg_title,0);
  ag_draw(win, bg,0,0);
  //draw title name
  ag_textf(win,titW,elmP + 1,elmP+1,bg_title,acfg()->titlebg_g,0);
  ag_text(win,titW,elmP,elmP,bg_title,acfg()->titlefg,0);
  //draw battery
  _miui_draw_battery(win, agw()/2 + 12*elmP, elmP, acfg()->titlefg, acfg()->titlebg_g);
  //draw time
  snprintf(bg_title, 64, "%02d:%02d", (p->tm_hour + 8) % 24, p->tm_min);
  titW = ag_txtwidth(bg_title, 0);
  int timeX = agw() - titW - elmP;
  ag_textf(win,titW,timeX + 1,elmP+1,bg_title,acfg()->titlebg_g,0);
  ag_text(win,titW,timeX,elmP,bg_title,acfg()->titlefg,0);
  pthread_mutex_unlock(&title_mutex);
  return 2*elmP + ag_fontheight(0);
}

int miui_setbg_title() {
    return _miui_setbg_title(&miui_win_bg, &miui_bg);
}
int miui_setbg_title_win(AWINDOWP win){
    return _miui_setbg_title(&win->c, win->bg);
}

int miui_set_title(char * titlev){
  char title[64];
  snprintf(title,64,"%s",titlev);
  int elmP  = agdp()*8 + agdp()*4 + ag_fontheight(0);
  int titW  = ag_txtwidth(title,0);
  ag_textf(&miui_win_bg,titW,((agw()/2)-(titW/2))+1,elmP+1,title,acfg()->winbg,0);
  ag_text(&miui_win_bg,titW,(agw()/2)-(titW/2),elmP,title,acfg()->winfg,0);
  return elmP + ag_fontheight(0) + agdp()*4;
}
//* 
//* Draw Navigation Bar
//*
void miui_drawnav(CANVAS * bg,int x, int y, int w, int h){
  if (!atheme_id_draw(2, bg, x, y, w, h)){
    ag_roundgrad_ex(
      bg,x,y,w,h,
      acfg()->navbg,
      acfg()->navbg_g,
      (acfg()->winroundsz*agdp())-2,0,0,1,1
    );
  }
}

//* 
//* Read Strings From filesystem
//* 
char * miui_readfromfs(char * name){
  char* buffer = NULL;
  struct stat st;
  if (stat(name,&st) < 0) return NULL;
  if (st.st_size>MAX_FILE_GETPROP_SIZE) return NULL;
  buffer = malloc(st.st_size+1);
  if (buffer == NULL) goto done;
  FILE* f = fopen(name, "rb");
  if (f == NULL) goto done;
  if (fread(buffer, 1, st.st_size, f) != st.st_size){
      fclose(f);
      goto done;
  }
  buffer[st.st_size] = '\0';
  fclose(f);
  return buffer;
done:
  free(buffer);
  return NULL;
}

//* 
//* Write Strings into file
//* 
void miui_writetofs(char * name, char * value){
  FILE * fp = fopen(name,"wb");
  if (fp!=NULL){
    fwrite(value,1,strlen(value),fp);
    fclose(fp);
  }
}

//* 
//* Read Strings From Temporary File
//*
char * miui_readfromtmp(char * name){
  char path[256];
  snprintf(path,256,"%s/%s",MIUI_TMP,name);
  return miui_readfromfs(path);
  
}

//* 
//* Write Strings From Temporary File
//*
void miui_writetotmp(char * name, char * value){
  char path[256];
  snprintf(path,256,"%s/%s",MIUI_TMP,name);
  miui_writetofs(path,value);
}

//* 
//* Read Strings From ZIP
//* 
char * miui_readfromzip(char * name){
  AZMEM filedata;
  if (!az_readmem(&filedata,name,0)) return NULL;
  return (char *)filedata.data;
}

//* 
//* Parse PROP String
//* 
static char * miui_parsepropstring(char * bf,char *key){
  char* result = NULL;  
  if (bf==NULL) return result;
  char* buffer=strdup(bf);
  char* line = strtok(buffer, "\n");
  do {
      while (*line && isspace(*line)) ++line;
      if (*line == '\0' || *line == '#') continue;
      char* equal = strchr(line, '=');
      if (equal == NULL) goto done;

      char* key_end = equal-1;
      while (key_end > line && isspace(*key_end)) --key_end;
      key_end[1] = '\0';

      if (strcmp(key, line) != 0) continue;

      char* val_start = equal+1;
      while(*val_start && isspace(*val_start)) ++val_start;

      char* val_end = val_start + strlen(val_start)-1;
      while (val_end > val_start && isspace(*val_end)) --val_end;
      val_end[1] = '\0';

      result = strdup(val_start);
      break;
  } while ((line = strtok(NULL, "\n")));
  free(buffer);
done:
  
  return result;
}

//* 
//* Parse PROP Files
//* 
char * miui_parseprop(char * filename,char *key){
  char * buffer = miui_readfromfs(filename);
  char * result = miui_parsepropstring(buffer,key);
  free(buffer);
  return result;
}

//* 
//* Parse PROP from ZIP
//* 
char * miui_parsepropzip(char * filename,char *key){
  char * buffer = miui_readfromzip(filename);
  char * result = miui_parsepropstring(buffer,key);
  free(buffer);
  return result;
}

//* 
//* Read Variable
//* 
char * miui_getvar(char * name){
  char path[256];
  snprintf(path,256,"%s/.__%s.var",MIUI_TMP,name);
  return miui_readfromfs(path);
}

//* 
//* Set Variable
//* 
void miui_setvar(char * name, char * value){
  char path[256];
  snprintf(path,256,"%s/.__%s.var",MIUI_TMP,name);
  miui_writetofs(path,value);
}

//* 
//* Append Variable
//* 
void miui_appendvar(char * name, char * value){
  char path[256];
  snprintf(path,256,"%s/.__%s.var",MIUI_TMP,name);
  FILE * fp = fopen(path,"ab");
  if (fp!=NULL){
    fwrite(value,1,strlen(value),fp);
    fclose(fp);
  }
}

//* 
//* Delete Variable
//* 
void miui_delvar(char * name){
  char path[256];
  snprintf(path,256,"%s/.__%s.var",MIUI_TMP,name);
  unlink(path);
}

//* 
//* Prepend Variable
//* 
void miui_prependvar(char * name, char * value){
  char path[256];
  snprintf(path,256,"%s/.__%s.var",MIUI_TMP,name);
  char * buf = miui_getvar(name);
  FILE * fp = fopen(path,"wb");
  if (fp!=NULL){
    fwrite(value,1,strlen(value),fp);
    if (buf!=NULL){
      fwrite(buf,1,strlen(buf),fp);
    }
    fclose(fp);
  }
  if (buf!=NULL){
    free(buf);
  }
}

//* 
//* Set Colorset From Prop String
//* 
void miui_setthemecolor(char * prop, char * key, color * cl){
  char * val = miui_parsepropstring(prop,key);
  if (val!=NULL){
    cl[0] = strtocolor(val);
    free(val);
  }
}
//* 
//* Set Drawing Config From Prop String
//* 
void miui_setthemeconfig(char * prop, char * key, byte * b){
  char * val = miui_parsepropstring(prop,key);
  if (val!=NULL){
    b[0] = (byte) min(atoi(val),255);
    free(val);
  }
}

/************************************[ MIUI EDIFY HANDLERS ]************************************/
//* 
//* loadtruefont
//*
STATUS miui_font(char *ttf_type, char *ttf_file, char *ttf_size){
  
  //-- This is Busy Function
  return_val_if_fail(ttf_type != NULL, RET_FAIL);
  return_val_if_fail(ttf_file != NULL, RET_FAIL);
  return_val_if_fail(ttf_size != NULL, RET_FAIL);
  ag_setbusy();
  
  //-- Get Arguments
  
  char zpath[256];
  snprintf(zpath,256,"%s/",MIUI_DIR);
  
  int size = atoi(ttf_size);
  if (ttf_type[0]=='0'){
    if (!ag_loadsmallfont(ttf_file, size, zpath))
      ag_loadsmallfont("fonts/small",0,NULL);
  }
  else{
    if (!ag_loadbigfont(ttf_file, size, zpath))
      ag_loadbigfont("fonts/big",0,NULL);
  }
  
  
  //-- Return
  return RET_OK;
  
}
//* 
//* set_theme
//* 
STATUS  miui_theme(char *theme) {
  return_val_if_fail(theme != NULL, RET_FAIL);
  
  //-- This is Busy Function
  ag_setbusy();
  
  if ((strcmp(theme,"")==0)||(strcmp(theme,"generic")==0)){
    //-- Background Should Be Redrawed
    miui_isbgredraw = 1;
    
    
    //-- Return
    return RET_OK;
  }

  //-- Parse The Prop
  char themename[256];
  snprintf(themename,256,"%s/themes/%s/theme.prop",MIUI_DIR,theme);
  snprintf(acfg()->themename,64,"%s",theme);
  char * propstr = miui_readfromzip(themename);
  if (propstr){
    int i=0;
    for (i=0;i<MIUI_THEME_CNT;i++){
      char * key = atheme_key(i);
      char * val = miui_parsepropstring(propstr,key);
      if (val!=NULL){
        if (strcmp(val,"")!=0){
          snprintf(themename,256,"themes/%s/%s",theme,val);
          atheme_create(key,themename);
        }
        free(val);
      }
    }
    //printf("PASS THEME\n");
    miui_setthemecolor(propstr,  "color.winbg",              &acfg()->winbg);
    miui_setthemecolor(propstr,  "color.winbg_g",            &acfg()->winbg_g);
    miui_setthemecolor(propstr,  "color.winfg",              &acfg()->winfg);
    miui_setthemecolor(propstr,  "color.winfg_gray",         &acfg()->winfg_gray);
    miui_setthemecolor(propstr,  "color.dialogbg",           &acfg()->dialogbg);
    miui_setthemecolor(propstr,  "color.dialogbg_g",         &acfg()->dialogbg_g);
    miui_setthemecolor(propstr,  "color.dialogfg",           &acfg()->dialogfg);
    miui_setthemecolor(propstr,  "color.textbg",             &acfg()->textbg);
    miui_setthemecolor(propstr,  "color.textfg",             &acfg()->textfg);
    miui_setthemecolor(propstr,  "color.textfg_gray",        &acfg()->textfg_gray);
    miui_setthemecolor(propstr,  "color.controlbg",          &acfg()->controlbg);
    miui_setthemecolor(propstr,  "color.controlbg_g",        &acfg()->controlbg_g);
    miui_setthemecolor(propstr,  "color.controlfg",          &acfg()->controlfg);
    miui_setthemecolor(propstr,  "color.selectbg",           &acfg()->selectbg);
    miui_setthemecolor(propstr,  "color.selectbg_g",         &acfg()->selectbg_g);
    miui_setthemecolor(propstr,  "color.selectfg",           &acfg()->selectfg);
    miui_setthemecolor(propstr,  "color.titlebg",            &acfg()->titlebg);
    miui_setthemecolor(propstr,  "color.titlebg_g",          &acfg()->titlebg_g);
    miui_setthemecolor(propstr,  "color.titlefg",            &acfg()->titlefg);
    miui_setthemecolor(propstr,  "color.dlgtitlebg",         &acfg()->dlgtitlebg);
    miui_setthemecolor(propstr,  "color.dlgtitlebg_g",       &acfg()->dlgtitlebg_g);
    miui_setthemecolor(propstr,  "color.dlgtitlefg",         &acfg()->dlgtitlefg);
    miui_setthemecolor(propstr,  "color.scrollbar",          &acfg()->scrollbar);
    miui_setthemecolor(propstr,  "color.navbg",              &acfg()->navbg);
    miui_setthemecolor(propstr,  "color.navbg_g",            &acfg()->navbg_g);
    miui_setthemecolor(propstr,  "color.border",             &acfg()->border);
    miui_setthemecolor(propstr,  "color.border_g",           &acfg()->border_g);
    miui_setthemecolor(propstr,  "color.progressglow",       &acfg()->progressglow);
    
    
    
    miui_setthemeconfig(propstr, "config.roundsize",         &acfg()->roundsz);
    miui_setthemeconfig(propstr, "config.button_roundsize",  &acfg()->btnroundsz);
    miui_setthemeconfig(propstr, "config.window_roundsize",  &acfg()->winroundsz);
    miui_setthemeconfig(propstr, "config.transition_frame",  &acfg()->fadeframes);
    
    //printf("PASS THEME V\n");
    
    
    free(propstr);
  }
  else{
    memset(acfg()->themename, 0x00, 64);
  }

  //-- Background Should Be Redrawed
  miui_isbgredraw = 1;
  
  
  //-- Return
  return RET_OK;
}

char * miui_getprop(char *file, char *key) {
  return_null_if_fail(file != NULL); 
  return_null_if_fail(key != NULL); 
  //-- This is Busy Function
  ag_setbusy();

  //-- Parse The Prop
  char* result;
  char path[256];
  snprintf(path,256,"%s/%s",MIUI_DIR,file);
  result = miui_parseprop(path,key);
  return result;
}
char * miui_gettmpprop(char *file, char *key) {
  return_null_if_fail(file != NULL); 
  return_null_if_fail(key != NULL); 
  //-- This is Busy Function
  ag_setbusy();

  //-- Parse The Prop
  char* result;
  char path[256];
  snprintf(path,256,"%s/%s",MIUI_TMP,file);
  result = miui_parseprop(path,key);
  return result;
}

//* 
//* resread, readfile_miui
//* 
char * miui_resread(char *file) {
  
  return_null_if_fail(file != NULL);
  //-- This is Busy Function
  ag_setbusy();
  
    
  //-- Create Path Into Resource Dir
  char path[256];
  snprintf(path,256,"%s/%s",MIUI_DIR,file);
  
  //-- Read From Zip
  char * buf = miui_readfromzip(path);
  
  
  //-- Return
  return buf;
}


//* 
//* pleasewait
//* 
STATUS miui_pleasewait(char *message) {
  
  
  return_val_if_fail(message != NULL, RET_FAIL);
  
  //-- Set Busy Text
  char txt[32];
  snprintf(txt, 32, "%s", message);
  ag_setbusy_withtext(txt);
  
  //-- Return
  return RET_OK;
}


STATUS miui_busy_process()
{
  //-- Set Busy before everythings ready
  ag_setbusy();
  miui_isbgredraw = 1;

  int chkH        = agh();
  int chkY        = miui_setbg_title();
  int chkW          = agw();
  char *text = "Please wait ....";

  chkH -= chkY;
  int big = 1;
  int txtW = ag_txtwidth(text, big);
  int txtH = ag_fontheight(big);
  int txtX = (agw()/2) - (txtW/2);
  int txtY = (agh()/2) - (txtH/2) - (agdp() *2);
  ag_rect(&miui_win_bg, 0, chkY, chkW, chkH, acfg()->titlebg_g);
  ag_textf(&miui_win_bg, txtW, txtX, txtY, text, acfg()->titlefg, big);
  ag_draw(NULL, &miui_win_bg, 0, 0);
  ag_sync();
  return RET_OK;
}



//* 
//* setcolor
//*
STATUS miui_setcolor(char *item, char *val) {
  return_val_if_fail(item != NULL, RET_FAIL);
  return_val_if_fail(val != NULL, RET_FAIL); 
  //-- This is Busy Function
  ag_setbusy();
  
  
  //-- Convert String into Color
  color cl = strtocolor(val);
  
  //-- Set Color Property
  if      (strcmp(item,"winbg") == 0)          acfg()->winbg=cl;
  else if (strcmp(item,"winbg_g") == 0)        acfg()->winbg_g=cl;
  else if (strcmp(item,"winfg") == 0)          acfg()->winfg=cl;
  else if (strcmp(item,"winfg_gray") == 0)     acfg()->winfg_gray=cl;
  else if (strcmp(item,"dialogbg") == 0)       acfg()->dialogbg=cl;
  else if (strcmp(item,"dialogbg_g") == 0)     acfg()->dialogbg_g=cl;
  else if (strcmp(item,"dialogfg") == 0)       acfg()->dialogfg=cl;
  else if (strcmp(item,"textbg") == 0)         acfg()->textbg=cl;
  else if (strcmp(item,"textfg") == 0)         acfg()->textfg=cl;
  else if (strcmp(item,"textfg_gray") == 0)    acfg()->textfg_gray=cl;
  else if (strcmp(item,"controlbg") == 0)      acfg()->controlbg=cl;
  else if (strcmp(item,"controlbg_g") == 0)    acfg()->controlbg_g=cl;
  else if (strcmp(item,"controlfg") == 0)      acfg()->controlfg=cl;
  else if (strcmp(item,"selectbg") == 0)       acfg()->selectbg=cl;
  else if (strcmp(item,"selectbg_g") == 0)     acfg()->selectbg_g=cl;
  else if (strcmp(item,"selectfg") == 0)       acfg()->selectfg=cl;
  else if (strcmp(item,"titlebg") == 0)        acfg()->titlebg=cl;
  else if (strcmp(item,"titlebg_g") == 0)      acfg()->titlebg_g=cl;
  else if (strcmp(item,"titlefg") == 0)        acfg()->titlefg=cl;
  else if (strcmp(item,"dlgtitlebg") == 0)     acfg()->dlgtitlebg=cl;
  else if (strcmp(item,"dlgtitlebg_g") == 0)   acfg()->dlgtitlebg_g=cl;
  else if (strcmp(item,"dlgtitlefg") == 0)     acfg()->dlgtitlefg=cl;
  else if (strcmp(item,"scrollbar") == 0)      acfg()->scrollbar=cl;
  else if (strcmp(item,"navbg") == 0)          acfg()->navbg=cl;
  else if (strcmp(item,"navbg_g") == 0)        acfg()->navbg_g=cl;
  else if (strcmp(item,"border") == 0)         acfg()->border=cl;
  else if (strcmp(item,"border_g") == 0)       acfg()->border_g=cl;
  else if (strcmp(item,"progressglow") == 0)   acfg()->progressglow=cl;
  
  //-- Background Should Be Redrawed
  miui_isbgredraw = 1;
  

  //-- Return
  return RET_OK;
}


//* 
//* ini_get
//*
char * miui_ini_get(char *item) {
  
  return_null_if_fail(item != NULL);
  //-- This is Busy Function
  ag_setbusy();
  
  
  //-- Convert Arguments
  char retval[128];
  memset(retval,0,128);
  
  //-- Set Property
  if      (strcmp(item,"roundsize") == 0)          snprintf(retval,128,"%i",acfg()->roundsz);
  else if (strcmp(item,"button_roundsize") == 0)   snprintf(retval,128,"%i",acfg()->btnroundsz);
  else if (strcmp(item,"window_roundsize") == 0)   snprintf(retval,128,"%i",acfg()->winroundsz);
  else if (strcmp(item,"transition_frame") == 0)   snprintf(retval,128,"%i",acfg()->fadeframes);

  else if (strcmp(item,"text_ok") == 0)            snprintf(retval,128,"%s",acfg()->text_ok);
  else if (strcmp(item,"text_next") == 0)          snprintf(retval,128,"%s",acfg()->text_next);
  else if (strcmp(item,"text_back") == 0)          snprintf(retval,128,"%s",acfg()->text_back);

  else if (strcmp(item,"text_yes") == 0)           snprintf(retval,128,"%s",acfg()->text_yes);
  else if (strcmp(item,"text_no") == 0)            snprintf(retval,128,"%s",acfg()->text_no);
  else if (strcmp(item,"text_about") == 0)         snprintf(retval,128,"%s",acfg()->text_about);
  else if (strcmp(item,"text_calibrating") == 0)   snprintf(retval,128,"%s",acfg()->text_calibrating);
  else if (strcmp(item,"text_quit") == 0)          snprintf(retval,128,"%s",acfg()->text_quit);
  else if (strcmp(item,"text_quit_msg") == 0)      snprintf(retval,128,"%s",acfg()->text_quit_msg);
    
  else if (strcmp(item,"rom_name") == 0)           snprintf(retval,128,"%s",acfg()->rom_name);
  else if (strcmp(item,"rom_version") == 0)        snprintf(retval,128,"%s",acfg()->rom_version);
  else if (strcmp(item,"rom_author") == 0)         snprintf(retval,128,"%s",acfg()->rom_author);
  else if (strcmp(item,"rom_device") == 0)         snprintf(retval,128,"%s",acfg()->rom_device);
  else if (strcmp(item,"rom_date") == 0)           snprintf(retval,128,"%s",acfg()->rom_date);
  
  else if (strcmp(item,"customkeycode_up")==0)     snprintf(retval,128,"%i",acfg()->ckey_up);
  else if (strcmp(item,"customkeycode_down")==0)   snprintf(retval,128,"%i",acfg()->ckey_down);
  else if (strcmp(item,"customkeycode_select")==0) snprintf(retval,128,"%i",acfg()->ckey_select);
  else if (strcmp(item,"customkeycode_back") == 0) snprintf(retval,128,"%i",acfg()->ckey_back);
  else if (strcmp(item,"customkeycode_menu") == 0) snprintf(retval,128,"%i",acfg()->ckey_menu);
  else if (strcmp(item,"dp") == 0) snprintf(retval,128,"%i",agdp());
  

  //-- Return
  return strdup(retval);
}

//* 
//* viewbox
//*
STATUS miui_viewbox(int argc, char *format, ...) {

  //-- is plain textbox or agreement
  if (argc!=3) return miui_error("%s() expects 3 args (title,desc,ico), got %d", __FUNCTION__, argc);
  
  //-- Set Busy before everythings ready
  ag_setbusy();
  
  //-- Get Arguments
  _INITARGS();
  
  //-- Init Background
  miui_setbg(args[0]);
  char text[1024];
  snprintf(text,1024,"%s",args[1]);
  
  //-- Init Drawing Data
  int pad         = agdp() * 4;
  int chkW        = agw() - (pad*2);
  int bntH        = agdp() * 20;
  int chkH        = agh() - ( miui_minY + bntH + (pad*4));
  int chkY        = miui_minY + pad;
  int btnY        = chkY + chkH + (pad*2);
  
  //-- Draw Navigation Bar
  miui_drawnav(&miui_win_bg,0,btnY-pad,agw(),bntH+(pad*2));
    
  //-- Load Icon
  PNGCANVAS ap;
  byte imgE       = 0;
  int  imgA       = 0;
  int  imgW       = 0;
  int  imgH       = 0;
  int  tifX       = pad*2;
  int  imgX       = pad;
  int  tifY       = chkY;
  int  imgY       = chkY;
  if (apng_load(&ap,args[2])){
    imgE  = 1;
    imgW  = min(ap.w,agdp()*30);
    imgH  = min(ap.h,agdp()*30);
    imgA  = imgW;
    tifX += imgA; 
  }
  int txtH        = ag_txtheight(chkW-((pad*2)+imgA),text,0);
  if (imgE){
    if (txtH<imgH){
      tifY+= (imgH-txtH)/2;
      txtH=imgH;
    }
    apng_draw_ex(&miui_win_bg,&ap,imgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }
  
  //-- Draw Text
  ag_textf(&miui_win_bg,chkW-((pad*2)+imgA),tifX+1,tifY+1,text,acfg()->winbg,0);
  ag_text(&miui_win_bg,chkW-((pad*2)+imgA),tifX,tifY,text,acfg()->winfg,0);
  
  color sepcl = ag_calculatealpha(acfg()->winbg,0x0000,80);
  color sepcb = ag_calculatealpha(acfg()->winbg,0xffff,127);
  ag_rect(&miui_win_bg,tifX,tifY+pad+txtH,chkW-((pad*2)+imgA),1,sepcl);
  ag_rect(&miui_win_bg,tifX,tifY+pad+txtH+1,chkW-((pad*2)+imgA),1,sepcb);
  
  //-- Resize Checkbox Size & Pos
  chkY+=txtH+pad;
  chkH-=txtH+pad;
  
  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  ACONTROLP txtcb = NULL;
  
  
  //-- NEXT BUTTON
  ACONTROLP nxtbtn=acbutton(
    hWin,
    pad+(agdp()*2)+(chkW/2),btnY,(chkW/2)-(agdp()*2),bntH,acfg()->text_next,0,
    6
  );

  char save_var_name[256];
  if (argc==6){
    //-- Save Variable Name
    snprintf(save_var_name,256,"%s",args[5]);
  }
            
  //-- Release Arguments
  _FREEARGS();
  
  //-- Dispatch Message
  aw_show(hWin);
  aw_setfocus(hWin,nxtbtn);
  byte ondispatch     = 1;
  byte is_checked     = 0;
      
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
        ondispatch = 0;
      }
      break;
      case 5:{
          ondispatch      = 0;
      }
      break;
      case 4:{
        //-- EXIT
        ondispatch      = 0;
      }
      break;
    }
  }
  //-- Window
  aw_destroy(hWin);
  
  //-- Return
  
  if (is_checked) return RET_YES;
  return RET_NO;
}

//* 
//* textbox, agreebox
//*
STATUS miui_textbox(char *title, char *desc, char* icon, char *message) {
  
  
  //-- Set Busy before everythings ready
  ag_setbusy();
  miui_isbgredraw = 1;
  
  //-- Init Background
  miui_setbg_title();
  char text[256];
  snprintf(text,256,"%s",title);
  
  //-- Unchecked Alert Message
    
  
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkW        = agw() - (pad*2);
  int bntH        = agdp() * 20;
  int chkH        = agh() - ( miui_minY + bntH + (pad*4));
  int chkY        = miui_minY + pad;
  int btnY        = chkY + chkH + (pad*2);
  
  //-- Draw Navigation Bar
  miui_drawnav(&miui_win_bg,0,btnY-pad,agw(),bntH+(pad*2));
    
  //-- Load Icon
  PNGCANVAS ap;
  byte imgE       = 0;
  int  imgA       = 0;
  int  imgW       = 0;
  int  imgH       = 0;
  int  tifX       = pad*2;
  int  imgX       = pad;
  int  tifY       = chkY;
  int  imgY       = chkY;
  if (apng_load(&ap,icon)){
    imgE  = 1;
    imgW  = min(ap.w,agdp()*30);
    imgH  = min(ap.h,agdp()*30);
    imgA  = imgW;
    tifX += imgA; 
  }
  int txtH        = ag_txtheight(chkW-((pad*2)+imgA),text,0);
  if (imgE){
    if (txtH<imgH){
      tifY+= (imgH-txtH)/2;
      txtH=imgH;
    }
    apng_draw_ex(&miui_win_bg,&ap,imgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }
  
  //-- Draw Text
  ag_textf(&miui_win_bg,chkW-((pad*2)+imgA),tifX+1,tifY+1,text,acfg()->winbg,0);
  ag_text(&miui_win_bg,chkW-((pad*2)+imgA),tifX,tifY,text,acfg()->winfg,0);
  
  //-- Resize Checkbox Size & Pos
  chkY+=txtH+pad;
  chkH-=txtH+pad;
  
  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  
  //-- Create Controls
  ACONTROLP txtbox;
  ACONTROLP agreecb;  
  txtbox = actext(hWin,pad,chkY,chkW,chkH,message,0);

  //-- NEXT BUTTON
  ACONTROLP nxtbtn=acbutton(
    hWin,
    pad+(agdp()*2)+(chkW/2),btnY,(chkW/2)-(agdp()*2),bntH,acfg()->text_back,0,
    6
  );

  
  //-- Dispatch Message
  aw_show(hWin);
  aw_setfocus(hWin,nxtbtn);
  byte ondispatch     = 1;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
            ondispatch = 0;
      }
      break;
      case 5:{
        //-- BACK
      
          ondispatch      = 0;
      }
      break;
      case 4:{
        //-- EXIT
        ondispatch      = 0;
      }
      break;
    }
  }
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return RET_OK;
}

//* 
//* checkbox
//*
STATUS miui_checkbox(int argc, char *format, ...) {
  if (argc<7){
    return miui_error("%s() expects more than 7 args, got %d", __FUNCTION__, argc);
  }
  else if ((argc-4)%3!=0){
    return miui_error("%s() expects 4 args + 3 args per items, got %d", __FUNCTION__, argc);
  }
  
  //-- Set Busy before everythings ready
  ag_setbusy();
  
  //-- Get Arguments
  _INITARGS();
  
  //-- Variable Def
  int i;
  
  //-- Init Background
  miui_setbg(args[0]);
  
  //-- Init Strings
  char path[256];
  char text[256];
  snprintf(path,256,"%s/%s",MIUI_TMP,args[3]);
  snprintf(text,256,"%s",args[1]);
  
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkW        = agw() - (pad*2);
  int bntH        = agdp() * 20;
  int chkH        = agh() - ( miui_minY + bntH + (pad*4));
  int chkY        = miui_minY + pad;
  int btnY        = chkY + chkH + (pad*2);
  
  //-- Draw Navigation Bar
  miui_drawnav(&miui_win_bg,0,btnY-pad,agw(),bntH+(pad*2));
    
  //-- Load Icon
  PNGCANVAS ap;
  byte imgE       = 0;
  int  imgA       = 0;
  int  imgW       = 0;
  int  imgH       = 0;
  int  tifX       = pad*2;
  int  imgX       = pad;
  int  tifY       = chkY;
  int  imgY       = chkY;
  if (apng_load(&ap,args[2])){
    imgE  = 1;
    imgW  = min(ap.w,agdp()*30);
    imgH  = min(ap.h,agdp()*30);
    imgA  = imgW;
    tifX += imgA; 
  }
  int txtH        = ag_txtheight(chkW-((pad*2)+imgA),text,0);
  if (imgE){
    if (txtH<imgH){
      tifY+= (imgH-txtH)/2;
      txtH=imgH;
    }
    apng_draw_ex(&miui_win_bg,&ap,imgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }
  
  //-- Draw Text
  ag_textf(&miui_win_bg,chkW-((pad*2)+imgA),tifX+1,tifY+1,text,acfg()->winbg,0);
  ag_text(&miui_win_bg,chkW-((pad*2)+imgA),tifX,tifY,text,acfg()->winfg,0);
  
  //-- Resize Checkbox Size & Pos
  chkY+=txtH+pad;
  chkH-=txtH+pad;
  
  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  
  //-- Check Box
  ACONTROLP chk1  = accheck(hWin,pad,chkY,chkW,chkH);
  
  //-- NEXT BUTTON
  ACONTROLP nxtbtn=acbutton(
    hWin,
    pad+(agdp()*2)+(chkW/2),btnY,(chkW/2)-(agdp()*2),bntH,acfg()->text_next,0,
    6
  );
  
  //-- Populate Checkbox Items
  char propkey[64];
  int idx = 0;
  int group_id = 0;
  for (i=4;i<argc;i+=3) {
    byte defchk = (byte) atoi(args[i+2]);
    if (defchk==2){
      if (accheck_addgroup(chk1,args[i],args[i+1])){
        group_id++;
        idx=0;
      }
    }
    else if (defchk!=3){
      idx++;
      snprintf(propkey,64,"item.%d.%d",group_id,idx);
      char * res = miui_parseprop(path,propkey);
      if (res!=NULL){
        defchk = (strcmp(res,"1")==0)?1:0;
        free(res);
      }
      accheck_add(chk1,args[i],args[i+1],defchk);
    }
  }
  
  //-- Release Arguments
  _FREEARGS();
  
  //-- Dispatch Message
  aw_show(hWin);
  aw_setfocus(hWin,nxtbtn);
  byte ondispatch = 1;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6: ondispatch = 0; break;
      case 5:{
        //-- BACK
          ondispatch      = 0;
      }
      break;
      case 4:{
        //-- EXIT
        ondispatch      = 0;
      }
      break;
    }
  }
  //-- Collecting Items:
  FILE * fp = fopen(path,"wb");
  if (fp!=NULL){
    int itemcnt = accheck_itemcount(chk1);
    for (i=0;i<itemcnt;i++) {
      if (!accheck_isgroup(chk1,i)){
        byte state = accheck_ischecked(chk1,i);
        snprintf(propkey,64,"item.%d.%d=%d\n",accheck_getgroup(chk1,i),accheck_getgroupid(chk1,i)+1,state);
        fwrite(propkey,1,strlen(propkey),fp);
      }
    }
    fclose(fp);
  }
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return RET_OK;
}

//* 
//* selectbox
//*
STATUS miui_selectbox(int argc, char *format, ...) {
  if (argc<7) {
    return miui_error("%s() expects more than 7 args, got %d", __FUNCTION__, argc);
  }
  else if ((argc-4)%3!=0){
    return miui_error("%s() expects 4 args + 3 args per items, got %d", __FUNCTION__, argc);
  }
  
  //-- Set Busy before everythings ready
  ag_setbusy();
  
  //-- Get Arguments
  _INITARGS();
  
  //-- Variable Def
  int i;
  
  //-- Init Background
  miui_setbg_title();
  
  //-- Init Strings
  char path[256];
  char text[256];
  snprintf(path,256,"%s/%s",MIUI_TMP,args[3]);
  snprintf(text,256,"%s",args[1]);
  
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkW        = agw() - (pad*2);
  int bntH        = agdp() * 20;
  int chkH        = agh() - ( miui_minY + bntH + (pad*4));
  int chkY        = miui_minY + pad;
  int btnY        = chkY + chkH + (pad*2);
  
  //-- Draw Navigation Bar
  miui_drawnav(&miui_win_bg,0,btnY-pad,agw(),bntH+(pad*2));
    
  //-- Load Icon
  PNGCANVAS ap;
  byte imgE       = 0;
  int  imgA       = 0;
  int  imgW       = 0;
  int  imgH       = 0;
  int  tifX       = pad*2;
  int  imgX       = pad;
  int  tifY       = chkY;
  int  imgY       = chkY;
  if (apng_load(&ap,args[2])){
    imgE  = 1;
    imgW  = min(ap.w,agdp()*30);
    imgH  = min(ap.h,agdp()*30);
    imgA  = imgW;
    tifX += imgA; 
  }
  int txtH        = ag_txtheight(chkW-((pad*2)+imgA),text,0);
  if (imgE){
    if (txtH<imgH){
      tifY+= (imgH-txtH)/2;
      txtH=imgH;
    }
    apng_draw_ex(&miui_win_bg,&ap,imgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }

  //-- Draw Text
  ag_textf(&miui_win_bg,chkW-((pad*2)+imgA),tifX+1,tifY+1,text,acfg()->winbg,0);
  ag_text(&miui_win_bg,chkW-((pad*2)+imgA),tifX,tifY,text,acfg()->winfg,0);

  //-- Resize Checkbox Size & Pos
  chkY+=txtH+pad;
  chkH-=txtH+pad;

  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);

  //-- Check Box
  ACONTROLP opt1  = acopt(hWin,pad,chkY,chkW,chkH);

  //-- NEXT BUTTON
  ACONTROLP nxtbtn = acbutton(
    hWin,
    pad+(agdp()*2)+(chkW/2),btnY,(chkW/2)-(agdp()*2),bntH,acfg()->text_next,0,
    6
  );

  char propkey[64];

  //-- Populate Checkbox Items
  int group_id = 0;
  int idx      = 0;
  for (i=4;i<argc;i+=3) {
    byte defchk = (byte) atoi(args[i+2]);
    if (defchk==2){
      if (acopt_addgroup(opt1,args[i],args[i+1])){
        group_id++;
        idx      = 0;
      }
    }
    else if (defchk!=3){
      idx++;
      snprintf(propkey,64,"selected.%d",group_id);
      char * savedsel = miui_parseprop(path,propkey);

      snprintf(propkey,64,"%d",idx);
      if (savedsel!=NULL){
        defchk = (strcmp(savedsel,propkey)==0)?1:0;
        free(savedsel);
      }
      acopt_add(opt1,args[i],args[i+1],defchk);
    }
  }

  //-- Release Arguments
  _FREEARGS();

  //-- Dispatch Message
  aw_show(hWin);
  aw_setfocus(hWin,nxtbtn);
  byte ondispatch = 1;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
        ondispatch = 0;
      }
      break;
      case 5:{
        //-- BACK
       
          ondispatch      = 0;
      }
      break;
      case 4:{
        //-- EXIT
        ondispatch      = 0;
      }
      break;
    }
  }
  
  //-- Collecting Items:
  FILE * fp = fopen(path,"wb");
  if (fp!=NULL){
    for (i=0;i<=group_id;i++){
      int selidx   = acopt_getselectedindex(opt1,i);
      if (selidx!=-1){
        int selindex = acopt_getgroupid(opt1,selidx)+1;
        snprintf(propkey,64,"selected.%d=%d\n",i,selindex);
        fwrite(propkey,1,strlen(propkey),fp);
      }
    }
    fclose(fp);
  }
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return RET_OK;
}

//* 
//* lang
//*
STATUS miui_langmenu(char *title_name, char *title_icon) {
  ag_setbusy();
  miui_isbgredraw=1;
  //-- Get Arguments
  
  //-- Variable Def
  int i;
  
  //-- Init Strings
  
  //-- Init Strings
  char text[256];
  snprintf(text,256,"%s",title_name);
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkH        = agh();
  int chkW          = agw();
  
  //-- Draw Navigation Bar
  int chkY= miui_setbg_title() + pad;
     
  int const_pad = agdp() * 24;
  int txtH        = ag_txtheight(chkW-((pad*2)+const_pad),text,0);
  
  int tifX = 2*pad + const_pad;
  int tifY = chkY + (const_pad - txtH)/2;
  //-- Draw Text
  ag_textf(&miui_win_bg,chkW-((pad*2)+const_pad),tifX+1,tifY+1,text,acfg()->winbg,0);
  ag_text(&miui_win_bg,chkW-((pad*2)+const_pad),tifX,tifY,text,acfg()->winfg,0);
  
  chkY += const_pad + pad ;
  chkH = agh() - chkY;
  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  
  //-- Check Box
  ACONTROLP menu1  = acsdmenu(hWin,0,chkY,chkW,chkH,6);
  //-- Populate Checkbox Items
  acsdmenu_add(menu1, "简体中文", "欢迎到MIUI Recovery", "@lang.cn");
  acsdmenu_add(menu1, "English", "Welcome to MIUI Recovery", "@lang.en");

  //-- Dispatch Message
  aw_show(hWin);
  byte ondispatch = 1;
  byte onback = 0;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
            ondispatch = 0;
            onback = 0;
      }
      break;
      case 5:{
        //-- BACK
          onback = 1;
          ondispatch      = 0;
      }
      break;
      case 4:{
        //-- EXIT
        onback = 1;
        ondispatch      = 0;
      }
      break;
    }
  }
  
  int selindex;
  if (onback == 0)
      selindex = acsdmenu_getselectedindex(menu1);
   else selindex = 0;
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return selindex;
}
//* 
//* menubox
//*
STATUS miui_mainmenu(char *title_name, char **item, char **item_icon, char **item_icon_append, int item_cnt) {
  //-- Set Busy before everythings ready
  return_val_if_fail(title_name != NULL, RET_FAIL);
  return_val_if_fail(item_cnt >= 0, RET_FAIL);
  ag_setbusy();
  miui_isbgredraw=1;
  //-- Get Arguments
  
  //-- Variable Def
  int i;
  
  //-- Init Background
  
  //-- Init Strings
  
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkH        = agh();
  int chkW          = agw();
  
  //-- Draw Navigation Bar
  int chkY= miui_setbg_title();
  chkH -= chkY; 
  
  
  
  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  
  //-- Check Box
  ACONTROLP backmenu = actitle(hWin, 0, chkY, chkW, &chkH, title_name, 1, 5);
  chkY = chkY + chkH;
  chkH = agh() - chkY;
  ACONTROLP menu1  = acmenu(hWin,0,chkY,chkW,chkH,6);

  //-- Populate Checkbox Items
  for (i=0;i<item_cnt;i++) {
    if (item[i] != NULL && strcmp(item[i],"")!=0){
      if (item_icon != NULL && item_icon_append != NULL)
           acmenu_add(menu1, item[i], item_icon[i], item_icon_append[i]);
      else {
          if (item_icon != NULL)
              acmenu_add(menu1, item[i], item_icon[i], NULL);
          else {
              if (item_icon_append != NULL)
                  acmenu_add(menu1, item[i], NULL, item_icon_append[i]);
              else 
                  acmenu_add(menu1, item[i], NULL, NULL);
          }
      }
    }
  }

  
  //-- Dispatch Message
  aw_show(hWin);
  byte ondispatch = 1;
  byte onback = 0;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
            ondispatch = 0;
      }
      break;
      case 5:{
        //-- BACK
          onback = 1;
          ondispatch      = 0;
      }
      break;
      case 4:{
        //-- EXIT
        onback = 1;
        ondispatch      = 0;
      }
      break;
    }
  }
  
  int selindex;
  if (onback == 0)
      selindex = acmenu_getselectedindex(menu1)+1;
   else selindex = 0;
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return selindex;
}

STATUS miui_menubox(char *title_name, char **item,  int item_cnt) {
  return_val_if_fail(item_cnt >= 1, RET_FAIL); 
  return_val_if_fail(title_name != NULL, RET_FAIL);
  return_val_if_fail(item != NULL, RET_FAIL);
  //-- Set Busy before everythings ready
  ag_setbusy();
  miui_isbgredraw = 1;
  
  
  //-- Variable Def
  int i;
  
  
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkH        = agh();
  int chkY        = miui_setbg_title();
  int chkW          = agw();
  
  //-- Draw Navigation Bar
  chkH -= chkY; 
  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  
  ACONTROLP backmenu = actitle(hWin, 0, chkY, chkW, &chkH, title_name, 1, 5);
  chkY = chkY + chkH;
  chkH = agh() - chkY;
  //-- Check Box
  ACONTROLP menu1  = acmenu(hWin,0,chkY,chkW,chkH,6);

  //-- Populate Checkbox Items
  for (i = 0; i < item_cnt; i++) {
    if (item[i] != NULL && strcmp(item[i],"")!=0)
      acmenu_add(menu1,item[i], NULL, NULL);
  }

  //-- Dispatch Message
  aw_show(hWin);
  byte ondispatch = 1;
  byte onback = 0;
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
           ondispatch = 0;

      }
      break;
      case 5:{
        //-- BACK
          onback = 1; 
          ondispatch      = 0;
        
      }
      break;
      case 4:{
        //-- EXIT
        onback = 1;
        ondispatch      = 0;
      }
      break;
    }
  }
  
  int selindex;
  if (onback == 0)
      selindex = acmenu_getselectedindex(menu1)+1;
  else selindex = 0;
  
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return selindex;
}

#define DIR_ICON  "@dir"
#define FILE_ICON "@file"
#define BACK_ICON "@back"
/*
 *print sd file system,
 *menu titlname, use set_bg print title 
 *menu name, 
 *
 *
 */
/*
 *
 *miui_get_title "MIUI recovery  batt:n% time:21:30"
 *
 */
STATUS miui_sdmenu(char *title_name, char **item, char **item_sub, int item_cnt) {

  //-- Set Busy before everythings ready
  ag_setbusy();
  miui_isbgredraw = 1;

  //-- Get Arguments

  //-- Variable Def
  int i;
  miui_setbg_title(); 
  //-- Init Background

  //-- Init Strings

  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkY        = miui_set_title(title_name);
  int chkH        = agh() - chkY;
  int chkW          = agw();

  //-- Draw Navigation Bar


  //-- Create Window
  AWINDOWP hWin   = aw(&miui_win_bg);
  
  //-- Check Box
  ACONTROLP menu1  = acsdmenu(hWin,0,chkY,chkW,chkH,6);

  //-- Populate Checkbox Items
  for (i=0;i<item_cnt;i++) {
    int item_len = strlen(item[i]);
    if (strcmp(item[i],"") != 0)
    {
      if (strncmp(item[i], "../", 3) == 0)
          acsdmenu_add(menu1, "<~sd.back.name>"," <~sd.back.desc>", BACK_ICON);
      else if(item[i][item_len - 1] == '/')
          acsdmenu_add(menu1, item[i], item_sub[i], DIR_ICON);
      else 
          acsdmenu_add(menu1, item[i], item_sub[i], FILE_ICON);
    }
  }

  
  //-- Dispatch Message
  aw_show(hWin);
  byte ondispatch = 1;  
  while(ondispatch){
    dword msg=aw_dispatch(hWin);
    switch (aw_gm(msg)){
      case 6:{
        ondispatch = 0;
      }
      break;
      case 5:{
        //-- BACK
      
          ondispatch      = 0;
        
      }
      break;
      case 4:{
        //-- EXIT
        ondispatch      = 0;
      }
      break;
    }
  }
  
  int selindex = acsdmenu_getselectedindex(menu1) ;
  
  //-- Destroy Window
  aw_destroy(hWin);
  
  //-- Finish
  return selindex;
}

/*
 *show aboutmenu  
 */
STATUS miui_aboutmenu(char *title, char *icon, char *content)
{
    return_val_if_fail(title != NULL, RET_FAIL);
    return_val_if_fail(icon != NULL, RET_FAIL);
    return_val_if_fail(content != NULL, RET_FAIL);
    ag_setbusy();
    miui_isbgredraw = 1;

    int chkY= miui_setbg_title();
    int chkX = 0;
    int chkW = agw();
    int chkH = agh();
    //Create Window 
    AWINDOWP hWin = aw(&miui_win_bg);
    
    ACONTROLP backmenu = actitle(hWin, chkX, chkY, chkW, &chkH, title, 1, 5);
    chkY += chkH;
    chkH = agh() - chkY;
    CANVAS *about_canvas = &hWin->c;
    //--load icon
    PNGCANVAS ap;
    int imgW = 0;
    int imgH = 0;
    if (apng_load(&ap, icon)){
        imgW = min(ap.w, agw());
        if (imgW >= agw()) chkX = 0;
        else chkX = (agw() - imgW)/2;
        imgH = min(ap.h, agh());
    }
    else miui_error("can not load %s\n", icon); 
    ag_rect(about_canvas, 0, chkY, agw(), chkH, acfg()->textbg); 
    apng_draw_ex(about_canvas, &ap, chkX, chkY, 0, 0, imgW, imgH);
    apng_close(&ap);
    chkY += imgH;
    chkX = agdp() *4;
   
    ag_textf(about_canvas,agw(), chkX+1, chkY+1, content, acfg()->textbg, 0);
    ag_text(about_canvas, agw(), chkX, chkY, content, acfg()->textfg, 0);

    aw_show(hWin);
    byte ondispatch = 1;
    while(ondispatch) {
        dword msg = aw_dispatch(hWin);
        switch (aw_gm(msg)) {
            //back menu
            case 5:ondispatch = 0;break;
        }
    }
    //finally
    miui_isbgredraw = 0;
    aw_destroy(hWin);
    return RET_OK;
}
//* 
//* alert
//*

STATUS miui_alert(int argc, char *format, ...) {
  if ((argc<2)||(argc>4)) {
    return miui_error("%s() expects 2-4 args (title, text, [icon, ok text]), got %d", __FUNCTION__, argc);
  }
  
  //-- Set Busy before everythings ready
  ag_setbusy();
  
  //-- Get Arguments
  _INITARGS();
  
  //-- Show Alert
  aw_alert(
    NULL,
    args[0],
    args[1],
    (argc>2)?args[2]:"",
    (argc>3)?args[3]:NULL
  );
  
  //-- Release Arguments
  _FREEARGS();
  
  //-- Return
  return RET_OK;
}

//* 
//* confirm
//*
STATUS miui_confirm(int argc, char *format, ...) {
  if ((argc<2)||(argc>5)) {
    return miui_error("%s() expects 2-4 args (title, text, [icon, yes text, no text]), got %d", __FUNCTION__, argc);
  }
  
  //-- Set Busy before everythings ready
  ag_setbusy();
  
  //-- Get Arguments
  _INITARGS();
  
  //-- Show Confirm
  byte res = aw_confirm(
    NULL,
    args[0],
    args[1],
    (argc>2)?args[2]:"",
    (argc>3)?args[3]:NULL,
    (argc>4)?args[4]:NULL
  );
  
  //-- Release Arguments
  _FREEARGS();
  
  //-- Return
  if (res) return RET_YES;
  return RET_NO;
}

//* 
//* textdialog
//*
STATUS miui_textdialog(int argc, char *format, ...){
   return_val_if_fail(argc > 1, RET_FAIL);
  _INITARGS(); 
  //-- Set Busy before everythings ready
  ag_setbusy();
  //-- Show Text Dialog
  aw_textdialog(
    NULL,
    args[0],
    args[1],
    (argc>2)?args[2]:NULL
  );
 
  _FREEARGS(); 
  //-- Return
  return RET_OK;
}

//* 
//* loadlang
//*
static void miui_langloadsave(char * dest, int max, char * key){
  char * val = alang_get(key);
  if (val!=NULL) snprintf(dest,max,"%s",val);
}

STATUS miui_loadlang(char * name)
{
  ag_setbusy();
  
  
  //-- Load Language Data
  char path[256];
  snprintf(path,256,"%s/%s",MIUI_DIR,name);
  byte res = alang_load(path);
  
  //-- Replace Text
  if (res){
    acfg_reset_text();
    miui_langloadsave(acfg()->text_ok, 64, "text_ok");
    miui_langloadsave(acfg()->text_next, 64, "text_next");
    miui_langloadsave(acfg()->text_back, 64, "text_back");
    miui_langloadsave(acfg()->text_yes, 64, "text_yes");
    miui_langloadsave(acfg()->text_no, 64, "text_no");
    miui_langloadsave(acfg()->text_about, 64, "text_about");
    miui_langloadsave(acfg()->text_calibrating, 64, "text_calibrating");
    miui_langloadsave(acfg()->text_quit, 64, "text_quit");
    miui_langloadsave(acfg()->text_quit_msg, 128, "text_quit_msg");
  }
  
  return res; 
}

//* 
//* lang
//*
  
char * miui_lang(char *name){
  //-- Set Busy before everythings ready
  ag_setbusy();

  char * out = alang_get(name);

  return out;
}

#define INSTALL_NAME "<~sd.install.name>"
#define INSTALL_ICON "@sd.install"
STATUS miui_install(int echo) {
  //-- Set Busy before everythings ready
  ag_setbusy();
  
  miui_isbgredraw = 1;
  
  //-- Init Background
  miui_setbg_title();
  
  //-- Init Strings
  char text[256];                   //-- Text When Installing
  char finish_text[256];            //-- Text After Installing
  char * icon = INSTALL_ICON;
  snprintf(text,256,"%s", INSTALL_NAME);
  snprintf(finish_text,256,"%s", INSTALL_NAME);
  
  //-- Drawing Data
  int pad         = agdp() * 4;
  int chkW        = agw() - (pad*2);
  int bntH        = agdp() * 20;
  int chkH        = agh() - ( miui_minY + bntH + (pad*4));
  int chkY        = miui_minY + pad;
  int btnY        = chkY + chkH + (pad*2);

  //-- Load Icon
  PNGCANVAS ap;
  byte imgE       = 0;
  int  imgA       = 0;
  int  imgW       = 0;
  int  imgH       = 0;
  int  tifX       = pad*2;
  int  imgX       = pad;
  int  tifY       = chkY;
  int  imgY       = chkY;
  if (apng_load(&ap,icon)){
    imgE  = 1;
    imgW  = min(ap.w,agdp()*30);
    imgH  = min(ap.h,agdp()*30);
    imgA  = imgW;
    tifX += imgA; 
  }
  int txtH        = ag_txtheight(chkW-((pad*2)+imgA),text,0);
  
  int txtFH       = ag_txtheight(chkW-((pad*2)+imgA),finish_text,0);
  int tifFY       = tifY;
  
  if (imgE){
    if (txtH<imgH){
      tifY+= (imgH-txtH)/2;
      txtH=imgH;
    }
    if (txtFH<imgH){
      tifFY+= (imgH-txtFH)/2;
      txtFH = imgH;
    }
    apng_draw_ex(&miui_win_bg,&ap,imgX,imgY,0,0,imgW,imgH);
    apng_close(&ap);
  }
  else 
  {
      miui_error("load %s icon error!\n", icon);
  }
  
  //-- Finished Text Canvas
  CANVAS cvf;
  ag_canvas(&cvf,agw(),((txtFH>txtH)?txtFH:txtH));
  ag_draw_ex(&cvf,&miui_win_bg,0,0,0,imgY,agw(),cvf.h);
  
  //-- Draw Finished Text
  ag_textf(&cvf, chkW-((pad*2)+imgA), tifX+1, tifFY+1-imgY, finish_text,    acfg()->winbg,0);
  ag_text (&cvf, chkW-((pad*2)+imgA), tifX,   tifFY-imgY,   finish_text,    acfg()->winfg,0);
  
  //-- Draw Text
  ag_textf(&miui_win_bg,chkW-((pad*2)+imgA),tifX+1,tifY+1,text,acfg()->winbg,0);
  ag_text(&miui_win_bg, chkW-((pad*2)+imgA),tifX,tifY,text,    acfg()->winfg,0);
  
  //-- Resize Checkbox Size & Pos
  int chkFY  = chkY + (txtFH+pad);
  int chkFH  = chkH - (txtFH+pad);
  
  chkY      += txtH+pad;
  chkH      -= txtH+pad;
  
  
  //-- Start Installer Proccess
  //if ret_status == 0 && echo = 1, install sucesss, make succes dialog
  //if ret_status != 0, install failed , list error log dialog
  //ret_status == -1; install failed
  int ret_status = miui_start_install(
    &miui_win_bg,
    pad,chkY,chkW,chkH,
    pad,btnY,chkW,bntH,
    &cvf, imgY, chkFY, chkFH, 
    echo
  );
  
  //-- Release Finished Canvas
  ag_ccanvas(&cvf);

  
  //-- Installer Not Return OK
  return ret_status;
}

#if 0
static int time_echo_enable = 1;
static pthread_t time_thread_t;
static void *time_echo_thread(void *cookie){
    while(time_echo_enable)
    {
        miui_setbg_title();
        ag_draw(NULL, &miui_win_bg, 0, 0);
        ag_sync_fade(0);
        //interval can't be small
        sleep(10);
    }
    return NULL;
}
#endif

#define MIUI_INITARGS() \
          char** args = ReadVarArgs(state, argc, argv); \
          if (args==NULL) return NULL;

#define MIUI_FREEARGS() \
          int freearg_i; \
          for (freearg_i=0;freearg_i<argc;++freearg_i) free(args[freearg_i]); \
          free(args);

Value* MIUI_INI_SET(const char* name, State* state, int argc, Expr* argv[]) {
  if (argc != 2) {
    miui_error("%s() expects 2 args(config name, config value in string), got %d", name, argc);
    return StringValue(strdup(""));
  }
  
  //-- This is Busy Function
  ag_setbusy();
  
  //-- Get Arguments
  MIUI_INITARGS();
  //-- Convert Arguments
  byte valint = (byte) min(atoi(args[1]),255);
  int  valkey = (int) atoi(args[1]);
  
  //-- Set Property
  if      (strcmp(args[0],"roundsize") == 0)          acfg()->roundsz=valint;
  else if (strcmp(args[0],"button_roundsize") == 0)   acfg()->btnroundsz=valint;
  else if (strcmp(args[0],"window_roundsize") == 0)   acfg()->winroundsz=valint;
  else if (strcmp(args[0],"transition_frame") == 0)   acfg()->fadeframes=valint;

  else if (strcmp(args[0],"text_ok") == 0)            snprintf(acfg()->text_ok,64,"%s", args[1]);
  else if (strcmp(args[0],"text_next") == 0)          snprintf(acfg()->text_next,64,"%s", args[1]);
  else if (strcmp(args[0],"text_back") == 0)          snprintf(acfg()->text_back,64,"%s", args[1]);

  else if (strcmp(args[0],"text_yes") == 0)           snprintf(acfg()->text_yes,64,"%s", args[1]);
  else if (strcmp(args[0],"text_no") == 0)            snprintf(acfg()->text_no,64,"%s", args[1]);
  else if (strcmp(args[0],"text_about") == 0)         snprintf(acfg()->text_about,64, "%s", args[1]);
  else if (strcmp(args[0],"text_calibrating") == 0)   snprintf(acfg()->text_calibrating,64,"%s", args[1]);
  else if (strcmp(args[0],"text_quit") == 0)          snprintf(acfg()->text_quit,64,"%s", args[1]);
  else if (strcmp(args[0],"text_quit_msg") == 0)      snprintf(acfg()->text_quit_msg,128,"%s", args[1]);
    
  else if (strcmp(args[0],"rom_name") == 0)           snprintf(acfg()->rom_name,128,"%s", args[1]);
  else if (strcmp(args[0],"rom_version") == 0)        snprintf(acfg()->rom_version,128,"%s", args[1]);
  else if (strcmp(args[0],"rom_author") == 0)         snprintf(acfg()->rom_author,128,"%s", args[1]);
  else if (strcmp(args[0],"rom_device") == 0)         snprintf(acfg()->rom_device,128,"%s", args[1]);
  else if (strcmp(args[0],"rom_date") == 0)           snprintf(acfg()->rom_date,128,"%s", args[1]);
  else if (strcmp(args[0],"brightness_path") == 0)     snprintf(acfg()->brightness_path, PATH_MAX, "%s", args[1]);
  else if (strcmp(args[0],"lun_file") == 0)     snprintf(acfg()->lun_file, PATH_MAX, "%s", args[1]);
  
  
  else if (strcmp(args[0],"customkeycode_up")==0)     acfg()->ckey_up=valkey;
  else if (strcmp(args[0],"customkeycode_down")==0)   acfg()->ckey_down=valkey;
  else if (strcmp(args[0],"customkeycode_select")==0) acfg()->ckey_select=valkey;
  else if (strcmp(args[0],"customkeycode_back") == 0) acfg()->ckey_back=valkey;
  else if (strcmp(args[0],"customkeycode_menu") == 0) acfg()->ckey_menu=valkey;
  //add for input event filter
  else if (strcmp(args[0], "input_filter")  == 0) {
     acfg()->input_filter = valkey;
	 miui_debug("input is 0x%x\n", acfg()->input_filter);
  }
    
  //-- Force Color Space  
  else if (strcmp(args[0],"force_colorspace") == 0){
         ag_changcolor(args[1][0], args[1][1], args[1][2], args[1][3]);
  }
  else if (strcmp(args[0],"dp") == 0){
    set_agdp(valint);
  }
  else if (strcmp(args[0], "sd_ext")== 0) {
    acfg()->sd_ext=valint;
  }
  
  miui_isbgredraw = 1;
  //-- Release Arguments
  MIUI_FREEARGS();

  //-- Return
  return StringValue(strdup(""));
}

Value* MIUI_CALIBRATE(const char* name, State* state, int argc, Expr* argv[]) {
  //-- Return
  return StringValue(strdup(""));
}

static void miui_ui_register()
{
//todo
    //--CONFIG FUNCTIONS
    //
    RegisterFunction("ini_set",       MIUI_INI_SET);       //-- SET INI CONFIGURATION
    RegisterFunction("calibrate", MIUI_CALIBRATE);
    //RegisterFunction("calibtool", MIUI_CALIB);
}
int miui_ui_init()
{
    acfg_init();
    //register function
    RegisterBuiltins();
    miui_ui_register();
    FinishRegistration();
	return 0;
}
//read config file if exist and then execute it
int miui_ui_config(const char *file)
{
    //if file exist 
    return_val_if_fail(file != NULL, RET_FAIL);
    struct stat file_stat;
    if (stat(file, &file_stat) < 0)
    {
        miui_printf("stat file error, file is not exist");
        return -1;
    }
    char *script_data = miui_readfromfs(file);
    return_val_if_fail(script_data != NULL, RET_FAIL);

   //--PARSE CONFIG SCRIPT
   Expr* root;
   int error_count = 0;
   yy_scan_string(script_data); 
   int error = yyparse(&root, &error_count);
   if (error != 0 || error_count > 0) {
       miui_printf("read file %s failed!\n", file);
       goto config_fail;
   }
   //--- EVALUATE CONFIG SCRIPT 
   State state;
   state.cookie = NULL;
   state.script = script_data;
   state.errmsg = NULL;
   char* result = Evaluate(&state, root);
   if (result == NULL) {
       if (state.errmsg == NULL) {
           miui_printf("script abortedl\n");
       } else {
           free(state.errmsg);
       }
   } else {
       miui_printf("scripte aborted!\n");
       free(result);
   }
config_fail:
   if (script_data != NULL) free(script_data);
   return -1;
}

STATUS miui_ui_start()
{
    int i = 0;
    for (i = 0; i < MIUI_THEME_CNT; i++)
    {
        acfg()->theme[i] = NULL;
        acfg()->theme_9p[i] = 0;
    }
    ag_canvas(&miui_win_bg, agw(), agh());
    ag_canvas(&miui_bg, agw(), agh());
    miui_theme("miui4");
    ag_loadsmallfont("fonts/small", 0, NULL);
    ag_loadbigfont("fonts/big", 0, NULL);
    alang_release();
    return RET_OK;
}
STATUS miui_ui_end()
{
    ag_ccanvas(&miui_win_bg);
    ag_ccanvas(&miui_bg);
    alang_release();
    atheme_releaseall();
    return RET_OK;
}
