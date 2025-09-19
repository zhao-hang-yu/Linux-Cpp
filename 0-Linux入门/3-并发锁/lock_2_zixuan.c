#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_COUNT 10

// 互斥锁
pthread_mutex_t mutex;
// 自旋锁
pthread_spinlock_t spinlock;

// 返回值类型：void * 意味着函数可以返回指向任何类型数据的指针
// 参数列表的void * 是一个通用指针，可以接收指向任何类型数据的指针
void *thread_callback(void *arg) 
{
    int *pcount = (int *)arg;
    int i = 0;
    while (i ++ < 100000)
    {
#if 0
        (*pcount) ++;
#elif 0        
        pthread_mutex_lock(&mutex);
        (*pcount) ++;
        pthread_mutex_unlock(&mutex);
#else
        pthread_spin_lock(&spinlock);
        (*pcount) ++;
        pthread_spin_unlock(&spinlock);
#endif
        usleep(1);
    }
}

int main()
{
    pthread_t threadid[THREAD_COUNT] = {0};

    // 对锁初始化
    pthread_mutex_init(&mutex, NULL);
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);

    int i = 0;
    int count = 0;
    for (i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threadid[i], NULL, thread_callback, &count);
    }

    for (i = 0; i < 100; i++)
    {
        printf("count:%d\n", count);
        sleep(1);
    }
}