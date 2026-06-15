#include "ires.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#ifndef COMMON_IMPL
  #include"../common.h"
#endif



void* read_x_bytes(int* fd, unsigned int x){
  void* buf = malloc(MAX_INPUT_SIZE);
  if(x>MAX_INPUT_SIZE){
    DEBUG("[Warning] the input length is too high, issues might occur!\n");
    void* t = realloc(buf, x);
  }
  int bytesRead = 0;
  int res;
  while(bytesRead<(int)x){
    res = bytesRead;
    res = read(*fd, buf+bytesRead, x-bytesRead);
    if(res<1){
      DEBUG("Could not read enough bytes.\n");
      exit(1);
    }
    bytesRead += res;
  }
  return buf;
}


char* resolve_input_str(int* fd, unsigned int x){
  return (char*)read_x_bytes(fd,x);
}

int* resolve_id(int* fd){
  return (int*)read_x_bytes(fd,TAGS_SZ_B);
}

int* resolve_sz(int* fd){
  return (int*)read_x_bytes(fd,TAGS_SZ_B);
}



