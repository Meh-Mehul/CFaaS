#include "funcex.h"
#include<sys/types.h>
#include<unistd.h>

Worker* fx_newWorker(Worker* workerpool, int id){
    
}




int fx_sched(int* fd, char* input){
  


}


// TODO:
// newWorker steps:
//  [] fork to a child process
//  [] lower perms, do the parent process's job (getpid, set-up sun path and pipe), return worker
//  [] in child, run the worker code (unix socket server, recv and run and state change machine)


