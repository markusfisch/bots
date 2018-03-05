#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAP_LENGTH 32
#define MAX_PLAYERS 16
#define VIEW_RADIUS 2

struct Game {
	char map[MAP_LENGTH * MAP_LENGTH];
	int nplayers;
	int started;
	struct Player {
		int fd;
		int x;
		int y;
		int bearing;
		int power;
	} players[MAX_PLAYERS];
} game;

static void init_map() {
	char tiles[] = "                 .~#";
	size_t ntiles = sizeof(tiles);
	size_t size = sizeof(game.map);
	char *p = game.map;
	for (size_t i = 0; i < size; ++i) {
		*p++ = tiles[rand() % ntiles];
	}
}

static void init_game() {
	init_map();
	game.started = 0;
	game.nplayers = 0;
}

static void write_map(int fd) {
	char *p = game.map;
	for (int y = 0; y < MAP_LENGTH; ++y) {
		write(fd, p, MAP_LENGTH);
		write(fd, "\n", 1);
		p += MAP_LENGTH;
	}
}

static int wrap(int coord) {
	return (coord + MAP_LENGTH) % MAP_LENGTH;
}

static char get_tile(int x, int y) {
	unsigned long offset = wrap(y) * MAP_LENGTH + wrap(x);
	return game.map[offset % sizeof(game.map)];
}

static void write_view(struct Player *player) {
	int radius = VIEW_RADIUS;
	int len = radius * 2 + 1;
	char line[len];
	int left;
	int top;
	int xx;
	int xy;
	int yx;
	int yy;
	switch (player->bearing) {
		default:
		case 0:
			left = -radius;
			top = -radius;
			xx = 1;
			xy = 0;
			yx = 0;
			yy = 1;
			break;
		case 1:
			left = radius;
			top = -radius;
			xx = 0;
			xy = 1;
			yx = -1;
			yy = 0;
			break;
		case 2:
			left = radius;
			top = radius;
			xx = -1;
			xy = 0;
			yx = 0;
			yy = -1;
			break;
		case 3:
			left = -radius;
			top = radius;
			xx = 0;
			xy = -1;
			yx = 1;
			yy = 0;
			break;
	}
	left = wrap(player->x + left);
	top = wrap(player->y + top);
	for (int y = 0; y < len; ++y) {
		char *p = line;
		int l = left;
		int t = top;
		for (int x = 0; x < len; ++x, ++p) {
			if (x == radius && y == radius) {
				*p = 'X';
			} else {
				*p = get_tile(l, t);
			}
			l += xx;
			t += xy;
		}
		left += yx;
		top += yy;
		write(player->fd, line, len);
		write(player->fd, "\n", 1);
	}
}

static int bind_to_port(int fd, int port) {
	struct sockaddr_in a;

	memset(&a, 0, sizeof(a));
	a.sin_len = sizeof(a);
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_ANY);
	a.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &a, sizeof(a))) {
		close(fd);
		return 1;
	}

	return 0;
}

static void close_fds(fd_set *ro, int nfds) {
	for (int s = 0; s < nfds; ++s) {
		if (FD_ISSET(s, ro)) {
			close(s);
		}
	}
}

static struct Player *get_player(int fd) {
	struct Player *p = game.players;
	for (int i = 0; i < game.nplayers; ++i, ++p) {
		if (p->fd == fd) {
			return p;
		}
	}
	return NULL;
}

static void remove_player(int fd) {
	struct Player *p = get_player(fd);
	if (p != NULL) {
		p->fd = 0;
	}
}

static void set_player(struct Player *p, int x, int y) {
	p->x = wrap(p->x + x);
	p->y = wrap(p->y + y);
}

static void move_player(struct Player *p, int step) {
	switch (p->bearing) {
		default:
		case 0:
			set_player(p, 0, -step);
			break;
		case 1:
			set_player(p, step, 0);
			break;
		case 2:
			set_player(p, 0, step);
			break;
		case 3:
			set_player(p, -step, 0);
			break;
	}
}

static void turn_player(struct Player *p, int direction) {
	p->bearing = (p->bearing + direction + 4) % 4;
}

static void process(fd_set *r, fd_set *ro, int nfds) {
	char cmd;
	for (int s = 0; s < nfds; ++s) {
		if (FD_ISSET(s, r)) {
			int b;
			if ((b = recv(s, &cmd, sizeof(cmd), 0)) < 1) {
				perror("recv");
				close(s);
				FD_CLR(s, ro);
				remove_player(s);
			}
			struct Player *p = get_player(s);
			if (p == NULL) {
				continue;
			}
			switch (cmd) {
				case '^':
					move_player(p, 1);
					break;
				case '<':
					turn_player(p, -1);
					break;
				case '>':
					turn_player(p, 1);
					break;
				case 'V':
					move_player(p, -1);
					break;
			}
			write_view(p);
		}
	}
}

static void send_maps() {
	struct Player *p = game.players;
	for (int i = 0; i < game.nplayers; ++i, ++p) {
		if (p->fd != 0) {
			write_view(p);
		}
	}
}

static void add_player(int fd) {
	struct Player *p = &game.players[game.nplayers];
	p->fd = fd;
	p->bearing = rand() % 4;
	p->x = rand() % MAP_LENGTH;
	p->y = rand() % MAP_LENGTH;
	++game.nplayers;
}

static int serve(int ls) {
	struct timeval tv = { 10, 0 }, *ptv = &tv;
	int nfds = ls + 1;
	fd_set r, ro;

	FD_ZERO(&ro);
	FD_SET(ls, &ro);

	for (;;) {
		memcpy(&r, &ro, sizeof(r));

		int ready = select(nfds, &r, NULL, NULL, ptv);
		if (ready < 0) {
			perror("select");
			return 1;
		} else if (ready == 0) {
			ptv = NULL;
			game.started = 1;
			write_map(1);
			send_maps();
			continue;
		}

		if (FD_ISSET(ls, &r)) {
			struct sockaddr a;
			socklen_t l;
			int s = accept(ls, &a, &l);
			if (!game.started && game.nplayers < MAX_PLAYERS) {
				FD_SET(s, &ro);
				add_player(s);
				if (++s > nfds) {
					nfds = s;
				}
			} else {
				close(s);
			}
		} else {
			process(&r, &ro, nfds);
		}
	}

	close_fds(&ro, nfds);

	return 0;
}

int main(void) {
	init_game();

	int s;
	if ((s = socket(PF_INET, SOCK_STREAM, 6)) < 1) {
		perror("socket");
		return 1;
	}

	if (bind_to_port(s, 8080) != 0) {
		perror("bind");
		return 1;
	}

	if (listen(s, 4)) {
		perror("listen");
		close(s);
		return 1;
	}

	return serve(s);
}
