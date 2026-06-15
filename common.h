#define COMMON_IMPL
#include<stdio.h>
#define DEBUG_MODE
#ifdef DEBUG_MODE
  #define DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
  #define DEBUG(fmt, ...) ((void)0) 
#endif
#define LIBCT_PORT 6969
#define FAAS_PORT 8000