#define _GNU_SOURCE
#include <arpa/inet.h>
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

#define ROLE_PLAYER "player"
#define ROLE_SPECTATOR "spectator"

struct Config config;
struct Game game;

static int stop = 0;

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
		if (p->ws.fd > 0) {
			websocket_close(&p->ws);
		} else {
			close(p->fd);
		}
		p->fd = 0;
	}
}

static void game_remove_spectator(Spectator *s) {
	if (s->ws.fd > 0) {
		FD_CLR(s->ws.fd, &game.watch);
		websocket_close(&s->ws);
		s->ws.fd = 0;
	}
}

static void game_send_spectators(const char *message) {
	Spectator *p = game.spectators, *e = p + game.nspectators;
	for (; p < e; ++p) {
		if (p->ws.fd > 0 && websocket_send_text_message(&p->ws, message,
				strlen(message)) < 0) {
			game_remove_spectator(p);
		}
	}
}

static void game_write_results_in_format(FILE *fp, int format,
		int standalone) {
	char *pfmt;
	switch (format) {
	default:
	case FORMAT_PLAIN:
		fprintf(fp, "================== RESULTS ==================\n");
		fprintf(fp, "Place Address         Name Score Moves Killer\n");
		pfmt = "% 4d. %-15s %c    % 5d % 5d %c\n";
		break;
	case FORMAT_JSON:
		if (!standalone) {
			// close turns array and open results
			fprintf(fp, "],\n\"results\":[\n");
		} else {
			fprintf(fp, "{\"results\":[\n");
		}
		pfmt = "{\"place\":%d,\"addr\":\"%s\",\"name\":\"%c\","\
			"\"score\":%d,\"moves\":%d,\"killer\":\"%c\"}";
		break;
	}
	int place = 1;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++place) {
		if (format == FORMAT_JSON && p > game.players) {
			fprintf(fp, ",\n");
		}
		fprintf(fp, pfmt, place, p->addr, p->name, p->score, p->moves,
			p->killed_by ?: '-');
	}
	switch (format) {
	case FORMAT_JSON:
		// close results and root object
		fprintf(fp, "\n]}\n");
		break;
	}
	fflush(fp);
}

static int game_compare_player_score(const void *a, const void *b) {
	return ((Player *) b)->score - ((Player *) a)->score;
}

static void game_write_results(FILE *fp) {
	qsort(game.players, game.nplayers, sizeof(Player),
		game_compare_player_score);
	game_write_results_in_format(fp, config.output_format, 0);
	if (game.nspectators > 0) {
		char buf[2048];
		FILE *fp = fmemopen(buf, sizeof(buf), "w");
		game_write_results_in_format(fp, FORMAT_JSON, 1);
		fclose(fp);
		game_send_spectators(buf);
	}
}

static void game_remove_spectators() {
	Spectator *p = game.spectators, *e = p + game.nspectators;
	for (; p < e; ++p) {
		if (p->ws.fd > 0) {
			game_remove_spectator(p);
		}
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
	}
	if (x == left) {
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
	} while (--tries > 0 && player_cannot_move_to(px, py));
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
			Player *p = NULL;
			while ((p = player_at(x, y, p))) {
				game_inset_player(p, x, y, left, top, right, bottom);
			}
			map_set(&game.map, x, y, TILE_GONE);
		}
	}
	++game.shrink_level;
}

static void game_write_json(FILE *fp, const char *map, int standalone) {
	if (!standalone && game.turn > 1) {
		fprintf(fp, ",\n");
	}
	fprintf(fp, "{\"turn\":%d,\"players\":[\n", game.turn);
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p > game.players) {
			fprintf(fp, ",");
		}
		fprintf(fp, "{\"addr\":\"%s\",\"name\":\"%c\",\"x\":%d,\"y\":%d,"\
			"\"bearing\":\"%c\",\"life\":%d,\"moves\":%d,\"score\":%d",
			p->addr,
			p->name,
			p->x,
			p->y,
			player_bearing(p->bearing),
			p->fd > 0 ? p->life : 0,
			p->moves,
			p->score);
		if (p->killed_by > 0) {
			fprintf(fp, ",\"killed_by\":\"%c\"", p->killed_by);
		}
		if (p->attack_x > -1) {
			fprintf(fp, ",\"attack_x\":%d,\"attack_y\":%d",
				p->attack_x, p->attack_y);
		}
		fprintf(fp, "}");
	}
	fprintf(fp, "\n],\"map\":\"");
	unsigned int y;
	for (y = 0; y < game.map.height; ++y) {
		fwrite(map, sizeof(char), game.map.width, fp);
		map += game.map.width;
	}
	fprintf(fp, "\"}");
}

