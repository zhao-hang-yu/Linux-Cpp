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

#define BUFFER_SIZE     4096
#define HTTP_VERSION    "HTTP/1.1"
#define CONNETION_TYPE	"Connection: close\r\n"


// baidu.com --> "182.61.244.181"
char *host_to_ip(const char *hostname)
{
    struct hostent *host_entry = gethostbyname(hostname); // DNS的实现
    if (host_entry != NULL)
    {
        // h_addr_list是获取的IP地址列表（例如baidu.com就有两个IP地址）
        // struct in_addr是sockaddr_in下包含的一个结构体，用无符号整数表示IP地址
        // inet_ntoa把点分十进制转为字符串"xxx.xxx.xxx.xxx"
        return inet_ntoa(*(struct in_addr*)*host_entry->h_addr_list);
    }
    return NULL;
}

int http_create_socket(char* ip)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPV4, SOCK_STREAM: TCP
    struct sockaddr_in sin = {0}; // 形容服务器的IP地址和端口
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr(ip); // inet_ntoa的反过程，把字符串转为无符号整数
    if (0 != connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)))
        return -1;
    fcntl(sockfd, F_SETFL, O_NONBLOCK); // 设置 文件描述符的标志位（flags）、非阻塞模式
    return sockfd;
}

char *http_send_request(const char* hostname, const char *resource)
{   
    char *ip = host_to_ip(hostname);
    int sockfd = http_create_socket(ip);
    char buffer[BUFFER_SIZE] = {0};
    sprintf(buffer, 
"GET %s %s\r\n\
HOST: %s\r\n\
%s\r\n\
\r\n", 
        resource, HTTP_VERSION,
        hostname,
        CONNETION_TYPE);
    send(sockfd, buffer, strlen(buffer), 0);
    
    // select + recv
    fd_set fdread; // 文件描述符集合, select() 通过它检测I/O是否就绪
    FD_ZERO(&fdread);
    FD_SET(sockfd, &fdread); // 把sockfd加入集合，以判断其是否可读c

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    char *result = malloc(sizeof(int));
    memset(result, 0, sizeof(int));
    while (1) 
    {
        // select()用于监听多个文件描述符的状态变化
        // nfds: 所有监听的fd中最大值+1， readfds: 监听是否可读， writefds是否可写， exceptfds监听异常， timeout超时时间
        int selection = select(sockfd + 1, &fdread, NULL, NULL, &tv);
        if (!selection || !FD_ISSET(sockfd, &fdread)) break;
        else // 接收数据
        {
            memset(buffer, 0, BUFFER_SIZE);
            int len = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (len == 0) break; // disconnect
            result = realloc(result, (strlen(result) + len + 1) * sizeof(char));
            strncat(result, buffer, len);
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 3) return -1;
    char *response = http_send_request(argv[1], argv[2]);
    printf("response: %s\n", response);
    return 0;
}