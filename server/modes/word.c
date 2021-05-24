#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game.h"
#include "../player.h"
#include "word.h"

#define CHARS "abcdefghijklmnopqrstuwxyz"

extern struct Config config;
extern struct Game game;

static char random_word[9] = { 0 };
static size_t word_len;
static int points;

static int matches(char *buffer, unsigned int pos, char *pattern,
		unsigned int len) {
	char forward = 1;
	char backward = 1;
	unsigned int i;
	for (i = 0; i < len && (forward || backward); ++i) {
		char c = buffer[((pos - i) + len) % len];
		if (backward && c != pattern[i]) {
			backward = 0;
		}
		if (forward && c != pattern[len - i - 1]) {
			forward = 0;
		}
	}
	return forward ? 1 : backward ? -1 : 0;
}

static void move(Player *p, char cmd) {
	((char *) p->trunk)[p->counter % word_len] = cmd;
	if (matches(p->trunk, p->counter, config.word, word_len)) {
		p->score = points--;
		game_remove_player(p);
		return;
	}
	++p->counter;
	player_move(p, cmd);
}

static void place_word() {
	int x = (game.map.width - word_len) / 2;
	int y = game.map.height / 2;
	if (x < 0) {
		x = 0;
		word_len = game.map.width;
		*(config.word + word_len) = 0;
	}
	memcpy(game.map.data + (y * game.map.width + x), config.word,
		word_len);
}

static void free_buffers() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (p->trunk) {
			free(p->trunk);
			p->trunk = NULL;
		}
	}
}

static void alloc_buffers() {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		p->trunk = calloc(word_len, sizeof(char));
		if (!p->trunk) {
			perror("calloc");
			exit(-1);
		}
	}
}

static void start() {
	points = MAX_PLAYERS;
	word_len = strlen(config.word);
	place_word();
	alloc_buffers();
}

static char *generate_random_word() {
	size_t len = config.rand() % (sizeof(random_word) - 1);
	if (len < 6) {
		len = 6;
	}
	char *chars = CHARS;
	int max = strlen(chars);
	char *p = random_word, *e = p + len;
	for (; p < e; ++p) {
		*p = chars[config.rand() % max];
	}
	*p = 0;
	return random_word;
}

int impassable(Map *map, const int x, const int y) {
	char tile = map_get(map, x, y);
	return map_impassable(map, x, y) || strchr(CHARS, tile);
}

void word() {
	config.move = move;
	config.start = start;
	config.end = free_buffers;
	SET_IF_NULL(config.word, generate_random_word())
	config.impassable = impassable;
}
