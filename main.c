/* CFaaS main
 its got the following roles:
 1. Thread off to library Creator Server
 2. Create workers
 3. main thread runs the recieving server
*/
#include"ires/ires.h"
#include"funcexe/funcex.h"
#include"libct/libct.h"
#ifndef LIBRES_IMPL
  #include"libres/libres.h"
#endif

#define LIBCT_PORT 6969


// Starts a mutli-threaded socker server for handling 
// requests related to library creation in C
void* start_lib_creator_ss(void* args){
 
}

// Creates Workers (equal to NUM_WORKERS)
void* create_workers(void* args){
 
}


// the code where the rest of the main spends its time
void start_main_server(){
 
}

// CFaaS main
int main(){
 
}
