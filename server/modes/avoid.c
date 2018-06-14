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
	char tile;
} *asteroids = NULL;
size_t nasteroids;
int score;

static void asteroids_move() {
	struct Asteroid *p = asteroids, *e = p + nasteroids;
	for (; p < e; ++p) {
		map_set(&game.map, p->x, p->y, p->tile);
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
	}
}

static int player_near(int x, int y) {
	int v;
	int u;
	for (v = -1; v < 2; ++v) {
		for (u = -1; u < 2; ++u) {
			if (player_at(map_wrap(x + v, game.map.width),
					map_wrap(y + u, game.map.height), NULL)) {
				return 1;
			}
		}
	}
	return 0;
}

static void asteroids_place() {
	struct Asteroid *p = asteroids, *e = p + nasteroids;
	for (; p < e; ++p) {
		do {
			p->x = rand() % game.map.width;
			p->y = rand() % game.map.height;
		} while (map_get(&game.map, p->x, p->y) == ASTEROID ||
			player_near(p->x, p->y));
		do {
			p->vx = (rand() % 3) - 1;
			p->vy = (rand() % 3) - 1;
		} while (!p->vx && !p->vy);
		p->tile = map_get(&game.map, p->x, p->y);
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
	nasteroids = asteroids_create(round(game.map.size * .1));
	asteroids_place();
	score = MAX_PLAYERS - (game.nplayers - 1);
}

static void end() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd) {
			p->score = score;
		}
	}
	free(asteroids);
	nasteroids = 0;
}

void avoid() {
	config.placing = config.placing ?: PLACING_RANDOM;
	config.view_radius = config.view_radius ?: 4;

	config.start = start;
	config.turn_start = asteroids_move;
	config.end = end;
}
