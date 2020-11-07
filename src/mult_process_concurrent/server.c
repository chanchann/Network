#include "wrap.h"
#include <signal.h>

#define SERV_PORT 6669

void catch_child(int signum) {
    while(waitpid(0, NULL, WNOHANG) > 0) {};
    return;
}

int main() {
    int lfd, cfd;
    int ret, i;
    pid_t pid;
    char buf[BUFSIZ];
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len = sizeof(clit_addr);
    // memset(&serv_addr, 0, sizeof(serv_addr)); //将地址结构清零
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 128);

    while(1) {   
        cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
        pid = fork();
        if(pid < 0) {
            perr_exit("fork error");
        } else if(pid == 0) {  // 子进程
            close(lfd);
            break;
        } else {  // 父进程
            // 回收子进程，避免僵尸进程出现
            struct sigaction act;
            act.sa_handler =  catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            ret = sigaction(SIGCHLD, &act, NULL);
            if(ret != 0) {
                perr_exit("sigaction error");
            }
            close(cfd);
            continue;
        }
    }
    if(pid == 0) {
        while(1) {
            ret = Read(cfd, buf, sizeof(buf));
            if(ret == 0) {
                close(cfd);
                exit(1);
            }
            for(i = 0; i < ret; i++) {
                buf[i] = toupper(buf[i]);
            }
            Write(cfd, buf, ret);
            Write(STDOUT_FILENO, buf, ret);
        }
    }
    return 0;

}