#include "lib/common.h"
// 缓冲区实验

// 我们用一个客户端 - 服务器的例子来解释一下读取缓冲区和发送缓冲区的概念。
// 在这个例子中客户端不断地发送数据，
// 服务器端每读取一段数据之后进行休眠，以模拟实际业务处理所需要的时间。

/*
/// 从socketfd描述字中读取"size"个字节. 

size_t readn(int fd, void *buffer, size_t size) {
    char *buffer_pointer = buffer;
    int length = size;

    /// 循环条件表示的是，在没读满 size 个字节之前，一直都要循环下去
    while (length > 0) {
        int result = read(fd, buffer_pointer, length);

        if (result < 0) {
            /// 非阻塞 I/O 的情况下，没有数据可以读，需要继续调用 read
            if (errno == EINTR)
                continue;    
            else
                return (-1);
        } else if (result == 0)
            break;                /// 读到对方发出的 FIN 包，表现形式是 EOF，此时需要关闭套接字。

        /// 需要读取的字符数减少，缓存指针往下移动。
        length -= result;
        buffer_pointer += result;
    }
    return (size - length);        /// 返回的是实际读取的字节数
}

*/


void read_data(int sockfd) {
    ssize_t n;
    char buf[1024];

    int time = 0;
    for (;;) {
        fprintf(stdout, "block in read\n");
        if ((n = readn(sockfd, buf, 1024)) == 0)
            return;

        time++;
        fprintf(stdout, "1K read for %d \n", time);
        usleep(1000);
    }
}


int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(12345);

    /* bind到本地地址，端口为12345 */
    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    /* listen的backlog为1024 */
    listen(listenfd, 1024);

    /* 循环处理用户请求 */
    for (;;) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
        read_data(connfd);   /* 读取数据 */
        close(connfd);          /* 关闭连接套接字，注意不是监听套接字*/
    }
}

