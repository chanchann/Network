#include "clientSock.h"
#include "server.h"
#include <csignal>

Server server;
// 此处能否不放全局
void handleExitSignal( int /* signal */ ) {
    server.close();
}

int main() {
    signal(SIGINT, handleExitSignal );

    server.setPort( 10031 );

    server.onRead( [&] ( std::weak_ptr<ClientSock> socket ) {
        if( auto s = socket.lock() ) {
            auto data = s->read();
            s->write( data );
        }
    } );

    server.listen();

    return 0;
}