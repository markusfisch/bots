#ifndef _game_h_
#define _game_h_

#include <sys/time.h>
#include <sys/types.h>

#include "map.h"

#define MAX_PLAYERS 16
#define MAP_TYPE_PLAIN 0
#define MAP_TYPE_RANDOM 1
#define MAP_TYPE_MAZE 2

struct Game {
	struct timeval tick;
	fd_set watch;
	fd_set ready;
	int nfds;
	int listening_fd;
	time_t started;
	time_t stopped;
	time_t usec_per_turn;
	unsigned int min_players;
	unsigned int view_radius;
	unsigned int max_turns;
	unsigned int turn;
	unsigned int shrink_level;
	unsigned int shrink_after;
	unsigned int nplayers;
	struct Map map;
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
	void (*start)(struct Game *);
	void (*turn_start)(struct Game *);
	int (*impassable)(struct Map *, int, int);
	void (*move)(struct Game *, struct Player *, char);
};

struct Config {
	void (*init)(struct Game *);
	unsigned int port;
	unsigned int map_width;
	unsigned int map_height;
	unsigned int map_type;
	unsigned int view_radius;
	unsigned int max_turns;
	unsigned int shrink_after;
	time_t usec_per_turn;
};

void game_remove_player(struct Game *, struct Player *);
size_t game_joined(struct Game *);
void game_set_players_life(struct Game *, int);
void game_end(struct Game *);
int game_serve(struct Config *);

#endif
