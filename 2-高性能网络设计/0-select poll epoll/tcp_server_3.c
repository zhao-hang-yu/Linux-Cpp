// poll
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <poll.h>

/*
    流程与select方法相似，只是select采用FD_ISSET判断是否可读
    poll方法采用位运算的方式判断：fds[i].revents & POLLIN
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

    // 定义pollfd数组，数组大小约等于监听的sockfd数
    struct pollfd fds[1024] = {0};
    fds[sockfd].fd = sockfd;
    fds[sockfd].events = POLLIN;
    int maxfd = sockfd;

    while (1) 
    {
        int nready = poll(fds, maxfd + 1, -1);

        if (fds[sockfd].revents & POLLIN) // 位运算判断sockfd是否可读
        {
            int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &client_len);
            printf("accept: %d\n", clientfd);
            fds[clientfd].fd = clientfd;
            fds[clientfd].events = POLLIN;
            if (clientfd > maxfd) maxfd = clientfd;
        }
        
        for (int i = sockfd + 1; i <= maxfd; i++) // 对i个clientfd
        {
            if(fds[i].revents & POLLIN)
            {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                if (count == 0)
                {
                    close(i);
                    printf("client disconnect: %d\n", i);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    continue;
                }
                printf("RECV: %s\n", buffer);
                count = send(i, buffer, count, 0);
                printf("SEND: %d\n", count);
            }
        }

    }
    return 0;
}
