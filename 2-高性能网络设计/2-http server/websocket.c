/*
#include "server.h"

#include <stdio.h>

int ws_request(struct conn *c)
{
    printf("request: %s", c->r_buffer);
}

int ws_response(struct conn *c)
{

}
*/



#include <stdio.h>

#include "server.h"

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>



#define GUID  "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
/*
key: "fUNa6rJwr4/VDpwcgvceYA=="
fUNa6rJwr4/VDpwcgvceYA==258EAFA5-E914-47DA-95CA-C5AB0DC85B11

SHA-1

20 bytes 

base64


handshark

transmission
  decode
  encode

*/




struct _nty_ophdr {

	unsigned char opcode:4,
		 rsv3:1,
		 rsv2:1,
		 rsv1:1,
		 fin:1;
	unsigned char payload_length:7,
		mask:1;

} __attribute__ ((packed));

struct _nty_websocket_head_126 {
	unsigned short payload_length;
	char mask_key[4];
	unsigned char data[8];
} __attribute__ ((packed));

struct _nty_websocket_head_127 {

	unsigned long long payload_length;
	char mask_key[4];

	unsigned char data[8];
	
} __attribute__ ((packed));

typedef struct _nty_websocket_head_127 nty_websocket_head_127;
typedef struct _nty_websocket_head_126 nty_websocket_head_126;
typedef struct _nty_ophdr nty_ophdr;




int base64_encode(char *in_str, int in_len, char *out_str) {    
	BIO *b64, *bio;    
	BUF_MEM *bptr = NULL;    
	size_t size = 0;    

	if (in_str == NULL || out_str == NULL)        
		return -1;    

	b64 = BIO_new(BIO_f_base64());    
	bio = BIO_new(BIO_s_mem());    
	bio = BIO_push(b64, bio);
	
	BIO_write(bio, in_str, in_len);    
	BIO_flush(bio);    

	BIO_get_mem_ptr(bio, &bptr);    
	memcpy(out_str, bptr->data, bptr->length);    
	out_str[bptr->length-1] = '\0';    
	size = bptr->length;    

	BIO_free_all(bio);    
	return size;
}


int readline(char* allbuf,int level,char* linebuf){    
	int len = strlen(allbuf);    

	for (;level < len; ++level)    {        
		if(allbuf[level]=='\r' && allbuf[level+1]=='\n')            
			return level+2;        
		else            
			*(linebuf++) = allbuf[level];    
	}    

	return -1;
}

void demask(char *data,int len,char *mask){    
	int i;    
	for (i = 0;i < len;i ++)        
		*(data+i) ^= *(mask+(i%4));
}



char* decode_packet(unsigned char *stream, char *mask, int length, int *ret) {

	nty_ophdr *hdr =  (nty_ophdr*)stream;
	unsigned char *data = stream + sizeof(nty_ophdr);
	int size = 0;
	int start = 0;
	//char mask[4] = {0};
	int i = 0;

	if ((hdr->mask & 0x7F) == 126) {

		nty_websocket_head_126 *hdr126 = (nty_websocket_head_126*)data;
		size = hdr126->payload_length;
		
		for (i = 0;i < 4;i ++) {
			mask[i] = hdr126->mask_key[i];
		}
		
		start = 8;
		
	} else if ((hdr->mask & 0x7F) == 127) {

		nty_websocket_head_127 *hdr127 = (nty_websocket_head_127*)data;
		size = hdr127->payload_length;
		
		for (i = 0;i < 4;i ++) {
			mask[i] = hdr127->mask_key[i];
		}
		
		start = 14;

	} else {
		size = hdr->payload_length;

		memcpy(mask, data, 4);
		start = 6;
	}

	*ret = size;
	demask(stream+start, size, mask);

	return stream + start;
	
}

int encode_packet(char *buffer,char *mask, char *stream, int length) {

	nty_ophdr head = {0};
	head.fin = 1;
	head.opcode = 1;
	int size = 0;

	if (length < 126) {
		head.payload_length = length;
		memcpy(buffer, &head, sizeof(nty_ophdr));
		size = 2;
	} else if (length < 0xffff) {
		nty_websocket_head_126 hdr = {0};
		hdr.payload_length = length;
		memcpy(hdr.mask_key, mask, 4);

		memcpy(buffer, &head, sizeof(nty_ophdr));
		memcpy(buffer+sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_126));
		size = sizeof(nty_websocket_head_126);
		
	} else {
		
		nty_websocket_head_127 hdr = {0};
		hdr.payload_length = length;
		memcpy(hdr.mask_key, mask, 4);
		
		memcpy(buffer, &head, sizeof(nty_ophdr));
		memcpy(buffer+sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_127));

		size = sizeof(nty_websocket_head_127);
		
	}

	memcpy(buffer+2, stream, length);

	return length + 2;
}

#define WEBSOCK_KEY_LENGTH	19

int handshark(struct conn *c) {

	//ev->buffer , ev->length

	char linebuf[1024] = {0};
	int idx = 0;
	char sec_data[128] = {0};
	char sec_accept[32] = {0};

	do {

		memset(linebuf, 0, 1024);
		idx = readline(c->r_buffer, idx, linebuf);

		if (strstr(linebuf, "Sec-WebSocket-Key")) {

			//linebuf: Sec-WebSocket-Key: QWz1vB/77j8J8JcT/qtiLQ==
			strcat(linebuf, GUID);

			//linebuf: 
			//Sec-WebSocket-Key: QWz1vB/77j8J8JcT/qtiLQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11

			
			SHA1(linebuf + WEBSOCK_KEY_LENGTH, strlen(linebuf + WEBSOCK_KEY_LENGTH), sec_data); // openssl

			base64_encode(sec_data, strlen(sec_data), sec_accept);

			memset(c->w_buffer, 0, BUFFER_LENGTH); 

			c->w_length = sprintf(c->w_buffer, "HTTP/1.1 101 Switching Protocols\r\n"
					"Upgrade: websocket\r\n"
					"Connection: Upgrade\r\n"
					"Sec-WebSocket-Accept: %s\r\n\r\n", sec_accept);

			printf("ws response : %s\n", c->w_buffer);

			break;
			
		}

	} while((c->r_buffer[idx] != '\r' || c->r_buffer[idx+1] != '\n') && idx != -1 );

	return 0;
}





int ws_request(struct conn *c) {

	printf("request: %s\n", c->r_buffer);


	if (c->status == 0) {
		
		handshark(c);
		c->status = 1;
		
	} else if (c->status == 1) {
		char mask[4] = {0};
		int ret = 0;

		c->payload = decode_packet(c->r_buffer, c->mask, c->r_length, &ret);

		printf("data : %s , length : %d\n", c->payload, ret);

		c->w_length = ret;

		c->status = 2;
	}

	return 0;
}


int ws_response(struct conn *c) {

	if (c->status == 2) {
		
		c->w_length = encode_packet(c->w_buffer, c->mask, c->payload, c->w_length);
		c->status = 1;
	}

	return 0;
}
