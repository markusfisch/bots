#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_view(const int fd, char *view, size_t width) {
	size_t y;
	for (y = 0; y < width; ++y) {
		write(fd, view, width);
		write(fd, "\n", 1);
		view += width;
	}
}

static int read_line(const int fd, char *buf, size_t size) {
	char *p = buf;
	for (; --size > 0; ++p) {
		if (read(fd, p, 1) < 1) {
			perror("read");
			break;
		}
		if (*p == '\n') {
			*p = 0;
			return p - buf;
		}
	}
	return -1;
}

static int read_view(const int fd, size_t *width, char *view, size_t size) {
	size_t lines = 0;
	char *p = view;
	do {
		int bytes = read_line(fd, p, size);
		if (bytes < 0) {
			// the server has closed the socket
			return 0;
		}
		size -= bytes;
		if (size < 1) {
			// view too big
			return 0;
		}
		p += bytes;
		if (!lines) {
			// we know the view has as many rows as columns
			*width = lines = strlen(view);
		}
	} while (--lines > 0);
	return 1;
}

static int run(const int fd) {
	char view[4096];
	size_t width = 0;
	// disable buffering on STDOUT
	setbuf(stdout, NULL);
	while (read_view(fd, &width, view, sizeof(view))) {
		print_view(STDOUT_FILENO, view, width);
		printf("Command (q<>^v): ");
		char cmd[2];
		read(STDIN_FILENO, cmd, sizeof(cmd));
		if (*cmd == 'q') {
			break;
		}
		write(fd, cmd, 1);
	}
	close(fd);
	return 0;
}

static int connect_to(const char *host, const int port) {
	struct hostent *he = gethostbyname(host);
	if (!he) {
		unsigned a = inet_addr(host);
		he = gethostbyaddr((char *) &a, 4, AF_INET);
		if (!he) {
			return -1;
		}
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	memcpy(&(addr.sin_addr), he->h_addr, he->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	int fd = socket(PF_INET, SOCK_STREAM, 6);
	if (fd < 0) {
		return -1;
	}

	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

int main(int argc, char **argv) {
	char *host = --argc > 0 ? *++argv : "localhost";
	int port = --argc > 0 ? atoi(*++argv) : 63187;
	int fd = connect_to(host, port);
	return fd > 0 ? run(fd) : -1;
}
