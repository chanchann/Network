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

    // �˿ڶ�·����
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    Listen(lfd, 128);
    // �����ĵ�һ���ļ�������lfd ����client[0]
    client[0].fd = lfd;
    client[0].events = POLLIN;  // lfd������ͨ���¼�

    // client[]ȫ����ʼ��Ϊ-1,0Ҳ���ļ���������������
    // ע��ӵ�һ����ʼ����0���Ѿ���ֵ��
    for(i = 1; i < OPEN_MAX; i++) {
        client[i].fd = -1;
    }
    // client[]��ЧԪ�����Ԫ���±�
    maxi = 0;
    while(1) {
        // -1 : ��������
        nready = poll(client, maxi + 1, -1);
        if(nready == -1) {
            perr_exit("poll error");
        }
        if(client[0].revents & POLLIN) {  // lfd�ж��¼�����
            clit_addr_len = sizeof(clit_addr);
            // ���տͻ�������Accept��������
            cfd = Accept(lfd, (struct sockaddr*)&clit_addr_len, &clit_addr_len);
            printf("receive from %s at PORT %d\n",
            inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
            ntohs(clit_addr.sin_port));

            for(i = 1; i < OPEN_MAX; i++) {
                if(client[i].fd < 0) {
                    client[i].fd = cfd;  //�ҵ�client[]�п��е�λ�ã����accept���ص�cfd
                    break;
                }
            }
            if(i == OPEN_MAX) {
                perr_exit("Too many clients !!!");
            }
            // ���øոշ��ص�cfd,��ض��¼�
            client[i].events = POLLIN;
            if(i > maxi) {
                maxi = i;
            }
            // ֻ��lfd,û�и�������¼��������ص�poll����
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
                    // �յ�RST��־
                    if(errno == ECONNRESET) {
                        printf("client[%d] aborted connection\n", i);
                        Close(sockfd);
                        // poll�в���ظ��ļ�����������ֱ����-1��������select�����Ƴ�
                        client[i].fd = -1;
                    } else {
                        perr_exit("read error");
                    } 
                } else if(n == 0) {
                    // �ͻ����ȹر�����
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