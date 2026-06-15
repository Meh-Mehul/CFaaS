#include<stdio.h>



typedef struct Job{
    int id;
    char* title;
}Job;


int fn(void* args){
    Job* job = (Job*)args;
    printf("Job ID:%d, Title:%s Successfully Executed\n", job->id, job->title);
    return 0;
}

