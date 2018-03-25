#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "find_exit.h"

static void start(struct Game *game) {
	placing_random(game);
}

void training(struct Game *game) {
	game->start = start;
	game->move = player_move;
	game->impassable = map_impassable;
}
