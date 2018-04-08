#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "maze.h"
#include "placing.h"
#include "player.h"

struct Config config;
struct Game game;

static int stop = 0;

void game_end() {
	game.stopped = time(NULL);
}

unsigned int game_joined() {
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
	if (p->fd > 0) {
		FD_CLR(p->fd, &game.watch);
		close(p->fd);
		p->fd = 0;
	}
}

unsigned char game_marker_show_life(struct Player *p) {
	return p->life < config.player_life ? (unsigned char) 48 + p->life :
		p->name;
}

static int game_compare_player(const void *a, const void *b) {
	return ((Player *) b)->score - ((Player *) a)->score;
}

static void game_print_results() {
	qsort(game.players, game.nplayers, sizeof(Player), game_compare_player);
	char *format;
	switch (config.output_format) {
	default:
	case FORMAT_PLAIN:
		printf("Place Name Score Moves Killer\n");
		format = "% 4d. %c    % 5d % 5d %c\n";
		break;
	case FORMAT_JSON:
		// close turns array and open results
		printf("],\n\"results\":[\n");
		format = "{\"place\":%d,\"name\":\"%c\",\"score\":%d,"\
			"\"moves\":%d,\"killer\":\"%c\"}";
		break;
	}
	int place = 1;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++place) {
		if (config.output_format == FORMAT_JSON && p > game.players) {
			printf(",\n");
		}
		printf(format, place, p->name, p->score, p->moves,
			p->killed_by ?: ' ');
	}
	switch (config.output_format) {
	case FORMAT_JSON:
		// close results
		printf("\n]\n");
		// close root object
		printf("}\n");
		break;
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
	} while (--tries > 0 && (config.impassable(&game.map, px, py) ||
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

static void game_write_json(char *buf) {
	if (game.turn > 1) {
		printf(",\n");
	}
	printf("{\"turn\":%d,\"players\":[\n", game.turn);
	Player *p = game.players, *e = p + game.nplayers;
	for (p = game.players; p < e; ++p) {
		if (p > game.players) {
			puts(",");
		}
		printf("{\"name\":\"%c\",\"facing\":\"%c\",\"life\":%d,"\
			"\"moves\":%d,\"killed_by\":\"%c\"}",
			p->name,
			p->fd > 0 ? player_bearing(p->bearing) : 'x',
			p->life,
			p->moves,
			p->killed_by > 0 ?: ' ');
	}
	printf("\n],\"map\":[\n");
	fflush(stdout);
	unsigned int y;
	for (y = 0; y < game.map.height; ++y) {
		if (y > 0) {
			write(STDOUT_FILENO, ",\n", 2);
		}
		write(STDOUT_FILENO, "\"", 1);
		write(STDOUT_FILENO, buf, game.map.width);
		write(STDOUT_FILENO, "\"", 1);
		buf += game.map.width;
	}
	fflush(stdout);
	printf("\n]\n}");
}

static void game_write_plain(char *buf) {
	map_write(STDOUT_FILENO, buf, game.map.width, game.map.height);
	printf("Turn %d of %d.\n", game.turn, config.max_turns);
	printf("%d players of %d alive.\n", game.nplayers, game_joined());
	printf("Player Facing Life Moves Killer\n");
	Player *p = game.players, *e = p + game.nplayers;
	for (p = game.players; p < e; ++p) {
		printf("%c      %c      % 4d % 5d %c\n",
			p->name,
			p->fd > 0 ? player_bearing(p->bearing) : 'x',
			p->life,
			p->moves,
			p->killed_by > 0 ?: ' ');
	}
}

static void game_write() {
	size_t size = game.map.size;
	char buf[size];
	memcpy(buf, game.map.data, size);
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			buf[(p->y * game.map.width + p->x) % size] = p->name;
		}
	}
	switch (config.output_format) {
	default:
	case FORMAT_PLAIN:
		game_write_plain(buf);
		break;
	case FORMAT_JSON:
		game_write_json(buf);
		break;
	}
}

