
#include "wrap.h"

#define SERV_PORT 9527
/*
server 
*/

int main(int argc, char** argv)
{
    int lfd = 0, cfd = 0; //用于连接的文件描述符
    int ret;
    char buf[BUFSIZ], client_IP[BUFSIZ]; // BUFSIZ : 1024
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    // init serv_addr 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
    ret = bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret == -1) {
        sys_err("bind error");
    }
    Listen(lfd, 128);
    clit_addr_len = sizeof(clit_addr);
    cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
    if(cfd == -1) {
        sys_err("accept error");
    }
    printf("client ip: %s port: %d\n", 
        inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_IP, sizeof(client_IP)),
        ntohs(clit_addr.sin_port));
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