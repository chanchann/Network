/*
epoll 基于非阻塞I/O事件驱动
*/

#include "wrap.h"

#define MAX_EVENTS 1024
#define BUFLEN 4096
#define SERV_PORT 6666

void revdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

/* 描述就绪文件描述符相关信息 */

struct myevents_s {
    int fd;   //要监听的文件描述符
    int events;   // 对应的监听事件
    void *arg;   // 泛型参数
    void (*call_back)(int fd, int events, void* arg); //回调函数
    int status;  //是否在监听， 1--> 在红黑树上(监听) , 0--> 不在(不监听)
    char buf[BUFLEN];
    int len;
    long last_active;   // 记录每次加入红黑树 g_efd的时间值
};

int g_efd;    // 全局变量，保存epoll_create返回的文件描述符
struct myevent_s g_events[MAX_EVENTS + 1];    // 自定义结构体类型数组， +1 --> lfd

// // 将结构体myevent_s 成员变量初始化
// void eventset(struct myevent_s* ev, int fd, void (*call_back)(int, int, void*), void* arg) {
//     ev->fd = fd;

// }

void initlistensocket(int efd, short port) {
    struct sockaddr_in sin;
    
}

int main() {
    unsigned short port = SERV_PORT;
    if(argc == 2) {
        port = atoi(argv[1]);
    }
    g_efd = epoll_create(MAX_EVENTS + 1);  // 创建红黑树，返回给全局g_efd
    if(g_efd <= 0) {
        // __func__ : 当前代码行所在的函数名称，其值为字符串
        printf("create efd in %s err %s\n", __func__, strerror(errno));
    }
    initlistensocket(g_efd, port);   // 初始化监听socket

    struct epoll_event events[MAX_EVENT+1];   // 保存已经满足就绪事件的文件描述符数组
    printf("server running:port[%d]\n", port);
    int checkpos = 0, i;
    while(1) {
        // 超时验证,每次测试100个链接，不测试lfd
        // 当客户端60s内没有和服务器通信，则关闭此客户端链接
        long now = time(NULL);  // 当前时间
        for(i = 0; i < 100; i++, checkpos++) {
            if(checkpos == MAX_EVENTS) {
                checkpos = 0;
            }
            // 不在红黑树上
            if(g_events[checkpos].status != 1) {
                continue;
            }
            long duration = now - g_events[checkpos].last_active;  // 客户端不活跃的时间
            if(duation >= 60) {
                Close(g_events[checkpos].fd);   // 关闭和该客户端连接
                printf("[fd = %d] timeout\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]);   // 将该客户端从红黑树g_efd移除
            }
        }

        // 监听红黑树g_efd, 将满足的事件的文件描述符加至events数组中，1s没有时间满嘴，返回0
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000);
        if(nfd < 0) {
            printf("epoll wait error, exit\n");
            break;
        }
        for(i = 0; i< nfd; i++) {
            // 使用自定义结构体myevent_s类型指针,接受联合体data的void* ptr成员
            if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
                // 读就绪事件
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
                // 写就绪事件
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }
    return 0;
}  