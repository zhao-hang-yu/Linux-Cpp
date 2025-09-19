#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>

#define DNS_SERVER_PORT		53
#define DNS_SERVER_IP		"114.114.114.114"
#define DNS_HOST			0x01
#define DNS_CNAME			0x05

struct dns_header
{
    unsigned short id; // 会话标识
    unsigned short flags; // 标志
    unsigned short questions; // 问题数
    unsigned short answer; // 回答
    unsigned short authority; // 授权
    unsigned short additional; // 附加
};

struct dns_question
{
    int length;
    unsigned char *name; // 查询名
    unsigned short type; // 查询类型
    unsigned short class; // 查询类
};

struct dns_item {
	char *domain;
	char *ip;
};

int dns_create_header(struct dns_header *header)
{
    if (header == NULL) return -1;
    memset(header, 0, sizeof(struct dns_header));
    //随机出id
    srandom(time(NULL));
    header->id = htons(random() % 65536);
    // header->id = random();
    // htons(Host TO Network Short)转换为网络字节序
    header->flags = htons(0x0100);
    header->questions = htons(1);
    return 0;
}

int dns_create_question(struct dns_question *question, char *hostname)
{
    if (question == NULL || hostname == NULL) return -1;
    memset(question, 0, sizeof(struct dns_question));
    
    question->name = (char *)malloc(strlen(hostname) + 2); //0结尾
    if (question->name == NULL) return -2;
    question->length = strlen(hostname) + 2;
    question->type = htons(1);
	question->class = htons(1);
    // 把hostname转换成合适的格式（www.0voice.com ==> 3www60voice3com）
    char delimiter[] = ".";
    char *hostname_dup = strdup(hostname); // 新建一块空间并复制
    char *token = strtok(hostname_dup, delimiter);
    char *qname = question->name; // 指向question->name的指针，方便更改question->name
    while (token != NULL)
    {
        size_t len = strlen(token);
        *qname = len; // 当指针所指的位置改为len
        qname ++; // 指针移动
        strncpy(qname, token, len + 1); // 把末尾的0也拷贝过来
        qname += len;

        token = strtok(NULL, delimiter);
    }

    free(hostname_dup); // 因为strdup函数中使用了malloc
    return 0;
}

// 把header和question填充到request里
int dns_bulid_request(struct dns_header *header, struct dns_question *question, char *request, int rlen)
{
    if (header == NULL || question == NULL || request == NULL) return -1;
    memset(request, 0, rlen);
    // header -> request
    memcpy(request, header, sizeof(struct dns_header));
    int offset = sizeof(struct dns_header);
    // question -> request
    memcpy(request + offset, question->name, question->length);
    offset += question->length;
    memcpy(request + offset, &question->type, sizeof(question->type));
    offset += sizeof(question->type);
    memcpy(request + offset, &question->class, sizeof(question->class));
    offset += sizeof(question->class);
    return offset;
}

// 以下三个函数用于解析response
static int is_pointer(int in) {
	return ((in & 0xC0) == 0xC0);
}

static void dns_parse_name(unsigned char *chunk, unsigned char *ptr, char *out, int *len) {

	int flag = 0, n = 0, alen = 0;
	char *pos = out + (*len);

	while (1) {

		flag = (int)ptr[0];
		if (flag == 0) break;

		if (is_pointer(flag)) {
			
			n = (int)ptr[1];
			ptr = chunk + n;
			dns_parse_name(chunk, ptr, out, len);
			break;
			
		} else {

			ptr ++;
			memcpy(pos, ptr, flag);
			pos += flag;
			ptr += flag;

			*len += flag;
			if ((int)ptr[0] != 0) {
				memcpy(pos, ".", 1);
				pos += 1;
				(*len) += 1;
			}
		}
	
	}
	
}

static int dns_parse_response(char *buffer, struct dns_item **domains) {

	int i = 0;
	unsigned char *ptr = buffer;

	ptr += 4;
	int querys = ntohs(*(unsigned short*)ptr);

	ptr += 2;
	int answers = ntohs(*(unsigned short*)ptr);

	ptr += 6;
	for (i = 0;i < querys;i ++) {
		while (1) {
			int flag = (int)ptr[0];
			ptr += (flag + 1);

			if (flag == 0) break;
		}
		ptr += 4;
	}

	char cname[128], aname[128], ip[20], netip[4];
	int len, type, ttl, datalen;

	int cnt = 0;
	struct dns_item *list = (struct dns_item*)calloc(answers, sizeof(struct dns_item));
	if (list == NULL) {
		return -1;
	}

	for (i = 0;i < answers;i ++) {
		
		bzero(aname, sizeof(aname));
		len = 0;

		dns_parse_name(buffer, ptr, aname, &len);
		ptr += 2;

		type = htons(*(unsigned short*)ptr);
		ptr += 4;

		ttl = htons(*(unsigned short*)ptr);
		ptr += 4;

		datalen = ntohs(*(unsigned short*)ptr);
		ptr += 2;

		if (type == DNS_CNAME) {

			bzero(cname, sizeof(cname));
			len = 0;
			dns_parse_name(buffer, ptr, cname, &len);
			ptr += datalen;
			
		} else if (type == DNS_HOST) {

			bzero(ip, sizeof(ip));

			if (datalen == 4) {
				memcpy(netip, ptr, datalen);
				inet_ntop(AF_INET , netip , ip , sizeof(struct sockaddr));

				printf("%s has address %s\n" , aname, ip);
				printf("\tTime to live: %d minutes , %d seconds\n", ttl / 60, ttl % 60);

				list[cnt].domain = (char *)calloc(strlen(aname) + 1, 1);
				memcpy(list[cnt].domain, aname, strlen(aname));
				
				list[cnt].ip = (char *)calloc(strlen(ip) + 1, 1);
				memcpy(list[cnt].ip, ip, strlen(ip));
				
				cnt ++;
			}
			
			ptr += datalen;
		}
	}

	*domains = list;
	ptr += 2;

	return cnt;
}


int dns_client_commit(char *domain)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); // IPv4、UDP数据报、默认协议
    if (sockfd < 0) return -1;

    // UDP客户端固定写法
    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_port = htons(DNS_SERVER_PORT);
    servaddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

    // 在发送请求前 打通连接
    int c = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    printf("connect: %d\n", c);
    // 准备发送请求
    struct dns_header header = {0};
    dns_create_header(&header);
    struct dns_question question = {0};
    dns_create_question(&question, domain);
    char request[1024] = {0};
    int length = dns_bulid_request(&header, &question, request, sizeof(request));
    // printf("dns_bulid_request length: %d\n", length);
    // request
    size_t addr_len = sizeof(struct sockaddr_in);
    int slen = sendto(sockfd, request, length, 0, (struct sockaddr*)&servaddr, (socklen_t)addr_len);
    // recvfrom
    char response[1024] = {0};
    struct sockaddr_in addr;
    int n = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
    // printf("recvfrom: %d, %s\n", n, response);

    // 解析response
    struct dns_item *dns_domain = NULL;
	dns_parse_response(response, &dns_domain);
	free(dns_domain);

    return n;
}

int main(int argc, char *argv[])
{   
    if (argc < 2) return -1;
    dns_client_commit(argv[1]);
    return 0;
}