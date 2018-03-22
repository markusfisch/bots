#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "player.h"
#include "game.h"

#define USEC_PER_SEC 1000000L
#define SECONDS_TO_JOIN 10

static int stop = 0;

void game_end(struct Game *game) {
	game->stopped = time(NULL);
}

int game_joined(struct Game *game) {
	int n = 0;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			++n;
		}
	}
	return n;
}

void game_remove_player(struct Game *game, struct Player *p) {
	FD_CLR(p->fd, &game->watch);
	close(p->fd);
	p->fd = 0;
}

static int game_compare_player(const void *a, const void *b) {
	return ((struct Player *) b)->score - ((struct Player *) a)->score;
}

static void game_print_results(struct Game *game) {
	qsort(game->players, game->nplayers, sizeof(struct Player),
		game_compare_player);
	printf("Place Name Score Moves\n");
	int place = 1;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p, ++place) {
		printf("% 4d. %c    % 5d % 5d\n", place, p->name,
			p->score, p->moves);
	}
}

static void game_remove_players(struct Game *game) {
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			game_remove_player(game, p);
		}
	}
}

static void game_inset_player(struct Game *game, struct Player *p,
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
		(game->impassable(&game->map, px, py) ||
			player_at(game, px, py)));
	if (tries > 0) {
		p->x = px;
		p->y = py;
	}
}

static void game_shrink(struct Game *game) {
	size_t level = game->shrink_level;
	size_t mw = game->map.width;
	size_t mh = game->map.height;
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
			struct Player *p = player_at(game, x, y);
			if (p != NULL) {
				game_inset_player(game, p, x, y, left, top, right, bottom);
			}
			map_set(&game->map, x, y, TILE_GONE);
		}
	}
	++game->shrink_level;
}

static void game_write(struct Game *game) {
	size_t size = game->map.size;
	char buf[size];
	memcpy(buf, game->map.data, size);
	int i;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (i = 0; p < e; ++p, ++i) {
		if (p->fd > 0) {
			int x = p->x;
			int y = p->y;
			size_t offset = (y * game->map.width + x) % size;
			buf[offset] = p->name;
		}
	}
	map_write(1, buf, game->map.width, game->map.height);
	printf("turn: %d of %d, players: %d\n", game->turn, game->max_turns,
		game_joined(game));
}

static void game_send_views(struct Game *game) {
	int update = 0;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd != 0 && !p->can_move) {
			player_send_view(game, p);
			update = 1;
			p->can_move = 1;
			++p->moves;
		}
	}
	if (update) {
		if (++game->turn >= game->max_turns) {
			game_end(game);
		} else if (game->turn > game->shrink_at_turn) {
			game_shrink(game);
		}
		game_write(game);
	}
}

static int game_turn_complete(struct Game *game) {
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->can_move) {
			return 0;
		}
	}
	return 1;
}

static time_t game_next_turn(struct Game *game) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t delta = (tv.tv_sec - game->tick.tv_sec) * USEC_PER_SEC -
		game->tick.tv_usec + tv.tv_usec;
	if (delta >= game->usec_per_turn || game_turn_complete(game)) {
		game_send_views(game);
		game->tick.tv_sec = tv.tv_sec;
		game->tick.tv_usec = tv.tv_usec;
		delta = game->usec_per_turn;
	} else {
		delta = game->usec_per_turn - delta;
	}
	return delta;
}

static void game_read_command(struct Game *game, struct Player *p) {
	char cmd;
	int b;
	if ((b = recv(p->fd, &cmd, sizeof(cmd), 0)) < 1) {
		perror("recv");
		game_remove_player(game, p);
		return;
	}
	if (game->started && p->can_move) {
		p->can_move = 0;
		game->move(game, p, cmd);
	}
}

static void game_read_commands(struct Game *game) {
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && FD_ISSET(p->fd, &game->ready)) {
			game_read_command(game, p);
		}
	}
}

static int game_add_player(struct Game *game, int fd) {
	if (game->nplayers >= MAX_PLAYERS) {
		return 1;
	}
	struct Player *p = &game->players[game->nplayers];
	p->fd = fd;
	p->name = 65 + game->nplayers;
	++game->nplayers;
	FD_SET(fd, &game->watch);
	if (++fd > game->nfds) {
		game->nfds = fd;
	}
	return 0;
}

static void game_handle_joins(struct Game *game) {
	if (!FD_ISSET(game->listening_fd, &game->ready)) {
		return;
	}
	struct sockaddr addr;
	socklen_t len;
	int fd = accept(game->listening_fd, &addr, &len);
	if (!game->started && !game_add_player(game, fd)) {
		printf("%d seats left, starting in %d seconds ...\n",
			MAX_PLAYERS - game->nplayers, SECONDS_TO_JOIN);
	} else {
		close(fd);
	}
}

static void game_start(struct Game *game) {
	game->started = time(NULL);
	gettimeofday(&game->tick, NULL);
	game->start(game);
}

static void game_shutdown(struct Game *game) {
	close(game->listening_fd);
	game_remove_players(game);
	map_free(&game->map);
}

static void game_reset(struct Game *game, const int lfd,
		void (*init)(struct Game *)) {
	srand(time(NULL));
	memset(game, 0, sizeof(struct Game));
	game->usec_per_turn = USEC_PER_SEC;
	game->min_players = 1;
	game->view_radius = 2;
	game->max_turns = 1024;
	game->shrink_at_turn = game->max_turns;
	game->listening_fd = lfd;
	game->nfds = lfd + 1;
	game->move = player_move;
	game->impassable = map_impassable;
	FD_SET(lfd, &game->watch);
	init(game);
	if (!game->start) {
		fprintf(stderr, "error: game didn't give a start function\n");
		stop = 1;
		return;
	}
	printf("waiting for players to join ...\n");
}

static int game_run(const int lfd, void (*init)(struct Game *)) {
	struct Game game;
	struct timeval tv;

	game_reset(&game, lfd, init);

	while (!stop) {
		memcpy(&game.ready, &game.watch, sizeof(fd_set));

		if (game.started) {
			time_t usec = game_next_turn(&game);
			tv.tv_sec = usec / USEC_PER_SEC;
			tv.tv_usec = usec - (tv.tv_sec * USEC_PER_SEC);
		} else {
			tv.tv_sec = game.nplayers < MAX_PLAYERS ? SECONDS_TO_JOIN : 0;
			tv.tv_usec = 0;
		}

		int ready = select(game.nfds, &game.ready, NULL, NULL, &tv);
		if (ready < 0) {
			perror("select");
			break;
		} else if (ready > 0) {
			game_handle_joins(&game);
			game_read_commands(&game);
		}

		if (game.started) {
			if (game.stopped || game_joined(&game) < game.min_players) {
				game_print_results(&game);
				game_remove_players(&game);
				game_reset(&game, lfd, init);
			}
		} else if (game.nplayers == MAX_PLAYERS ||
				(ready == 0 && game.nplayers >= game.min_players)) {
			game_start(&game);
		}
	}

	game_shutdown(&game);

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

int game_serve(void (*init)(struct Game *)) {
	int fd;
	if ((fd = socket(PF_INET, SOCK_STREAM, 6)) < 1) {
		perror("socket");
		return 1;
	}

	if (game_bind_port(fd, 51175) != 0) {
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

	return game_run(fd, init);
}
