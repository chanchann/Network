#include "wrap.h"

#define MAXLINE 10
#define SERV_PORT 6666
int main(int argc, char* argv[]) {
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    int lfd, cfd;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    int efd;
    int flag;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 20);
    
    struct epoll_event event;
    struct epoll_event resevent[10];
    int res, len;

    efd = epoll_create(10);
    event.events = EPOLLIN | EPOLLET;   // ET 边沿触发
    // event.events = EPOLLIN;  // 默认LT水平触发

    printf("Accepting connections ... \n");

    clit_addr_len = sizeof(clit_addr);
    cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
    printf("receive from %s at PORT %d\n",
        inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
        ntohs(clit_addr.sin_port));

    /* ------------------------------------- */
    // 修改cfd为非阻塞读
    flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    /* ------------------------------------- */
    event.data.fd = cfd;
    // 只监听了cfd
    epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &event);
    while(1) {
        printf("epoll_wait begin\n");
        res = epoll_wait(efd, resevent, 10, -1);
        printf("epoll_wait end res %d\n", res);
        if(resevent[0].data.fd = cfd) {   
            len = Read(cfd, buf, MAXLINE/2);  // 非阻塞读， 轮询
            Write(STDOUT_FILENO, buf, len);
        }
    }
    return 0;
    
}