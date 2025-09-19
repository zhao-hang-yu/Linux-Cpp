#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/epoll.h>

#define BUFFER_SIZE		1024
#define EPOLL_SIZE		1024

void *client_routine(void *arg)
{
    int clientfd = *(int *)arg;
    while (1)
    {
        char buffer[BUFFER_SIZE] = {0};
        int len = recv(clientfd, buffer, BUFFER_SIZE, 0);
        printf("len: %d\n", len);
        if (len < 0)
        {
            close(clientfd);
            break;
        }
        else if (len == 0)
        {
            close(clientfd);
            break;
        }
        else 
        {
            printf("Recv: %s, %d byte(s)\n", buffer, len);
        }
    }
}

// ./tcp_server 8888
int main(int argc, char *argv[])
{
    if (argc < 2) return -1;
    int port = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // 将创建的 socket 绑定到一个本地的 IP 地址和端口号（监听哪个端口）
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind");
        return -2;
    }
    // 将 socket 变为监听状态，准备接收连接。（客户端的请求会放入连接队列）
    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        return -3;
    }
#if 0
    while (1)
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(client_addr);
        // （阻塞）从连接队列中取出一个客户端连接，并创建一个新的 socket clientfd 专门与该客户端通信
        // 将 发起连接的客户端的地址信息 填写到这个结构体里（例如 IP 和端口号）
        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_routine, &clientfd);
    }
#else
    int epfd = epoll_create(1); // 只有0和非0的区别
    struct epoll_event events[EPOLL_SIZE] = {0};
    struct epoll_event ev;
    ev.events = EPOLLIN; // 表示关心可读数据
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev); // 注册监听 socket 到 epoll 中

    while (1)
    {
        int nready = epoll_wait(epfd, events, EPOLL_SIZE, -1); // 等待事件发生 -1是阻塞等待 0是一直 >0是时间间隔
        // if (nready == -1) continue; // 没有检测到这一群sockfd有没有数据

        for (int i = 0; i < nready; i++)
        {
            if (events[i].data.fd == sockfd) // 单独处理listen
            {
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(struct sockaddr_in));
                socklen_t client_len = sizeof(client_addr);
                // 1. 为该连接创建新的 socket（clientfd）
                // 2. 将客户端的地址信息写入你提供的 client_addr 中
                int clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

                ev.events = EPOLLIN | EPOLLET; // EPOLLET 表示使用边缘触发，更高效但要求非阻塞读写
                ev.data.fd = clientfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else 
            {
                int clientfd = events[i].data.fd;
                char buffer[BUFFER_SIZE] = {0};
                int len = recv(clientfd, buffer, BUFFER_SIZE, 0);
                if (len < 0)
                {
                    close(clientfd);
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                }
                else if (len == 0)
                {
                    close(clientfd);
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                }
                else 
                {
                    printf("Recv: %s, %d byte(s)\n", buffer, len);
                }
            }
        }
    }

#endif
    return 0;
}