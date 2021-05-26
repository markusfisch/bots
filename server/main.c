#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "player.h"
#include "modes/avoid.h"
#include "modes/boom.h"
#include "modes/collect.h"
#include "modes/dig.h"
#include "modes/escape.h"
#include "modes/horde.h"
#include "modes/rumble.h"
#include "modes/snakes.h"
#include "modes/training.h"
#include "modes/word.h"

#define MAP_TYPE_ARG_PLAIN "plain"
#define MAP_TYPE_ARG_RANDOM "random"
#define MAP_TYPE_ARG_MAZE "maze"
#define MAP_TYPE_ARG_TERRAIN "terrain"
#define PLACING_ARG_CIRCLE "circle"
#define PLACING_ARG_RANDOM "random"
#define PLACING_ARG_GRID "grid"
#define PLACING_ARG_DIAGONAL "diagonal"
#define FORMAT_ARG_PLAIN "plain"
#define FORMAT_ARG_JSON "json"
#define PLACING_SEPARATOR ":"
#define MAX_LINE 4096

extern struct Config config;

static const char flatland[] = { TILE_FLATLAND, 0 };
static const char obstacles[] = { TILE_WATER, TILE_WOOD, 0 };
static const struct Mode {
	char *name;
	char *description;
	void (*init)();
} modes[] = {
	{ "training", "just learn to see and move", training },
	{ "escape", "find the exit field 'o'", escape },
	{ "collect", "collect as many gems '@' as possible", collect },
	{ "snakes", "eat gems '@' and grow a tail", snakes },
	{ "rumble", "last man standing, shoot with 'f'", rumble },
	{ "avoid", "survive an asteroid shower of 'X'", avoid },
	{ "word", "find a word somewhere on the map", word },
	{ "boom", "last man standing, place bomb with '1' to '9'", boom },
	{ "horde", "survive hordes of enemies", horde },
	{ "dig", "dig for gold '@', dig with 'd', scan with 's'", dig },
	{ NULL, NULL, NULL }
};

// Park-Miller RNG
// https://en.wikipedia.org/wiki/Lehmer_random_number_generator
static uint32_t lcg_state = 1;
static int repeatable_rand() {
	lcg_state = ((uint64_t) lcg_state * 48271u) % 0x7fffffff;
	// cast to int because it's a stand-in for rand()
	return (int) lcg_state;
}

static void free_resources() {
	free(config.custom_map);
}

static void complete_config() {
	SET_IF_NULL(config.port, 63187)
	SET_IF_NULL(config.port_websocket, 63188)
	SET_IF_NULL(config.port_spectator, 63189)
	SET_IF_NULL(config.min_players, 1)
	SET_IF_NULL(config.min_starters, config.min_players)
	SET_IF_NULL(config.map_width, 32)
	SET_IF_NULL(config.map_height, config.map_width)
	SET_IF_NULL(config.map_type, MAP_TYPE_PLAIN)
	SET_IF_NULL(config.obstacles, obstacles)
	SET_IF_NULL(config.flatland, flatland)
	SET_IF_NULL(config.multiplier, 14)
	SET_IF_NULL(config.placing, PLACING_CIRCLE)
	SET_IF_NULL(config.view_radius, 2)
	SET_IF_NULL(config.max_games, 1)
	SET_IF_NULL(config.max_turns, 1024)
	SET_IF_NULL(config.max_lag, config.max_turns)
	SET_IF_NULL(config.shrink_after, config.max_turns)
	SET_IF_NULL(config.shrink_step, 1)
	SET_IF_NULL(config.player_life, 1)
	SET_IF_NULL(config.gems, config.map_width)
	SET_IF_NULL(config.spawn_frequency, 2)
	SET_IF_NULL(config.wait_for_joins, 1)
	SET_IF_NULL(config.usec_per_turn, USEC_PER_SEC)
	SET_IF_NULL(config.move, player_move)
	SET_IF_NULL(config.impassable, map_impassable)
}

