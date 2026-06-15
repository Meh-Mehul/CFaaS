#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

int fn(int* fd, char* input){
  char response[1024];
  snprintf(response, sizeof(response), "hello world - received: %s", input);
  send(*fd, response, strlen(response) + 1, 0);
  return 0;
}
