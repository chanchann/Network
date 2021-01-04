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
        int nready = select(maxfd+1, &rset, nullptr, nullptr, nullptr);
        if(nready < 0) {
            throw std::runtime_error("Select Error");
        }
        for(int i = 0; i <= maxfd; i++) {
            if(!FD_ISSET(i, &rset)) 
                continue;
            
            // 有新的客户端连接请求
            if(i == _sockfd) {
                struct sockaddr_in clit_addr;
                auto clit_addr_len = sizeof(clit_addr);
                // Accept 不会阻塞
                // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
                int cfd = accept(_sockfd, 
                        reinterpret_cast<struct sockaddr *>(&clit_addr), 
                        reinterpret_cast<socklen_t *>(&clit_addr_len));
                if( cfd == -1)
                    throw std::runtime_error("Accept Error");
                //向监控文件描述符集合allset添加新的文件描述符cfd
                FD_SET(cfd, &allset);
                if(maxfd < cfd) maxfd = cfd;
                auto clit_sock = std::make_shared<ClientSock>(cfd, *this);
                if(_handleAccept)
                    auto res = std::async(std::launch::async, _handleAccept, clit_sock);
                _clientSockets.push_back(clit_sock);
            } else {  // Known client sock
                char buffer[2] = {0, 0};

                // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
                /**
                 *  MSG_PEEK 
                 *  return data from the beginning of the receive queue
                 *  without removing that data from the queue.
                 * 
                 *  This way to read one byte, the server can see whether a client has closed the connection.
                 */
                int res = recv(i, buffer, 1, MSG_PEEK);
                /**
                 * It would be easier to use erase-remove here, but this leads to a deadlock.
                 * Instead, the current socket will be added to the list of stale sockets and be closed later on.
                 */
                if(res <= 0) {
                    this->close(i);
                } else {  // isConnected
                    auto itSocket = std::find_if(_clientSockets.begin(), _clientSockets.end(),
                                    [&](std::shared_ptr<ClientSock> sock) {
                                        return sock->getConnfd() == i;
                                    });
                    if(itSocket != _clientSockets.end() && _handleRead) {
                        auto res = std::async(std::launch::async, _handleRead, *itSocket);
                    }
                }
            }
            /**
             * andle stale connections. This is in an extra scope so that the
             * lock guard unlocks the mutex automatically.
             */
            {
                std::lock_guard<std::mutex> lock(_staleFdsMutex);
                for(auto&& sfd : _staleFds) {
                    FD_CLR(sfd, &allset);
                    ::close(sfd);
                }
                _staleFds.clear();
            }
        }
    }

}