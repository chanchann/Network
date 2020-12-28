#include "clientSock.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

ClientSock::ClientSock(int connfd, Server& server) 
        : _connfd(connfd)
        , _server(server) 
    {};

ClientSock::~ClientSock() 
{};
 
int ClientSock::getConnfd() const {
    return _connfd;
}
    
std::string ClientSock::read() {
    std::string msg;

    char buf[BUFSIZ] = { 0 };
    ssize_t numBytes = 0;
    // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    // MSG_DONTWAIT :将单个I／O操作设置为非阻塞模式
    while( ( numBytes = recv(_connfd, buf, sizeof(buf), MSG_DONTWAIT) ) > 0 ) {
        buf[numBytes] = 0;
        msg += buf;
    }
    return msg;
};

void ClientSock::write(const std::string& data) {
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    auto res = send( _connfd, 
                    reinterpret_cast<const void*>( data.c_str() ),
                    data.size(),
                    0 );
    if(res == -1) 
        throw std::runtime_error( std::string( strerror( errno ) ) );
}
void ClientSock::close() {
    _server.close(_connfd);
}