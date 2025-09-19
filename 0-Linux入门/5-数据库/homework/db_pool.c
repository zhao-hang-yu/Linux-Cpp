#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define DB_SERVER_IP    "192.168.100.131"
#define DB_SERVER_PORT  3306
#define DB_USERNAME     "admin"
#define DB_PASSWORD     "1234"
#define DB_DEFAULTDB    "DB_LEARN1"
#define MAX_CONNECTIONS 10

#define LIST_INSERT(item, list) do {    \
    item->prev = NULL;                  \
    item->next = list;                  \
    if ((list) != NULL)                 \
        (list)->prev = item;            \
    (list) = item;                      \
} while(0)

#define LIST_REMOVE(item, list) do {    \
    if (item->prev != NULL)             \
        item->prev->next = item->next;  \
    if (item->next != NULL)             \
        item->next->prev = item->prev;  \
    if (list == item)                   \
        list = item->next;              \
    item->prev = item->next = NULL;     \
} while(0)


 typedef struct DBConnection
 {
    MYSQL *conn;
    int is_use;

    struct DBConnection *prev;
    struct DBConnection *next;
 }DBConnection;
 
typedef struct ConnectionPool
{
    DBConnection *head;
    int total;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
}ConnectionPool;

int pool_init(ConnectionPool *pool, int init_size, const char* ip, const char* user, const char* pass, const char* db, const int port)
{
    // 初始化锁
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    // 初始化连接链表
    pool->head = NULL;
    pool->total = 0;
    // 创建、初始化、插入每一个连接
    for (int i = 0; i < init_size; i++) 
    {
        DBConnection *dbc = (DBConnection *)malloc(sizeof(DBConnection));
        if (dbc == NULL) return -1;
        dbc->conn = mysql_init(NULL);
        if (dbc->conn == NULL)
        {
            printf("[pool_init] mysql_init error: %s", mysql_error(dbc->conn));
            return -2;
        }
        if (!mysql_real_connect(dbc->conn, ip, user, pass, db, port, NULL, 0))
        {
            printf("[pool_init] mysql_real_connect error: %s", mysql_error(dbc->conn));
            return -3;
        }
        LIST_INSERT(dbc, pool->head);
        dbc->is_use = 0;
        pool->total++;
    }
    return 0;
}

MYSQL* get_connection(ConnectionPool *pool)
{
    pthread_mutex_lock(&pool->mutex);
    while (1)
    {
        DBConnection *dbc = pool->head;
        while (dbc != NULL)
        {
            if (dbc->is_use == 0)
            {
                dbc->is_use = 1;
                pthread_mutex_unlock(&pool->mutex);
                return dbc->conn;
            }
            dbc = dbc->next;
        }
        // 如果没有空闲连接，等待释放
        pthread_cond_wait(&pool->cond, &pool->mutex);
    }
}

int release_connection(ConnectionPool *pool, MYSQL *handle)
{
    pthread_mutex_lock(&pool->mutex);
    DBConnection *dbc = pool->head;
    while (dbc != NULL)
    {
        if (dbc->conn == handle)
        {
            dbc->is_use = 0;
            pthread_cond_signal(&pool->cond);
            break;
        }
        dbc = dbc->next;
    }
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

int pool_destory(ConnectionPool *pool)
{
    pthread_mutex_lock(&pool->mutex);
    DBConnection *dbc = pool->head;
    while (dbc != NULL)
    {
        DBConnection *tmp = dbc;
        dbc = dbc->next;
        mysql_close(tmp->conn);
        free(tmp);
    }
    pool->head = NULL;
    pool->total = 0;
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    return 0;
}

// 线程的callback方法
void* callback(void *arg)
{
    ConnectionPool *pool = (ConnectionPool *)arg;
    MYSQL *mysql = get_connection(pool);
    printf("Thread %ld got connection\n", pthread_self());
    mysql_query(mysql, "SELECT SLEEP(2)");
    int delay = rand() % 9 + 1;
    sleep(delay);
    release_connection(pool, mysql);
    printf("\tThread %ld released connection\n", pthread_self());
}

int main()
{
    ConnectionPool pool;
    pool_init(&pool, MAX_CONNECTIONS, DB_SERVER_IP, DB_USERNAME, DB_PASSWORD, DB_DEFAULTDB, DB_SERVER_PORT);
    pthread_t threads[20];
    srand(time(NULL));
    for (int i = 0; i < 20; ++i) 
    {
        pthread_create(&threads[i], NULL, callback, &pool);
    }

    for (int i = 0; i < 20; ++i) 
    {
        pthread_join(threads[i], NULL); // 让主线程（或调用者）等待该线程结束
    }

Exit:
    pool_destory(&pool);
    return 0;
}