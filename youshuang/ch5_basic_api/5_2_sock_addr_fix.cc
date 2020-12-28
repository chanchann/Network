/*
5_2的坑 
inet_ntoa函数, 是把这个返回结果放到了静态存储区

多次调用会被覆盖，多线程下会崩溃

https://blog.csdn.net/sing_hwang/article/details/86472475
*/

#include<iostream>
using namespace std;
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
					 
int main() {
	in_addr_t l1,l2;  
	l1 = inet_addr("1.2.3.4");  // ipv4 地址
	l2 = inet_addr("127.0.0.1");

	struct in_addr addr1;
	memcpy(&addr1, &l1, 4);
    struct in_addr addr2;
	memcpy(&addr2, &l2, 4);
    // inet_ntoa    网络字节序整数表示的IPv4地址->点分十进制字符串,成功返回字符串
	char* pTmp = inet_ntoa(addr1);
	char* pValue1 = (char*)malloc(sizeof(char) * sizeof(pTmp));
	strcpy(pValue1, pTmp);
	pTmp = inet_ntoa(addr2);
	char* pValue2 = (char*)malloc(sizeof(char) * sizeof(pTmp));
	strcpy(pValue2, pTmp);
	cout << pValue1 << endl;    // 解决
	cout << pValue2 << endl;
	cout << inet_ntoa(addr1) << endl;
	cout << inet_ntoa(addr2) << endl;
	free(pValue1); pValue1 = NULL;
	free(pValue2); pValue2 = NULL;
	return 0;
}