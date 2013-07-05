/*
 *description:implement of screen echo or black,depend on timeout in input event or invoke screen_set;
 *    this file is inner file within miui_input.c ,so more api bind static property, but function screen_is_black
 *    and screen_set_black could be invoked by external module
 *author:dennise
 *email:yanhao@xiaomi.com
 */
//typedef long time_t
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "miui_screen.h"

//#define TEST 1 //suport to print the set_brightness info 

static time_t time_orig;
//for further consideration, change time_interval in device.conf
static time_t time_interval;
static int bool_black = 0;
static pthread_mutex_t mutex_screen = PTHREAD_MUTEX_INITIALIZER;
//screen_ligt, [0-255] for config by device_config
static char screen_light_buf[4];

static int screen_set_time(time_t time)
{
    return_val_if_fail(time > 0, -1);
    miui_debug("set time %ld\n", time);
    time_orig = time;
    return 0;
}

int screen_set_brightness(char* brightness) {
	pthread_mutex_lock(&mutex_screen);
	int *fd = NULL;
	fd = open(acfg()->brightness_path, O_WRONLY);
	if (fd <= 0) {
		miui_error("open %s failed!\n", acfg()->brightness_path);
	} else {

 	if (write(fd, brightness, strlen(brightness)) <= 0) {
#ifdef TEST
			miui_error(" write %s to %s Failed",brightness,acfg()->brightness_path, strerror(errno));
#else 
			miui_error(" write error %s", acfg()->brightness_path, strerror(errno));		
#endif
		
#ifdef TEST 
		} else {
			printf(" Write %s to %s Sucess ", brightness, acfg()->brightness_path);
		}
#else 
	}
#endif 
		
		close(fd);
	}
	pthread_mutex_unlock(&mutex_screen);
	return 0;
}


int screen_set_black(int black)
{
    pthread_mutex_lock(&mutex_screen);
    int fd = open(acfg()->brightness_path, O_WRONLY);
    if (fd <= 0)
    {
        miui_error("open %s failed!\n", acfg()->brightness_path);
    }
    else 
    {
        bool_black = black;
        if (bool_black)
        {
            if (write(fd,"0", 1) <= 0)
            {
                miui_error("%s write error %s", acfg()->brightness_path, strerror(errno));
            } 
        }
        else 
        {
            miui_debug("screen_light_buf is %s\n", screen_light_buf);
            if (write(fd,screen_light_buf,strlen(screen_light_buf)) <= 0)
            {
                miui_error("%s write error %s", acfg()->brightness_path, strerror(errno));
            }

        }
        close(fd);
    }
    pthread_mutex_unlock(&mutex_screen);
    return 0;
}
int screen_is_black()
{
    return bool_black;
}
int screen_set_light(int light)
{
    return_val_if_fail(light > 10, -1);
    return_val_if_fail(light <= 255, -1);
    return_val_if_fail(0 <= snprintf(screen_light_buf,sizeof(screen_light_buf), "%d", light), -1);
    return 0;
}

int screen_set_interval(int interval)
{
    return_val_if_fail(interval > 0, -1);
    time_interval = interval;
    return 0;
}
static void *screen_black_thread(void *cookie)
{
    while(1) 
    {
        if (difftime(time((time_t *)NULL), time_orig) > time_interval)
        {
            if (0 == bool_black)
                screen_set_black(1);
        }
        else if (0 != bool_black)
        {
            screen_set_black(0);
        }
        sleep(1);

    }
    return NULL;
}
static pthread_t screen_thread_t;
static int screen_init()
{
    //default interval 120 seconds
    time_interval = 120;
    screen_set_light(60);
    screen_set_time(time((time_t*)NULL));
    screen_set_black(0);
    
    pthread_create(&screen_thread_t, NULL, screen_black_thread, NULL);
    pthread_detach(screen_thread_t);
    return 0;
}

