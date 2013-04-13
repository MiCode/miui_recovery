/*
 * Copyright@ Samsung Electronics Co. LTD
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

#ifndef _SEC_FB_LCD_
#define _SEC_FB_LCD_

/*
 * S T R U C T U R E S  F O R  C U S T O M  I O C T L S
 *
*/
struct secfb_user_window {
    int x;
    int y;
};

/*
 * C U S T O M  I O C T L S
 *
*/

#define FBIO_WAITFORVSYNC       _IO  ('F', 32)
#define SECFB_WIN_POSITION      _IOW ('F', 203, struct secfb_user_window)


#define DEFAULT_LCD_WIDTH   (480)
#define DEFAULT_LCD_HEIGHT  (800)
#define DEFAULT_LCD_BPP     (32)

/***************** LCD frame buffer *****************/
#define FB0_NAME    "/dev/fb0"
#define FB1_NAME    "/dev/fb1"
#define FB2_NAME    "/dev/fb2"
#define FB3_NAME    "/dev/fb3"
#define FB4_NAME    "/dev/fb4"

#endif
