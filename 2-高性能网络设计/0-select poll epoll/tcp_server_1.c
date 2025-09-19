// 一请求一线程
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

void *client_thread(void *arg);

/*
    1. 定义服务端sockfd、servaddr
    2. bind，绑定socket与servaddr
    3. listen，开始监听
    4. 定义客户端clientaddr
    5. accept拿到队首请求返回clientfd并将客户端信息传给clientaddr
    6. recv、send
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

    while (1)
    {
        int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &client_len);
        printf("accept: %d\n", clientfd);
        pthread_t pid;
        pthread_create(&pid, NULL, client_thread, &clientfd);
    }

    return 0;
}


void *client_thread(void *arg)
{
    int clinetfd = *(int *)arg;
    while (1)
    {
        char buffer[1024] = {0};
        int count = recv(clinetfd, buffer, 1024, 0);
        if (count == 0)
        {
            printf("client disconnect: %d\n", clinetfd);
            close(clinetfd);
            break;
        }
        printf("RECV: %s\n", buffer);
        send(clinetfd, buffer, count, 0);
        printf("SEND: %d\n", count);
    }
}