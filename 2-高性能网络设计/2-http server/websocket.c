#include "server.h"

#include <stdio.h>

int ws_request(struct conn *c)
{
    printf("request: %s", c->r_buffer);
}

int ws_response(struct conn *c)
{

}

