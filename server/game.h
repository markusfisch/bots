#ifndef _game_h_
#define _game_h_

#include <sys/time.h>
#include <sys/types.h>

#include "map.h"

#define USEC_PER_SEC 1000000L
#define MAX_PLAYERS 16
#define MAP_TYPE_CHESS 1
#define MAP_TYPE_PLAIN 2
#define MAP_TYPE_RANDOM 3
#define MAP_TYPE_MAZE 4
#define PLACING_CIRCLE 1
#define PLACING_RANDOM 2

typedef struct Game Game;
typedef struct Player Player;
typedef struct Config Config;

struct Game {
	struct timeval tick;
	fd_set watch;
	fd_set ready;
	int nfds;
	int listening_fd;
	time_t started;
	time_t stopped;
	unsigned int turn;
	unsigned int shrink_level;
	unsigned int shrink_step;
	unsigned int nplayers;
	Map map;
	struct Player {
		char name;
		int fd;
		int can_move;
		int x;
		int y;
		int bearing;
		unsigned int moves;
		int score;
		unsigned int life;
	} players[MAX_PLAYERS];
};

struct Config {
	unsigned int port;
	unsigned int min_players;
	unsigned int map_width;
	unsigned int map_height;
	unsigned int map_type;
	const char *obstacles;
	const char *flatland;
	unsigned int multiplier;
	unsigned int placing;
	unsigned int view_radius;
	unsigned int max_turns;
	unsigned int shrink_after;
	unsigned int shrink_step;
	unsigned int player_life;
	time_t usec_per_turn;
	void (*start)();
	void (*turn_start)();
	unsigned char (*marker)(Player *);
	int (*impassable)(Map *, int, int);
	void (*move)(Player *, char);
};

void game_remove_player(Player *);
unsigned int game_joined();
unsigned char game_marker_show_life(Player *);
void game_end();
int game_serve();

#endif
