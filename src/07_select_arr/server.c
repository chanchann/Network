#include "wrap.h"

#define SERV_PORT 6666

int main(int argc, char** argv)
{
    int i, j, n, nready, maxi;
    // 自定义数组client,防止遍历1024个文件描述符 FD_SETSIZE默认1024
    int client[FD_SETSIZE];
    int maxfd = 0, sockfd;
    int lfd = 0, cfd = 0; 
    char buf[BUFSIZ], str[INET_ADDRSTRLEN];  // #define INET_ADDRSTRLEN 16
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    
    // init serv_addr 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // 端口多路复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    Listen(lfd, 128);
    // rset读事件文件描述符集合，allset用来暂存
    fd_set rset, allset;
    maxfd = lfd;
    FD_ZERO(&allset);
    FD_SET(lfd, &allset);

    maxi = -1; // 将来作为client[]的下标，初始值指向0个元素之前下标位置
    // client[]全部初始化为-1
    for(i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }

    while(1) {
        // 每次循环都重新设置select监控信号集
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if(nready < 0) {
            perr_exit("select error");
        }
        if(FD_ISSET(lfd, &rset)) {  // 说明有新的客户端连接请求
            clit_addr_len = sizeof(clit_addr);
            // Accept 不会阻塞
            cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
            printf("received from %s at PORT %d\n",
            inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
            ntohs(clit_addr.sin_port));
            for(i = 0; i < FD_SETSIZE; i++) {  //找client[]中没有使用的位置
                if(client[i] < 0) {
                    client[i] = cfd;  // 保存accept返回的文件描述符到client[]里
                    break;
                }
            }
            // 达到select能监控的文件个数上限1024
            if(i == FD_SETSIZE) {
                printf("Too many clients\n");
                exit(1);
            }
            
            //向监控文件描述符集合allset添加新的文件描述符connfd
            FD_SET(cfd, &allset);
            if(maxfd < cfd) {
                maxfd = cfd;
            }
            // 保证maxi存的总是client[]最后一个元素下标
            if(maxi < i) {
                maxi = i;
            }

            if(nready == 1) { // 只有lfd有事件，后续的for不执行
                continue;
            }
        }
        for(i = lfd + 1; i <= maxfd; i++) {
            if(FD_ISSET(i, &rset)) {
                // 套接字 读到 0 --> 关闭
                if((n = Read(i, buf, sizeof(buf))) == 0) {
                    // 当client关闭连接时，服务端也关闭对应连接
                    Close(i);
                    // 解除select对此文件描述符的监控
                    FD_CLR(i, &allset);
                }else if (n > 0){
                    for(j = 0; j < n; j++) {
                        buf[j] = toupper(buf[j]);
                    }
                    Write(i, buf, n);
                    Write(STDOUT_FILENO, buf, n);
                }
            }
        }
    }
    Close(lfd);
    return 0;
}