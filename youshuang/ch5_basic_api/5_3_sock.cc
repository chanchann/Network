/*
1. socket 
socket()打开一个网络通讯端口，如果成功的话，就像open()一样返回一个文件描
述符，应用程序可以像读写文件一样用read/write在网络上收发数据，如果socket()调
用出错则返回-1
2. bind
bind()的作用是将参数sockfd和addr绑定在一起，使sockfd这个用于网络通讯的文件
描述符监听addr所描述的地址和端口号
3. listen
listen()声明sockfd处于监听状态，并且最多允许有
backlog个客户端处于连接待状态
4. accept
...

这里是研究listen 的 backlog

telnet `ip` `port`

netstat -nt | grep `port`

完整连接最多backlog + 1
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;

// SIGTERM处理函数，触发时结束主程序中的循环
static void handle_term( int sig ) {
    stop = true;
}

int main( int argc, char* argv[] ) {
    // 信号注册
    // https://zhuanlan.zhihu.com/p/143555199
    signal( SIGTERM, handle_term );

    if( argc <= 3 ) {
        printf( "usage: %s ip_address port_number backlog\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
    int backlog = atoi( argv[3] );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    // 创建一个IPV4 socket地址
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( sock, backlog );
    assert( ret != -1 );

    while ( ! stop ) {
        sleep( 1 );
    }

    close( sock );
    return 0;
}
