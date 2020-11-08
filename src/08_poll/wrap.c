#include "wrap.h"

void perr_exit(const char *s)
{
    perror(s);
    exit(1);
}
 
/* accept出错封装 */
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;//保存用于通信的socket文件描述符
again:
    if ( (n = accept(fd, sa, salenptr)) < 0)
    {
        if ((errno == ECONNABORTED) || (errno == EINTR))
            goto again;//如果连接终止 或 被信号中断 则再试一次
        else
            perr_exit("accept error");
    }
    return n;
}
 
/* 绑定IP、端口号和socket,出错则返回错误信息 */
void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (bind(fd, sa, salen) < 0)
        perr_exit("bind error");
}
 
/* 客户机连接服务器函数封装,错误则打印出错信息 */
void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (connect(fd, sa, salen) < 0)
        perr_exit("connect error");
}
 
/* listen函数出错处理封装 */
void Listen(int fd, int backlog)
{
    if (listen(fd, backlog) < 0)
        perr_exit("listen error");
}
 
/* 建立套接字socket出错处理封装 */
int Socket(int family, int type, int protocol)
{
    int n;//保存建立socket时返回的文件描述符
    if ( (n = socket(family, type, protocol)) < 0)
        perr_exit("socket error");
    return n;
}
 
/* read函数出错处理封装 */
ssize_t Read(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;//读到的字节数
again:
    if ( (n = read(fd, ptr, nbytes)) == -1)
    {
        if (errno == EINTR)
            goto again;//如果是由于信号被中断 则再试一次
        else
            return -1;
    }
    return n;
}
 
/* write函数出错处理封装 */
ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
    ssize_t n;//被写入的字节数
again:
    if ( (n = write(fd, ptr, nbytes)) == -1) {
        if (errno == EINTR)
            goto again;//如果是由于信号被中断 则再试一次
        else
            return -1;
    }
    return n;
}
 
/* close关闭文件出错处理封装 */
void Close(int fd)
{
    if (close(fd) == -1)
        perr_exit("close error");
}
 
/* 至少读满n个字符再返回 */
ssize_t Readn(int fd, void *vptr, size_t n)
{
    size_t nleft;   //剩下的字符数
    ssize_t nread;  //已读的字符数
    char *ptr;      //读写指针
 
    /* 初始化 */
    ptr = vptr;
    nleft = n;
 
    /* 至少读满n个字节返回,或出错返回 */
    while (nleft > 0)
    {
        if ( (nread = read(fd, ptr, nleft)) < 0)//如果读出错
        {
            if (errno == EINTR)//是由于信号中断的话 则退出读
                nread = 0;
            else
                return -1;
        }
        else if (nread == 0)//信号中断 或 读到空 则退出
            break;
 
        nleft -= nread;//记录每次读完,还剩的未读的字节数
        ptr += nread;  //为将下一次读到的值补充到这次的末尾做准备
    }
    return n - nleft;  //返回已读到的字节数
}
 
/* 写满n个字符才返回 */
ssize_t Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;       //剩下还要写的字节个数
    ssize_t nwritten;   //已经写的字节个数
    const char *ptr;    //写指针
 
    /* 初始化 */
    ptr = vptr;
    nleft = n;
 
    while (nleft > 0)
    {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
 
        nleft -= nwritten;  //写完这次之后,剩下还要写的字节个数
        ptr += nwritten;    //写完这次之后,从这次的末尾继续写
    }
    return n;
}
 
/*返回第一次调用时字符串的一个字符,调用一次返回下一个字符,供下面的函数调用*/
/* fd为要读的文件 ptr为要返回的字符(为传出参数) */
static ssize_t my_read(int fd, char *ptr)
{
    /* 定义静态变量,上一次调用的值会被保存 */
    static int read_cnt;    //剩下待返回的字符串的长度,系统将其初始化为0
    static char *read_ptr;  //当前读到的位置
    static char read_buf[128];  //读到的字符串
 
    if (read_cnt <= 0)
    {
again:
        if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
        {
            if (errno == EINTR)//如果是被信号中断 则再试一次
                goto again;
            return -1;
        } else if (read_cnt == 0)//如果读到的字节数为0(即读到的文件为空)
            return 0;
 
        /* 否则有读到内容 则将读指针指向该字符串的首部 */
        read_ptr = read_buf;
    }
 
    //返回当前指向的字符,之后指向下一个字符,供下一调用时使用
    *ptr = *read_ptr++;
    read_cnt--;//剩下待返回的字符串的长度减一
    return 1;//调用成功返回1
}
 
/* 一次读一行 */
ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
 
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(fd, &c)) == 1)//获取一个字符
        {
            *ptr++ = c;
            if (c == '\n')//如果是换行就退出
                break;
        } else if (rc == 0)//如果读完了就返回读到的字节数
        {
            *ptr = 0;
            return n - 1;
        } else
            return -1;
    }
    *ptr = 0;
    return n;
}