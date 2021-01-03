#ifndef CLIENTSOCK_H 
#define CLIENTSOCK_H 
#include "server.h"
#include <string>

class ClientSock {
public:
    ClientSock(int connfd, Server& server);
    ~ClientSock() = default;
    ClientSock(const ClientSock&) = delete;
    ClientSock& operator=(const ClientSock&) = delete; 

    int getConnfd() const;
    
    std::string read();
    void write(const std::string& data);
    void close();

private:
    int _connfd = -1;
    Server& _server;    // Aggregation

};



#endif	// CLIENTSOCK_H