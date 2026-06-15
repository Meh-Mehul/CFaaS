#include <stdbool.h>
#include<stdio.h>
#include "libres.h"
#include<sys/types.h>
#include<dirent.h>
#include<dlfcn.h>
#include<stdlib.h>
#include<assert.h>
#include<ctype.h>
#ifndef COMMON_IMPL
  #include"../common.h"
#endif

int get_id_from_fp(char* fp){
  int id = 0;
  int i = 0;
  char* filename = fp;
  for(int j = 0; fp[j] != '\0'; j++){
    if(fp[j] == '/'){
      filename = &fp[j+1];
    }
  }
  if(!filename || !isdigit(filename[0])) return -1;
  while(isdigit(filename[i])){
    id = id*10 + (filename[i]-'0');
    i++;
  }
  if(filename[i] == '.' && filename[i+1]=='s'&&filename[i+2]=='o'&&filename[i+3] == '\0'){
    return id;
  }
  if(filename[i] == '.' && filename[i+1]=='c'){
    return id;
  }
  DEBUG("[Error] Error in parsing Library Name %s\n", fp);
  return -1;
}

void get_fp(D_lib* lib){
  if(lib->lib_handle == NULL){
    DEBUG("[Error] Please call get_lib_handle first before calling get_fp for properly resolving library.\n");
    //exit(1);
    return;
  }
  dlerror();
  _fn_ptr fn = dlsym(lib->lib_handle, "fn");// fn is the main interface name
  // must be same for a lambda fn to work
  // not so much lambda eh?
  if(fn == NULL){
      DEBUG("[Error] Error in Loading Symbols\n The User Library (ID:%d) might not be properly configured!\n", lib->ID);
      exit(1);
  }
  lib->fn = fn;
}


void get_lib_pth(D_lib* lib){
  if(!lib->ID){
    DEBUG("[Error] No ID provided, exiting resolving the library\n");
    exit(1);
  }
  struct dirent *dp;
  DIR* dfd;
  char* dir = DIR_LIB_PATH;
  if ((dfd = opendir(dir)) == NULL){
    fprintf(stderr, "Can't open %s\n", dir);
    exit(1);
  } 
  char* lib_path = malloc(sizeof(char)*MAX_LIB_PATH_SZ);
  // TODO: This is very slow for largs systems
  // kindly reshape the storage idea of a dll
  while((dp = readdir(dfd)) != NULL){
    if(get_id_from_fp(dp->d_name) == lib->ID){
      sprintf(lib_path,"%s/%s", dir, dp->d_name);
      lib->lib_pth = lib_path;
      return;  
    }
  } 
  fprintf(stderr,"Function wrongly called, first check existence, could not find such lib with ID = %d\n", lib->ID);
  return;
}


bool checkLib(D_lib* lib){
   if(!lib->ID){
    DEBUG("[Error] No ID provided, exiting resolving the library\n");
    //exit(1);
    return false;
  }
  struct dirent *dp;
  DIR* dfd;
  char* dir = DIR_LIB_PATH;
  if ((dfd = opendir(dir)) == NULL){
    fprintf(stderr, "Can't open %s\n", dir);
    //exit(1);
    return false;
  } 
  char* lib_path = malloc(sizeof(char)*MAX_LIB_PATH_SZ);
  // TODO: This is very slow for largs systems
  // kindly reshape the storage idea of a dll
  while((dp = readdir(dfd)) != NULL){
    if(get_id_from_fp(dp->d_name) == lib->ID){
      return true; 
    }
  } 
  return false;  
}



void get_lib_handle(D_lib* lib){
  assert(checkLib(lib)==true);
  lib->lib_handle = dlopen(lib->lib_pth,RTLD_LAZY);// lazy loading
  // so as to avoid memory explosion in loading unecessary functions
}
