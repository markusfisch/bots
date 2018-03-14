#ifndef __game_h__
#define __game_h__

#include <sys/time.h>

#define SECONDS_TO_JOIN 10
#define USEC_PER_SEC 1000000L
#define MAX_PLAYERS 16

struct Map;
struct Game {
	struct timeval tick;
	int started;
	int min_players;
	int view_radius;
	time_t usec_per_turn;
	int nplayers;
	struct Map map;
	struct Player {
		char name;
		int fd;
		int can_move;
		int x;
		int y;
		int bearing;
	} players[MAX_PLAYERS];
	void (*start)(struct Game *);
	int (*impassable)(struct Map *, int, int);
	void (*moved)(struct Game *, struct Player *);
};

void game_start(struct Game *);
void game_remove_all(struct Game *);
int game_joined(struct Game *);
time_t game_next_turn(struct Game *);
void game_offset_circle(struct Game *);

#endif
