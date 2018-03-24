#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "last_man_standing.h"

static void start(struct Game *game) {
	size_t ntiles = 1;
	char tiles[] = { TILE_FLATLAND };
	map_init_random(&game->map, tiles, ntiles);
	placing_random(game);
	game_set_players_life(game, 1);
}

static void shoot(struct Game *game, struct Player *p) {
	int vx = 0;
	int vy = 0;
	switch (p->bearing % 4) {
	case 0:
		vy = -1;
		break;
	case 1:
		vx = 1;
		break;
	case 2:
		vy = 1;
		break;
	case 3:
		vx = -1;
		break;
	}
	int range = game->view_radius;
	int x = p->x;
	int y = p->y;
	while (range-- > 0) {
		x += vx;
		y += vy;
		struct Player *enemy = player_at(game, x, y);
		if (enemy != NULL && --enemy->life < 1) {
			++p->score;
			game_remove_player(game, enemy);
		}
	}
}

static void move(struct Game *game, struct Player *p, char cmd) {
	switch (cmd) {
	case 'f':
		shoot(game, p);
		break;
	default:
		player_move(game, p, cmd);
		break;
	}
}

void last_man_standing(struct Game *game) {
	game->min_players = 2;
	game->view_radius = 4;
	game->max_turns = 512;
	game->shrink_after = game->max_turns / 2;

	game->start = start;
	game->move = move;
	game->impassable = map_impassable;
}
