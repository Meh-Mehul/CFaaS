#define COMMON_IMPL
#include<stdio.h>
#define DEBUG_MODE
#ifdef DEBUG_MODE
  #define DEBUG(str) printf("[DEBUG] %s\n", str)
#else
  #define DEBUG(str) 
#endif
#define LIBCT_PORT 6969
#define FAAS_PORT 8000