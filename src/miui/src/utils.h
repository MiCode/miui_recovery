#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>

#define byte              unsigned char
#define dword             unsigned int
#define word              unsigned short
#define color             unsigned short
typedef int u32;

//##                                                   ##//
//##               LIST OF DEFINITIONS                 ##//
//##                                                   ##//
//#######################################################//
//
// Common Data Type
//

//
// MIUI Main Configurations
//
#define MIUI_NAME        "MIUI Recovery"
//rom_version
#define MIUI_VERSION     "2.00"
//rom date
#define MIUI_BUILD       "2012-10-26"
#define MIUI_BUILD_CN    "Weekend"
#define MIUI_BUILD_L     "Dennis"
#define MIUI_BUILD_A     "<yanhao@xiaomi.com>"
#define MIUI_BUILD_URL   "http://www.micode.net/"
#define MIUI_COPY        "(c) 2012 by xiaomi MIUI developers"

//-- Temporary Dir - Move from /tmp/miui-data to /tmp/miui symlink to /tmp/miui-data for backward compatibility
#define MIUI_SYSTMP      "/tmp"
//#define MIUI_SYSTMP      "/data"
#define MIUI_TMP         MIUI_SYSTMP "/miui"
#define MIUI_TMP_S       MIUI_SYSTMP "/miui-data"

#define MIUI_DIR         "/res"
#define MIUI_FRAMEBUFFER "/dev/graphics/fb0"

#define MIUI_THEME_CNT 24
// MIUI Canvas Structure
//
typedef struct{
	int     w;       // Width
	int     h;       // Height
	int     sz;      // Data Size
	color * data;    // Data 
} CANVAS;

//
// MIUI Assosiative Array Structure
//
typedef struct{
  char * key;
  char * val;
} AARRAY_ITEM, * AARRAY_ITEMP;

typedef struct{
  int length;
  AARRAY_ITEMP items;
} AARRAY, * AARRAYP;

AARRAYP   aarray_create();
char *    aarray_get(AARRAYP a, char * key);
byte      aarray_set(AARRAYP a, char * key, char * val);
byte      aarray_del(AARRAYP a, char * key);
byte      aarray_free(AARRAYP a);

//
// MIUI PNG Canvas Structure
//
typedef struct {
  int     w;       // Width
  int     h;       // Height
  int     s;       // Buffer Size
  byte    c;       // Channels
  byte *  r;       // Red Channel
  byte *  g;       // Green Channel
  byte *  b;       // Blue Channel
  byte *  a;       // Alpha Channel
} PNGCANVAS, * PNGCANVASP;


//
// MIUI PNG Font Canvas Structure
//
typedef struct {
  byte    loaded;    // Font is Loaded 
  int     fx[96];    // Font X Positions
  byte    fw[96];    // Font Width
  byte    fh;        // Font Height
  int     w;         // Png Width
  int     h;         // Png Height
  int     s;         // Buffer Size
  byte    c;         // Channels
  byte *  d;         // Fonts Alpha Channel
} PNGFONTS;

//
#ifdef DEBUG
#define miui_debug(fmt...) printf("(pid:%d)[%s]%s:%d::", getpid(), __FILE__, __FUNCTION__, __LINE__);printf(fmt)
#else
#define miui_debug(fmt...) do{}while(0)
#endif
#ifndef miui_printf
#define miui_printf printf
#endif
#ifndef miui_error
#define miui_error(fmt...) printf("(%d)[%s]%s:%d::", getpid(), __FILE__, __FUNCTION__, __LINE__);printf(fmt)
#endif
#ifndef return_val_if_fail
#define return_val_if_fail(p, val) \
	if (!(p)) { \
	   miui_printf("(pid:%d)function %s(line %d) cause %s failed  return %d\n", getpid(), __FUNCTION__, __LINE__, #p,  val);return val;}	
#endif
#ifndef return_null_if_fail
#define return_null_if_fail(p) \
	if (!(p)) { \
	   miui_printf("(pid:%d)[%s]function %s(line %d) " #p " \n",getpid(),__FILE__,  __FUNCTION__, __LINE__);return NULL;}	
#endif
#ifndef assert_if_fail
#define assert_if_fail(p) \
	if (!(p)) { \
	   miui_printf("(pid:%d)[%s]function %s(line %d) " #p " \n",getpid(), __FILE__,  __FUNCTION__, __LINE__);}	
#endif
// Customization Functions
//
//
// MIUI Main Configuration Structure
//
typedef struct  {
  // Colors
  color winbg;                // Window Background
  color winbg_g;              // Window Background Gradient
  color winfg;                // Window Foreground
  color winfg_gray;           // Window Foreground
  color dialogbg;             // Dialog Background
  color dialogbg_g;           // Dialog Background Gradient
  color dialogfg;             // Dialog Foreground
  color textbg;               // Text / List Background
  color textfg;               // Text / List Font Color
  color textfg_gray;          // List Grayed Font Color ( List Description )
  color controlbg;            // Control/Button Background
  color controlbg_g;          // Control/Button Background Gradient
  color controlfg;            // Control/Button Font Color
  color selectbg;             // Selected Item/Control Background
  color selectbg_g;           // Selected Item/Control Background Gradient
  color selectfg;             // Selected Item/Control Font Color
  color titlebg;              // Title Background
  color titlebg_g;            // Title Background Gradient
  color titlefg;              // Title Font Color
  color dlgtitlebg;           // Dialog Title Background
  color dlgtitlebg_g;         // Dialog Title Background Gradient
  color dlgtitlefg;           // Dialog Title Font Color
  color navbg;                // Scrollbar Color
  color navbg_g;              // Navigation Bar Background
  color scrollbar;            // Navigation Bar Background Gradient
  color border;               // Border Color
  color border_g;             // Border Color Gradient
  color progressglow;         // Progress Bar Glow Color
  
  // Property
  byte  roundsz;              // Control Rounded Size
  byte  btnroundsz;           // Button Control Rounded Size
  byte  winroundsz;           // Window Rounded Size
  
  // Transition
  byte  fadeframes;           // Number of Frame used for Fade Transition
  
  // Common Text
  char  text_ok[64];          // OK
  char  text_next[64];        // Next >
  char  text_back[64];        // < Back
  
  char  text_yes[64];         // Yes
  char  text_no[64];          // No
  char  text_about[64];       // About
  char  text_calibrating[64]; // Calibration Tools
  char  text_quit[64];        // Quit
  char  text_quit_msg[128];   // Quit Message
  char  brightness_path[PATH_MAX]; //brightness_path
  char  lun_file[PATH_MAX]; //mass_storage path
  
  // ROM Text
  char rom_name[128];          // ROM Name
  char rom_version[128];       // ROM Version
  char rom_author[128];        // ROM Author
  char rom_device[128];        // ROM Device Name
  char rom_date[128];          // ROM Date
  
  // CUSTOM KEY
  int ckey_up;
  int ckey_down;
  int ckey_select;
  int ckey_back;
  int ckey_menu;
  u32 input_filter;
  u32 sd_ext;
  
  // THEME
  PNGCANVASP theme[MIUI_THEME_CNT];
  byte       theme_9p[MIUI_THEME_CNT];
  char themename[64];
} AC_CONFIG;

AC_CONFIG * acfg();           // Get Config Structure
void        acfg_init();      // Set Default Config
void acfg_init_ex(byte themeonly);
#endif
