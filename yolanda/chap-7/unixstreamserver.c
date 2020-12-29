//
// Created by shengym on 2019-07-14.
//

#include  "lib/common.h"

// 这个本地文件，必须是一个“文件”，不能是一个“目录”。如果文件不存在，后面 bind 操作时会自动创建这个文件。
// 在 Linux 下，任何文件操作都有权限的概念
int main(int argc, char **argv) {
    if (argc != 2) {
        error(1, 0, "usage: unixstreamserver <local_path>");
    }

    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_un cliaddr, servaddr;
    // 这里创建的套接字类型，注意是 AF_LOCAL，并且使用字节流格式。
    listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (listenfd < 0) {
        error(1, errno, "socket create failed");
    }
    // 创建了一个本地地址
    // 关于本地文件路径，最好是“绝对路径"
    char *local_path = argv[1];
    // unlink 操作，以便把存在的文件删除掉，这样可以保持幂等性
    unlink(local_path);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, local_path);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        error(1, errno, "bind failed");
    }

    if (listen(listenfd, LISTENQ) < 0) {
        error(1, errno, "listen failed");
    }

    clilen = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
        if (errno == EINTR)
            error(1, errno, "accept failed");        /* back to for() */
        else
            error(1, errno, "accept failed");
    }

    char buf[BUFFER_SIZE];

    while (1) {
        bzero(buf, sizeof(buf));
        if (read(connfd, buf, BUFFER_SIZE) == 0) {
            printf("client quit");
            break;
        }
        printf("Receive: %s", buf);

        char send_line[MAXLINE];
        bzero(send_line, MAXLINE);
        sprintf(send_line, "Hi, %s", buf);

        int nbytes = sizeof(send_line);

        if (write(connfd, send_line, nbytes) != nbytes)
            error(1, errno, "write error");
    }

    close(listenfd);
    close(connfd);

    exit(0);

}
