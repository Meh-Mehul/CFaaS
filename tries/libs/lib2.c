#include<stdio.h>

typedef struct Job{
  int id;
  char* string;
  char* user;
} Job;


int fn(void* args){
  Job* job = (Job*)args;
  printf("User : %s Submitted the job.\n", job->user);
  printf("Job ID:%d\n", job->id);
  return 0;
}
