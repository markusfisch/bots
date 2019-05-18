#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "horde.h"

#define PORTAL_SET_RADIUS 5
#define TILE_PORTAL '&'
#define ENEMY 'e'
#define LIFE '+'

extern struct Config config;
extern struct Game game;

static struct Portal {
	int x;
	int y;
} *portals = NULL;
static size_t nportals;
static struct Enemy {
	int x;
	int y;
	int vx;
	int vy;
	int life;
	int change;
	char tile;
} *enemies = NULL;
static size_t nenemies;
static int score;

static int attacking(Player *player) {
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (p->life > 0 &&
				p->x == player->attack_x &&
				p->y == player->attack_y) {
			++player->score;
			p->life = 0;
			map_set(&game.map, p->x, p->y, LIFE);
			return 1;
		}
	}
	return 0;
}

static void move(Player *player, char cmd) {
	player_move(player, cmd);
	if (map_get(&game.map, player->x, player->y) == LIFE) {
		++player->life;
		map_set(&game.map, player->x, player->y, *config.flatland);
	}
}

static int impassable(Map *map, int x, int y) {
	return (!config.non_exclusive && map_get(map, x, y) == ENEMY) ||
		map_impassable(map, x, y);
}

static Player *find_closest_player(const int x, const int y) {
	double min = config.map_width * config.map_height;
	double dist;
	Player *closest = NULL;
	Player *p = NULL;
	while ((p = player_near(x, y, 5, p, &dist))) {
		if (dist < min) {
			min = dist;
			closest = p;
		}
	}
	return closest;
}

static int normalize(int v) {
	if (v > 1) {
		return 1;
	} else if (v < -1) {
		return -1;
	} else {
		return v;
	}
}

static void enemy_turn_to(struct Enemy *e, const int x, const int y) {
	int dx = x - e->x;
	int dy = y - e->y;
	int adx = abs(dx);
	int ady = abs(dy);
	if (config.diagonal_interval) {
		e->vx = normalize(dx);
		e->vy = normalize(dy);
	} else if (adx > ady) {
		e->vx = normalize(dx);
		e->vy = 0;
	} else {
		e->vx = 0;
		e->vy = normalize(dy);
	}
	e->change = adx + ady;
}

static void enemy_new_direction(struct Enemy *p) {
	p->change = 10;
	do {
		if (config.diagonal_interval) {
			p->vx = (config.rand() % 3) - 1;
			p->vy = (config.rand() % 3) - 1;
		} else {
			int v = (config.rand() % 3) - 1;
			if (config.rand() % 2) {
				p->vx = v;
				p->vy = 0;
			} else {
				p->vx = 0;
				p->vy = v;
			}
		}
	} while (!p->vx && !p->vy);
}

static void enemy_spawn_at(const int x, const int y) {
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (p->life < 1) {
			p->life = 1;
			p->x = x;
			p->y = y;
			enemy_new_direction(p);
			break;
		}
	}
}

static void enemy_spawn() {
	struct Portal *p = portals + (config.rand() % nportals);
	enemy_spawn_at(p->x, p->y);
}

static void enemies_move() {
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (p->life > 0) {
			map_set(&game.map, p->x, p->y, p->tile);
		}
	}
	if (game.turn % config.spawn_frequency == 0) {
		enemy_spawn();
	}
	for (p = enemies; p < e; ++p) {
		if (p->life < 1) {
			continue;
		}
		Player *closest = find_closest_player(p->x, p->y);
		if (closest) {
			enemy_turn_to(p, closest->x, closest->y);
		}
		int x = map_wrap(p->x + p->vx, game.map.width);
		int y = map_wrap(p->y + p->vy, game.map.height);
		int strike = 0;
		Player *hit = NULL;
		while ((hit = player_at(x, y, hit))) {
			if (--hit->life < 1) {
				hit->score = score++;
				hit->killed_by = ENEMY;
				game_remove_player(hit);
			}
			strike = 1;
		}
		if (!strike && (config.non_exclusive ||
				!config.impassable(&game.map, x, y))) {
			p->x = x;
			p->y = y;
		}
		if (--p->change < 1) {
			enemy_new_direction(p);
		}
		p->tile = map_get(&game.map, p->x, p->y);
		map_set(&game.map, p->x, p->y, ENEMY);
	}
}

static size_t enemies_create(int amount) {
	free(enemies);
	if (amount < 2) {
		amount = 2;
	}
	enemies = calloc(amount, sizeof(struct Enemy));
	return amount;
}

static int find_free_spot(int x, int y) {
	return !config.impassable(&game.map, x, y);
}

static void portals_place() {
	double between = 6.2831 / nportals;
	double angle = 0;
	int cx = game.map.width / 2;
	int cy = game.map.height / 2;
	int radius = round((cx < cy ? cx : cy) * .9);
	struct Portal *p = portals, *e = p + nportals;
	for (; p < e; ++p) {
		int x = round(cx + cos(angle) * radius);
		int y = round(cy + sin(angle) * radius);
		map_find(&game.map, &x, &y, PORTAL_SET_RADIUS, find_free_spot);
		map_set(&game.map, x, y, TILE_PORTAL);
		p->x = x;
		p->y = y;
		angle += between;
	}
}

static void portals_find() {
	struct Portal *p = portals, *e = p + nportals;
	size_t size = game.map.size;
	char *s;
	for (s = game.map.data; (s = memchr(s, TILE_PORTAL, size)) && p < e; ++p) {
		int offset = s - game.map.data;
		p->x = offset % game.map.width;
		p->y = offset / game.map.width;
	}
}

static size_t portals_create(int amount) {
	free(portals);
	if (amount < 1) {
		amount = 1;
	}
	portals = calloc(amount, sizeof(struct Enemy));
	return amount;
}

static void start() {
	score = MAX_PLAYERS - (game.nplayers - 1);
	nenemies = enemies_create(round(game.map.size * .5));
	if ((nportals = map_count(&game.map, TILE_PORTAL)) > 0) {
		nportals = portals_create(nportals);
		portals_find();
	} else {
		nportals = portals_create(game.nplayers + 1);
		portals_place();
	}
}

static void end() {
	game_set_players_score(score);
	free(enemies);
	enemies = NULL;
	nenemies = 0;
	free(portals);
	portals = NULL;
	nportals = 0;
}

void horde() {
	config.placing = config.placing ?: PLACING_CIRCLE;
	config.view_radius = config.view_radius ?: 8;
	config.can_shoot = config.can_shoot ?: 1;
	config.diagonal_interval = config.diagonal_interval ?: 1;

	config.start = start;
	config.turn_start = enemies_move;
	config.impassable = impassable;
	config.move = move;
	config.attacking = attacking;
	config.end = end;
}
