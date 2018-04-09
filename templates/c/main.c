#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static int read_map(const int fd) {
	char line[4096];
	size_t lines = 0;
	for (;;) {
		if (read_line(fd, line, sizeof(line)) < 0) {
			return 0;
		}
		if (!lines) {
			lines = strlen(line);
		}
		puts(line);
		if (--lines < 1) {
			break;
		}
	}
	return 1;
}

static int run(const int fd) {
	setbuf(stdout, NULL);
	while (read_map(fd)) {
		char cmd[2];
		printf("Command (q<>^v): ");
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

	int fd = socket(AF_INET, SOCK_STREAM, 0);
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
