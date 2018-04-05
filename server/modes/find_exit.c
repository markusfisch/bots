#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "find_exit.h"

#define TILE_EXIT 'O'

extern struct Config config;
extern struct Game game;

static int points;

static void start() {
	points = 16;
	map_set(&game.map, game.map.width / 2, game.map.height / 2, TILE_EXIT);
}

static void move(Player *p, char cmd) {
	player_move(p, cmd);
	if (map_get(&game.map, p->x, p->y) == TILE_EXIT) {
		p->score = points--;
		game_remove_player(p);
	}
}

void find_exit() {
	config.start = start;
	config.move = move;
}
