#ifndef __game_h__
#define __game_h__

#include <sys/time.h>
#include <sys/types.h>

#include "map.h"

#define MAX_PLAYERS 16

struct Game {
	struct timeval tick;
	fd_set watch;
	fd_set ready;
	int nfds;
	int listening_fd;
	time_t started;
	time_t stopped;
	time_t usec_per_turn;
	int min_players;
	int view_radius;
	int max_turns;
	int turn;
	int nplayers;
	struct Map map;
	struct Player {
		char name;
		int fd;
		int can_move;
		int x;
		int y;
		int bearing;
		int moves;
		int score;
	} players[MAX_PLAYERS];
	void (*start)(struct Game *);
	int (*impassable)(struct Map *, int, int);
	void (*moved)(struct Game *, struct Player *);
};

void game_remove_player(struct Game *, struct Player *);
int game_joined(struct Game *);
void game_end(struct Game *);
int game_serve(void (*)(struct Game *));

#endif