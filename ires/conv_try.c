// to test hwo the encoding and decoding part could be possible
// if i can convert char* <-> void* then im done
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
typedef struct Complex_struct{
    int a;
    int b;
    char* c;
    void* d;
}Compl;


int main(int argv, char** argc){
  Compl* a = malloc(sizeof(Compl));
  a->a = 5;
  a->b = 56;
  a->c = "HelloGuys";
  a->d = (void*)"Yo5";
  char* b = (char*)a;
  strcat(b, "\0");
  printf("Trying to Print Complex Struct = %s\n", b);
  Compl* c = (Compl*)b;
  printf("Re-Covering a from string %d %d %s %s\n", c->a, c->b, c->c, (char*)c->d);
  return 0;
}

// sadly it doesnt work and ill have to have char* only
