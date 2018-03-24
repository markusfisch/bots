#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "asteroid_shower.h"

static struct Asteroid {
	int x;
	int y;
} *asteroids = NULL;
size_t nasteroids;
int vx;
int vy;
int score;

static void asteroids_move(struct Game *game) {
	struct Player *hit;
	struct Asteroid *p = asteroids, *e = asteroids + nasteroids;
	for (; p < e; ++p) {
		map_set(&game->map, p->x, p->y, TILE_FLATLAND);
		p->x = map_wrap(p->x + vx, game->map.width);
		p->y = map_wrap(p->y + vy, game->map.height);
		map_set(&game->map, p->x, p->y, TILE_WOOD);
		if ((hit = player_at(game, p->x, p->y))) {
			hit->score = score++;
			game_remove_player(game, hit);
		}
	}
}

static void asteroids_place(struct Game *game) {
	struct Asteroid *p = asteroids, *e = asteroids + nasteroids;
	for (; p < e; ++p) {
		do {
			p->x = rand() % game->map.width;
			p->y = rand() % game->map.height;
		} while (player_at(game, p->x, p->y));
	}
}

static size_t asteroids_create(int amount) {
	if (asteroids) {
		free(asteroids);
	}
	if (amount < 2) {
		amount = 2;
	}
	asteroids = calloc(amount, sizeof(struct Asteroid));
	return amount;
}

static void start(struct Game *game) {
	placing_random(game);
	nasteroids = asteroids_create(round(game->map.size * .1));
	asteroids_place(game);
	do {
		vx = (rand() % 3) - 1;
		vy = (rand() % 3) - 1;
	} while (!vx && !vy);
	score = 0;
}

static void turn_start(struct Game *game) {
	asteroids_move(game);
}

void asteroid_shower(struct Game *game) {
	game->view_radius = 4;

	game->start = start;
	game->turn_start = turn_start;
	game->move = player_move;
	game->impassable = map_impassable;
}
