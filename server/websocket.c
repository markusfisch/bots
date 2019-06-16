#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "websocket.h"
#include "base64.h"
#include "sha1.h"
#include "nosignal.h"

static char *websocket_answer(const char *key) {
	if (!key) {
		return NULL;
	}
	char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	char buf[strlen(key) + strlen(guid) + 1];
	strcpy(buf, key);
	strcat(buf, guid);
	unsigned char result[20];
	sha1(result, (unsigned char *) buf, strlen(buf));
	return base64_encode(result, sizeof(result));
}

static char *websocket_key(const char *req) {
	char *name = "Sec-WebSocket-Key:";
	char *key = strstr(req, name);
	return key ? strtok(key + strlen(name), " \r\n") : NULL;
}

static int websocket_handshake(WebSocket *ws) {
	char *eod = strstr(ws->buffer, "\r\n\r\n");
	if (!eod) {
		return 0;
	}
	char *answer = websocket_answer(websocket_key(ws->buffer));
	if (!answer) {
		return 0;
	}
	char response[1024];
	snprintf(response, sizeof(response),
		"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: %s\r\n"
			"\r\n",
		answer);
	free(answer);
	size_t len = strlen(response);
	if (send(ws->fd, response, len, MSG_NOSIGNAL) < (ssize_t) len) {
		return -1;
	}
	ws->handshaked = 1;
	ws->available = 0;
	return 0;
}

static int websocket_send_message(WebSocket *ws, const unsigned char opcode,
		const void *data, uint64_t len) {
	unsigned char header[10];
	unsigned int hlen;
	memset(header, 0, sizeof(header));
	header[0] = (1 << 7) | opcode;
	if (len < 126) {
		header[1] = len & 127;
		hlen = 2;
	} else if (len < 0xffff) {
		hlen = 4;
		header[1] = 126;
		header[2] = (len >> 8) & 255;
		header[3] = len & 255;
	} else {
		hlen = 10;
		header[1] = 127;
		header[2] = (len >> 56) & 255;
		header[3] = (len >> 48) & 255;
		header[4] = (len >> 40) & 255;
		header[5] = (len >> 32) & 255;
		header[6] = (len >> 24) & 255;
		header[7] = (len >> 16) & 255;
		header[8] = (len >> 8) & 255;
		header[9] = len & 255;
	}
	if (send(ws->fd, header, hlen, MSG_NOSIGNAL) < (ssize_t) hlen) {
		return -1;
	}
	if (len > 0 && send(ws->fd, data, len, MSG_NOSIGNAL) < (ssize_t) len) {
		return -1;
	}
	return 0;
}

int websocket_send_text_message(WebSocket *ws, const char *data,
		unsigned int len) {
	return websocket_send_message(ws, 1, data, len);
}

void websocket_close(WebSocket *ws) {
	websocket_send_message(ws, 8, NULL, 0);
	close(ws->fd);
	ws->fd = 0;
	ws->handshaked = 0;
}

static int websocket_read_message(WebSocket *ws, char **message) {
	unsigned int hlen = 2;
	if (ws->available < hlen) {
		return 0;
	}
	uint64_t len = ws->buffer[1] & 127;
	if (len == 126) {
		hlen = 4;
		if (ws->available < hlen) {
			return 0;
		}
		len = (unsigned long) ws->buffer[2] << 8 |
			(unsigned long) ws->buffer[3];
	} else if (len == 127) {
		hlen = 10;
		if (ws->available < hlen) {
			return 0;
		}
		len = (uint64_t) ws->buffer[2] << 56 |
			(uint64_t) ws->buffer[3] << 48 |
			(uint64_t) ws->buffer[4] << 40 |
			(uint64_t) ws->buffer[5] << 32 |
			(uint64_t) ws->buffer[6] << 24 |
			(uint64_t) ws->buffer[7] << 16 |
			(uint64_t) ws->buffer[8] << 8 |
			(uint64_t) ws->buffer[9];
	}
	char *data = ws->buffer + hlen;
	unsigned char *mask = NULL;
	if (ws->buffer[1] & 128) {
		mask = (unsigned char *) data;
		data += 4;
		hlen += 4;
	}
	if (ws->available - hlen < len) {
		return 0;
	}
	ws->available = 0;
	if (mask) {
		unsigned char *p = (unsigned char *) data;
		unsigned int i;
		for (i = 0; i < len; ++i, ++p) {
			*p ^= mask[i % 4];
		}
	} else {
		// messages from the client to the server must be masked
		return -1;
	}
	if ((*ws->buffer & 15) == 8) {
		// got close frame
		return -1;
	}
	if ((*ws->buffer & 15) == 9) {
		// got ping, send pong
		return websocket_send_message(ws, 0xa, data, len);
	}
	if (message) {
		*message = data;
	}
	return len;
}

int websocket_read(WebSocket *ws, char **message) {
	int rest = sizeof(ws->buffer) - 1 - ws->available;
	if (rest < 1) {
		// message (or handshake header) too big for buffer
		return -1;
	}
	char *buf = ws->buffer + ws->available;
	int b;
	if ((b = recv(ws->fd, buf, rest, 0)) < 1) {
		return -1;
	}
	// terminate so we can use strstr() on the header
	// because glib doesn't have strnstr()
	buf[b] = 0;
	ws->available += b;
	return ws->handshaked ?
		websocket_read_message(ws, message) :
		websocket_handshake(ws);
}
