// select
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>

/*
    1. 定义fd_set集合、maxfd
    2. 循环调用select进行阻塞等待
    3. 对于TCP请求有两种处理方式：
        a. 对于listenfd ==> accept创建clientfd，并FD_SET
        b. 对于clientfd ==> for循环处理（recv，当count==0时close、FD_CLR，send）
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

    // 创建fd_set
    fd_set rfds, rset;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    int maxfd = sockfd;

    while (1)
    {
        rset = rfds;
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        
        if (FD_ISSET(sockfd, &rset)) // 处理监听fd ==> accept
        {
            int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &client_len);
            printf("accept: %d\n", clientfd);
            FD_SET(clientfd, &rfds);
            if (clientfd > maxfd) maxfd = clientfd;
        }
        // clientfd ==> recv/send
        for (int i = sockfd + 1; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &rset))
            {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                if (count == 0) 
                {
                    printf("client disconnect: %d\n", i);
                    close(i);
                    FD_CLR(i, &rfds);
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
