#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "threadpool.h"

#define DEFAULT_TIME 10 // 10s检测一次
#define MIN_WAIT_TASK_NUM 10   // 如果queue_size > MIN_WAIT_TASK_NUM 条件加新的线程到线程池
#define DEFAULT_THREAD_VARY 10   // 每次创建和销毁线程的个数
#define true 1
#define false 0

typedef struct {
    void *(*function)(void *);    // 函数指针，回调函数
    void *arg;     // 上面函数的参数
} threadpool_task_t;     // 各子线程任务结构体

/* 描述线程池信息 */

struct threadpool_t {
    pthread_mutex_t lock;   // 用于锁住本结构体
    pthread_mutex_t thread_counter;   // 记录忙状态线程个数的锁  --- busy_thr_num

    pthread_cond_t queue_not_full;   // 当任务队列满时，添加任务的线程阻塞，等待此条件变量
    pthread_cont_t queue_not_empty;  // 任务队列不为空时，通知等待任务的线程

    pthread_t *threads;    // 存放线程池中每个线程的tid, 数组
    pthread_t adjust_tid;   // 存管理线程tid
    pthread_task_t *task_queue;  // 任务队列(数组首地址)

    int min_thr_num;    // 线程池最小线程数
    int max_thr_num;    // 线程池最大线程数
    int live_thr_num;   // 当前存活线程数
    int busy_thr_num;   // 忙状态线程个数
    int wait_exit_thr_num;  // 要销毁的线程个数

    int queue_front;    // task_queue队头下标
    int queue_rear;     // task_queue队尾下标
    int queue_size;     // task_queue队中实际任务数
    int queue_max_size; // task_queue队列可容纳任务数上限

    int shutdown;       // 标志位，线程池使用状，true or false
};

int main(int argc, char* argv[]) {

    // 创建线程池，池里最小3个线程，最大100，队列最大100
    threadpool_t *thp = threadpool_create(3, 100, 100); 
    printf("pool init");

    // 产生任务
    int num[20], i;
    for(i =0; i < 20; i++) {
        num[i] = i;
        printf("add task %d\n", i);
        // 向线程池添加任务
        threadpool_add(thp, process, (void*)&num[i]);   
    }
    sleep(10);
    threadpool_destroy(thp);
}