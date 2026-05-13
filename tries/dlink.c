#include <bits/types/FILE.h>
#include<stdio.h>
#include<semaphore.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
#include<dlfcn.h>
typedef struct Interface{
    int hello;
    char* message;
}Interface;

void* threadfn(void* args){
    Interface* msg = (Interface*)args;
    printf("In the Thread we print %s\n", msg->message);
    int* a = (int*)malloc(sizeof(int));
    *a = 30;
    return (void*)a;
}


typedef struct Job{
    int id;
    char* title;
    char* user;
}Job;

int main(int argv, char** argc){
    if(argv<2){
        printf("Please Provide a library path as argument!\n");
        return 0;
    }
    char* path = argc[1];
    pthread_t tid;
    char* string = "Hello_world!";
    Interface* msg = (Interface*)malloc(sizeof(Interface));
    msg->message = string;
    pthread_create(&tid, NULL, threadfn, (void*)msg);
    int* a;
    pthread_join(tid, (void*)&a);    
    printf("Thread is now done! and returned => %d\n", *a);
    void* handle = dlopen(path, RTLD_LAZY);
    int (*fn)(void*) = dlsym(handle, "fn");
    char* error =dlerror();
    if(error != NULL){
        printf("Error Occured %s\n", error);
        return 0;
    }
    Job* job = malloc(sizeof(Job));
    job->id = 69;
    job->user = "Whachamacallit?";
    fn((void*)job);
    return 0;
}

