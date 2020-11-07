#ifndef _WRAP_H_
#define _WRAP_H_
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

void sys_err(const char* str);
int Socket(int domain, int type, int protocol);
int Listen(int socket, int backlog);
#endif