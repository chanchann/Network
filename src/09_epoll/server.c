#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 6690
#define OPEN_MAX 1024

int main(int argc, char** argv)
{
    int i, n, num;
    int lfd, cfd, sockfd;
    ssize_t nready, efd, res; 
    char buf[MAXLINE], str[INET_ADDRSTRLEN];  // #define INET_ADDRSTRLEN 16
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    // tep : epoll_ctl参数, ep[]: epoll_wait参数
    struct epoll_event tep, ep[OPEN_MAX]; 

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
    
    // 创建epoll模型,efd指向红黑树根节点
    efd = epoll_create(OPEN_MAX);    
    if(efd == -1) {
        perr_exit("epoll_create error");
    }
    // 创建lfd的监听事件为读
    tep.events = EPOLLIN;
    tep.data.fd = lfd;

    // 将lfd及对应的结构体设置到树上，efd可找到该树
    res = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);
    if(res == -1) {
        perr_exit("epoll_ctl err");
    }

    while(1) {
        // epoll为server阻塞监听事件，ep为struct epoll_event类型数组
        // OPEN_MAX为数组容量, -1表示永久阻塞
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if(nready == -1) {
            perr_exit("epoll_wait error");
        }
        for(i = 0; i < nready; i++) {
            if(!(ep[i].events & EPOLLIN))  // 如果不是读事件，继续循环
                continue; 
            if(ep[i].data.fd == lfd) {   // 判断满足事件的fd是不是lfd
                clit_addr_len = sizeof(clit_addr);
                cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);

                printf("recieve from %s at PORT %d\n",
                inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
                ntohs(clit_addr.sin_port));
                printf("cfd %d --- client %d\n", cfd, ++num);
                
                tep.events = EPOLLIN;
                tep.data.fd = cfd;
                // 加入红黑树
                res = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);
                if(res == -1) {
                    perr_exit("epoll_ctl error");
                }
            } else {        // 不是lfd
                sockfd = ep[i].data.fd;
                n = Read(sockfd, buf, MAXLINE);
                if(n == 0) {            // 读到0，说明客户端关闭连接
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, cfd, &tep);  // 将该文件描述符从红黑树摘除
                    if(res == -1) {
                        perr_exit("epoll_ctl error");
                    }
                    Close(sockfd);
                    printf("client[%d] closed connection\n", sockfd);
                } else if(n < 0) { // 出错
                    perror("read n < 0");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL); // 摘除结点
                    Close(sockfd);
                } else {
                    for(i = 0; i < n; i++) {
                        buf[i] = toupper(buf[i]);
                    }
                    Write(STDOUT_FILENO, buf, n);
                    Write(sockfd, buf, n);
                }
            }
        }
    }
    Close(lfd);
    return 0;
}