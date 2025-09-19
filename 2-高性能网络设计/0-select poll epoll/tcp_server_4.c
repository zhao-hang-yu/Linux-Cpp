// epoll
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>

/*
    
*/
int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(2000);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

    listen(sockfd, 10);

    struct sockaddr_in clientaddr;
    socklen_t client_len = sizeof(clientaddr);

    // 创建epfd
    int epfd = epoll_create(1);
    
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev); // sockfd是要监控的fd，ev是监听哪些事件
    
    while (1) 
    {
        struct epoll_event events[1024];
        int nready = epoll_wait(epfd, events, 1024, -1);

        for (int i = 0; i < nready; i++)
        {
            int connfd = events[i].data.fd;
            if (connfd == sockfd)
            {
                int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &client_len);
                printf("accept: %d\n", clientfd);
                ev.data.fd = clientfd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else if (events[i].events & EPOLLIN)
            {
                char buffer[1024] = {0};
                int count = recv(connfd, buffer, 1024, 0);
                if (count == 0) 
                {
                    close(connfd);
                    printf("client disconnect: %d\n", i);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                    continue;
                }
                printf("RECV: %s\n", buffer);
                count = send(connfd, buffer, count, 0);
                printf("SEND: %d\n", count);
            }
        }
    }


    return 0;
}