static void game_write_plain(FILE *fp, const char *map) {
	unsigned int y;
	for (y = 0; y < game.map.height; ++y) {
		fwrite(map, sizeof(char), game.map.width, fp);
		fwrite("\n", sizeof(char),  1, fp);
		map += game.map.width;
	}
	fprintf(fp, "Turn %d of %d. %d of %d players alive.\n", game.turn,
		config.max_turns, game_joined(), game.nplayers);
	fprintf(fp, "Address          Name    X       Y ° "\
		"Life Moves Score Attacks\n");
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		fprintf(fp, "%-16s %c % 7d % 7d %c ",
			p->addr,
			p->name,
			p->x,
			p->y,
			player_bearing(p->bearing));
		if (p->fd < 1) {
			fprintf(fp, "   0 ");
		} else {
			fprintf(fp, "% 4d ", p->life);
		}
		fprintf(fp, "% 5d % 5d", p->moves, p->score);
		if (p->attack_x > -1) {
			fprintf(fp, " %d/%d", p->attack_x, p->attack_y);
		}
		fprintf(fp, "\n");
	}
}

static void game_write(FILE *fp) {
	size_t size = game.map.size;
	char map[size];
	memcpy(map, game.map.data, size);
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			map[(p->y * game.map.width + p->x) % size] = p->name;
		}
	}
	switch (config.output_format) {
	default:
	case FORMAT_PLAIN:
		game_write_plain(fp, map);
		break;
	case FORMAT_JSON:
		game_write_json(fp, map, 0);
		break;
	}
	fflush(fp);
	if (game.nspectators > 0) {
		char buf[game.map.width * game.map.height + 1024];
		FILE *fp = fmemopen(buf, sizeof(buf), "w");
		game_write_json(fp, map, 1);
		fclose(fp);
		game_send_spectators(buf);
	}
}

static void game_remove_defunkt_players() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->moves + config.max_lag < game.turn) {
			game_remove_player(p);
		}
	}
}

static void game_stop() {
	game.stopped = time(NULL);
}

void game_terminate() {
	if (!game.stopped) {
		if (config.turn_start) {
			config.turn_start();
		}
		++game.turn;
		game_write(stdout);
		game_stop();
	}
}

static void game_send_players() {
	int turn_started = 0;
	int update = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && !p->can_move) {
			update = 1;
			if (!turn_started && config.turn_start) {
				config.turn_start();
				turn_started = 1;
				// check again if this player got removed
				if (p->fd < 1) {
					continue;
				}
			}
			if (player_send_view(p) < 0) {
				game_remove_player(p);
				continue;
			}
			p->can_move = 1;
			++p->moves;
		}
	}
	if (update) {
		++game.turn;
		if (game.turn >= config.max_turns) {
			game_stop();
		} else if (game.turn > config.shrink_after) {
			if (game.shrink_step > 0) {
				--game.shrink_step;
			}
			if (game.shrink_step < 1) {
				game_shrink();
				game.shrink_step = config.shrink_step;
			}
		}
		game_write(stdout);
	}
	for (p = game.players; p < e; ++p) {
		if (p->fd > 0) {
			p->attack_x = p->attack_y = -1;
		}
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
		game_send_players();
		game.tick.tv_sec = tv.tv_sec;
		game.tick.tv_usec = tv.tv_usec;
		delta = config.usec_per_turn;
	} else {
		delta = config.usec_per_turn - delta;
	}
	return delta;
}

static void game_read_spectators() {
	Spectator *p = game.spectators, *e = p + game.nspectators;
	for (; p < e; ++p) {
		if (p->ws.fd > 0 && FD_ISSET(p->ws.fd, &game.ready)) {
			if (websocket_read(&p->ws, NULL) < 0) {
				game_remove_spectator(p);
			}
		}
	}
}

