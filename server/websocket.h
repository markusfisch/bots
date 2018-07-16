#ifndef _websocket_h_
#define _websocket_h_

typedef struct {
	int fd;
	char buffer[1024];
	unsigned int available;
	int handshaked;
} WebSocket;

void websocket_close(WebSocket *);
int websocket_send_text_message(WebSocket *, const char *, unsigned int);
int websocket_read(WebSocket *, char **);

#endif
