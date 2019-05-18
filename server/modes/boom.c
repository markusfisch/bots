#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "collect.h"
#include "boom.h"

#define TILE_FIRE '*'
#define TILE_HIDDEN_POWER_UP 'Y'
#define TILE_POWER_UP '+'
#define MAX_BOMBS MAX_PLAYERS * 4

extern struct Config config;
extern struct Game game;

static struct Bomb {
	int x;
	int y;
	int timer;
	unsigned int power;
} *bombs = NULL;
static int score;

static int burn(int, int);

static void set_effect(struct Bomb *b, int (*apply)(int, int)) {
	int x = b->x;
	int y = b->y;
	apply(x, y);
	int left = 1;
	int right = 1;
	int top = 1;
	int bottom = 1;
	unsigned int max = game.map.width > game.map.height ?
			game.map.width : game.map.height;
	unsigned int r = b->power < max ? b->power : max;
	unsigned int i;
	for (i = 1; i <= r; ++i) {
		if (left) {
			left = apply(x - i, y);
		}
		if (right) {
			right = apply(x + i, y);
		}
		if (top) {
			top = apply(x, y - i);
		}
		if (bottom) {
			bottom = apply(x, y + i);
		}
	}
}

static int restore(int x, int y) {
	if (map_get(&game.map, x, y) == TILE_FIRE) {
		map_set(&game.map, x, y, *config.flatland);
	}
	return 1;
}

static void ignite(int x, int y) {
	struct Bomb *b = bombs, *e = b + MAX_BOMBS;
	for (; b < e; ++b) {
		if (b->timer > 0 && b->x == x && b->y == y) {
			b->timer = 0;
			set_effect(b, burn);
		}
	}
}

static int burn(int x, int y) {
	if (map_get(&game.map, x, y) == TILE_HIDDEN_POWER_UP) {
		map_set(&game.map, x, y, TILE_POWER_UP);
		return 0;
	} else if (config.impassable(&game.map, x, y)) {
		return 0;
	}
	Player *hit = NULL;
	while ((hit = player_at(x, y, hit))) {
		hit->score = score++;
		game_remove_player(hit);
	}
	map_set(&game.map, x, y, TILE_FIRE);
	ignite(x, y);
	return 1;
}

static void update_bomb(struct Bomb *b) {
	map_set(&game.map, b->x, b->y, 48 + b->timer);
}

static void update_bombs() {
	struct Bomb *b = bombs, *e = b + MAX_BOMBS;
	for (; b < e; ++b) {
		if (b->timer < 1) {
			continue;
		} else if (--b->timer < 1) {
			set_effect(b, burn);
		} else {
			update_bomb(b);
		}
	}
}

static void remove_fires() {
	struct Bomb *b = bombs, *e = b + MAX_BOMBS;
	for (; b < e; ++b) {
		if (b->timer == 0) {
			b->timer = -1;
			set_effect(b, restore);
		}
	}
}

static void turn_start() {
	remove_fires();
	update_bombs();
}

static int plant_bomb(int x, int y, unsigned int timer, unsigned int power) {
	struct Bomb *b = bombs, *e = b + MAX_BOMBS;
	for (; b < e; ++b) {
		if (b->timer < 0) {
			b->x = map_wrap(x, game.map.width);
			b->y = map_wrap(y, game.map.height);
			b->timer = timer > 0 ? timer : 1;
			b->power = power > 0 ? power : 1;
			update_bomb(b);
			return 1;
		}
	}
	return 0;
}

static void move(Player *player, char cmd) {
	switch (cmd) {
	case '9':
	case '8':
	case '7':
	case '6':
	case '5':
	case '4':
	case '3':
	case '2':
	case '1':
		plant_bomb(player->x, player->y, cmd - 48, player->counter + 1);
		break;
	default:
		player_move(player, cmd);
		break;
	}
	if (map_get(&game.map, player->x, player->y) == TILE_POWER_UP) {
		++player->counter;
		map_set(&game.map, player->x, player->y, *config.flatland);
	}
}

static void free_bombs() {
	free(bombs);
	bombs = NULL;
}

static void end() {
	free_bombs();
	players_set_remaining_scores(score);
}

static void init_bombs() {
	struct Bomb *b = bombs, *e = b + MAX_BOMBS;
	for (; b < e; ++b) {
		b->timer = -1;
	}
}

static void start() {
	score = MAX_PLAYERS - (game.nplayers - 1);

	config.gems = scatter(TILE_HIDDEN_POWER_UP, config.gems);

	free_bombs();
	bombs = calloc(MAX_BOMBS, sizeof(struct Bomb));
	if (bombs == NULL) {
		perror("calloc");
		exit(-1);
	}
	init_bombs();
}

void boom() {
	config.min_players = config.min_players ?: 2;
	config.view_radius = config.view_radius ?: 4;
	static const char obstacles[] = { TILE_HIDDEN_POWER_UP };
	config.obstacles = config.obstacles ?: obstacles;

	config.start = start;
	config.move = move;
	config.turn_start = turn_start;
	config.end = end;
}
