#include "funcex.h"
#include "../libres/libres.h"
#include "../ires/ires.h"
#include <stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include<sys/stat.h>


// this function will be run inside the worker
// so that it can set-up its fd comm channel with teh 
// main process.  
static int worker_sv_setup(char* sock_path){
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(sock == -1){
    perror("ESVSETUPWORKER_SOCK");
    exit(1);
  } 
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, sock_path);
  unlink(sock_path);

  if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1){
    perror("ESVSETUPWORKER_BIND");
    exit(1);
  }  
  if(listen(sock, 2) == -1){
    perror("ESVSETUPWORKER_LISTEN");
    exit(1);
  }
  chmod(sock_path, 0777);
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

    ssize_t n = recvmsg(conn, &msg, 0);
    if (n <= 0) {
        perror("recvmsg");
        return -1;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) {
        printf("No control message\n");
        return -1;
    }

    if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        printf("Invalid cmsg\n");
        return -1;
    }

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
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
      printf("Worker with id:%d has been scheduled by the main\n", id);
      int job_conn_fd = worker_recv_fd(conn);
      if(job_conn_fd == -1){
        perror("WORKERSV_JOBFD");
        continue;
      }
      int* job_lib_id = resolve_id(&job_conn_fd);
      int* job_ip_str_sz = resolve_sz(&job_conn_fd);
      char* job_ip_str = resolve_input_str(&job_conn_fd, *job_ip_str_sz);
      D_lib* dlib = (D_lib*)malloc(sizeof(D_lib));
      dlib->ID = *job_lib_id;
      if(!checkLib(dlib)){
        perror("EWORKER_LIBSEARCH");
        continue;
      }      
      get_lib_pth(dlib);
      get_lib_handle(dlib);
      get_fp(dlib);
      // call to parent to find that its successfully found and
      // is about to be scheduled (i mean what else can go wrong now?)
      char resp_msg[] = "sched";
      int faas_resp_n = write(pipe_fd[1], resp_msg, sizeof(resp_msg)+1);
      if(faas_resp_n<0){
        perror("WORKER_SCHED_RESP");
        continue;
      }
      // and this is the call; kinda underwhelming?
      // TODO: Add timeout limits so as to limit the function's 
      // execution to only 1minute at most
      int job_result = dlib->fn(job_ip_str);
      char job_res_msg[10];// not more than 10-digit codes allowed for now
      snprintf(job_res_msg,sizeof(job_result),"%d",job_result);
      // TODO: Check the following, i find it sus
      send(job_conn_fd, &job_res_msg, sizeof(job_res_msg)-1, 0);
      // since the job is now done, send state-change resp
      char job_done_msg[] = "done";
      int faas_done_n = write(pipe_fd[1], job_done_msg, sizeof(job_done_msg)+1);
      if(faas_done_n<0){
        perror("WORKER_DONE_RESP");
        continue;
      }
      close(job_conn_fd);
      close(conn);
    }
}


// TODO: (i probably wont lower worker perms as for now idc much about security.)
//  [] lower perms, do the parent process's job. (probably using setuid, although i would need to read : https://people.eecs.berkeley.edu/~daw/papers/setuid-usenix02.pdf)
Worker* fx_newWorker(int id){
    Worker* new_Worker = (Worker*)malloc(sizeof(Worker));
    char sock_path[24];
    snprintf(sock_path,sizeof(sock_path),"/tmp/worker_%d.sock",id);
    int pipe_fd[2];
    if(pipe(pipe_fd) == -1){
      perror("EWORKER_PIPE");
      exit(1);
    }
    pid_t worker_pid = fork();
    if(worker_pid<0){
      perror("EWORKER_FORK");
      exit(1);
    }    
    if(worker_pid == 0){
      worker_systemd(sock_path, id, pipe_fd);
    }
    else{
      new_Worker->pid = worker_pid;
      new_Worker->com_fd[0] = pipe_fd[0];
      new_Worker->com_fd[1] = pipe_fd[1];
      // TODO: Ideally we should communicate and wait for confimation of
      // UNIX socker server being set up and then do the following, but im lazy
      snprintf(new_Worker->sun_path,sizeof(new_Worker->sun_path),"%s",sock_path);
    }
    // after all necessary things have been set up  
    new_Worker->state = 0;
    return new_Worker;
}



// the scheduler for cfaas main
// for now, its a simple iterable and lock-based
// scheduler, but later on i may do some heuristics
// it iterates through the workers and then schedules the job
// in the first one it finds to be free, if none are free, then waits
int fx_sched(Worker* workers, int* fd, char* input){
  //lock here
  for(int worker_id = 0; worker_id<NUM_WORKERS; worker_id++){
    if(workers[worker_id].state == 0){
      
    }
  }


  // unlock here


}



