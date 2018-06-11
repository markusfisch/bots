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

char game_marker_show_life(struct Player *p) {
	return p->life < config.player_life ? (char) (48 + p->life) : p->name;
}

static int game_compare_player_score(const void *a, const void *b) {
	return ((Player *) b)->score - ((Player *) a)->score;
}

static void game_print_results(FILE *fp) {
	qsort(game.players, game.nplayers, sizeof(Player),
		game_compare_player_score);
	char *format;
	switch (config.output_format) {
	default:
	case FORMAT_PLAIN:
		fprintf(fp, "========== RESULTS ==========\n");
		fprintf(fp, "Place Name Score Moves Killer\n");
		format = "% 4d. %c    % 5d % 5d %c\n";
		break;
	case FORMAT_JSON:
		// close turns array and open results
		fprintf(fp, "],\n\"results\":[\n");
		format = "{\"place\":%d,\"name\":\"%c\",\"score\":%d,"\
			"\"moves\":%d,\"killer\":\"%c\"}";
		break;
	}
	int place = 1;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p, ++place) {
		if (config.output_format == FORMAT_JSON && p > game.players) {
			fprintf(fp, ",\n");
		}
		fprintf(fp, format, place, p->name, p->score, p->moves,
			p->killed_by ?: '-');
	}
	switch (config.output_format) {
	case FORMAT_JSON:
		// close results
		fprintf(fp, "\n]\n");
		// close root object
		fprintf(fp, "}\n");
		break;
	}
	fflush(fp);
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

static void game_write_json(FILE *fp, char *buf) {
	if (game.turn > 1) {
		fprintf(fp, ",\n");
	}
	fprintf(fp, "{\"turn\":%d,\"players\":[\n", game.turn);
	Player *p = game.players, *e = p + game.nplayers;
	for (p = game.players; p < e; ++p) {
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
		fwrite(buf, sizeof(char), game.map.width, fp);
		buf += game.map.width;
	}
	fprintf(fp, "\"}");
	fflush(fp);
}

static void game_write_plain(FILE *fp, char *buf) {
	unsigned int y;
	for (y = 0; y < game.map.height; ++y) {
		fwrite(buf, sizeof(char), game.map.width, fp);
		fwrite("\n", sizeof(char),  1, fp);
		buf += game.map.width;
	}
	fprintf(fp, "Turn %d of %d. %d of %d players alive.\n", game.turn,
		config.max_turns, game_joined(), game.nplayers);
	fprintf(fp, "Address          Name    X       Y ° "\
		"Life Moves Score Attacks\n");
	Player *p = game.players, *e = p + game.nplayers;
	for (p = game.players; p < e; ++p) {
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
	fflush(fp);
}

static void game_write(FILE *fp) {
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
		game_write_plain(fp, buf);
		break;
	case FORMAT_JSON:
		game_write_json(fp, buf);
		break;
	}
}

static void game_send_spectators(void (*writer)(FILE *)) {
	Spectator *s = game.spectators, *e = s + game.nspectators;
	for (; s < e; ++s) {
		if (s->fp != NULL) {
			writer(s->fp);
		}
	}
	writer(stdout);
}

static void game_remove_defunkt_players() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->moves + config.max_lag < game.turn) {
			game_remove_player(p);
		}
	}
}

