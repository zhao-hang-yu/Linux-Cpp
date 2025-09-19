#ifndef __SERVER_H__
#define __SERVER_H__

#define BUFFER_LENGTH		1024


typedef int (*RCALLBACK)(int fd);


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

    int status;
#if 1 // websocket
	char *payload;
	char mask[4];
#endif
};


int http_request(struct conn *c);
int http_response(struct conn *c);

int ws_request(struct conn *c);
int ws_response(struct conn *c);



#endif