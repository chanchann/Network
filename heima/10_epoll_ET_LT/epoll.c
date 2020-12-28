#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#define MAXLINE 10

int main(int argc, char *argv[]) {
    int efd, i;
    int pfd[2];
    pid_t pid;
    char buf[MAXLINE], ch = 'a';

    pipe(pfd);  // pipe一个读端一个写端
    pid = fork();
    if(pid == 0) {  // 子 写
        close(pfd[0]);  // 关闭读端
        while(1) {
            // aaaa\n
            for(i = 0; i < MAXLINE/2; i++) {
                buf[i] = ch;
            }
            buf[i - 1] = '\n';
            ch++;
            // bbbb\n
            for(; i < MAXLINE; i++) {
                buf[i] = ch;
            }
            buf[i - 1] = '\n';
            ch++;  // 'c' 进入下一个循环
            // aaaa\nbbbb\n 
            write(pfd[1], buf, sizeof(buf));
            sleep(5);
        }
        close(pfd[1]);
    } else if(pid > 0) {  // 父 读
        struct epoll_event event;  // epoll_ctl
        struct epoll_event resevent[10];  // epoll_wait就绪返回event
        int res, len;
        close(pfd[1]);  // 关闭写端
        efd = epoll_create(10);  // 创建个红黑树

        // event.events = EPOLLIN | EPOLLET; // ET边沿触发
        event.events = EPOLLIN;  // LT 水平触发(默认)
        event.data.fd = pfd[0];
        epoll_ctl(efd, EPOLL_CTL_ADD, pfd[0], &event);  // 绑定到红黑树上去
        while(1) {
            res = epoll_wait(efd, resevent, 10, -1);
            printf("res %d\n", res);  // 我们这里只有一个子进程，res是最大上限，按理应循环判断
            // 我们这里简化就只拿一个
            if(resevent[0].data.fd == pfd[0]) {
                len = read(pfd[0], buf, MAXLINE/2);  // 只读一半
                // LT 和 ET 区别在这
                // LT 会 aaaa\n 然后下一次出发epoll_wait读完bbbb\n
                // 而ET : 虽然缓冲区还有bbbb]\n,但不是因为发送了数据过来，所以不会触发epoll_wait.
                // 等到下次写道cccc\ndddd\n,我们读入的是bbbb
                write(STDOUT_FILENO, buf, len);
            }
        }
        close(pfd[0]);
        close(efd);
    } else {
        perror("fork");
        exit(-1);
    }
    return 0;
} 
