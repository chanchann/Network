#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 6690
#define OPEN_MAX 1024

int main(int argc, char** argv)
{
    int i, j, nready, maxi;
    int lfd, cfd, sockfd;
    ssize_t n; 
    char buf[MAXLINE], str[INET_ADDRSTRLEN];  // #define INET_ADDRSTRLEN 16
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    struct pollfd client[OPEN_MAX];
    
    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // init serv_addr 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 端口多路复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 128);
    // 监听的第一个文件描述符lfd 存入client[0]
    client[0].fd = lfd;
    client[0].events = POLLIN;  // lfd监听普通读事件

    // client[]全部初始化为-1,0也是文件描述符，不能用
    // 注意从第一个开始，第0个已经赋值了
    for(i = 1; i < OPEN_MAX; i++) {
        client[i].fd = -1;
    }
    // client[]有效元素最大元素下标
    maxi = 0;
    while(1) {
        // -1 : 阻塞监听
        nready = poll(client, maxi + 1, -1);
        if(nready == -1) {
            perr_exit("poll error");
        }
        if(client[0].revents & POLLIN) {  // lfd有读事件就绪
            clit_addr_len = sizeof(clit_addr);
            // 接收客户端请求Accept不会阻塞
            cfd = Accept(lfd, (struct sockaddr*)&clit_addr_len, &clit_addr_len);
            printf("receive from %s at PORT %d\n",
            inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
            ntohs(clit_addr.sin_port));

            for(i = 1; i < OPEN_MAX; i++) {
                if(client[i].fd < 0) {
                    client[i].fd = cfd;  //找到client[]中空闲的位置，存放accept返回的cfd
                    break;
                }
            }
            if(i == OPEN_MAX) {
                perr_exit("Too many clients !!!");
            }
            // 设置刚刚返回的cfd,监控读事件
            client[i].events = POLLIN;
            if(i > maxi) {
                maxi = i;
            }
            // 只有lfd,没有更多就绪事件，继续回到poll阻塞
            if(--nready <= 0) {
                continue;
            }
        }
        for(i = 1; i <= maxi; i++) {
            if((sockfd = client[i].fd) < 0) {
                continue;
            }
            if(client[i].revents & POLLIN) {
                if( (n = Read(sockfd, buf, MAXLINE)) < 0) {
                    // 收到RST标志
                    if(errno == ECONNRESET) {
                        printf("client[%d] aborted connection\n", i);
                        Close(sockfd);
                        // poll中不监控该文件描述符，可直接置-1，不用像select那样移除
                        client[i].fd = -1;
                    } else {
                        perr_exit("read error");
                    } 
                } else if(n == 0) {
                    // 客户端先关闭连接
                    printf("client[%d] closed connection\n", i);
                    Close(sockfd);
                    client[i].fd = -1;
                }else {
                    for(j = 0; j < n; j++) {
                        buf[j] = toupper(buf[j]);
                    }
                    Write(sockfd, buf, n);
                    Write(STDOUT_FILENO, buf, n);
                }
                if(--nready == 0) break;
            }
        }
    }
    Close(lfd);
    return 0;
}