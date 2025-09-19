#include "server.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sendfile.h>
#include <errno.h>

int http_request(struct conn *c)
{
    printf("request: %s\n", c->r_buffer);

	memset(c->w_buffer, 0, BUFFER_LENGTH);
	c->w_length = 0;
	c->status = 0;

}

int http_response(struct conn *c)
{
#if 0
    c->w_length = sprintf(c->w_buffer, 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: 82\r\n"
		"Date: Tue, 30 Apr 2024 13:16:46 GMT\r\n\r\n"
		"<html><head><title>0voice.king</title></head><body><h1>King</h1></body></html>\r\n\r\n");
#elif 0
	int filefd = open("index.html", O_RDONLY);
	struct stat stat_buf;
	fstat(filefd, &stat_buf);

	c->w_length = sprintf(c->w_buffer, 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %ld\r\n"
		"Date: Tue, 30 Apr 2024 13:16:46 GMT\r\n\r\n", 
		stat_buf.st_size);
	
	// 读到w_buffer后面
	int count = read(filefd, c->w_buffer + c->w_length, BUFFER_LENGTH - c->w_length);
	c->w_length += count;
	close(filefd);
#elif 0 // 多次发送（一次发head，一次发body）
	int filefd = open("index.html", O_RDONLY);

	struct stat stat_buf;
	fstat(filefd, &stat_buf);
	
	if (c->status == 0) {
		c->w_length = sprintf(c->w_buffer, 
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Accept-Ranges: bytes\r\n"
			"Content-Length: %ld\r\n"
			"Date: Tue, 30 Apr 2024 13:16:46 GMT\r\n\r\n", 
			stat_buf.st_size);
		c->status = 1;
	} else if (c->status == 1) {
		int ret = sendfile(c->fd, filefd, NULL, stat_buf.st_size);
		if (ret == -1) {
			printf("errno: %d\n", errno);
		}
		//c->wlength = 0;
		//memset(c->wbuffer, 0, BUFFER_LENGTH);
		c->status = 2;
	} else if (c->status == 2) {
		c->w_length = 0;
		memset(c->w_buffer, 0, BUFFER_LENGTH);
		c->status = 0;
	}

	close(filefd);

#elif 1
	int filefd = open("Z30_4098.JPG", O_RDONLY);

	struct stat stat_buf;
	fstat(filefd, &stat_buf);
	
	if (c->status == 0) {
		c->w_length = sprintf(c->w_buffer, 
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: image\r\n"
			"Accept-Ranges: bytes\r\n"
			"Content-Length: %ld\r\n"
			"Date: Tue, 30 Apr 2024 13:16:46 GMT\r\n\r\n", 
			stat_buf.st_size);
		c->status = 1;
	} else if (c->status == 1) {
		int ret = sendfile(c->fd, filefd, NULL, stat_buf.st_size);
		if (ret == -1) {
			printf("errno: %d\n", errno);
		}
		//c->wlength = 0;
		//memset(c->wbuffer, 0, BUFFER_LENGTH);
		c->status = 2;
	} else if (c->status == 2) {
		c->w_length = 0;
		memset(c->w_buffer, 0, BUFFER_LENGTH);
		c->status = 0;
	}

	close(filefd);

#endif
}