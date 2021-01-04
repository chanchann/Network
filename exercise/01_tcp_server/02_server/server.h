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

class ClientSock; // Solve circular include
class Server {
public:
    Server() = default;
    Server(int port, int backlog = 5) : _port(port), _backlog(backlog) {}
    ~Server() { this->close(); } 

    void close();
    void close(int sockfd);
    
    void listen();

    inline void setBacklog(int backlog) { _backlog = backlog; }
    inline void setPort(int port) { _port = port; }

    template <class T> void onAccept( T&& t ) { _handleAccept = t; }
    template <class T> void onRead( T&& t ) { _handleRead = t; }

private:
    int _port = 10000;
    int _backlog = 5;
    int _sockfd = -1;
 
    std::vector< std::shared_ptr<ClientSock> > _clientSockets; // 存储已经连接上的client sock
    std::vector<int> _staleFds;
    std::mutex _staleFdsMutex;

    std::function< void ( std::weak_ptr<ClientSock> socket ) > _handleAccept;
    std::function< void ( std::weak_ptr<ClientSock> socket ) > _handleRead;
};


#endif	// SERVER_H
