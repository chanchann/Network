#ifndef SERVER_H 
#define SERVER_H 

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>

#include "clientSock.h"

#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
class Server {
public:
    Server() = default;
    Server(int port, int backlog = 5) : _port(port), _backlog(backlog) {}
    ~Server() { this->close() }; 

    void close();
    void close(int sockfd);
    
    void listen();

    inline void setBacklog(int backlog) { _backlog = backlog; }
    inline void setPort(int port) { _port = port; }

private:
    int _port = 10000;
    int _backlog = 5;
    int _sockfd = -1;
 
    std::vector< std::shared_ptr<ClientSock> > _clientSockets; // 存储已经连接上的client sock
    std::vector<int> _staleFds;
    std::mutex _staleFdsMutex;
};






#endif	// SERVER_H
