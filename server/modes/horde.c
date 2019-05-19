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
	int life; // it's an extra life if negative
	int change;
} *enemies = NULL;
static size_t nenemies;
static char *backup_map_data = NULL;
static int score;

static int attacking(Player *player) {
	if (map_get(&game.map, player->attack_x, player->attack_y) != ENEMY) {
		return 0;
	}
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (p->life > 0 &&
				p->x == player->attack_x &&
				p->y == player->attack_y &&
				--p->life < 1) {
			++player->score;
			p->life = -10;
			return 1;
		}
	}
	return 0;
}

static void lives_pick_up(Player *player) {
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (p->life < 0 && p->x == player->x && p->y == player->y) {
			++player->life;
			p->life = 0;
		}
	}
	map_restore_at(&game.map, backup_map_data, player->x, player->y);
}

static void move(Player *player, const char cmd) {
	player_move(player, cmd);
	if (map_get(&game.map, player->x, player->y) == LIFE) {
		lives_pick_up(player);
	}
}

static void move_blocked_at(Player *player, const int x, const int y) {
	if (map_get(&game.map, x, y) == ENEMY) {
		player->attack_x = x;
		player->attack_y = y;
		attacking(player);
	}
}

static int impassable(Map *map, const int x, const int y) {
	return map_impassable(map, x, y) || map_get(map, x, y) == ENEMY;
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

static struct Direction {
	int x;
	int y;
} diagonal_directions[] = {
	{-1, -1},
	{0, -1},
	{1, -1},
	{-1, 0},
	{1, 0},
	{-1, 1},
	{0, 1},
	{1, 1}
}, orthogonal_directions[] = {
	{-1, 0},
	{1, 0},
	{0, -1},
	{0, 1}
};
static void enemy_new_direction(struct Enemy *p) {
	p->change = 4 + (config.rand() % 16);
	struct Direction *directions;
	int ndirections;
	if (config.diagonal_interval) {
		directions = diagonal_directions;
		ndirections = 8;
	} else {
		directions = orthogonal_directions;
		ndirections = 4;
	}
	int offset = config.rand();
	int i;
	for (i = 0; i < ndirections; ++i, ++offset) {
		struct Direction *d = directions + (offset % ndirections);
		if (!config.impassable(&game.map, p->x + d->x, p->y + d->y)) {
			p->vx = d->x;
			p->vy = d->y;
			return;
		}
	}
	p->vx = p->vy = 0;
}

static int clamp(int min, int max, int value) {
	if (value > max) {
		return max;
	} else if (value < min) {
		return min;
	} else {
		return value;
	}
}

static int clamp_uniform(int value) {
	return clamp(-1, 1, value);
}

static void enemy_turn_to(struct Enemy *e, const int x, const int y) {
	int dx = x - e->x;
	int dy = y - e->y;
	int adx = abs(dx);
	int ady = abs(dy);
	if (config.diagonal_interval) {
		e->vx = clamp_uniform(dx);
		e->vy = clamp_uniform(dy);
	} else if (adx > ady) {
		e->vx = clamp_uniform(dx);
		e->vy = 0;
	} else {
		e->vx = 0;
		e->vy = clamp_uniform(dy);
	}
	if (config.impassable(&game.map, e->x + e->vx, e->y + e->vy)) {
		enemy_new_direction(e);
		return;
	}
	e->change = adx + ady;
}

static void enemy_spawn_at(const int x, const int y) {
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (!p->life) {
			p->life = 1;
			p->x = x;
			p->y = y;
			p->vx = 0;
			p->vy = 0;
			p->change = 0;
			break;
		}
	}
}

static int enemy_spawn() {
	size_t offset = config.rand();
	size_t i;
	for (i = 0; i < nportals; ++i, ++offset) {
		struct Portal *p = portals + (offset % nportals);
		if (player_at(p->x, p->y, NULL) ||
				map_get(&game.map, p->x, p->y) == ENEMY) {
			continue;
		}
		enemy_spawn_at(p->x, p->y);
		return 1;
	}
	return 0;
}

static int hit_players(const int x, const int y) {
	Player *hit = NULL;
	while ((hit = player_at(x, y, hit))) {
		if (--hit->life < 1) {
			hit->score += score++;
			hit->killed_by = ENEMY;
			game_remove_player(hit);
		}
		return 1;
	}
	return 0;
}

static void life_degrade(struct Enemy *e) {
	if (e->life > -1) {
		return;
	}
	if (map_get(&game.map, e->x, e->y) != ENEMY) {
		map_set(&game.map, e->x, e->y, LIFE);
	}
	++e->life;
}

static struct Enemy *enemy_at(const int x, const int y) {
	struct Enemy *p = enemies, *e = p + nenemies;
	for (; p < e; ++p) {
		if (p->life > 0 && p->x == x && p->y == y) {
			return p;
		}
	}
	return NULL;
}

static void enemies_move() {
	struct Enemy *p = enemies, *e = p + nenemies;
	if (game.turn % config.spawn_frequency == 0) {
		enemy_spawn();
	}
	for (; p < e; ++p) {
		if (p->life) {
			map_restore_at(&game.map, backup_map_data, p->x, p->y);
		}
	}
	for (p = enemies; p < e; ++p) {
		if (p->life < 1) {
			life_degrade(p);
			continue;
		}
		Player *closest = find_closest_player(p->x, p->y);
		if (closest) {
			enemy_turn_to(p, closest->x, closest->y);
		}
		int x = map_wrap(p->x + p->vx, game.map.width);
		int y = map_wrap(p->y + p->vy, game.map.height);
		if (!hit_players(x, y)) {
			if (config.impassable(&game.map, x, y) || enemy_at(x, y)) {
				p->change = 0;
			} else {
				p->x = x;
				p->y = y;
			}
		}
		map_set(&game.map, p->x, p->y, ENEMY);
		if (--p->change < 1) {
			enemy_new_direction(p);
		}
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
	nenemies = enemies_create(round(game.map.size * .5));
	if ((nportals = map_count(&game.map, TILE_PORTAL)) > 0) {
		nportals = portals_create(nportals);
		portals_find();
	} else {
		nportals = portals_create(game.nplayers + 1);
		portals_place();
	}
	backup_map_data = calloc(game.map.size, sizeof(char));
	memcpy(backup_map_data, game.map.data, game.map.size);
	score = MAX_PLAYERS - (game.nplayers - 1);
}

static void add_scores_for_survival(int score) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->life > 0) {
			p->score += score;
		}
	}
}

static void end() {
	add_scores_for_survival(score);
	free(enemies);
	enemies = NULL;
	nenemies = 0;
	free(portals);
	portals = NULL;
	nportals = 0;
	free(backup_map_data);
	backup_map_data = NULL;
}

void horde() {
	config.placing = config.placing ?: PLACING_CIRCLE;
	config.view_radius = config.view_radius ?: 8;
	config.diagonal_interval = config.diagonal_interval ?: 1;

	config.start = start;
	config.turn_start = enemies_move;
	config.impassable = impassable;
	config.move_blocked_at = move_blocked_at;
	config.move = move;
	config.attacking = attacking;
	config.end = end;
}