static void usage() {
	printf("usage: bots [OPTION...] MODE\n");
	printf("\nMODE must be one of:\n");
	const struct Mode *m;
	for (m = modes; m->name; ++m) {
		printf("  %s - %s\n", m->name, m->description);
	}
	printf("\nOPTION can be any of:\n"\
		"  -P, --port N                port to listen for players, "\
			"default is 63187\n"\
		"  -W, --websocket-port N      port for WebSocket players, "\
			"default is 63188\n"\
		"  -O, --spectator-port N      port for WebSocket spectators, "\
			"default is 63189\n"\
		"  -V, --max-spectators N      maximum number of spectators, "\
			"default is 0\n"\
		"  -r, --remote-spectators     allow remote spectators for "\
			"multiplayer games\n"\
		"  -b, --min-starters N        minimum number of players to start "\
			"a game,\n"\
		"                              default is 1\n"\
		"  -m, --min-players N         minimum number of alive players, "\
			"default depends\n"\
		"                              on mode\n"\
		"  -n, --name-file FILE        list of IP addresses with player "
			"names\n"\
		"  -s, --map-size N[xN]        map size, default is 32x32\n"\
		"  -t, --map-type TYPE         map type, either \""\
			MAP_TYPE_ARG_PLAIN"\", \""\
			MAP_TYPE_ARG_RANDOM"\", \""\
			MAP_TYPE_ARG_MAZE"\" or\n"\
		"                              \""MAP_TYPE_ARG_TERRAIN\
			"\", default is \""MAP_TYPE_ARG_PLAIN"\"\n"\
		"  -c, --custom-map FILE       custom map\n"\
		"  -o, --obstacles STRING      characters a player cannot enter\n"\
		"  -f, --flatland STRING       characters a player can enter\n"\
		"  -x, --multiplier N          multiplier of flatland string, "\
			"default is 14\n"\
		"  -p, --placing TYPE          player placing, either \""\
			PLACING_ARG_CIRCLE"\", \""\
			PLACING_ARG_RANDOM"\",\n"\
		"                              \""PLACING_ARG_GRID"\" or "\
			"\""PLACING_ARG_DIAGONAL"\", default depends on mode\n"\
		"  -Z, --fuzzy N               maximum potential deviaton "\
			"from calculated\n"\
		"                              position, default is 0\n"\
		"  -A, --place-at X,Y[,D]"PLACING_SEPARATOR"...  manually "\
			"place players at given coordinates and\n"\
		"                              in given direction, either "\
			"'^', '>', 'v' or '<'\n"
		"  -N, --non-exclusive         multiple players can occupy "\
			"the same cell\n"\
		"  -Y, --translate-walls       translate '-' and '|' according "\
			"to orientation\n"\
		"  -v, --view-radius N         how many fields a player can "\
			"see in every\n"\
		"                              direction, default is 2\n"\
		"  -G, --max-games N           maximum number of games, "\
			"default is 1,\n"\
		"                              use -1 for unlimited games\n"\
		"  -M, --max-turns N           maximum number of turns, "\
			"default is 1024\n"\
		"  -L, --max-lag N             number of turns a player can "\
			"miss before getting\n"\
		"                              disconnected, unlimited by default\n"\
		"  -S, --shrink-after N        shrink map after that many "\
			"turns, default is 1024\n"\
		"  -T, --shrink-step N         amount of turns until next "\
			"shrink, default is 1\n"\
		"  -l, --player-life N         life value of players, "\
			"default is 1\n"\
		"  -X, --shoot                 players can shoot, "\
			"default depends on mode\n"\
		"  -D, --diagonal-interval N   players can move diagonally every "\
			"N turns,\n"\
		"                              default is 0 for no diagonal "\
			"movement\n"\
		"  -g, --gems N                number of gems if there are "\
			"gems, default equals\n"\
		"                              map width\n"\
		"  -Q, --spawn-frequency N     spawn a new enemy every N turns, "\
			"for modes with\n"\
		"                              enemies, default is 2\n"\
		"  -R, --word STRING           custom word for \"word\" mode, "\
			"random by default\n"\
		"  -F, --format TYPE           server output format, either \""\
			FORMAT_ARG_PLAIN"\" or \""\
			FORMAT_ARG_JSON"\",\n"\
		"                              default is \""\
			FORMAT_ARG_PLAIN"\"\n"\
		"  -w, --wait-for-joins N      number of seconds to wait "\
			"for joins,\n"\
		"                              default is 1\n"\
		"  -u, --usec-per-turn N       maximum number of milliseconds "\
			"per turn,\n"\
		"                              default is 1000000 (one second)\n"\
		"  -d, --deterministic         don't seed the random number "\
			"generator\n");
}

