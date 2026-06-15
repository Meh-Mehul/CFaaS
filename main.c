/* CFaaS main
 its got the following roles:
 1. Thread off to library Creator Server
 2. Create workers
 3. main thread runs the recieving server
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<unistd.h>
#include"ires/ires.h"
#include"funcexe/funcex.h"
#include"libct/libct.h"
#ifndef LIBRES_IMPL
  #include"libres/libres.h" // just because why not?
#endif
#ifndef COMMON_IMPL
  #include"common.h"
#endif


Worker* g_workers = NULL;
pthread_mutex_t g_w_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
  int fd;
}ClientArgs;

// client conn handler for library creation requests
// No, i will not handle errors
void* handle_libct_client(void* args){
  ClientArgs* cargs = (ClientArgs*)args;
  int client_fd = cargs->fd;
  free(cargs);
  char* temp_file_path = recv_file_n_save(client_fd);
  DEBUG("Received file");
  if(temp_file_path == NULL){
    char msg[100];
    snprintf(msg, sizeof(msg), "Sorry, we could not recieve you function!.");
    send(client_fd, msg, sizeof(msg)+1, 0);
    close(client_fd);
    return NULL;
  }
  char* lib_path = compile_to_lib(temp_file_path);
  if(validate_file(lib_path)){
    send(client_fd, lib_path, strlen(lib_path) + 1, 0);
  }
  else{
    char msg[100];
    snprintf(msg, sizeof(msg), "Sorry, we could not upload you function!.");
    send(client_fd, msg, sizeof(msg)+1, 0);
  }
  close(client_fd);
  return NULL;
}

// Start the socket server for 
// the library creator
void* start_lib_creator_ss(void* args){
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(LIBCT_PORT);
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
  listen(server_fd, 5);
  while(1){
    int client_fd = accept(server_fd, NULL, NULL);
    DEBUG("Connection");
    ClientArgs* args = (ClientArgs*)malloc(sizeof(ClientArgs));
    args->fd = client_fd;
    pthread_t thread;
    pthread_create(&thread, NULL, handle_libct_client, (void*)args);
    pthread_detach(thread);
  }
  return NULL;
}

// Creates Workers (equal to NUM_WORKERS)
void* create_workers(void* args){
  Worker* workers = (Worker*)malloc(sizeof(Worker) * NUM_WORKERS);
  for(int i = 0; i < NUM_WORKERS; i++){
    workers[i] = *fx_newWorker(i);
  }
  g_workers = workers;
  return (void*)workers;
}

typedef struct{
  int fd;
}MainClientArgs;

void* handle_main_client(void* args){
  MainClientArgs* cargs = (MainClientArgs*)args;
  int client_fd = cargs->fd;
  free(cargs);
  
  printf("[DEBUG] handle_main_client: scheduling request on fd %d\n", client_fd);
  int result = fx_sched(g_workers, &g_w_lock, &client_fd);
  printf("[DEBUG] handle_main_client: scheduling returned %d\n", result);
  return NULL;
}

// main server (i.e. the funcexe scheduler)
void start_main_server(){
  printf("[DEBUG] Starting main server on port 8000...\n");
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(8000);
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
  listen(server_fd, 10);
  printf("[DEBUG] Main server listening...\n");
  while(1){
    int client_fd = accept(server_fd, NULL, NULL);
    printf("[DEBUG] Accepted client connection\n");
    MainClientArgs* cargs = (MainClientArgs*)malloc(sizeof(MainClientArgs));
    cargs->fd = client_fd;
    pthread_t thread;
    pthread_create(&thread, NULL, handle_main_client, (void*)cargs);
    pthread_detach(thread);
  }
}

// CFaaS main
int main(){
  printf("[DEBUG] CFaaS main starting...\n");
  fflush(stdout);
  pthread_t libct_thread, worker_thread;
  pthread_create(&libct_thread, NULL, start_lib_creator_ss, NULL);
  pthread_create(&worker_thread, NULL, create_workers, NULL);
  printf("[DEBUG] Threads created, sleeping for worker setup...\n");
  fflush(stdout);
  sleep(5);
  printf("[DEBUG] Sleep done, starting main server\n");
  fflush(stdout);
  start_main_server();
  pthread_join(libct_thread, NULL);
  pthread_join(worker_thread, NULL);
  return 0;
}
