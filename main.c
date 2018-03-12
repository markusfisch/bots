#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SECONDS_TO_JOIN 10
#define USEC_PER_SEC 1000000L
#define USEC_PER_TURN USEC_PER_SEC
#define MAP_LENGTH 32
#define MAX_PLAYERS 16
#define MIN_PLAYERS 1
#define VIEW_RADIUS 2
#define TILE_FLATLAND '.'
#define TILE_WOOD '#'
#define TILE_WATER '~'
#define TILE_EXIT 'O'
#define STDOUT 1

static struct Game {
	char map[MAP_LENGTH * MAP_LENGTH];
	int started;
	int shutdown;
	int nplayers;
	struct Player {
		char name;
		int fd;
		int can_move;
		int x;
		int y;
		int bearing;
	} players[MAX_PLAYERS];
} game;

static int game_joined();
static void game_end();
static void game_init();

static void write_square(int fd, char *p, size_t len) {
	size_t y;
	for (y = 0; y < len; ++y) {
		write(fd, p, len);
		write(fd, "\n", 1);
		p += len;
	}
}

static void map_init() {
	size_t ntiles = 16;
	char tiles[ntiles];
	size_t size = sizeof(game.map);
	char *p = game.map;
	size_t i;
	memset(&tiles, TILE_FLATLAND, ntiles);
	tiles[0] = TILE_WATER;
	tiles[1] = TILE_WOOD;
	for (i = 0; i < size; ++i) {
		*p++ = tiles[rand() % ntiles];
	}
}

static void map_write(int fd) {
	size_t size = sizeof(game.map);
	char buf[size];
	memcpy(buf, game.map, size);
	int i;
	struct Player *p = game.players, *e = p + game.nplayers;
	for (i = 0; p < e; ++p, ++i) {
		if (p->fd > 0) {
			int x = p->x;
			int y = p->y;
			size_t offset = (y * MAP_LENGTH + x) % size;
			buf[offset] = p->name;
		}
	}
	write_square(fd, buf, MAP_LENGTH);
	printf("players: %d\n", game_joined());
}

static int map_wrap(int pos) {
	return (pos + MAP_LENGTH) % MAP_LENGTH;
}

static size_t map_offset(int x, int y) {
	return (map_wrap(y) * MAP_LENGTH + map_wrap(x)) % sizeof(game.map);
}

static char map_get(int x, int y) {
	return game.map[map_offset(x, y)];
}

static void map_set(int x, int y, char tile) {
	game.map[map_offset(x, y)] = tile;
}

static int map_impassable(int x, int y) {
	switch (map_get(x, y)) {
	case TILE_WATER:
	case TILE_WOOD:
		return 1;
	default:
		return 0;
	}
}

static char player_bearing(int bearing) {
	switch (bearing % 4) {
	default:
	case 0:
		return '^';
	case 1:
		return '>';
	case 2:
		return 'V';
	case 3:
		return '<';
	}
}

static struct Player *player_at(int x, int y) {
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->x == x && p->y == y) {
			return p;
		}
	}
	return NULL;
}

static void player_view_write(struct Player *player) {
	int radius = VIEW_RADIUS;
	int len = radius * 2 + 1;
	char view[len * len];
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
	left = map_wrap(player->x + left);
	top = map_wrap(player->y + top);
	char *p = view;
	int y;
	int x;
	for (y = 0; y < len; ++y) {
		int l = left;
		int t = top;
		for (x = 0; x < len; ++x, ++p) {
			*p = map_get(l, t);
			struct Player *enemy = player_at(map_wrap(l), map_wrap(t));
			if (enemy) {
				*p = player_bearing(player->bearing + enemy->bearing);
			}
			l += xx;
			t += xy;
		}
		left += yx;
		top += yy;
	}
	view[radius * len + radius] = player->name;
	write_square(player->fd, view, len);
}

static struct Player *player_get(int fd) {
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd == fd) {
			return p;
		}
	}
	return NULL;
}

static void player_remove(int fd) {
	struct Player *p = player_get(fd);
	if (p != NULL) {
		p->fd = 0;
	}
}

static void player_move_by(struct Player *p, int x, int y) {
	x = map_wrap(p->x + x);
	y = map_wrap(p->y + y);
	if (map_impassable(x, y) || player_at(x, y)) {
		return;
	}
	p->x = x;
	p->y = y;
}

static void player_move(struct Player *p, int step) {
	switch (p->bearing % 4) {
	case 0: // north
		player_move_by(p, 0, -step);
		break;
	case 1: // east
		player_move_by(p, step, 0);
		break;
	case 2: // south
		player_move_by(p, 0, step);
		break;
	case 3: // west
		player_move_by(p, -step, 0);
		break;
	}
}

static void player_turn(struct Player *p, int direction) {
	p->bearing = (p->bearing + direction + 4) % 4;
}

static int player_do(struct Player *p, char cmd) {
	if (p == NULL || !p->can_move) {
		return 0;
	}
	p->can_move = 0;

	switch (cmd) {
	case '^':
		player_move(p, 1);
		break;
	case '<':
		player_turn(p, -1);
		break;
	case '>':
		player_turn(p, 1);
		break;
	case 'V':
		player_move(p, -1);
		break;
	}

	if (map_get(p->x, p->y) == TILE_EXIT) {
		printf("%c found the exit\n", p->name);
		game_end();
		return 1;
	}
	return 0;
}

