/*
不管断开网络连接还是或退出客户端程序，accept调用都正常返回
用netstat nt | grep port 查看状态
accept只从监听队列中取出连接，而不论连接处于何种状态(ESTABLISHED or CLOSE_WAIT),更不关心任何网络状况变化

*/


#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<assert.h>
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>


int main(int argc,char* argv[]) { /*传入ip port backlog*/ 

	if(argc < 4 ){
		printf("usage:%s ip port backlog\n",argv[0]);
		return -1;
	}

	char* ip = argv[1];
	int port = atoi(argv[2]);
	int backlog = atoi(argv[3]);

	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd >= 0);

	struct sockaddr_in addr;
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	
	int ret = bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
	assert(ret != -1);

	ret = listen(sockfd,backlog);
	assert(ret != -1);
    //保证足够的时间，等待客户端连接和相关操作(掉线或退出)完成
	sleep(20);  

	struct sockaddr_in clientaddr;
	socklen_t nlen = sizeof(clientaddr);
	int confd = accept(sockfd, (struct sockaddr*)&clientaddr, &nlen);

	if(confd < 0)
		printf("errno is:%d\n",errno);
	else {
		printf("client ip:%s, port: %d\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
		close(confd);
	}

	close(sockfd);
	return 0;
}