#include <stdio.h>

#include "game.h"
#include "games/find_exit.h"

int main() {
	return game_serve(init_find_exit);
}
