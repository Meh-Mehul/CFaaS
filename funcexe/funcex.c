#include "funcex.h"
#include "../libres/libres.h"
#include "../ires/ires.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include<sys/stat.h>
#ifndef COMMON_IMPL
  #include"../common.h"
#endif
#define MAX_SCHED_RETRIES 10
// this function will be run inside the worker
// so that it can set-up its fd comm channel with teh 
// main process.  
static int worker_sv_setup(char* sock_path){
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(sock == -1){
    perror("ESVSETUPWORKER_SOCK");
    return -1;
  } 
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);
  unlink(sock_path);

  if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1){
    perror("ESVSETUPWORKER_BIND");
    DEBUG("[DEBUG] Failed to bind to: %s\n", sock_path);
    return -1;
  }  
  if(listen(sock, 2) == -1){
    perror("ESVSETUPWORKER_LISTEN");
    return -1;
  }
  chmod(sock_path, 0777);
  DEBUG("[DEBUG] Worker socket created at: %s\n", sock_path);
  return sock;
}

// inpiration from cloudflare's blog on scm_rights
// basically just the helper to actually send the msg
// and its scm_rights header (with kernel-level fd info)
static int worker_recv_fd(int conn) {
    char buf[2];
    struct iovec iov = {
        .iov_base = buf,
        .iov_len = sizeof(buf)
    };

    union {
        char buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } u;

    struct msghdr msg = {0};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);

    DEBUG("[DEBUG] worker_recv_fd: Before recvmsg, controllen=%lu\n", msg.msg_controllen);
    ssize_t n = recvmsg(conn, &msg, 0);
    DEBUG("[DEBUG] worker_recv_fd: recvmsg returned %ld, errno=%d\n", n, errno);
    if (n <= 0) {
        perror("recvmsg");
        return -1;
    }

    DEBUG("[DEBUG] worker_recv_fd: After recvmsg, controllen=%lu\n", msg.msg_controllen);
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    DEBUG("[DEBUG] worker_recv_fd: cmsg pointer=%p\n", (void*)cmsg);
    if (!cmsg) {
        DEBUG("No control message\n");
        return -1;
    }

    DEBUG("[DEBUG] worker_recv_fd: cmsg_level=%d, cmsg_type=%d\n", cmsg->cmsg_level, cmsg->cmsg_type);
    if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        DEBUG("Invalid cmsg: level=%d (expected %d), type=%d (expected %d)\n", cmsg->cmsg_level, SOL_SOCKET, cmsg->cmsg_type, SCM_RIGHTS);
        return -1;
    }

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    DEBUG("[DEBUG] worker_recv_fd: Extracted fd=%d\n", fd);
    return fd;
}

static void worker_notify_done(int* pipe_fd){
  char job_done_msg[] = "done";
  int faas_done_n = write(pipe_fd[1], job_done_msg, sizeof(job_done_msg));
  if(faas_done_n < 0){
    perror("WORKER_DONE_RESP");
  }
}


// since its essentially the startup for a worker
// why not call it systemd?
// also it is the sole process the 
// forked worker ever runs (and possible forever)
static void worker_systemd(char* sock_path, int id, int* pipe_fd){
    int sock_sv = worker_sv_setup(sock_path);
    while(1){
      int conn = accept(sock_sv,NULL ,NULL);
      if(conn == -1){
        perror("WORKERSV_ACCEPT");
        continue;
      }
      DEBUG("Worker with id:%d has been scheduled by the main\n", id);
      int job_conn_fd = worker_recv_fd(conn);
      if(job_conn_fd == -1){
        perror("WORKERSV_JOBFD");
        close(conn);
        continue;
      }
      int* job_lib_id = resolve_id(&job_conn_fd);
      int* job_ip_str_sz = resolve_sz(&job_conn_fd);
      char* job_ip_str = resolve_input_str(&job_conn_fd, *job_ip_str_sz);
      D_lib* dlib = (D_lib*)malloc(sizeof(D_lib));
      dlib->ID = *job_lib_id;
      if(!checkLib(dlib)){
        perror("EWORKER_LIBSEARCH");
        worker_notify_done(pipe_fd);
        close(job_conn_fd);
        close(conn);
        continue;
      }      
      get_lib_pth(dlib);
      get_lib_handle(dlib);
      get_fp(dlib);
      // and this is the call; kinda underwhelming?
      // TODO: Add timeout limits so as to limit the function's 
      // execution to only 1minute at most
      // or add this config to some store or smth (but again, im lazy)
      int job_result = dlib->fn(&job_conn_fd, job_ip_str);
      char job_res_msg[10];// not more than 10-digit codes allowed for now
      snprintf(job_res_msg,sizeof(job_result),"%d",job_result);
      // TODO: Check the following, i find it sus
      send(job_conn_fd, &job_res_msg, sizeof(job_res_msg)-1, 0);
      // since the job is now done, send state-change resp
      worker_notify_done(pipe_fd);
      close(job_conn_fd);
      close(conn);
    }
}


