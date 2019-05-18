#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "avoid.h"

#define ASTEROID TILE_GONE

extern struct Config config;
extern struct Game game;

static struct Asteroid {
	int x;
	int y;
	int vx;
	int vy;
	int change;
} *asteroids = NULL;
static size_t nasteroids;
static char *backup_map_data = NULL;
static int score;

static void asteroid_new_direction(struct Asteroid *p) {
	p->change = 10;
	do {
		p->vx = (config.rand() % 3) - 1;
		p->vy = (config.rand() % 3) - 1;
	} while (!p->vx || !p->vy);
}

static void asteroids_move() {
	struct Asteroid *p = asteroids, *e = p + nasteroids;
	for (; p < e; ++p) {
		map_restore_at(&game.map, backup_map_data, p->x, p->y);
	}
	for (p = asteroids; p < e; ++p) {
		p->x = map_wrap(p->x + p->vx, game.map.width);
		p->y = map_wrap(p->y + p->vy, game.map.height);
		map_set(&game.map, p->x, p->y, ASTEROID);
		Player *hit = NULL;
		while ((hit = player_at(p->x, p->y, hit))) {
			hit->score = score++;
			game_remove_player(hit);
		}
		if (--p->change < 1) {
			asteroid_new_direction(p);
		}
	}
}

static void asteroids_place() {
	struct Asteroid *p = asteroids, *e = p + nasteroids;
	for (; p < e; ++p) {
		do {
			p->x = config.rand() % game.map.width;
			p->y = config.rand() % game.map.height;
		} while (map_get(&game.map, p->x, p->y) == ASTEROID ||
			player_near(p->x, p->y, 5, NULL, NULL));
		asteroid_new_direction(p);
		map_set(&game.map, p->x, p->y, ASTEROID);
	}
}

static size_t asteroids_create(int amount) {
	free(asteroids);
	if (amount < 2) {
		amount = 2;
	}
	asteroids = calloc(amount, sizeof(struct Asteroid));
	return amount;
}

static void start() {
	backup_map_data = calloc(game.map.size, sizeof(char));
	memcpy(backup_map_data, game.map.data, game.map.size);
	nasteroids = asteroids_create(round(game.map.size * .1));
	asteroids_place();
	score = MAX_PLAYERS - (game.nplayers - 1);
}

static void end() {
	game_set_players_score(score);
	free(backup_map_data);
	backup_map_data = NULL;
	free(asteroids);
	asteroids = NULL;
	nasteroids = 0;
}

void avoid() {
	config.placing = config.placing ?: PLACING_RANDOM;
	config.view_radius = config.view_radius ?: 4;
	config.diagonal_interval = config.diagonal_interval ?: 1;

	config.start = start;
	config.turn_start = asteroids_move;
	config.end = end;
}