static void game_do_command(Player *p, char cmd) {
	if (game.started && p->can_move) {
		p->can_move = 0;
		config.move(p, cmd);
	}
}

static void game_read_websocket_command(Player *p) {
	char *message;
	int len = websocket_read(&p->ws, &message);
	if (len < 0) {
		game_remove_player(p);
	} else if (len > 0) {
		game_do_command(p, *message);
	}
}

static void game_read_command(Player *p) {
	char cmd;
	int b;
	if ((b = recv(p->fd, &cmd, sizeof(cmd), 0)) < 1) {
		game_remove_player(p);
		return;
	}
	game_do_command(p, cmd);
}

static void game_read_commands() {
	// use a Fisher-Yates shuffle to process commands in random order
	Player *queue[MAX_PLAYERS];
	Player **q = queue, **e = queue + game.nplayers;
	Player *p = game.players;
	for (; q < e; ++q, ++p) {
		*q = p;
	}
	int i;
	for (i = game.nplayers; i-- > 1;) {
		int j = config.rand() % i;
		Player *tmp = queue[j];
		queue[j] = queue[i];
		queue[i] = tmp;
	}
	for (q = queue; q < e; ++q) {
		if ((*q)->fd > 0 && FD_ISSET((*q)->fd, &game.ready)) {
			if ((*q)->ws.fd > 0) {
				game_read_websocket_command(*q);
			} else {
				game_read_command(*q);
			}
		}
	}
}

static void game_watch_fd(int fd) {
	FD_SET(fd, &game.watch);
	if (++fd > game.nfds) {
		game.nfds = fd;
	}
}

static int game_add_spectator(int fd, char *addr) {
	if (game.nspectators >= config.max_spectators ||
			game.nspectators >= MAX_SPECTATORS) {
		return 0;
	}
	Spectator *s = &game.spectators[game.nspectators];
	strcpy(s->addr, addr);
	s->ws.fd = fd;
	game_watch_fd(fd);
	++game.nspectators;
	return 1;
}

static Player *game_add_player(int fd, char *addr) {
	if (game.nplayers >= MAX_PLAYERS) {
		return NULL;
	}
	Player *p = &game.players[game.nplayers];
	strcpy(p->addr, addr);
	p->fd = fd;
	p->life = 1;
	p->x = p->y = -1;
	p->attack_x = p->attack_y = -1;
	++game.nplayers;
	game_watch_fd(fd);
	return p;
}

static int game_add_player_socket(int fd, char *addr) {
	return game_add_player(fd, addr) != NULL;
}

static int game_add_player_websocket(int fd, char *addr) {
	Player *p = game_add_player(fd, addr);
	if (!p) {
		return 0;
	}
	p->ws.fd = fd;
	return 1;
}

static void game_join(const int lfd, int (*add)(int, char *),
		const char *role) {
	struct sockaddr addr;
	socklen_t len = sizeof(addr);
	int fd = accept(lfd, &addr, &len);
	if (game.started) {
		close(fd);
		return;
	}
	struct sockaddr_in* ipv4 = (struct sockaddr_in *) &addr;
	struct in_addr sin_addr = ipv4->sin_addr;
	char ip_str[INET_ADDRSTRLEN] = { 0 };
	inet_ntop(AF_INET, &sin_addr, ip_str, INET_ADDRSTRLEN);
	if (!add(fd, ip_str)) {
		close(fd);
		return;
	}
	if (config.output_format == FORMAT_PLAIN) {
		printf("%s joined as %s\n", ip_str, role);
		fflush(stdout);
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
		placing_circle(config.placing_fuzz);
		break;
	case PLACING_RANDOM:
		placing_random(config.placing_fuzz);
		break;
	case PLACING_GRID:
		placing_grid(config.placing_fuzz);
		break;
	case PLACING_DIAGONAL:
		placing_diagonal(config.placing_fuzz);
		break;
	case PLACING_MANUAL:
		placing_manual(config.coords, config.placing_fuzz);
		break;
	}
}

