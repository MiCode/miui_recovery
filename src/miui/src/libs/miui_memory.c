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
 
#include <stdio.h>
#include <stdlib.h>

#ifndef _MIUI_NODEBUG
#include <sys/statfs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

  #define MIUI_MEM_TMPDIR "/tmp/miui-memory"
  long miui_isexist_mem(void * x){
    char pn[256];
    snprintf(pn,256,"%s/%i",MIUI_MEM_TMPDIR,(long) x);
    FILE * fp = fopen(pn, "r");
    if (fp){
      long sz=0;
      fread(&sz,1,sizeof(long),fp);
      fclose(fp);
      return sz;
    }
    return 0;
  }
  void miui_touch_memaddr(void * x,long sz,long line,char * filename){
    long fz=miui_isexist_mem(x);
    if (fz==0){
      char pn[256];
      snprintf(pn,256,"%s/%i",MIUI_MEM_TMPDIR,(long) x);
      FILE * fp=fopen(pn,"wb");
      fwrite(&sz,1,sizeof(long),fp);
      fwrite(&line,1,sizeof(long),fp);
      fwrite(filename,1,strlen(filename),fp);
      if (fp) fclose(fp);
    }
  }
  void miui_unlink_memaddr(void * x){
    long fz=miui_isexist_mem(x);
    if (fz){
      char pn[256];
      snprintf(pn,256,"%s/%i",MIUI_MEM_TMPDIR,(long) x);
      unlink(pn);
    }
  }
  void miui_dump_malloc(){
     const char *path = MIUI_MEM_TMPDIR;
     DIR *d = opendir(path);
     size_t path_len = strlen(path);
     
     printf("\n\n===================================================\n");
     printf(    "|                   LEAK INFO:                    |\n");
     printf(    "===================================================\n\n");
     if (d)
     {
        struct dirent *p;
        while ((p=readdir(d)))
        {
            char *buf;
            size_t len;
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            {
               continue;
            }
            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);
            if (buf)
            {
               struct stat statbuf;
               snprintf(buf, len, "%s/%s", path, p->d_name);
               if (!stat(buf, &statbuf))
               {
                  if (S_ISDIR(statbuf.st_mode)){
                  }
                  else
                  {
                    long memaddr = atoi(p->d_name);
                    
                    char pn[256];
                    long fz = 0;
                    long fl = 0;
                    char fn[256];
                    memset(fn,0,256);
                    snprintf(pn,256,"%s/%i",MIUI_MEM_TMPDIR,(long) memaddr);
                    FILE * fp = fopen(pn, "r");
                    if (fp){
                      fread(&fz,1,sizeof(long),fp);
                      fread(&fl,1,sizeof(long),fp);
                      fread(fn,1,255,fp);
                      fclose(fp);
                    }
                    char str[10];
                    memset(str,0,10);
                    if (fz>0) snprintf(str,10,"%s", (char *) memaddr);
                    printf("[0x%x %ib] = \"%s\" LINE %i <%s>\n",memaddr,fz,fn,fl,str);
                  }
               }
               free(buf);
            }
        }
        closedir(d);
     }
     printf("\n\n===================================================\n\n");
  }
#endif


void miui_memory_terminate(const char * message){
  fprintf(stdout,"\n\nmiui/FATAL-ERROR: %s\n\n",message);
  exit(-1);
}
void * miui_realloc ( void * x, size_t size
#ifndef _MIUI_NODEBUG
, long line, char * filename
#endif
){
#ifndef _MIUI_NODEBUG
miui_unlink_memaddr(x);
#endif
  void *ret = realloc(x,size);
#ifndef _MIUI_NODEBUG
  miui_touch_memaddr(ret,size,line,filename);
#endif
  return ret;
}

void *miui_malloc(size_t size
#ifndef _MIUI_NODEBUG
, long line, char * filename
#endif
){
    void *ret = malloc(size);
    if (!ret && !size)
        ret = malloc(1);
    if (!ret) {
        if (!ret && !size) ret = malloc(1);
        if (!ret) miui_memory_terminate("Out Of Memory...\n");
    }
#ifndef _MIUI_NODEBUG
  miui_touch_memaddr(ret,size,line,filename);
#endif
    return ret;
}

void miui_free(void ** x){
#ifndef _MIUI_NODEBUG
    miui_unlink_memaddr(*x);
#endif
  if (*x!=NULL){
    free(*x);
    *x=NULL;
  }
}
  
#ifndef _MIUI_NODEBUG
  void miui_memory_debug_init(){
	mkdir(MIUI_MEM_TMPDIR, 0777);
  }
#endif