// TODO: (i probably wont lower worker perms as for now idc much about security.)
//  [] lower perms, do the parent process's job. (probably using setuid, although i would need to read : https://people.eecs.berkeley.edu/~daw/papers/setuid-usenix02.pdf)
Worker* fx_newWorker(int id){
    DEBUG("[DEBUG] Creating worker %d...\n", id);
    Worker* new_Worker = (Worker*)malloc(sizeof(Worker));
    char sock_path[24];
    snprintf(sock_path,sizeof(sock_path),"/tmp/worker_%d.sock",id);
    int pipe_fd[2];
    if(pipe(pipe_fd) == -1){
      perror("EWORKER_PIPE");
      return NULL;
    }
    pid_t worker_pid = fork();
    if(worker_pid<0){
      perror("EWORKER_FORK");
      return NULL;
    }    
    if(worker_pid == 0){
      DEBUG("[DEBUG] Worker %d child process starting...\n", id);
      worker_systemd(sock_path, id, pipe_fd);
      return NULL;
    }
    else{
      DEBUG("[DEBUG] Worker %d parent: forked with PID %d\n", id, worker_pid);
      new_Worker->pid = worker_pid;
      new_Worker->com_fd[0] = pipe_fd[0];
      new_Worker->com_fd[1] = pipe_fd[1];
      snprintf(new_Worker->sun_path,sizeof(new_Worker->sun_path),"%s",sock_path);
    }
    new_Worker->id = id;
    new_Worker->state = 0;
    DEBUG("[DEBUG] Worker %d created successfully\n", id);
    return new_Worker;
}

// structure to handle thread invocation with params
typedef struct{
  Worker* worker;
  int fd;
  pthread_mutex_t* w_lock;
}W_input;


// internal func to connect to unix socket of the worker (for fd transfer)
static int wm_connect_unix(int id){
  char sock_path[24];
  snprintf(sock_path,sizeof(sock_path),"/tmp/worker_%d.sock",id);
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);
  
  int retries = 50;
  while(retries > 0){
    int unix_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(unix_sock == -1){
      perror("EWM_SOCKET");
      return -1;
    }
    if(connect(unix_sock, (struct sockaddr*)&addr, sizeof(addr)) == 0){
      DEBUG("[DEBUG] Connected to worker %d socket\n", id);
      return unix_sock;
    }
    close(unix_sock);
    DEBUG("[DEBUG] Retry %d: Waiting for worker %d socket...\n", 51-retries, id);
    usleep(100000);
    retries--;
  }
  DEBUG("[ERROR] Failed to connect to worker %d at %s after retries\n", id, sock_path);
  perror("EWM_CONNECT_UN");
  return -1;
}

