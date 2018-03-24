#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "game.h"
#include "setups/find_exit_plain.h"
#include "setups/find_exit_obstacles.h"
#include "setups/last_man_standing.h"
#include "setups/asteroid_shower.h"

#define PORT "--port"
#define MAP_SIZE "--map-size"
#define VIEW_RADIUS "--view-radius"
#define MAX_TURNS "--max-turns"
#define SHRINK_AFTER "--shrink-after"
#define USECS_PER_TURN "--usecs-per-turn"

static struct Setup {
	char letter;
	char *description;
	void *init;
} setups[] = {
	{ '1', "find exit on a plain grid", find_exit_plain },
	{ '2', "find exit with obstacles on the grid", find_exit_obstacles },
	{ '3', "last man standing", last_man_standing },
	{ '4', "avoid getting hit by floating astroids", asteroid_shower },
	{ 0, NULL, NULL }
};

static void help(char *bin) {
	printf("usage: %s NUMBER [FLAGS...]\n", basename(bin));
	printf("\nNUMBER must be one of:\n");
	struct Setup *s;
	for (s = setups; s->letter; ++s) {
		printf("%c - %s\n", s->letter, s->description);
	}
	printf("\nFLAGS can be any of:\n");
	printf(PORT" N\n");
	printf(MAP_SIZE" N[xN]\n");
	printf(VIEW_RADIUS" N\n");
	printf(MAX_TURNS" N\n");
	printf(SHRINK_AFTER" N\n");
	printf(USECS_PER_TURN" N\n");
}

static void *pick_setup(const char letter) {
	struct Setup *s;
	for (s = setups; s->letter; ++s) {
		if (s->letter == letter) {
			return s->init;
		}
	}
	return NULL;
}

int main(int argc, char **argv) {
	#define HAS_ARG(flag) if (--argc < 1) {\
		fprintf(stderr, "error: missing argument for %s\n", flag);\
		return 1;\
	}
	struct Config cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.port = 63187;
	char *bin = *argv;
	while (--argc) {
		++argv;
		if (!strcmp(*argv, PORT)) {
			HAS_ARG(PORT)
			cfg.port = atoi(*++argv);
		} else if (!strcmp(*argv, MAP_SIZE)) {
			HAS_ARG(MAP_SIZE)
			int width = atoi(strtok(*++argv, "x"));
			char *height = strtok(NULL, "x");
			cfg.map_width = width;
			cfg.map_height = height != NULL ? atoi(height) : width;
		} else if (!strcmp(*argv, VIEW_RADIUS)) {
			HAS_ARG(VIEW_RADIUS)
			cfg.view_radius = atoi(*++argv);
		} else if (!strcmp(*argv, MAX_TURNS)) {
			HAS_ARG(MAX_TURNS)
			cfg.max_turns = atoi(*++argv);
		} else if (!strcmp(*argv, SHRINK_AFTER)) {
			HAS_ARG(SHRINK_AFTER)
			cfg.shrink_after = atoi(*++argv);
		} else if (!strcmp(*argv, USECS_PER_TURN)) {
			HAS_ARG(USECS_PER_TURN)
			cfg.usec_per_turn = atoi(*++argv);
		} else if (!(cfg.init = pick_setup(**argv))) {
			fprintf(stderr, "error: invalid argument \"%s\"\n", *argv);
			help(bin);
			return 1;
		}
	}
	if (!cfg.init) {
		help(bin);
		return 0;
	}
	return game_serve(&cfg);
}
