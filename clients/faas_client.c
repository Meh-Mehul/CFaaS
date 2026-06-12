#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT 8000

int main(int argc, char** argv){
    if(argc < 3){
        fprintf(stderr, "Usage: %s <lib_id> <input-string>\n", argv[0]);
        return 1;
    }
    int lib_id = atoi(argv[1]);
    char* input = argv[2];
    int input_sz = (int)strlen(input) + 1; // include null terminator

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){ perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("connect"); close(sock); return 1;
    }

    // send lib id (int)
    if(send(sock, &lib_id, sizeof(lib_id), 0) != sizeof(lib_id)){
        perror("send id"); close(sock); return 1;
    }

    // send input size (int)
    if(send(sock, &input_sz, sizeof(input_sz), 0) != sizeof(input_sz)){
        perror("send sz"); close(sock); return 1;
    }

    // send input string (including null)
    int sent = 0;
    while(sent < input_sz){
        ssize_t n = send(sock, input + sent, input_sz - sent, 0);
        if(n <= 0){ perror("send input"); close(sock); return 1; }
        sent += n;
    }

    // receive response
    char resp[1024];
    ssize_t n = recv(sock, resp, sizeof(resp)-1, 0);
    if(n > 0){ resp[n] = '\0'; printf("Result: %s\n", resp); }
    else if(n == 0) { printf("Server closed connection\n"); }
    else { perror("recv"); }

    close(sock);
    return 0;
}