static int player_read_command(fd_set *r, fd_set *ro, int nfds) {
	char cmd;
	int fd;
	for (fd = 0; fd < nfds; ++fd) {
		if (FD_ISSET(fd, r)) {
			int b;
			if ((b = recv(fd, &cmd, sizeof(cmd), 0)) < 1) {
				perror("recv");
				close(fd);
				FD_CLR(fd, ro);
				player_remove(fd);
				if (game_joined() < 1) {
					return 1;
				}
				continue;
			}
			if (!game.started) {
				continue;
			}
			if (player_do(player_get(fd), cmd)) {
				return 1;
			}
		}
	}
	return 0;
}

static int player_add(int fd) {
	if (game.nplayers >= MAX_PLAYERS) {
		return 1;
	}
	struct Player *p = &game.players[game.nplayers];
	p->fd = fd;
	p->name = 65 + game.nplayers;
	++game.nplayers;
	return 0;
}

static void game_offset_circle() {
	double between = 6.2831 / game.nplayers;
	double angle = between * rand();
	int center = MAP_LENGTH / 2;
	int radius = center / 2;
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			p->bearing = rand() % 4;
			p->x = round(center + cos(angle) * radius);
			p->y = round(center + sin(angle) * radius);
			map_set(p->x, p->y, TILE_FLATLAND);
			angle += between;
		}
	}
}

static int game_joined() {
	int n = 0;
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			++n;
		}
	}
	return n;
}

static void game_send_views() {
	int update = 0;
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd != 0 && !p->can_move) {
			player_view_write(p);
			update = 1;
			p->can_move = 1;
		}
	}
	if (update) {
		map_write(STDOUT);
	}
}

static int game_turn_complete() {
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->can_move) {
			return 0;
		}
	}
	return 1;
}

static void game_end() {
	struct Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			close(p->fd);
			player_remove(p->fd);
		}
	}
}

static void game_init() {
	memset(&game, 0, sizeof(game));
	srand(time(NULL));
	map_init();
	map_set(MAP_LENGTH / 2, MAP_LENGTH / 2, TILE_EXIT);
}

static void close_fds(fd_set *ro, int nfds) {
	int fd;
	for (fd = 0; fd < nfds; ++fd) {
		if (FD_ISSET(fd, ro)) {
			close(fd);
		}
	}
}

static int serve(int lfd) {
	struct timeval tick;
	struct timeval tv;
	int nfds;
	fd_set r, ro;

	#define RESET {\
		FD_ZERO(&ro);\
		FD_SET(lfd, &ro);\
		nfds = lfd + 1;\
	}
	RESET

	while (!game.shutdown) {
		memcpy(&r, &ro, sizeof(r));

		if (game.started) {
			if (gettimeofday(&tv, NULL)) {
				perror("gettimeofday");
				break;
			}
			time_t delta = (tv.tv_sec - tick.tv_sec) * USEC_PER_SEC -
				tick.tv_usec + tv.tv_usec;
			if (delta >= USEC_PER_TURN || game_turn_complete()) {
				game_send_views();
				tick.tv_sec = tv.tv_sec;
				tick.tv_usec = tv.tv_usec;
				delta = USEC_PER_TURN;
			} else {
				delta = USEC_PER_TURN - delta;
			}
			tv.tv_sec = delta / USEC_PER_SEC;
			tv.tv_usec = delta - (tv.tv_sec * USEC_PER_SEC);
		} else {
			tv.tv_sec = SECONDS_TO_JOIN;
			tv.tv_usec = 0;
		}

		int ready = select(nfds, &r, NULL, NULL, &tv);
		if (ready < 0) {
			perror("select");
			break;
		} else if (ready == 0) {
			if (!game.started && game.nplayers >= MIN_PLAYERS) {
				game_offset_circle();
				game.started = 1;
				if (gettimeofday(&tick, NULL)) {
					perror("gettimeofday");
					break;
				}
			}
			continue;
		}

		if (FD_ISSET(lfd, &r)) {
			struct sockaddr addr;
			socklen_t len;
			int fd = accept(lfd, &addr, &len);
			if (!game.started && !player_add(fd)) {
				FD_SET(fd, &ro);
				if (++fd > nfds) {
					nfds = fd;
				}
			} else {
				close(fd);
			}
		} else if (player_read_command(&r, &ro, nfds)) {
			game_init();
			RESET
		}
	}

	close_fds(&ro, nfds);

	return 0;
}

static int bind_to_port(int fd, int port) {
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
		close(fd);
		return 1;
	}

	return 0;
}

static void signalHandler(int id) {
	switch (id) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		game.shutdown = 1;
		break;
	}
}

int main(void) {
	game_init();

	int fd;
	if ((fd = socket(PF_INET, SOCK_STREAM, 6)) < 1) {
		perror("socket");
		return 1;
	}

	if (bind_to_port(fd, 51175) != 0) {
		perror("bind");
		return 1;
	}

	if (listen(fd, 4)) {
		perror("listen");
		close(fd);
		return 1;
	}

	signal(SIGHUP, signalHandler);
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	return serve(fd);
}
