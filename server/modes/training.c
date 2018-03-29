#include <stdio.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "../placing.h"
#include "find_exit.h"

extern struct Config config;
extern struct Game game;

static void start() {
	placing_random();
}

void training() {
	config.start = start;
}