static void game_send_players() {
	int update = 0;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && !p->can_move) {
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
		game_send_spectators(game_write);
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

static void game_remove_spectator(Spectator *s) {
	if (s->fd > 0) {
		FD_CLR(s->fd, &game.watch);
		fclose(s->fp);
		s->fd = 0;
		s->fp = NULL;
		s->match = NULL;
	}
}

static void game_remove_spectators() {
	Spectator *s = game.spectators, *e = s + game.nspectators;
	for (; s < e; ++s) {
		if (s->fd > 0) {
			game_remove_spectator(s);
		}
	}
}

static void game_read_spectator_key(Spectator *s) {
	char key[0xff];
	int b;
	if ((b = recv(s->fd, key, sizeof(key), 0)) < 1) {
		if (b) {
			perror("recv");
		}
		game_remove_spectator(s);
		return;
	}
	if (game.started || !config.spectator_key || !s->match) {
		return;
	}
	int already = s->match - config.spectator_key;
	size_t max = strlen(config.spectator_key) - already;
	if (max < 1) {
		return;
	}
	size_t rest = (size_t) b < max ? (size_t) b : max;
	if (strncmp(s->match + already, key, rest)) {
		game_remove_spectator(s);
		return;
	}
	s->match += rest;
}

static void game_read_spectators() {
	Spectator *s = game.spectators, *e = s + game.nspectators;
	for (; s < e; ++s) {
		if (s->fd > 0 && FD_ISSET(s->fd, &game.ready)) {
			game_read_spectator_key(s);
		}
	}
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
	// use a Fisher-Yates shuffle to process commands in random order
	Player *queue[MAX_PLAYERS];
	Player **q = queue, **e = queue + game.nplayers;
	Player *p = game.players;
	for (; q < e; ++q, ++p) {
		*q = p;
	}
	int i;
	for (i = game.nplayers; i-- > 1;) {
		int j = rand() % i;
		Player *tmp = queue[j];
		queue[j] = queue[i];
		queue[i] = tmp;
	}
	for (q = queue; q < e; ++q) {
		if ((*q)->fd > 0 && FD_ISSET((*q)->fd, &game.ready)) {
			game_read_command(*q);
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
	if (game.nspectators >= MAX_SPECTATORS) {
		return 0;
	}
	Spectator *s = &game.spectators[game.nspectators];
	strcpy(s->addr, addr);
	s->fd = fd;
	s->fp = fdopen(fd, "a");
	s->match = config.spectator_key;
	++game.nspectators;
	game_watch_fd(fd);
	return 1;
}

static int game_add_player(int fd, char *addr) {
	if (game.nplayers >= MAX_PLAYERS) {
		return 0;
	}
	Player *p = &game.players[game.nplayers];
	strcpy(p->addr, addr);
	p->fd = fd;
	p->life = 1;
	p->x = p->y = -1;
	p->attack_x = p->attack_y = -1;
	++game.nplayers;
	game_watch_fd(fd);
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
	if (add(fd, ip_str) && config.output_format == FORMAT_PLAIN) {
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

static int game_compare_player_address(const void *a, const void *b) {
	return strcmp(((Player *) b)->addr, ((Player *) a)->addr);
}

static void game_assign_player_names() {
	qsort(game.players, game.nplayers, sizeof(Player),
		game_compare_player_address);
	Player *p = game.players, *e = p + game.nplayers;
	char name = 'A';
	for (; p < e; ++p) {
		p->name = name++;
	}
}

static int game_spectator_authorized(const Spectator *s) {
	return s->match && !*s->match;
}

static void game_remove_unauthorized_spectators() {
	Spectator *s = game.spectators, *e = s + game.nspectators;
	for (; s < e; ++s) {
		if (s->fd > 0 && !game_spectator_authorized(s)) {
			game_remove_spectator(s);
		}
	}
}

static void game_start() {
	game_remove_unauthorized_spectators();
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
	if (config.output_format == FORMAT_JSON) {
		printf("{\n\"max_turns\":%d,\n"\
				"\"map_width\":%d,\n"\
				"\"map_height\":%d,\n"\
				"\"view_radius\":%d,\n"\
				"\"obstacles\":\"%s\",\n"\
				"\"flatland\":\"%s\",\n"\
				"\"turns\":[\n",
			config.max_turns,
			config.map_width,
			config.map_height,
			config.view_radius,
			config.obstacles,
			config.flatland);
	}
}

static void game_shutdown(const int fd_player, const int fd_spectator) {
	close(fd_player);
	close(fd_spectator);
	game_remove_players();
	game_remove_spectators();
	map_free(&game.map);
}

static void game_reset(const int fd_player, const int fd_spectator) {
	memset(&game, 0, sizeof(Game));
	game_watch_fd(fd_player);
	game_watch_fd(fd_spectator);
	if (config.output_format == FORMAT_PLAIN) {
		printf("waiting for players (at least %d) to join ...\n",
			config.min_starters);
	}
}

static int game_run(const int fd_player, const int fd_spectator) {
	struct timeval tv;

	game_reset(fd_player, fd_spectator);

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
			if (FD_ISSET(fd_player, &game.ready)) {
				game_join(fd_player, game_add_player, "player");
			}
			if (FD_ISSET(fd_spectator, &game.ready)) {
				game_join(fd_spectator, game_add_spectator, "spectator");
			}
			game_read_commands();
			game_read_spectators();
		}

		if (game.started) {
			game_remove_defunkt_players();
			if (game.stopped || game_joined() < config.min_players) {
				if (config.end) {
					config.end();
				}
				game_send_spectators(game_write);
				game_send_spectators(game_print_results);
				if (config.max_games > 0 && --config.max_games < 1) {
					break;
				} else {
					game_remove_players();
					map_free(&game.map);
					game_reset(fd_player, fd_spectator);
				}
			}
		} else if (game.nplayers == MAX_PLAYERS ||
				(ready == 0 && game.nplayers >= config.min_starters)) {
			game_start();
		}
	}

	game_shutdown(fd_player, fd_spectator);

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
	int fd_player = game_listen(config.port_player);
	if (fd_player < 0) {
		return -1;
	}
	int fd_spectator = game_listen(config.port_spectator);
	if (fd_player < 0) {
		return -1;
	}

	signal(SIGHUP, game_handle_signal);
	signal(SIGINT, game_handle_signal);
	signal(SIGTERM, game_handle_signal);

	return game_run(fd_player, fd_spectator);
}
