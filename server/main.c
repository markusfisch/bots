#include <stdio.h>
#include <string.h>

#include "game.h"
#include "games/find_exit_plain.h"
#include "games/find_exit_obstacles.h"
#include "games/last_man_standing.h"

static struct Setup {
	char letter;
	char *description;
	void *init;
} setups[] = {
	{ '1', "find exit on a plain grid", find_exit_plain },
	{ '2', "find exit with obstacles on the grid", find_exit_obstacles },
	{ '3', "last man standing", last_man_standing },
	{ 0, NULL, NULL }
};

static void *pick_setup(const char letter) {
	struct Setup *s;
	for (s = setups; s->letter; ++s) {
		if (s->letter == letter) {
			return s->init;
		}
	}
	return NULL;
}

static void help(const char *arg) {
	printf("invalid argument \"%s\", available arguments are:\n", arg);
	struct Setup *s;
	for (s = setups; s->letter; ++s) {
		printf("%c - %s\n", s->letter, s->description);
	}
}

int main(int argc, char **argv) {
	void *game = find_exit_plain;
	while (--argc) {
		if (!(game = pick_setup(**++argv))) {
			help(*argv);
			return 0;
		}
	}
	return game_serve(game);
}
