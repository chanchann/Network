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

void *threadpool_thread(void *threadpool);

void *adjust_thread(void* threadpool);

int is_thread_alive(pthread_t tid);

int threadpool_free(threadpool_t *pool);

// threadpool_create(3, 100, 100);
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size) {
    int i;
    threadpool_t *pool = NULL; // 线程池 结构体
    do {  // 顶替goto，这个不是循环作用，某个地方出错就break跳出
        if((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL) {
            printf("malloc threadpool fail");
            break;   // 跳出do while
        }
        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num =0;
        pool->live_thr_num = min_thr_num;   // 活着的线程数，初值 - 最小线程数
        pool->wait_exit_thr_num = 0;
        pool->queue_size = 0;      // 有0个产品
        pool->queue_max_size = queue_max_size;      // 最大任务队列数
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;     // 不关闭线程池

        // 根据最大线程上限数，给工作线程数组开辟空间，并清零
        pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * max_thr_num);
        if(pool->threads == NULL) {
            printf("malloc threads fail");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t) * max_thr_num);
        
        // 给任务队列开辟空间
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_max_size);
        if(pool->task_queue == NULL) {
            printf("malloc task_queue fail");
            break;
        }
        // 初始化互斥锁，条件变量
        if(pthread_mutex_init(&(pool->lock), NULL) != 0
            || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
            || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
            || pthread_cond_init(&(pool->queue_not_full), NULL) != 0) {
                printf("init the lock or cond fail");
                break;
        }

        // 启动min_thr_num个work thread
        for(i = 0; i < min_thr_num; i++) {
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool);  // pool 指向当前线程池
            printf("start thread 0x%x...\n", (unsigned int)pool->threads[i]);
        } 
        // 创建管理者线程
        pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void*)pool);
        return pool;
    } while(0);
    threadpool_free(pool);      // 前面代码调用失败，释放poll存储空间
    return NULL;
}

// 线程池中各个工作线程
void *threadpool_thread(void *threadpool) {
    threadpool_t *pool = (threadpool_t*)(threadpool);
    threadpool_task_t task;

    while(true) {
        /* Lock must be taken to wait on conditional varable */
        // 刚创建出线程，等待任务队列里有任务，否则阻塞等待任务队列里有任务后再唤醒接收任务
        pthread_mutex_lock(&(pool->lock));

        // queue_size == 0 说明没有任务，调wait阻塞在条件变量上，若有任务，跳过该while
        while((pool->queue_size == 0) && (!pool->shutdown)) {
            printf("thread 0X%x is waiting \n", (unsigned int)pthread_self());
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
        
            // 清除指定数目的空闲线程，如果要结束的线程个数大于0，结束线程
            if(pool->wait_exit_thr_num > 0) {
                pool->wait_exit_thr_num--;

                // 如果线程池里线程个数大于最小值可以结束当前线程
                if(pool->live_thr_num > pool->min_thr_num) {
                    printf("thread 0x%x is exiting\n", （unsigned int)pthread_self());
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
            }
        }

        // 如果指定了true，要关闭线程池里的每个线程，自行退出处理 -- 销毁线程池
        if(pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
            pthread_detach(pthread_self());
            pthread_exit(NULL);       // 线程自行结束
        } 
        // 从任务队列里获取任务，是一个出队操作
        task.function = pool->task_queue[pool->queue_front].function;
        task.arg = pool->task_queue[pool->queue_front].arg;

        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;  // 出队，模拟环形队列
        pool->queue_size--;

        // 通知可以有新的任务添加进来
        pthread_cond_broadcast(&(pool->queue_not_full));

        // 任务取出后，立即将 线程池锁 释放
        pthread_mutex_unlock(&(pool->lock));

        // 执行任务
        printf("thread 0x%x start working\n", (unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));   // 忙状态线程数变量锁
        pool->busy_thr_num++;       // 忙状态线程数+1
        pthread_mutex_unlock(&(pool->thread_counter));

        (*(task.function))(task.arg);       // 执行回调函数任务  这里简化为sleep(1)
        // task.function(task.arg);

        // 任务结束处理
        printf("thread 0x%x end working\n", (unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num--;       //处理掉一个任务，忙状态数线程数-1
        pthread_mutex_unlock(&(pool->thread_counter));
    }
    pthread_exit(NULL);
}

// 向线程池中 添加一个任务
// threadpool_add(thp, process, (void*)&num[i]);  // 向线程池中添加任务process: 小写-->大写
int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), void* arg) {
    pthread_mutex_lock(&(pool->lock));
    // == 为真，队列已经满，调wait阻塞
    while((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)) {
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }
    if(pool->shutdown) {
        pthread_cond_broadcast(&(pool->queue_not_empty));
        pthread_mutex_unlock(&(pool->lock));
        return 0;
    }
    // 清空工作线程，调用的回调函数的参数arg
    if(pool->task_queue[pool->queue_rear].arg != NULL) {
        pool->task_queue[pool->queue_rear].arg = NULL;
    }
    // 添加任务到任务队列里
    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;  // 队尾指针移动，模拟环形
    poll->queue_size++;

    // 添加完任务后，队列不为空，唤醒线程池 等待处理任务的线程
    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

// 线程池中的线程，模拟处理业务
void *process(void* arg) {
    printf("thread 0x%x working on task %d\n", (unsigned int)pthread_self(), (int)arg);
    sleep(1);    // 模拟处理事件
    printf("task %d is end\n", (int)arg);
    return NULL; 
}

int main(int argc, char* argv[]) {

    // threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size); 
    // 创建线程池，池里最小3个线程，最大100，队列最大100
    threadpool_t *thp = threadpool_create(3, 100, 100); 
    printf("pool init");

    // 产生任务
    int num[20], i;
    for(i =0; i < 20; i++) {
        num[i] = i;
        printf("add task %d\n", i);

        // int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), void* arg)
        // 向线程池添加任务
        // 用process 这个回调函数去处理任务
        threadpool_add(thp, process, (void*)&num[i]);   
    }
    sleep(10);
    threadpool_destroy(thp);
}