static void game_send_views() {
	int update = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd != 0 && !p->can_move) {
			if (!update && config.turn_start) {
				config.turn_start();
			}
			player_send_view(p);
			update = 1;
			p->can_move = 1;
			++p->moves;
		}
	}
	if (update) {
		++game.turn;
		if (game.turn >= config.max_turns) {
			game_end();
		} else if (game.turn > config.shrink_after) {
			if (game.shrink_step > 0) {
				--game.shrink_step;
			}
			if (game.shrink_step < 1) {
				game_shrink();
				game.shrink_step = config.shrink_step;
			}
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
	if (delta >= config.usec_per_turn || game_turn_complete()) {
		game_send_views();
		game.tick.tv_sec = tv.tv_sec;
		game.tick.tv_usec = tv.tv_usec;
		delta = config.usec_per_turn;
	} else {
		delta = config.usec_per_turn - delta;
	}
	return delta;
}

static void game_read_command(Player *p) {
	char cmd;
	int b;
	if ((b = recv(p->fd, &cmd, sizeof(cmd), 0)) < 1) {
		if (b) {
			perror("recv");
		}
		game_remove_player(p);
		return;
	}
	if (game.started && p->can_move) {
		p->can_move = 0;
		config.move(p, cmd);
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
		return 0;
	}
	Player *p = &game.players[game.nplayers];
	p->fd = fd;
	p->name = 65 + game.nplayers;
	p->life = 1;
	p->x = p->y = -1;
	++game.nplayers;
	FD_SET(fd, &game.watch);
	if (++fd > game.nfds) {
		game.nfds = fd;
	}
	return 1;
}

static void game_handle_joins() {
	if (!FD_ISSET(game.listening_fd, &game.ready)) {
		return;
	}
	struct sockaddr addr;
	socklen_t len;
	int fd = accept(game.listening_fd, &addr, &len);
	if (!game.started && game_add_player(fd)) {
		if (config.output_format == FORMAT_PLAIN) {
			printf("%d seats left, starting in %ld seconds ...\n",
				MAX_PLAYERS - game.nplayers, config.wait_for_joins);
		}
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

static void game_place_players() {
	switch (config.placing) {
	default:
	case PLACING_CIRCLE:
		placing_circle();
		break;
	case PLACING_RANDOM:
		placing_random();
		break;
	case PLACING_MANUAL:
		placing_manual(config.coords);
		break;
	}
}

static void game_init_map() {
	if (game.map.width != config.map_width ||
			game.map.height != config.map_height) {
		map_create(&game.map, config.map_width, config.map_height);
	}
	switch (config.map_type) {
	default:
	case MAP_TYPE_CHESS:
		map_init_chess(&game.map, *config.flatland,
			strlen(config.flatland) > 1 ? config.flatland[1] : TILE_BLACK);
		break;
	case MAP_TYPE_PLAIN:
		memset(game.map.data, *config.flatland, game.map.size);
		break;
	case MAP_TYPE_RANDOM:
		map_init_random(&game.map, config.multiplier, config.flatland,
			config.obstacles);
		break;
	case MAP_TYPE_MAZE:
		maze_generate(&game.map, game.map.width / 2, game.map.height / 2,
			*config.flatland, TILE_GONE);
		break;
	}
}

static void game_start() {
	game_init_map();
	game_place_players();
	if (config.player_life > 0) {
		game_set_players_life(config.player_life < 10 ?
			config.player_life : 9);
	}
	game.started = time(NULL);
	gettimeofday(&game.tick, NULL);
	if (config.start) {
		config.start();
	}
	switch (config.output_format) {
	case FORMAT_JSON:
		printf("{\n\"max_turns\":%d,\n\"turns\":[\n", config.max_turns);
		break;
	}
}

static void game_shutdown() {
	close(game.listening_fd);
	game_remove_players();
	map_free(&game.map);
}

static void game_reset(const int lfd) {
	memset(&game, 0, sizeof(Game));
	FD_SET(lfd, &game.watch);
	game.listening_fd = lfd;
	game.nfds = lfd + 1;
	if (config.output_format == FORMAT_PLAIN) {
		printf("waiting for players (at least %d) to join ...\n",
			config.min_players);
	}
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
			tv.tv_sec = game.nplayers < MAX_PLAYERS ?
				config.wait_for_joins : 0;
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
			if (game.stopped || game_joined() < config.min_players) {
				if (config.end) {
					config.end();
				}
				game_print_results();
				game_remove_players();
				if (config.keep_running) {
					game_reset(lfd);
				} else {
					break;
				}
			}
		} else if (game.nplayers == MAX_PLAYERS ||
				(ready == 0 && game.nplayers >= config.min_players)) {
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
