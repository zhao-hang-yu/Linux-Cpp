#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>
#include <string.h>

#define BUFFER_LENGTH   1024
#define CONNECTION_SIZE 1024

typedef int (*RCALLBACK)(int fd);

int epfd = 0;

struct conn
{
    int fd;

    char r_buffer[BUFFER_LENGTH];
    int r_length;
    char w_buffer[BUFFER_LENGTH];
    int w_length;

    RCALLBACK send_callback;
    union // 定义联合，因为EPOLL_IN事件只会触发其中一个callback
    {
        RCALLBACK accept_callback;
        RCALLBACK recv_callback;
    } r_action;
};

struct conn conn_list[CONNECTION_SIZE] = {0}; // 结构体数组

int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);

int init_server(unsigned short port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

    listen(sockfd, 10);
    printf("listen finished: %d\n", sockfd);

    return sockfd;
}

int set_event(int fd, int event, int flag)
{
    if (flag) // 添加
    {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = event;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    else  // 修改
    {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = event;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
    
}

// 注册fd事件对应的回调函数，在发生该事件后 直接调用回调函数即可
int event_register(int fd, int event)
{
    conn_list[fd].fd = fd;
    conn_list[fd].r_action.recv_callback = recv_cb;
    conn_list[fd].send_callback = send_cb;
    
    memset(conn_list[fd].r_buffer, 0, BUFFER_LENGTH);
    conn_list[fd].r_length = 0;
    memset(conn_list[fd].w_buffer, 0, BUFFER_LENGTH);
    conn_list[fd].w_length = 0;

    set_event(fd, event, 1);
}

int accept_cb(int fd)
{
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &clientlen);
    printf("accept finished: %d\n", clientfd);

    // 设置clientfd
    event_register(clientfd, EPOLLIN);
    return 0;
}

int recv_cb(int fd)
{
    int count = recv(fd, conn_list[fd].r_buffer, BUFFER_LENGTH, 0);
    if (count == 0)
    {
        printf("client disconnect: %d\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return 0;
    }
    conn_list[fd].r_length = count;
    printf("RECV: %s\n", conn_list[fd].r_buffer);

    conn_list[fd].w_length = conn_list[fd].r_length;
    memcpy(conn_list[fd].w_buffer, conn_list[fd].r_buffer, conn_list[fd].w_length);
    set_event(fd, EPOLLOUT, 0);
    return count;
}

int send_cb(int fd)
{
    int count = send(fd, conn_list[fd].w_buffer, conn_list[fd].w_length, 0);
    printf("SEND: %d\n", count);
    set_event(fd, EPOLLIN, 0);
    return count;
}

int main()
{
    unsigned short port = 2000;
    int sockfd = init_server(port);

    epfd = epoll_create(1);
    conn_list[sockfd].fd = sockfd;
    conn_list[sockfd].r_action.accept_callback = accept_cb;
    set_event(sockfd, EPOLLIN, 1);

    while (1)
    {
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, -1);

        for (int i = 0; i < nready; i++)
        {
            int connfd = events[i].data.fd;

            if (events[i].events & EPOLLIN)
            {
                conn_list[connfd].r_action.recv_callback(connfd);
            }
            if (events[i].events & EPOLLOUT)
            {
                conn_list[connfd].send_callback(connfd);
            }
        }
    }

    return 0;
}