static void game_init_map() {
	if (game.map.width != config.map_width ||
			game.map.height != config.map_height) {
		map_create(&game.map, config.map_width, config.map_height);
	}
	if (config.custom_map) {
		memcpy(game.map.data, config.custom_map, game.map.size);
		game.map.obstacles = config.obstacles;
		return;
	}
	switch (config.map_type) {
	default:
	case MAP_TYPE_PLAIN:
		memset(game.map.data, *config.flatland, game.map.size);
		game.map.obstacles = config.obstacles;
		break;
	case MAP_TYPE_RANDOM:
		map_init_random(&game.map, config.multiplier, config.flatland,
			config.obstacles);
		break;
	case MAP_TYPE_MAZE:
		maze_generate(&game.map, game.map.width / 2, game.map.height / 2,
			*config.flatland, TILE_GONE);
		game.map.obstacles = config.obstacles;
		break;
	}
}

static char game_find_free_name(char *taken, int *last) {
	int i;
	for (i = *last; i < MAX_PLAYERS; ++i) {
		if (!taken[i]) {
			*last = i + 1;
			return 'A' + i;
		}
	}
	return 0;
}

static void game_assign_unnamed_players(char *taken) {
	int last = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (!p->name) {
			p->name = game_find_free_name(taken, &last);
		}
	}
}

static Player *game_find_unnamed_player_for_address(const char *addr) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (!p->name && !strcmp(p->addr, addr)) {
			return p;
		}
	}
	return NULL;
}

static void game_assign_mapped_names(char *taken) {
	int i;
	for (i = 0; i < MAX_NAMES; ++i) {
		Names *n = &config.names[i];
		if (!n->address) {
			break;
		}
		Player *p = game_find_unnamed_player_for_address(n->address);
		if (!p || p->name) {
			continue;
		}
		char name = n->name;
		int i = name - 'A';
		if (i < 0 || i >= MAX_PLAYERS || taken[i]) {
			continue;
		}
		p->name = name;
		taken[i] = 1;
	}
}

static int game_compare_player_address(const void *a, const void *b) {
	return strcmp(((Player *) b)->addr, ((Player *) a)->addr);
}

static void game_assign_player_names() {
	qsort(game.players, game.nplayers, sizeof(Player),
		game_compare_player_address);
	char taken[MAX_PLAYERS];
	memset(taken, 0, sizeof(taken));
	game_assign_mapped_names(taken);
	game_assign_unnamed_players(taken);
}

static void game_write_header() {
	if (config.output_format == FORMAT_JSON) {
		printf("{\"mode\":\"%s\",\n"\
				"\"max_turns\":%d,\n"\
				"\"map_width\":%d,\n"\
				"\"map_height\":%d,\n"\
				"\"view_radius\":%d,\n"\
				"\"obstacles\":\"%s\",\n"\
				"\"flatland\":\"%s\",\n"\
				"\"turns\":[\n",
			config.mode_name,
			config.max_turns,
			config.map_width,
			config.map_height,
			config.view_radius,
			config.obstacles,
			config.flatland);
	}
	if (game.nspectators > 0) {
		char buf[1024];
		snprintf(buf, sizeof(buf),
				"{\"mode\":\"%s\",\n"\
				"\"max_turns\":%d,\n"\
				"\"map_width\":%d,\n"\
				"\"map_height\":%d,\n"\
				"\"view_radius\":%d,\n"\
				"\"obstacles\":\"%s\",\n"\
				"\"flatland\":\"%s\"}",
			config.mode_name,
			config.max_turns,
			config.map_width,
			config.map_height,
			config.view_radius,
			config.obstacles,
			config.flatland);
		game_send_spectators(buf);
	}
}

static void game_restrict_spectators() {
	if (config.remote_spectators || game.nplayers < 2 ||
			game.nspectators < 1) {
		// no restrictions for single player games
		return;
	}
	// remove all but exactly one spectator that must be on localhost
	// if there are multiple players to prevent players from stealing
	// the whole map by sniffing network traffic or providing a fake
	// spectator
	int count = 0;
	Spectator *p = game.spectators, *e = p + game.nspectators;
	for (; p < e; ++p) {
		if (p->ws.fd > 0) {
			if (count > 0 || strcmp(p->addr, "127.0.0.1")) {
				game_remove_spectator(p);
			} else {
				++count;
			}
		}
	}
}

