#include<sys/types.h>
#include<unistd.h>
#include"../libres/libres.h"
#include<stdbool.h>
#define NUM_WORKERS 20


// Worker struct for cfaas system
// state:
//  0 - free worker
//  1 - busy worker
//  2 - dead worker
// com_fd is the pipe for communicating with the 
// corresponding worker
typedef struct{
    int state;
    pid_t pid;
    int com_fd[2]; // for the data comm (not the input, this is just in case)
    char* sun_path[24]; // for the fd intercomm
}Worker;

// Function for spawining and managing workers
// More: Ideally would do the following steps:
// 1. first allocs the pipes for comms.
// 2. Then forks a new child process
//    2.1 in child, it lower permissions
//    2.2 in child, it runs only the worker part of the 
//            looping construct (recieve, execute and write-back to fd)
//    2.3 in current (parent), it gets pid, creates the worker at id and 
//              assigns to workerpool
Worker* fx_newWorker(Worker* workerPool, int id);


// core function of the engine
// an ideal way to do this would be to spawn
// a worker node for this everytime (or allocate to an already
// allocated worker)
// but since whole faas is running via a single
// i have to manage workerpool as well as worker scheduling
// This function returns 1 if the function was put onto a proper
// worker for execution else -1
// also, the fd is passed as well, becuase result validity
// is returned directly by worker 
int fx_sched(int* fd, char* input);
    
    
