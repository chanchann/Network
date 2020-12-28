#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
using namespace std;	 

int main() {
	in_addr_t l1;  
	l1 = inet_addr("1.2.3.4"); 
	in_addr_t l2;  
	l2 = inet_addr("123.22.33.34"); 
	struct in_addr addr1;
	memcpy(&addr1, &l1, 4);
	struct in_addr addr2;
	memcpy(&addr2, &l2, 4);
	char* pValue1 = inet_ntoa(addr1);
	char* pValue2 = inet_ntoa(addr2);
	cout << pValue1 << endl;    
	cout << pValue2 << endl;    
	return 0;
}