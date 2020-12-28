#include "wrap.h"

void perr_exit(const char *s)
{
    perror(s);
    exit(1);
}
 
/* accept�����װ */
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;//��������ͨ�ŵ�socket�ļ�������
again:
    if ( (n = accept(fd, sa, salenptr)) < 0)
    {
        if ((errno == ECONNABORTED) || (errno == EINTR))
            goto again;//���������ֹ �� ���ź��ж� ������һ��
        else
            perr_exit("accept error");
    }
    return n;
}
 
/* ��IP���˿ںź�socket,�����򷵻ش�����Ϣ */
void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (bind(fd, sa, salen) < 0)
        perr_exit("bind error");
}
 
/* �ͻ������ӷ�����������װ,�������ӡ������Ϣ */
void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (connect(fd, sa, salen) < 0)
        perr_exit("connect error");
}
 
/* listen�����������װ */
void Listen(int fd, int backlog)
{
    if (listen(fd, backlog) < 0)
        perr_exit("listen error");
}
 
/* �����׽���socket�������װ */
int Socket(int family, int type, int protocol)
{
    int n;//���潨��socketʱ���ص��ļ�������
    if ( (n = socket(family, type, protocol)) < 0)
        perr_exit("socket error");
    return n;
}
 
/* read�����������װ */
ssize_t Read(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;//�������ֽ���
again:
    if ( (n = read(fd, ptr, nbytes)) == -1)
    {
        if (errno == EINTR)
            goto again;//����������źű��ж� ������һ��
        else
            return -1;
    }
    return n;
}
 
/* write�����������װ */
ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
    ssize_t n;//��д����ֽ���
again:
    if ( (n = write(fd, ptr, nbytes)) == -1) {
        if (errno == EINTR)
            goto again;//����������źű��ж� ������һ��
        else
            return -1;
    }
    return n;
}
 
/* close�ر��ļ��������װ */
void Close(int fd)
{
    if (close(fd) == -1)
        perr_exit("close error");
}
 
/* ���ٶ���n���ַ��ٷ��� */
ssize_t Readn(int fd, void *vptr, size_t n)
{
    size_t nleft;   //ʣ�µ��ַ���
    ssize_t nread;  //�Ѷ����ַ���
    char *ptr;      //��дָ��
 
    /* ��ʼ�� */
    ptr = vptr;
    nleft = n;
 
    /* ���ٶ���n���ֽڷ���,������� */
    while (nleft > 0)
    {
        if ( (nread = read(fd, ptr, nleft)) < 0)//���������
        {
            if (errno == EINTR)//�������ź��жϵĻ� ���˳���
                nread = 0;
            else
                return -1;
        }
        else if (nread == 0)//�ź��ж� �� ������ ���˳�
            break;
 
        nleft -= nread;//��¼ÿ�ζ���,��ʣ��δ�����ֽ���
        ptr += nread;  //Ϊ����һ�ζ�����ֵ���䵽��ε�ĩβ��׼��
    }
    return n - nleft;  //�����Ѷ������ֽ���
}
 
/* д��n���ַ��ŷ��� */
ssize_t Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;       //ʣ�»�Ҫд���ֽڸ���
    ssize_t nwritten;   //�Ѿ�д���ֽڸ���
    const char *ptr;    //дָ��
 
    /* ��ʼ�� */
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
 
        nleft -= nwritten;  //д�����֮��,ʣ�»�Ҫд���ֽڸ���
        ptr += nwritten;    //д�����֮��,����ε�ĩβ����д
    }
    return n;
}
 
/*���ص�һ�ε���ʱ�ַ�����һ���ַ�,����һ�η�����һ���ַ�,������ĺ�������*/
/* fdΪҪ�����ļ� ptrΪҪ���ص��ַ�(Ϊ��������) */
static ssize_t my_read(int fd, char *ptr)
{
    /* ���徲̬����,��һ�ε��õ�ֵ�ᱻ���� */
    static int read_cnt;    //ʣ�´����ص��ַ����ĳ���,ϵͳ�����ʼ��Ϊ0
    static char *read_ptr;  //��ǰ������λ��
    static char read_buf[128];  //�������ַ���
 
    if (read_cnt <= 0)
    {
again:
        if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
        {
            if (errno == EINTR)//����Ǳ��ź��ж� ������һ��
                goto again;
            return -1;
        } else if (read_cnt == 0)//����������ֽ���Ϊ0(���������ļ�Ϊ��)
            return 0;
 
        /* �����ж������� �򽫶�ָ��ָ����ַ������ײ� */
        read_ptr = read_buf;
    }
 
    //���ص�ǰָ����ַ�,֮��ָ����һ���ַ�,����һ����ʱʹ��
    *ptr = *read_ptr++;
    read_cnt--;//ʣ�´����ص��ַ����ĳ��ȼ�һ
    return 1;//���óɹ�����1
}
 
/* һ�ζ�һ�� */
ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
 
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(fd, &c)) == 1)//��ȡһ���ַ�
        {
            *ptr++ = c;
            if (c == '\n')//����ǻ��о��˳�
                break;
        } else if (rc == 0)//��������˾ͷ��ض������ֽ���
        {
            *ptr = 0;
            return n - 1;
        } else
            return -1;
    }
    *ptr = 0;
    return n;
}