static void load_name_file(Names *names, const char *file) {
	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}
	#define SEPERATOR " "
	char line[MAX_LINE];
	Names *e = names + MAX_NAMES;
	while (fgets(line, MAX_LINE, fp)) {
		char *address = strtok(line, SEPERATOR);
		char *name = strtok(NULL, SEPERATOR);
		if (!address || !name) {
			continue;
		}
		if (*name < 'A' || *name >= 'A' + MAX_PLAYERS) {
			fprintf(stderr, "error: invalid name for %s\n", address);
			continue;
		}
		strncpy(names->addr, address, sizeof(names->addr) - 1);
		names->name = *name;
		if (++names >= e) {
			fprintf(stderr, "warning: too many entries in name file\n");
			break;
		}
	}
	fclose(fp);
}

static void (*pick_mode(const char *name))() {
	const struct Mode *m;
	for (m = modes; m->name; ++m) {
		if (!strcmp(m->name, name)) {
			return m->init;
		}
	}
	return NULL;
}

static char *load_map(const char *file) {
	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}
	long size = fseek(fp, 0, SEEK_END) ? -1 : ftell(fp);
	if (size < 1 || fseek(fp, 0, SEEK_SET)) {
		perror("fseek");
		fclose(fp);
		exit(1);
	}
	char *buf = NULL, *p;
	char line[MAX_LINE];
	size_t width = 0;
	size_t height = 0;
	while (fgets(line, MAX_LINE, fp)) {
		size_t len = strcspn(line, "\n");
		if (len < 1) {
			break;
		} else if (!width) {
			width = len;
			height = size / (width + 1);
			if (height < 1 || height * (width + 1) != (size_t) size) {
				fprintf(stderr, "error: malformed map in \"%s\"\n", file);
				fclose(fp);
				exit(1);
			}
			p = buf = calloc(width * height, sizeof(char));
			if (!p) {
				perror("calloc");
				fclose(fp);
				exit(1);
			}
		} else if (width != len) {
			break;
		}
		memcpy(p, line, width);
		p += width;
	}
	if (ferror(fp)) {
		fprintf(stderr, "error: unknown map format in \"%s\"\n", file);
		fclose(fp);
		exit(1);
	}
	fclose(fp);
	config.map_width = width;
	config.map_height = height;
	return buf;
}

static int parse_format(char *arg) {
	if (!strcmp(arg, FORMAT_ARG_PLAIN)) {
		return FORMAT_PLAIN;
	} else if (!strcmp(arg, FORMAT_ARG_JSON)) {
		return FORMAT_JSON;
	}
	fprintf(stderr, "error: unknown format \"%s\"\n", arg);
	exit(1);
}

static int map_bearing(const char bearing) {
	switch (bearing) {
	default:
	case '^':
		return 0;
	case '>':
		return 1;
	case 'v':
		return 2;
	case '<':
		return 3;
	}
}

