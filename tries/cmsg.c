#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
// to forward a file descriptor we will need another socket
// ---sock0(AF_INET)-------appln1(this) ----sock1(UNIX)------common_sock-----sock2-----appln2
// and they comm over UNIX domain socket with control messages 

int connect_unix(){
  struct sockaddr_un addr = {
    .sun_path = "/tmp/scm_try.sock",
    .sun_family = AF_UNIX
  };
  // the socket used to interface this sending
  int unix_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(unix_sock == -1){
    return -1;
  }
  // now we connect to the common socket file
  if( connect(unix_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1){
      close(unix_sock);
      return -1;
  }
  return unix_sock;
}


int send_fd(int unix_sock, int fd){
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

  return sendmsg(unix_sock, &msg, 0);
}
int main(){
  struct sockaddr_in addr = {
    .sin_addr = (struct in_addr){.s_addr = INADDR_ANY},
    .sin_port = htons(8001),
    .sin_family = AF_INET
  };
  int server_sock = socket(AF_INET, SOCK_STREAM, 0);
  int _ =bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
  int __ = listen(server_sock, 50);
  while(1){
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    int tcp_conn = accept(server_sock, (struct sockaddr*)&remote_addr, &addr_len);
    char buf[2] = {0};
    ssize_t n = recv(tcp_conn, &buf, 2, 0);
    if(n == 2 && buf[0] == 'o' && buf[1] == 'k'){
      int unix_sock = connect_unix();
      int err = send_fd(unix_sock, tcp_conn);
      close(unix_sock);
      close(tcp_conn);
    } 
    else{
      printf("This boi responded!\n");
      char msg[] = "C responded";
      send(tcp_conn, &msg, sizeof(msg)-1, 0);
      close(tcp_conn);
    }
  }
}
