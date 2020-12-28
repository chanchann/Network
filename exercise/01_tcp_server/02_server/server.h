#ifndef SERVER_H 
#define SERVER_H 

#include <vector>
#include <memory>

class Server {
public:
    Server() = default;
    Server(int port, int backlog = 1) : _port(port), _backlog(backlog) {}
    ~Server(); 

    void close(int connfd);
private:
    int _port = 10000;
    int _backlog = 1;
    int sockfd = -1;
 
    std::vector< std::shared_ptr<ClientSock> > _clientSockets; // 存储已经连接上的client sock

};






#endif	// SERVER_H
