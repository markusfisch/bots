#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "map.h"
#include "player.h"
#include "game.h"
#include "games/find_exit.h"

static int stop = 0;

static void close_fds(fd_set *ro, int nfds) {
	int fd;
	for (fd = 0; fd < nfds; ++fd) {
		if (FD_ISSET(fd, ro)) {
			close(fd);
		}
	}
}

static int read_command(struct Game *game, int fd, fd_set *ro) {
	char cmd;
	int b;
	if ((b = recv(fd, &cmd, sizeof(cmd), 0)) < 1) {
		perror("recv");
		close(fd);
		FD_CLR(fd, ro);
		player_remove(game, fd);
		if (game_joined(game) < 1) {
			return 1;
		}
		return 0;
	}
	if (!game->started) {
		return 0;
	}
	return player_do(game, player_get(game, fd), cmd);
}

static int read_commands(struct Game *game, fd_set *r, fd_set *ro, int nfds) {
	int fd;
	for (fd = 0; fd < nfds; ++fd) {
		if (FD_ISSET(fd, r) && read_command(game, fd, ro)) {
			return 1;
		}
	}
	return 0;
}

static int reset(struct Game *game, int lfd, fd_set *ro) {
	srand(time(NULL));
	memset(game, 0, sizeof(struct Game));
	init_find_exit(game);
	FD_ZERO(ro);
	FD_SET(lfd, ro);
	printf("waiting %d seconds for players to join ...\n",
		SECONDS_TO_JOIN);
	return lfd + 1;
}

static int serve(int lfd) {
	struct Game game;
	struct timeval tv;
	fd_set r, ro;
	int nfds = reset(&game, lfd, &ro);

	while (!stop) {
		memcpy(&r, &ro, sizeof(r));

		if (game.started) {
			time_t usec = game_next_turn(&game);
			tv.tv_sec = usec / USEC_PER_SEC;
			tv.tv_usec = usec - (tv.tv_sec * USEC_PER_SEC);
		} else {
			tv.tv_sec = game.nplayers < MAX_PLAYERS ? SECONDS_TO_JOIN : 0;
			tv.tv_usec = 0;
		}

		int ready = select(nfds, &r, NULL, NULL, &tv);
		if (ready < 0) {
			perror("select");
			break;
		} else if (ready == 0) {
			if (!game.started && game.nplayers >= game.min_players) {
				game_start(&game);
			}
			continue;
		}

		if (FD_ISSET(lfd, &r)) {
			struct sockaddr addr;
			socklen_t len;
			int fd = accept(lfd, &addr, &len);
			if (!game.started && !player_add(&game, fd)) {
				FD_SET(fd, &ro);
				if (++fd > nfds) {
					nfds = fd;
				}
				printf("player %d joined\n", game.nplayers);
			} else {
				close(fd);
			}
		} else if (read_commands(&game, &r, &ro, nfds)) {
			nfds = reset(&game, lfd, &ro);
		}
	}

	close_fds(&ro, nfds);

	return 0;
}

static void signalHandler(int id) {
	switch (id) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		stop = 1;
		break;
	}
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

int main() {
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
