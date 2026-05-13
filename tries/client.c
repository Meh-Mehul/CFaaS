
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT 8001

int connect_and_send(const char *input) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }

    // send input
    send(sock, input, strlen(input), 0);

    // receive response
    char buf[1024];
    ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);

    if (n > 0) {
        buf[n] = '\0';
        printf("Response: %s\n", buf);
    } else if (n == 0) {
        printf("Server closed connection (no response)\n");
    } else {
        perror("recv");
    }

    close(sock);
    return 0;
}

int main() {
    char input[1024];

    while (1) {
        printf("\nEnter message (type 'exit' to quit): ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        // remove newline
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0)
            break;

        connect_and_send(input);
    }

    return 0;
}