static int parse_placing_at(Coords *coords, char *arg) {
	Coords *p = coords, *e = p + MAX_PLAYERS;
	char *s = strtok(arg, PLACING_SEPARATOR);
	for (; s && coords < e; s = strtok(NULL, PLACING_SEPARATOR), ++p) {
		char bearing = 0;
		sscanf(s, "%d,%d,%c", &p->x, &p->y, &bearing);
		p->bearing = bearing == 0 ? 4 : map_bearing(bearing);
	}
	if (p > coords) {
		// replicate last coordinate for remaining players
		Coords *last = p - 1;
		for (; p < e; ++p) {
			p->x = last->x;
			p->y = last->y;
			p->bearing = last->bearing;
		}
	}
	return PLACING_MANUAL;
}

static int parse_placing(const char *arg) {
	if (!strcmp(arg, PLACING_ARG_CIRCLE)) {
		return PLACING_CIRCLE;
	} else if (!strcmp(arg, PLACING_ARG_RANDOM)) {
		return PLACING_RANDOM;
	} else if (!strcmp(arg, PLACING_ARG_GRID)) {
		return PLACING_GRID;
	} else if (!strcmp(arg, PLACING_ARG_DIAGONAL)) {
		return PLACING_DIAGONAL;
	}
	fprintf(stderr, "error: unknown placing \"%s\"\n", arg);
	exit(1);
}

static int parse_map_type(const char *arg) {
	if (!strcmp(arg, MAP_TYPE_ARG_PLAIN)) {
		return MAP_TYPE_PLAIN;
	} else if (!strcmp(arg, MAP_TYPE_ARG_RANDOM)) {
		return MAP_TYPE_RANDOM;
	} else if (!strcmp(arg, MAP_TYPE_ARG_MAZE)) {
		return MAP_TYPE_MAZE;
	} else if (!strcmp(arg, MAP_TYPE_ARG_TERRAIN)) {
		return MAP_TYPE_TERRAIN;
	}
	fprintf(stderr, "error: unknown map type \"%s\"\n", arg);
	exit(1);
}

