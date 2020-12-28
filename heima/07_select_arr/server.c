#include "wrap.h"

#define SERV_PORT 6666

int main(int argc, char** argv)
{
    int i, j, n, nready, maxi;
    // �Զ�������client,��ֹ����1024���ļ������� FD_SETSIZEĬ��1024
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

    // �˿ڶ�·����
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    Listen(lfd, 128);
    // rset���¼��ļ����������ϣ�allset�����ݴ�
    fd_set rset, allset;
    maxfd = lfd;
    FD_ZERO(&allset);
    FD_SET(lfd, &allset);

    maxi = -1; // ������Ϊclient[]���±꣬��ʼֵָ��0��Ԫ��֮ǰ�±�λ��
    // client[]ȫ����ʼ��Ϊ-1
    for(i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }

    while(1) {
        // ÿ��ѭ������������select����źż�
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if(nready < 0) {
            perr_exit("select error");
        }
        if(FD_ISSET(lfd, &rset)) {  // ˵�����µĿͻ�����������
            clit_addr_len = sizeof(clit_addr);
            // Accept ��������
            cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
            printf("received from %s at PORT %d\n",
            inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
            ntohs(clit_addr.sin_port));
            for(i = 0; i < FD_SETSIZE; i++) {  //��client[]��û��ʹ�õ�λ��
                if(client[i] < 0) {
                    client[i] = cfd;  // ����accept���ص��ļ���������client[]��
                    break;
                }
            }
            // �ﵽselect�ܼ�ص��ļ���������1024
            if(i == FD_SETSIZE) {
                printf("Too many clients\n");
                exit(1);
            }
            
            //�����ļ�����������allset�����µ��ļ�������connfd
            FD_SET(cfd, &allset);
            if(maxfd < cfd) {
                maxfd = cfd;
            }
            // ��֤maxi�������client[]���һ��Ԫ���±�
            if(maxi < i) {
                maxi = i;
            }

            if(--nready == 0) { // ֻ��lfd���¼���������for��ִ��
                continue;
            }
        }
        // ����ĸ�client �����ݾ���
        for(i = 0; i <= maxi; i++) {
            if((sockfd = client[i]) < 0) {
                continue;
            }
            if(FD_ISSET(sockfd, &rset)) {
                // ��client �ر�����ʱ�������Ҳ�ر���Ӧ����
                if((n = Read(sockfd, buf, sizeof(buf))) == 0) {
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else if(n > 0) {
                    for(j = 0; j < n; j++) {
                        buf[j] = toupper(buf[j]);
                    }
                    Write(sockfd, buf, n);
                    Write(STDOUT_FILENO, buf, n);
                }
                // ע��������break for
                if(--nready == 0) break;  // �ղ�--nready�ǰ�lfd��ȥ������ѵ�ǰcfd��ȷ�����ˣ����ü���������
            }
        }
    }
    Close(lfd);
    return 0;
}