static void game_start() {
	game_restrict_spectators();
	game_assign_player_names();
	if (config.prepare) {
		config.prepare();
	}
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
	game_write_header();
}

static void game_shutdown() {
	game_remove_players();
	game_remove_spectators();
	map_free(&game.map);
}

static void game_reset(const int fd_listen, const int fd_listen_websocket,
		const int fd_listen_spectator) {
	memset(&game, 0, sizeof(Game));
	game_watch_fd(fd_listen);
	game_watch_fd(fd_listen_websocket);
	if (fd_listen_spectator > -1) {
		game_watch_fd(fd_listen_spectator);
	}
	if (config.output_format == FORMAT_PLAIN) {
		printf("waiting for players (at least %d) to join ...\n",
			config.min_starters);
	}
}

static int game_run(const int fd_listen, const int fd_listen_websocket,
		const int fd_listen_spectator) {
	struct timeval tv;

	game_reset(fd_listen, fd_listen_websocket, fd_listen_spectator);

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
			if (FD_ISSET(fd_listen, &game.ready)) {
				game_join(fd_listen,
					game_add_player_socket,
					ROLE_PLAYER);
			}
			if (FD_ISSET(fd_listen_websocket, &game.ready)) {
				game_join(fd_listen_websocket,
					game_add_player_websocket,
					ROLE_PLAYER);
			}
			if (fd_listen_spectator > -1 &&
					FD_ISSET(fd_listen_spectator, &game.ready)) {
				game_join(fd_listen_spectator,
					game_add_spectator,
					ROLE_SPECTATOR);
			}
			game_read_commands();
			game_read_spectators();
		}

		if (game.started) {
			game_remove_defunkt_players();
			if (game.stopped || game_joined() < config.min_players) {
				if (!game.stopped) {
					game_terminate();
				}
				if (config.end) {
					config.end();
				}
				game_write_results(stdout);
				if (config.max_games > 0 && --config.max_games < 1) {
					break;
				} else {
					game_shutdown();
					game_reset(fd_listen, fd_listen_websocket,
						fd_listen_spectator);
				}
			}
		} else if (game.nplayers == MAX_PLAYERS ||
				(ready == 0 && game.nplayers >= config.min_starters)) {
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
#ifndef MSG_NOSIGNAL
	case SIGPIPE:
		// ignore; sent when a socket is shut down for
		// writing what shouldn't terminate the server
		break;
#endif
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

int game_listen(const int port) {
	int fd = socket(PF_INET, SOCK_STREAM, 6);
	if (fd < 1) {
		perror("socket");
		return -1;
	}

	// avoid blocking the port when a player hasn't closed it's side
	// of the socket and the server has just been terminated
	int enable = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable,
			sizeof(int)) < 0) {
		perror("setsockopt");
		close(fd);
		return -1;
	}

	if (game_bind_port(fd, port) != 0) {
		perror("bind");
		close(fd);
		return -1;
	}

	if (listen(fd, 4)) {
		perror("listen");
		close(fd);
		return -1;
	}

	return fd;
}

int game_serve() {
	int fd_listen = game_listen(config.port);
	if (fd_listen < 0) {
		return -1;
	}
	int fd_listen_websocket = game_listen(config.port_websocket);
	if (fd_listen_websocket < 0) {
		close(fd_listen);
		return -1;
	}
	int fd_listen_spectator = -1;
	if (config.max_spectators > 0) {
		fd_listen_spectator = game_listen(config.port_spectator);
		if (fd_listen_spectator < 0) {
			close(fd_listen);
			close(fd_listen_websocket);
			return -1;
		}
	}

	signal(SIGHUP, game_handle_signal);
	signal(SIGINT, game_handle_signal);
	signal(SIGTERM, game_handle_signal);
#ifndef MSG_NOSIGNAL
	signal(SIGPIPE, game_handle_signal);
#endif

	int rv = game_run(fd_listen, fd_listen_websocket, fd_listen_spectator);

	close(fd_listen);
	close(fd_listen_websocket);
	if (fd_listen_spectator > -1) {
		close(fd_listen_spectator);
	}

	return rv;
}
