#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "word.h"

extern struct Config config;
extern struct Game game;

static char random_word[16] = { 0 };
static int points;

static void move(Player *p, char cmd) {
	if (cmd == *((char *) p->trunk)) {
		if (!*((char *) ++p->trunk)) {
			p->score = points--;
			game_remove_player(p);
			return;
		}
	} else {
		p->trunk = config.word;
	}
	player_move(p, cmd);
}

static void start() {
	points = MAX_PLAYERS;
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->trunk = config.word;
	}
	size_t len = strlen(config.word);
	int x = (game.map.width - len) / 2;
	int y = game.map.height / 2;
	if (x < 0) {
		x = 0;
		len = game.map.width;
		*(config.word + len) = 0;
	}
	strncpy(game.map.data + (y * game.map.width + x), config.word, len);
}

static char *generate_random_word() {
	size_t len = rand() % sizeof(random_word);
	if (len < 6) {
		len = 6;
	}
	char *p = random_word, *e = p + len;
	for (; p < e; ++p) {
		*p = 97 + (rand() % 26);
	}
	*p = 0;
	return random_word;
}

void word() {
	config.move = move;
	config.start = start;
	config.word = config.word ?: generate_random_word();
}
