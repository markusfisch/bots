#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "last_man_standing.h"

extern struct Game game;

static void start() {
	placing_circle();
}

static void shoot(Player *p) {
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
	int range = game.view_radius;
	int x = p->x;
	int y = p->y;
	while (range-- > 0) {
		x += vx;
		y += vy;
		Player *enemy = player_at(x, y);
		if (enemy && --enemy->life < 1) {
			++p->score;
			game_remove_player(enemy);
		}
	}
}

static void move(Player *p, const char cmd) {
	switch (cmd) {
	case 'f':
		shoot(p);
		break;
	default:
		player_move(p, cmd);
		break;
	}
}

void last_man_standing() {
	game.min_players = 2;
	game.view_radius = 4;
	game.max_turns = game.map.size;
	game.shrink_after = game.map.width + game.map.height;

	game.start = start;
	game.move = move;
	game.marker = game_marker_show_life;
	game.impassable = map_impassable;
}