static void parse_arguments(int argc, char **argv) {
	int deterministic = 0;

	struct option longopts[] = {
		{ "port", required_argument, NULL, 'P' },
		{ "websocket-port", required_argument, NULL, 'W' },
		{ "spectator-port", required_argument, NULL, 'O' },
		{ "max-spectators", required_argument, NULL, 'V' },
		{ "remote-spectators", no_argument, &config.remote_spectators, 1 },
		{ "min-starters", required_argument, NULL, 'b' },
		{ "min-players", required_argument, NULL, 'm' },
		{ "name-file", required_argument, NULL, 'n' },
		{ "map-size", required_argument, NULL, 's' },
		{ "map-type", required_argument, NULL, 't' },
		{ "custom-map", required_argument, NULL, 'c' },
		{ "obstacles", required_argument, NULL, 'o' },
		{ "flatland", required_argument, NULL, 'f' },
		{ "multiplier", required_argument, NULL, 'x' },
		{ "placing", required_argument, NULL, 'p' },
		{ "fuzzy", required_argument, NULL, 'Z' },
		{ "place-at", required_argument, NULL, 'A' },
		{ "non-exclusive", no_argument, &config.non_exclusive, 1 },
		{ "translate-walls", no_argument, &config.translate_walls, 1 },
		{ "view-radius", required_argument, NULL, 'v' },
		{ "max-games", required_argument, NULL, 'G' },
		{ "max-turns", required_argument, NULL, 'M' },
		{ "max-lag", required_argument, NULL, 'L' },
		{ "shrink-after", required_argument, NULL, 'S' },
		{ "shrink-step", required_argument, NULL, 'T' },
		{ "player-life", required_argument, NULL, 'l' },
		{ "shoot", no_argument, &config.can_shoot, 1 },
		{ "diagonal-interval", required_argument, NULL, 'D' },
		{ "gems", required_argument, NULL, 'g' },
		{ "spawn-frequency", required_argument, NULL, 'Q' },
		{ "word", required_argument, NULL, 'R' },
		{ "format", required_argument, NULL, 'F' },
		{ "wait-for-joins", required_argument, NULL, 'w' },
		{ "usec-per-turn", required_argument, NULL, 'u' },
		{ "deterministic", no_argument, &deterministic, 1 },
		{ NULL, 0, NULL, 0 }
	};

	int ch;
	while ((ch = getopt_long(argc, argv,
			"P:W:O:V:rb:m:n:s:t:c:o:f:x:p:Z:A:NYv:G:M:L:S:T:l:XD:g:Q:R:F:w:u:d",
			longopts, NULL)) != -1) {
		switch (ch) {
		case 'P':
			config.port = atoi(optarg);
			break;
		case 'W':
			config.port_websocket = atoi(optarg);
			break;
		case 'O':
			config.port_spectator = atoi(optarg);
			if (config.max_spectators < 1) {
				config.max_spectators = 1;
			}
			break;
		case 'V':
			config.max_spectators = atoi(optarg);
			break;
		case 'r':
			config.remote_spectators = 1;
			if (config.max_spectators < 1) {
				config.max_spectators = 1;
			}
			break;
		case 'b':
			config.min_starters = atoi(optarg);
			break;
		case 'm':
			config.min_players = atoi(optarg);
			break;
		case 'n':
			load_name_file(config.names, optarg);
			break;
		case 's': {
			if (config.map_width || config.map_height) {
				fprintf(stderr, "error: map size already set\n");
				exit(1);
			}
			int width = atoi(strtok(optarg, "x"));
			char *height = strtok(NULL, "x");
			config.map_width = width;
			config.map_height = height ? atoi(height) : width;
			break;
		}
		case 't':
			config.map_type = parse_map_type(optarg);
			break;
		case 'c':
			config.custom_map = load_map(optarg);
			break;
		case 'o':
			config.obstacles = optarg && *optarg ? optarg : NULL;
			break;
		case 'f':
			config.flatland = optarg && *optarg ? optarg : NULL;
			break;
		case 'x':
			config.multiplier = atoi(optarg);
			break;
		case 'p':
			config.placing = parse_placing(optarg);
			break;
		case 'Z':
			config.placing_fuzz = atoi(optarg);
			break;
		case 'A':
			config.placing = parse_placing_at(config.coords, optarg);
			break;
		case 'N':
			config.non_exclusive = 1;
			break;
		case 'Y':
			config.translate_walls = 1;
			break;
		case 'v':
			config.view_radius = atoi(optarg);
			break;
		case 'G':
			config.max_games = atoi(optarg);
			break;
		case 'M':
			config.max_turns = atoi(optarg);
			break;
		case 'L':
			config.max_lag = atoi(optarg);
			break;
		case 'S':
			config.shrink_after = atoi(optarg);
			break;
		case 'T':
			config.shrink_step = atoi(optarg);
			break;
		case 'l':
			config.player_life = atoi(optarg);
			break;
		case 'X':
			config.can_shoot = 1;
			break;
		case 'D':
			config.diagonal_interval = atoi(optarg);
			break;
		case 'g':
			config.gems = atoi(optarg);
			break;
		case 'Q':
			config.spawn_frequency = atoi(optarg);
			break;
		case 'R':
			config.word = optarg;
			break;
		case 'F':
			config.output_format = parse_format(optarg);
			break;
		case 'w':
			config.wait_for_joins = atoi(optarg);
			break;
		case 'u':
			config.usec_per_turn = atoi(optarg);
			break;
		case 'd':
			deterministic = 1;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	void (*init_mode)() = NULL;
	if (argc < 1 || !(init_mode = pick_mode(*argv))) {
		usage();
		exit(0);
	}
	config.mode_name = *argv;

	if (!deterministic) {
		srand(time(NULL));
		config.rand = rand;
	} else {
		config.rand = repeatable_rand;
	}

	init_mode();
}

int main(int argc, char **argv) {
	memset(&config, 0, sizeof(config));
	parse_arguments(argc, argv);
	complete_config();
	int rv = game_serve();
	free_resources();
	return rv;
}
