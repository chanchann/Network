#include "wrap.h"

#define SERV_PORT 6666

int main(int argc, char** argv)
{
    int i, j, n, nready;
    int maxfd = 0;
    int lfd = 0, cfd = 0; 
    char buf[BUFSIZ]; // BUFSIZ : 1024
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
    while(1) {
        ret = read(cfd, buf, sizeof(buf));
        // 读到先显示一下
        write(STDOUT_FILENO, buf, ret);
        if(ret == -1) {
            sys_err("read error");
        }
        int i;
        for(i = 0; i < ret; i++) {
            buf[i] = toupper(buf[i]);
        }
        ret = write(cfd, buf, ret);
    }
    close(lfd);
    close(cfd);
    return 0;
}