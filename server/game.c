#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "player.h"
#include "game.h"

void game_start(struct Game *game) {
	game->started = 1;
	gettimeofday(&game->tick, NULL);
	game->start(game);
}

void game_remove_all(struct Game *game) {
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			close(p->fd);
			player_remove(game, p->fd);
		}
	}
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
			player_view_write(game, p);
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

time_t game_next_turn(struct Game *game) {
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

void game_offset_circle(struct Game *game) {
	double between = 6.2831 / game->nplayers;
	double angle = between * rand();
	int cx = game->map.width;
	int cy = game->map.height;
	int radius = (cx < cy ? cx : cy) / 2;
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0) {
			p->bearing = rand() % 4;
			p->x = round(cx + cos(angle) * radius);
			p->y = round(cy + sin(angle) * radius);
			map_set(&game->map, p->x, p->y, TILE_FLATLAND);
			angle += between;
		}
	}
}
