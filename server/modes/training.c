#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "find_exit.h"

extern struct Game game;

static void start() {
	placing_random();
}

void training() {
	game.start = start;
	game.move = player_move;
	game.impassable = map_impassable;
}
