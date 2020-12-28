#ifndef SERVER_H 
#define SERVER_H 
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

class Server {
public:
    Server(port) : serv_port(port) {};
    ~Server() = default;

    void setup() {
        bzero(&servaddr, sizeof(servaddr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(serv_port);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        lfd = socket(AF_INET, SOCK_STREAM, 0);
        if(lfd == -1) {
            
        }
    }
private:
    int _backlog = 1;
    int _port = 10000;
    int _sockfd = 0;
    struct sockaddr_in addr;

}







#endif	// SERVER_H