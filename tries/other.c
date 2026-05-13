#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SOCK_PATH "/tmp/scm_try.sock"

int setup_unix_server() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);

    unlink(SOCK_PATH);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sock, 5) == -1) {
        perror("listen");
        exit(1);
    }

    chmod(SOCK_PATH, 0777);

    printf("Listening on %s...\n", SOCK_PATH);
    return sock;
}

int recv_fd(int conn) {
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

int main() {
    int server = setup_unix_server();

    while (1) {
        int conn = accept(server, NULL, NULL);
        if (conn == -1) {
            perror("accept");
            continue;
        }

        printf("Sender connected\n");

        int fd = recv_fd(conn);
        if (fd == -1) {
            printf("Failed to receive FD\n");
            close(conn);
            continue;
        }

        printf("Received FD: %d\n", fd);

        // This FD is a TCP socket from your sender
        const char *msg = "Hello from C receiver!";
        write(fd, msg, strlen(msg));

        close(fd);
        close(conn);
    }

    close(server);
    return 0;
}