// this one sends the fd over to the worker's
// systemd process, and then leaves the task to it
int wm_send_fd(int unix_sock, int fd){
  DEBUG("[DEBUG] wm_send_fd: Sending fd=%d over socket=%d\n", fd, unix_sock);
  struct iovec iov;
  char dummy[2] = ":)";
  iov.iov_base = dummy;
  iov.iov_len = 2;

  union {
      char buf[CMSG_SPACE(sizeof(int))];
      struct cmsghdr align;
  } u;

  memset(&u, 0, sizeof(u));

  struct msghdr msg = {0};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = u.buf;
  msg.msg_controllen = sizeof(u.buf);

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));

  memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
  
  DEBUG("[DEBUG] wm_send_fd: controllen=%lu, cmsg_len=%lu\n", msg.msg_controllen, cmsg->cmsg_len);
  ssize_t result = sendmsg(unix_sock, &msg, 0);
  DEBUG("[DEBUG] wm_send_fd: sendmsg returned %ld, errno=%d\n", result, errno);

  return result;
}

// sent off by the scheduler to allocate the given task to the worker
// also handle state change and waits till that task is done
// since this is offloaded to a separate thread, its way easier.
static void* wm_worker_alloc(void* arg){
  W_input* worker_input = (W_input*)arg;
  Worker* worker = worker_input->worker;
  int fd = worker_input->fd;
  pthread_mutex_t* w_lock = worker_input->w_lock;
  int worker_unix_sock = wm_connect_unix(worker->id);
  if(worker_unix_sock == -1){
    fprintf(stderr, "[Major Error] Failed to connect to worker %d\n", worker->id);
    pthread_mutex_lock(w_lock);
    worker->state = 0;
    pthread_mutex_unlock(w_lock);
    free(worker_input);
    return NULL;
  }
  int err = wm_send_fd(worker_unix_sock, fd);
  if(err < 0){
    perror("[Major Error] WM Could not send fd to worker!");
    close(worker_unix_sock);
    close(worker_input->fd);
    pthread_mutex_lock(w_lock);
    worker->state = 0;
    pthread_mutex_unlock(w_lock);
    free(worker_input);
    return NULL;
  }
  close(worker_unix_sock);
  char buf[10];
  if(read(worker->com_fd[0], buf, sizeof(buf)) < 0){
    perror("WORKER_DONE_READ");
  }
  pthread_mutex_lock(w_lock);
  worker->state = 0;
  pthread_mutex_unlock(w_lock);
  free(worker_input);
  return NULL;
}



// the scheduler for cfaas main
// for now, its a simple iterable and lock-based
// scheduler, but later on i may do some heuristics
// it iterates through the workers and then schedules the job
// in the first one it finds to be free, if none are free, then waits
// returns 1 if scheduled, -1 if not scheduled
int fx_sched_once(Worker* workers,pthread_mutex_t* w_lock, int* fd){
  bool alloc = false;
  int tries_to_alloc = 0;
  while(!alloc && tries_to_alloc<SCHED_MAX_TRIES){
    pthread_mutex_lock(w_lock);
    for(int worker_id = 0; worker_id<NUM_WORKERS; worker_id++){
      if(workers[worker_id].state == 0){
        workers[worker_id].state = 1;
        pthread_t worker_sched_thread;
        W_input* worker_ip = (W_input*)malloc(sizeof(W_input));
        worker_ip->worker = &workers[worker_id];
        worker_ip->fd = *fd;
        worker_ip->w_lock  = w_lock;
        int err = pthread_create(&worker_sched_thread, NULL, wm_worker_alloc, (void*)worker_ip);
        if(err){
          perror("[Major Issue] could not create a thread to schedule to worker");
          workers[worker_id].state = 0;
          free(worker_ip);
        }
        else{
          pthread_detach(worker_sched_thread);
          alloc = true;
        }
        break;
      }
    }
    pthread_mutex_unlock(w_lock);
    tries_to_alloc++;
  }
  if(alloc){ 
    return 1;
  }
  else{
    return -1;
  }
}

// since, my scheduler is dumb, the one who wrote it
// deserves to have this function atleast
int fx_sched(Worker* workers, pthread_mutex_t* w_lock, int* fd){
  int sched_tries = 0;
  int res_fg = 0;
  while(sched_tries < MAX_SCHED_RETRIES){
    res_fg = fx_sched_once(workers, w_lock, fd);
    if(res_fg == 1) break;
    else sched_tries++;
  }
  return res_fg;
}
