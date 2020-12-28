#include "wrap.h"

#define SERV_PORT 6666
#define MAXLINE 10
int main(int argc, char* argv[]) {
    struct sockaddr_in serv_addr;
    char buf[MAXLINE];
    int sockfd, i;
    char ch = 'a';

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(SERV_PORT);

    Connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    while(1) {
        // aaaa\n
        for(i = 0; i < MAXLINE/2; i++) {
            buf[i] = ch;
        }
        buf[i-1] = '\n';
        ch++;
        // bbbb\n
        for(; i < MAXLINE; i++) {
            buf[i] = ch;
        }
        buf[i-1] = '\n';
        ch++;
        // aaaa\nbbbb\n
        Write(sockfd, buf, sizeof(buf));
        sleep(5);
    }
    Close(sockfd);
    return 0;
}