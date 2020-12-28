#include "wrap.h"
#include <signal.h>
#include <pthread.h>
#define MAXLINE 8192
#define SERV_PORT 6670

// 定义一个结构体，将地址结构和cfd捆绑
struct s_info {
    struct sockaddr_in cliaddr;
    int connfd;
};

void* do_work(void* arg) {
    int n, i;
    struct s_info* ts = (struct s_info*)arg;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN]; // #define INET_ADDRSTRLEN 16
    while(1) {
        n = Read(ts->connfd, buf, MAXLINE);   // 读客户端
        if(n == 0) {
            printf("The client %d closed ...\n", ts->connfd);
            break;
        } 
        printf("receive from %s at PORT %d\n",
                inet_ntop(AF_INET, &(*ts).cliaddr.sin_addr, str, sizeof(str)),
                ntohs((*ts).cliaddr.sin_port));
        for(i = 0; i < n; i++) {
            buf[i] = toupper(buf[i]);
        }
        Write(STDOUT_FILENO, buf, n);
        Write(ts->connfd, buf, n);  // 回写客户端
    }
    Close(ts->connfd);
    return (void*)0;   // the same as : pthread_exit(0);
}



int main() {
    struct sockaddr_in serv_addr, clit_addr;
    socklen_t clit_addr_len;
    int listenfd, connfd;
    int i = 0;
    pthread_t tid;
    struct s_info ts[256];  //创建结构体数组
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0); // 创建一个socket,得到lfd
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    Bind(listenfd, (struct sockaddr*)& serv_addr, sizeof(serv_addr));

    Listen(listenfd, 128);

    printf("Accepting client connect ....\n");

    while(1) {
        clit_addr_len = sizeof(clit_addr);
        connfd = Accept(listenfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
        ts[i].cliaddr = clit_addr;
        ts[i].connfd = connfd;

        pthread_create(&tid, NULL, do_work, (void*)&ts[i]);  // 注意这里最后一个参数为何这么传
        pthread_detach(tid);     // 子线程分离，防止僵尸线程产生
        i++;
    }
    return 0;

}