#include "lib/common.h"

# define MESSAGE_SIZE 10240000

void send_data(int sockfd) {
    char *query;
    query = malloc(MESSAGE_SIZE + 1);
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        query[i] = 'a';
    }
    query[MESSAGE_SIZE] = '\0';

    const char *cp;
    cp = query;
    size_t remaining = strlen(query);
    while (remaining) {
        // 在阻塞I/O的情况下，send函数将阻塞直至将数据全部发送至发送缓存区，此种情况下，n_written等于remaining； 
        // 而在非阻塞I/O的情况下，send函数是"能写多少写多少"，所以n_written就不等于remaining了，而send_data函数为了同时对阻塞I/O和非阻塞I/O起作用，就用while循环了
        int n_written = send(sockfd, cp, remaining, 0);
        // 最后才打印下面这句话，说明send这里一直是阻塞的
        // 也就是说阻塞式套接字最终发送返回的实际写入字节数和请求字节数是相等的。
        // 非阻塞的后面再看
        fprintf(stdout, "send into buffer %ld \n", n_written);
        if (n_written <= 0) {
            error(1, errno, "send failed");
            return;
        }
        remaining -= n_written;
        cp += n_written;
    }

    return;
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2)
        error(1, 0, "usage: tcpclient <IPaddress>");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    int connect_rt = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (connect_rt < 0) {
        error(1, errno, "connect failed ");
    }
    send_data(sockfd);
    exit(0);
}

