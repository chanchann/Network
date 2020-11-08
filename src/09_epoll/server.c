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
    // tep : epoll_ctl����, ep[]: epoll_wait����
    struct epoll_event tep, ep[OPEN_MAX]; 

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    // init serv_addr 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // �˿ڶ�·����
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 128);
    
    // ����epollģ��,efdָ���������ڵ�
    efd = epoll_create(OPEN_MAX);    
    if(efd == -1) {
        perr_exit("epoll_create error");
    }
    // ����lfd�ļ����¼�Ϊ��
    tep.events = EPOLLIN;
    tep.data.fd = lfd;

    // ��lfd����Ӧ�Ľṹ�����õ����ϣ�efd���ҵ�����
    res = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);
    if(res == -1) {
        perr_exit("epoll_ctl err");
    }

    while(1) {
        // epollΪserver���������¼���epΪstruct epoll_event��������
        // OPEN_MAXΪ��������, -1��ʾ��������
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if(nready == -1) {
            perr_exit("epoll_wait error");
        }
        for(i = 0; i < nready; i++) {
            if(!(ep[i].events & EPOLLIN))  // ������Ƕ��¼�������ѭ��
                continue; 
            if(ep[i].data.fd == lfd) {   // �ж������¼���fd�ǲ���lfd
                clit_addr_len = sizeof(clit_addr);
                cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);

                printf("recieve from %s at PORT %d\n",
                inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
                ntohs(clit_addr.sin_port));
                printf("cfd %d --- client %d\n", cfd, ++num);
                
                tep.events = EPOLLIN;
                tep.data.fd = cfd;
                // ��������
                res = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);
                if(res == -1) {
                    perr_exit("epoll_ctl error");
                }
            } else {        // ����lfd
                sockfd = ep[i].data.fd;
                n = Read(sockfd, buf, MAXLINE);
                if(n == 0) {            // ����0��˵���ͻ��˹ر�����
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, cfd, &tep);  // �����ļ��������Ӻ����ժ��
                    if(res == -1) {
                        perr_exit("epoll_ctl error");
                    }
                    Close(sockfd);
                    printf("client[%d] closed connection\n", sockfd);
                } else if(n < 0) { // ����
                    perror("read n < 0");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL); // ժ�����
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