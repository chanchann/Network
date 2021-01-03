#include "server.h"


void Server::close() {
    if(_sockfd)
        ::close(_sockfd);

    for(auto&& clientSocket : _clientSockets)
        clientSocket->close();

    _clientSockets.clear();
    // stale需要clear不
}


void Server::close(int sockfd) {
    std::lock_guard<std::mutex> lock(_staleFdsMutex);

    _clientSockets.erase( std::remove_if( _clientSockets.begin(), _clientSockets.end(),
                                            [&] (std::shared_ptr<ClientSock> socket) {
                                                return socket->getConnfd() == sockfd;
                                            } ),
                                            _clientSockets.end() );

    _staleFds.push_back(sockfd);
}

void Server::listen() {
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(_sockfd == -1)
        throw std::runtime_error(std::string(strerror(errno)));
    {
        int option = 1;

        setsockopt( _sockfd,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    reinterpret_cast<const void*>(&option),
                    sizeof(option));
    }
    struct sockaddr_in serv_addr;
    // std::fill(reinterpret_cast<char*>( &serv_addr ),
    //         reinterpret_cast<char*>( &serv_addr ) + sizeof(serv_addr),
    //         0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    {
        int result = bind( _sockfd,
                        reinterpret_cast<struct sockaddr*>(&serv_addr),
                        sizeof(serv_addr));

        if(result == -1)
            throw std::runtime_error(std::string(strerror(errno)));
    }
    {
        int result = ::listen(_sockfd, _backlog);

        if( result == -1 )
            throw std::runtime_error(std::string(strerror(errno)));
    }
    // rset读事件文件描述符集合，allset用来暂存
    fd_set rset, allset;
    int maxfd = _sockfd;
    FD_ZERO(&allset);
    FD_SET(_sockfd, &allset);

    while(1) {
        rset = allset;
        int nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if(nready < 0) {
            throw std::runtime_error("Select Error");
        }
        for(int i = 0; i <= maxfd; i++) {
            
        }
    }




    
}