#include "funcex.h"
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
      // TODO: Actually do stuff for the request now (finally!)
      // donot forget to communicate using pipe_fd
      close(job_conn_fd);
      
      close(conn);
    }
}


// TODO:
// newWorker steps:
//  [-] fork to a child process
//  [] lower perms, do the parent process's job (getpid, set-up sun path and pipe), return worker
//  [-] in child, run the worker code (unix socket server, recv and run and state change machine)
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




int fx_sched(int* fd, char* input){
  


}



