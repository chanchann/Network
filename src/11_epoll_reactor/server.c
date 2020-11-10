/*
epoll 基于非阻塞I/O事件驱动
*/

#include "wrap.h"

#define MAX_EVENTS 1024
#define BUFLEN 4096
#define SERV_PORT 6666

void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

/* 描述就绪文件描述符相关信息 */
struct myevent_s {
    int fd;   //要监听的文件描述符
    int events;   // 对应的监听事件，EPOLLIN和EPLLOUT
    void *arg;   // 泛型参数 ，指向自己结构体指针
    void (*call_back)(int fd, int events, void* arg); //回调函数
    int status;  //是否在监听， 1--> 在红黑树上(监听) , 0--> 不在(不监听)
    char buf[BUFLEN];
    int len;
    long last_active;   // 记录每次加入红黑树 g_efd的时间值
};

int g_efd;    // 全局变量，保存epoll_create返回的文件描述符 -- 红黑树根
struct myevent_s g_events[MAX_EVENTS + 1];    // 自定义结构体类型数组， +1 --> lfd ，将lfd放在最后MAX_EVENTS位置

// 将结构体myevent_s 成员变量初始化
/*
 * 封装一个自定义事件，包括fd，这个fd的回调函数，还有一个额外的参数项
 * 注意：在封装这个事件的时候，为这个事件指明了回调函数，一般来说，一个fd只对一个特定的事件
 * 感兴趣，当这个事件发生的时候，就调用这个回调函数
 */
void eventset(struct myevent_s* ev, int fd, void (*call_back)(int, int, void*), void* arg) {
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    memset(ev->buf, 0, sizeof(ev->buf));
    // ev->len = 0;  // 这里代码问题是每次重新设置事件的时候eventset都会将buf都清空，这样写回去的数据为空
    if(ev->len <= 0){
        memset(ev->buf, 0, sizeof(ev->buf));
        ev->len = 0;
    }
    ev->last_active = time(NULL);
    
    return;

}

// 从epoll监听的红黑树中删除一个文件描述符
void eventdel(int efd, struct myevent_s* ev) {
    struct epoll_event epv = {0, {0}};
    if(ev->status != 1) {  // 不在红黑树上
        return;
    }
    // epv.data.tpr = ev;
    epv.data.ptr = NULL;
    ev->status = 0;         // 修改状态
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);   // 从红黑树efd上将ev->fd摘除
    return;
}

// 向epoll监听的红黑树 添加一个文件描述符
// eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);
void eventadd(int efd, int events, struct myevent_s* ev) {
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;
    epv.events = ev->events = events;  // EPOLLIN OR EPOLLOUT
    
    if(ev->status == 0) {
        op = EPOLL_CTL_ADD;         // 将其加入红黑树 g_efd,将status置1
        ev->status = 1;
    }
    if(epoll_ctl(efd, op, ev->fd, &epv) < 0) {   // 实际添加修改
        printf("event add failed [fd = %d], events[%d]\n", ev->fd, events);
    } else {
        printf("event add OK [fd=%d], op = %d, events[%oX]\n", ev->fd, op, events);
    }
    return;
}

void senddata(int fd, int events, void* arg) {
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;
    len = send(fd, ev->buf, ev->len, 0);    // 直接将数据 回写给客户端，未作处理

    eventdel(g_efd, ev);
    if(len > 0) {  
        printf("send[fd = %d],[%d]%s\n", fd, len, ev->buf);
        eventset(ev, fd, recvdata, ev);  // 将该fd的回调函数设置为recvdata
        eventadd(g_efd, EPOLLIN, ev);  // 重新添加到红黑树上，设为监听读事件
    } else {
        Close(ev->fd);
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }
    return;
}
/*读取客户端发过来的数据的函数*/
void recvdata(int fd, int events, void *arg) {
    struct myevent_s *ev = (struct myevent_s*)arg;
    int len;
    // recv在这当read
    len = recv(fd, ev->buf, sizeof(ev->buf), 0);   // 读文件描述符，数据存入myevent_s成员buf中

    eventdel(g_efd, ev);    // 将该节点从红黑树上摘除
    if(len > 0) {
        ev->len = len;
        ev->buf[len] = '\0';   // 手动添加字符串结束标记
        printf("C[%d]: %s \n", fd, ev->buf);

        eventset(ev, fd, senddata, ev);    // 设置该fd对应的回调函数为senddata
        eventadd(g_efd, EPOLLOUT, ev);     // 将fd加入红黑树g_efd中，监听其写事件
    } else if(len == 0) {
        Close(ev->fd);
        // ev - g_events 地址相减得到偏移元素位置
        printf("[fd=%d] pos[%ld], closed\n", fd, ev - g_events);
    } else {
        Close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }
    return;
}


// 当有文件描述符就绪，epoll返回，调用该函数和客户端建立连接
void acceptconn(int lfd, int events, void* arg) {
    struct sockaddr_in cin;   // 客户端地址
    socklen_t cinlen = sizeof(cin);
    int cfd, i;
    cfd = Accept(lfd, (struct sockaddr*)&cin, &cinlen);
    do {
        for(i = 0; i < MAX_EVENTS; i++) {   // 从全局变量 g_events 中找到一个空闲元素
            if(g_events[i].status == 0) {   // 类似于select中找值为-1的元素
                break;                      // 跳出for
            }
        }
        if(i == MAX_EVENTS) {
            printf("%s : max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }
        int flag = 0;
        if((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) {   // 将cfd也设置为非阻塞
            printf("%s : fcntl nonblocking failed, %s\n", __func__, strerror(errno));  //__LINE__行号
            break;
        }
        // 给cfd设置一个myevent_s结构体,回调函数设置为recvdata
        eventset(&g_events[i], cfd, recvdata, &g_events[i]);
        eventadd(g_efd, EPOLLIN, &g_events[i]);      // 将cfd添加到红黑树g_efd中，监听读事件
    } while(0);

    printf("new connect [%s : %d][time:%ld], pos[%d]\n",
            inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);
    return;
}

void initlistensocket(int efd, short port) {
    struct sockaddr_in sin;
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    //这里有点硬设，我们还是最好按着三步走的方式
    // flag = fcntl(lfd, F_GETFL);
    // flag |= O_NONBLOCK;
    // fcntl(lfd, F_SETFL, flag);
    
    fcntl(lfd, F_SETFL, O_NONBLOCK);    // 讲socket 设置为 非阻塞

    // memset(&sin, 0, sizeof(sin));
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);
    
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Bind(lfd, (struct sockaddr*)&sin, sizeof(sin));
    Listen(lfd, 20);
   
    // void eventset(struct myevent_s* ev, int fd, void (*call_back)(int, int, void*), void* arg) {
    // &g_events[MAX_EVENTS] 最后一个元素地址, 我们将lfd放在最后一个元素
    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);

    eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);

    return;
    
}

int main(int argc, char* argv[]) {
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

    struct epoll_event events[MAX_EVENTS+1];   // 保存已经满足就绪事件的文件描述符数组
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
            if(duration >= 60) {
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
            struct myevent_s *ev = (struct myevent_s*)events[i].data.ptr;

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