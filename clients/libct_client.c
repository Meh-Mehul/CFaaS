#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#define PORT 6969
#define BUFFER_SIZE 4096

static int send_all(int sock, const void* data, size_t size){
    const char* cursor = (const char*)data;
    size_t total = 0;
    while(total < size){
        ssize_t n = send(sock, cursor + total, size - total, 0);
        if(n <= 0){
            return -1;
        }
        total += n;
    }
    return 0;
}

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Usage: %s <source-file.c>\n", argv[0]);
        return 1;
    }
    const char* path = argv[1];
    struct stat st;
    if(stat(path, &st) != 0){
        perror("stat");
        return 1;
    }
    long file_size = st.st_size;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){ perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("connect"); close(sock); return 1;
    }

    // send file size (server expects a long)
    if(send_all(sock, &file_size, sizeof(file_size)) < 0){
        perror("send filesize"); close(sock); return 1;
    }

    // send file contents
    int fd = open(path, O_RDONLY);
    if(fd == -1){ perror("open"); close(sock); return 1; }
    char buf[BUFFER_SIZE];
    ssize_t r;
    while((r = read(fd, buf, sizeof(buf))) > 0){
        if(send_all(sock, buf, r) < 0){
            perror("send file"); close(fd); close(sock); return 1;
        }
    }
    close(fd);

    // receive server response (path or error)
    char resp[1024];
    ssize_t n = recv(sock, resp, sizeof(resp)-1, 0);
    if(n > 0){ resp[n] = '\0'; printf("Server: %s\n", resp); }
    else if(n == 0) { printf("Server closed connection\n"); }
    else { perror("recv"); }

    close(sock);
    return 0;
}
