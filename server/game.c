#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "maze.h"
#include "player.h"

#define USEC_PER_SEC 1000000L
#define SECONDS_TO_JOIN 10

struct Config config;
struct Game game;

static int stop = 0;

void game_end() {
	game.stopped = time(NULL);
}

size_t game_joined() {
	size_t n = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			++n;
		}
	}
	return n;
}

void game_remove_player(Player *p) {
	FD_CLR(p->fd, &game.watch);
	close(p->fd);
	p->fd = 0;
}

unsigned char game_marker_show_life(struct Player *p) {
	return p->life < config.player_life ? (unsigned char) 48 + p->life :
		p->name;
}

static int game_compare_player(const void *a, const void *b) {
	return ((Player *) b)->score - ((Player *) a)->score;
}

static void game_print_results() {
	qsort(game.players, game.nplayers, sizeof(Player),
		game_compare_player);
	printf("Place Name Score Moves\n");
	int place = 1;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++place) {
		printf("% 4d. %c    % 5d % 5d\n", place, p->name,
			p->score, p->moves);
	}
}

static void game_remove_players() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			game_remove_player(p);
		}
	}
}

static void game_inset_player(Player *p,
		const int x, const int y,
		const int left, const int top,
		const int right, const int bottom) {
	int shiftX = 0;
	int shiftY = 0;
	if (y == top) {
		shiftY = 1;
	} else if (y == bottom) {
		shiftY = -1;
	} else if (x == left) {
		shiftX = 1;
	} else if (x == right) {
		shiftX = -1;
	}
	int px = p->x;
	int py = p->y;
	int tries = 10;
	do {
		px += shiftX;
		py += shiftY;
	} while (--tries > 0 &&
		(game.impassable(&game.map, px, py) ||
			player_at(px, py)));
	if (tries > 0) {
		p->x = px;
		p->y = py;
	}
}

static void game_shrink() {
	size_t level = game.shrink_level;
	size_t mw = game.map.width;
	size_t mh = game.map.height;
	size_t min = mw < mh ? mw : mh;
	if (level > min / 3) {
		return;
	}
	int left = level;
	int top = level;
	int right = mw - left - 1;
	int bottom = mh - top - 1;
	int width = right - left;
	int step;
	int x;
	int y;
	for (y = top; y <= bottom; ++y) {
		step = (y == top || y == bottom) ? 1 : width;
		for (x = left; x <= right; x += step) {
			Player *p = player_at(x, y);
			if (p) {
				game_inset_player(p, x, y, left, top, right, bottom);
			}
			map_set(&game.map, x, y, TILE_GONE);
		}
	}
	++game.shrink_level;
}

static void game_write() {
	size_t size = game.map.size;
	char buf[size];
	memcpy(buf, game.map.data, size);
	int i;
	Player *p = game.players, *e = p + game.nplayers;
	for (i = 0; p < e; ++p, ++i) {
		if (p->fd > 0) {
			int x = p->x;
			int y = p->y;
			size_t offset = (y * game.map.width + x) % size;
			buf[offset] = p->name;
		}
	}
	map_write(1, buf, game.map.width, game.map.height);
	printf("turn: %d of %d, players: %ld\n", game.turn, game.max_turns,
		game_joined());
}

static void game_send_views() {
	int update = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd != 0 && !p->can_move) {
			if (!update && game.turn_start) {
				game.turn_start();
			}
			player_send_view(p);
			update = 1;
			p->can_move = 1;
			++p->moves;
		}
	}
	if (update) {
		++game.turn;
		if (game.turn >= game.max_turns) {
			game_end();
		} else if (game.turn > game.shrink_after) {
			game_shrink();
		}
		game_write();
	}
}

static int game_turn_complete() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->can_move) {
			return 0;
		}
	}
	return 1;
}

static time_t game_next_turn() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t delta = (tv.tv_sec - game.tick.tv_sec) * USEC_PER_SEC -
		game.tick.tv_usec + tv.tv_usec;
	if (delta >= game.usec_per_turn || game_turn_complete()) {
		game_send_views();
		game.tick.tv_sec = tv.tv_sec;
		game.tick.tv_usec = tv.tv_usec;
		delta = game.usec_per_turn;
	} else {
		delta = game.usec_per_turn - delta;
	}
	return delta;
}

static void game_read_command(Player *p) {
	char cmd;
	int b;
	if ((b = recv(p->fd, &cmd, sizeof(cmd), 0)) < 1) {
		perror("recv");
		game_remove_player(p);
		return;
	}
	if (game.started && p->can_move) {
		p->can_move = 0;
		game.move(p, cmd);
	}
}

static void game_read_commands() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && FD_ISSET(p->fd, &game.ready)) {
			game_read_command(p);
		}
	}
}

