#include "wrap.h"

#define SERV_PORT 6666

int main() {
    struct sockaddr_in serv_addr;
    char buf[BUFSIZ];
    int sockfd, n;
    
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    Connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("------------- Connect ok!!! -------------");
    while(fgets(buf, BUFSIZ, stdin) != NULL) {
        Write(sockfd, buf, strlen(buf));
        n = Read(sockfd, buf, BUFSIZ);
        if(n == 0) {
            printf("The other side has been closed.\n");
            break;
        } else {
            Write(STDOUT_FILENO, buf, n);
        }
    }
    Close(sockfd);
    return 0;
}