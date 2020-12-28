/*
recv / send -- TCP流数据读写的系统调用

1. 和对文件读写的区别
2. flags

这个是send端，相当于client端

UDP : recvfrom / sendto

通用 : recvmsg / sendmsg
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

/*传入ip port*/
int main(int argc,char* argv[]) {
	if(argc < 3) {
		printf("usage:%s ip port \n",argv[0]);
		return -1;
	}
	char* ip = argv[1];
	int port = atoi(argv[2]);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);  // serv port
	addr.sin_addr.s_addr = inet_addr(ip);
	
	int ret = connect(sockfd,(struct sockaddr*)&addr,sizeof(addr));
	assert(ret != -1);

	const char* oob_data = "abc"; //带外数据
	const char* normal_data = "123";
	send(sockfd, normal_data,strlen(norata), MSG_OOB);
	send(sockfd, normal_data, strlen(nomal_data), 0);
	send(sockfd, oob_data, strlen(oob_drmal_data), 0);


	close(sockfd);
	return 0;
}