static int game_add_player(int fd) {
	if (game.nplayers >= MAX_PLAYERS) {
		return 1;
	}
	Player *p = &game.players[game.nplayers];
	p->fd = fd;
	p->name = 65 + game.nplayers;
	p->life = 1;
	++game.nplayers;
	FD_SET(fd, &game.watch);
	if (++fd > game.nfds) {
		game.nfds = fd;
	}
	return 0;
}

static void game_handle_joins() {
	if (!FD_ISSET(game.listening_fd, &game.ready)) {
		return;
	}
	struct sockaddr addr;
	socklen_t len;
	int fd = accept(game.listening_fd, &addr, &len);
	if (!game.started && !game_add_player(fd)) {
		printf("%d seats left, starting in %d seconds ...\n",
			MAX_PLAYERS - game.nplayers, SECONDS_TO_JOIN);
	} else {
		close(fd);
	}
}

static void game_set_players_life(int life) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd) {
			p->life = life;
		}
	}
}

static void game_start() {
	if (config.player_life > 0) {
		game_set_players_life(config.player_life < 10 ?
			config.player_life : 9);
	}
	game.started = time(NULL);
	gettimeofday(&game.tick, NULL);
	if (game.start) {
		game.start();
	}
}

static void game_shutdown() {
	close(game.listening_fd);
	game_remove_players();
	map_free(&game.map);
}

static void game_init_map() {
	if (game.map.width != config.map_width ||
			game.map.height != config.map_height) {
		map_create(&game.map, config.map_width, config.map_height);
	}
	switch (config.map_type) {
	default:
	case MAP_TYPE_CHESS:
		map_init_chess(&game.map);
		break;
	case MAP_TYPE_PLAIN:
		memset(game.map.data, TILE_FLATLAND, game.map.size);
		break;
	case MAP_TYPE_RANDOM:
		map_init_random(&game.map, 14, (char[]) {TILE_FLATLAND, 0},
			(char[]) {TILE_WATER, TILE_WOOD, 0});
		break;
	case MAP_TYPE_MAZE:
		maze_generate(&game.map, (game.map.width / 2) | 1,
			(game.map.height / 2) | 1);
		break;
	}
}

static void game_configure() {
	if (config.view_radius > 0) {
		game.view_radius = config.view_radius;
	}
	if (config.max_turns > 0) {
		game.max_turns = config.max_turns;
	}
	if (config.shrink_after > 0) {
		game.shrink_after = config.shrink_after;
	}
	if (config.usec_per_turn > 0) {
		game.usec_per_turn = config.usec_per_turn;
	}
}

static void game_set_defaults(const int lfd) {
	memset(&game, 0, sizeof(Game));
	FD_SET(lfd, &game.watch);
	game.listening_fd = lfd;
	game.nfds = lfd + 1;
	game.usec_per_turn = USEC_PER_SEC;
	game.min_players = 1;
	game.view_radius = 2;
	game.max_turns = 1024;
	game.shrink_after = game.max_turns;
	game.move = player_move;
	game.impassable = map_impassable;
}

static void game_reset(const int lfd) {
	srand(time(NULL));
	game_set_defaults(lfd);
	game_init_map();
	config.init();
	game_configure();
	printf("waiting for players (at least %d) to join ...\n",
		game.min_players);
}

static int game_run(const int lfd) {
	struct timeval tv;

	game_reset(lfd);

	while (!stop) {
		if (game.started) {
			time_t usec = game_next_turn();
			tv.tv_sec = usec / USEC_PER_SEC;
			tv.tv_usec = usec - (tv.tv_sec * USEC_PER_SEC);
		} else {
			tv.tv_sec = game.nplayers < MAX_PLAYERS ? SECONDS_TO_JOIN : 0;
			tv.tv_usec = 0;
		}

		memcpy(&game.ready, &game.watch, sizeof(fd_set));
		int ready = select(game.nfds, &game.ready, NULL, NULL, &tv);
		if (ready < 0) {
			perror("select");
			break;
		} else if (ready > 0) {
			game_handle_joins();
			game_read_commands();
		}

		if (game.started) {
			if (game.stopped || game_joined() < game.min_players) {
				game_print_results();
				game_remove_players();
				game_reset(lfd);
			}
		} else if (game.nplayers == MAX_PLAYERS ||
				(ready == 0 && game.nplayers >= game.min_players)) {
			game_start();
		}
	}

	game_shutdown();

	return 0;
}

static void game_handle_signal(const int id) {
	switch (id) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		stop = 1;
		break;
	}
}

static int game_bind_port(const int fd, const int port) {
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

int game_serve() {
	int fd;
	if ((fd = socket(PF_INET, SOCK_STREAM, 6)) < 1) {
		perror("socket");
		return 1;
	}

	if (game_bind_port(fd, config.port) != 0) {
		perror("bind");
		return 1;
	}

	if (listen(fd, 4)) {
		perror("listen");
		close(fd);
		return 1;
	}

	signal(SIGHUP, game_handle_signal);
	signal(SIGINT, game_handle_signal);
	signal(SIGTERM, game_handle_signal);

	return game_run(fd);
}
