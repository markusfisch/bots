#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "find_exit.h"

#define TILE_EXIT 'O'

extern struct Game game;
static int points;

static void start() {
	map_set(&game.map, game.map.width / 2, game.map.height / 2, TILE_EXIT);
	placing_circle();
}

static void move(Player *p, char cmd) {
	player_move(p, cmd);
	if (map_get(&game.map, p->x, p->y) == TILE_EXIT) {
		printf("%c escaped\n", p->name);
		p->score = points--;
		game_remove_player(p);
	}
}

void find_exit() {
	points = 16;
	game.start = start;
	game.move = move;
	game.impassable = map_impassable;
}
