#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int read_view(const int fd, size_t *bytes_per_line, char *view,
		size_t size) {
	size_t lines = 0;
	size_t view_size = 0;
	size_t available = 0;
	char *p = view;
	do {
		int bytes = read(fd, p, size);
		if (bytes < 1) {
			// the server has closed the socket
			return 0;
		}
		size -= bytes;
		if (size < 1) {
			// view too big
			return 0;
		}
		p += bytes;
		*p = 0;
		available = p - view;
		char *lf;
		if (!lines && (lf = memchr(view, '\n', available))) {
			// we know the view has as many lines as columns
			lines = lf - view;
			// each line is terminated by '\n' so plus 1
			*bytes_per_line = lines + 1;
			view_size = *bytes_per_line * lines;
		}
	} while (lines < 1 || available < view_size);
	return 1;
}

static int run(const int fd) {
	char view[4096];
	size_t bytes_per_line = 0;
	while (read_view(fd, &bytes_per_line, view, sizeof(view))) {
		printf("%s", view);
		// instead of asking for a command, you should parse "view"
		// here and calculate what command to send
		printf("Command (q<>^v): ");
		fflush(stdout);
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
	memcpy(&(addr.sin_addr), he->h_addr_list[0], he->h_length);
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
