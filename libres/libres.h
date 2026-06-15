// for the library resolving part
#ifndef LIBREAS_IMPL
  #define LIBRES_IMPL
#endif
#include<stdbool.h>
typedef int (*_fn_ptr)(int*, char*);// core type for the system btw
#define DIR_LIB_PATH "./libs"
#define MAX_LIB_PATH_SZ 1024
// yes, its a hardcoded const, deal with it

// ik, there are better way, but this the 
// main (ONLY) interface to communicate with Library Resolver atp
typedef struct{
  int ID;
  char* lib_pth;
  void* lib_handle;
  _fn_ptr fn;
}D_lib;

// to check if an library exists with the given ID
bool checkLib(D_lib* lib);

// to set the library path with the given ID
void get_lib_pth(D_lib* lib);

// to set the handle for the library
void get_lib_handle(D_lib* lib);



// sets core function ptr from the library handle
// the library is loaded in a lazy manner to optimize function 
// loading and cold-start times (hopefully)
void get_fp(D_lib* lib);


// just because its needed in some other libs
int get_id_from_fp(char* fp);

