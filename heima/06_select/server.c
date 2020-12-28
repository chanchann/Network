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
            //�����ļ�����������allset����µ��ļ�������connfd
            FD_SET(cfd, &allset);
            if(maxfd < cfd) {
                maxfd = cfd;
            }
            if(nready == 1) { // ֻ��lfd���¼���������for��ִ��
                continue;
            }
        }
        for(i = lfd + 1; i <= maxfd; i++) {
            if(FD_ISSET(i, &rset)) {
                // �׽��� ���� 0 --> �ر�
                if((n = Read(i, buf, sizeof(buf))) == 0) {
                    // ��client�ر�����ʱ�������Ҳ�رն�Ӧ����
                    Close(i);
                    // ���select�Դ��ļ��������ļ��
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