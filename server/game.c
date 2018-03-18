#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "player.h"
#include "game.h"
#include "games/find_exit.h"

static int stop = 0;

void game_end(struct Game *game) {
	game->stopped = time(NULL);
}

static void game_remove_player(struct Game *game, struct Player *p) {
	FD_CLR(p->fd, &game->watch);
	close(p->fd);
	p->fd = 0;
}

static void game_remove_players(struct Game *game) {
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			game_remove_player(game, p);
		}
	}
}

static int game_joined(struct Game *game) {
	int n = 0;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			++n;
		}
	}
	return n;
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
	printf("players: %d\n", game_joined(game));
}

static void game_send_views(struct Game *game) {
	int update = 0;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd != 0 && !p->can_move) {
			player_send_view(game, p);
			update = 1;
			p->can_move = 1;
		}
	}
	if (update) {
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
	if (game->started) {
		player_do(game, p, cmd);
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

static void game_reset(struct Game *game, int lfd) {
	srand(time(NULL));
	memset(game, 0, sizeof(struct Game));
	game->listening_fd = lfd;
	game->nfds = lfd + 1;
	FD_SET(game->listening_fd, &game->watch);
	init_find_exit(game);
	printf("waiting for players to join ...\n");
}

static void game_shutdown(struct Game *game) {
	close(game->listening_fd);
	game_remove_players(game);
	map_free(&game->map);
}

static int game_run(int lfd) {
	struct Game game;
	struct timeval tv;

	game_reset(&game, lfd);

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
			if (game.stopped || game_joined(&game) < 1) {
				game_remove_players(&game);
				game_reset(&game, lfd);
			}
		} else if (game.nplayers == MAX_PLAYERS ||
				(ready == 0 && game.nplayers >= game.min_players)) {
			game_start(&game);
		}
	}

	game_shutdown(&game);

	return 0;
}

static void game_handle_signal(int id) {
	switch (id) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		stop = 1;
		break;
	}
}

static int game_bind_port(int fd, int port) {
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

	return game_run(fd);
}
