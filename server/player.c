#include <unistd.h>

#include "game.h"
#include "player.h"

extern struct Config config;
extern struct Game game;

Player *player_at(const int x, const int y, Player *last) {
	Player *p = last ?: game.players, *e = game.players + game.nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->x == x && p->y == y) {
			return p;
		}
	}
	return NULL;
}

int player_cannot_move_to(const int x, const int y) {
	return config.impassable(&game.map, x, y) ||
		(!config.non_exclusive && player_at(x, y, NULL));
}

char player_bearing(const int bearing) {
	switch (bearing % 4) {
	default:
	case 0:
		return '^';
	case 1:
		return '>';
	case 2:
		return 'v';
	case 3:
		return '<';
	}
}

static char player_view_at(Player *p, const int x, const int y) {
	char tile = map_get(&game.map, x, y);
	Player *enemy = player_at(
		map_wrap(x, game.map.width),
		map_wrap(y, game.map.height),
		NULL);
	if (enemy) {
		tile = player_bearing(enemy->bearing + 4 - p->bearing);
	}
	return tile;
}

void player_send_view(Player *player) {
	int radius = config.view_radius;
	size_t len = radius * 2 + 1;
	char view[len * len];
	int left;
	int top;
	int xx;
	int xy;
	int yx;
	int yy;
	switch (player->bearing) {
	default:
	case 0:
		left = -radius;
		top = -radius;
		xx = 1;
		xy = 0;
		yx = 0;
		yy = 1;
		break;
	case 1:
		left = radius;
		top = -radius;
		xx = 0;
		xy = 1;
		yx = -1;
		yy = 0;
		break;
	case 2:
		left = radius;
		top = radius;
		xx = -1;
		xy = 0;
		yx = 0;
		yy = -1;
		break;
	case 3:
		left = -radius;
		top = radius;
		xx = 0;
		xy = -1;
		yx = 1;
		yy = 0;
		break;
	}
	left = map_wrap(player->x + left, game.map.width);
	top = map_wrap(player->y + top, game.map.height);
	char *p = view;
	size_t y;
	size_t x;
	for (y = 0; y < len; ++y) {
		int l = left;
		int t = top;
		for (x = 0; x < len; ++x, ++p) {
			*p = player_view_at(player, l, t);
			l += xx;
			t += xy;
		}
		left += yx;
		top += yy;
	}
	view[radius * len + radius] = config.marker ?
		config.marker(player) : player->name;
	map_write(player->fd, view, len, len);
}

static void player_move_by(Player *p, int x, int y) {
	x = map_wrap(p->x + x, game.map.width);
	y = map_wrap(p->y + y, game.map.height);
	if (player_cannot_move_to(x, y)) {
		return;
	}
	p->x = x;
	p->y = y;
}

static void player_step(Player *p, const int steps) {
	switch (p->bearing % 4) {
	case 0: // north
		player_move_by(p, 0, -steps);
		break;
	case 1: // east
		player_move_by(p, steps, 0);
		break;
	case 2: // south
		player_move_by(p, 0, steps);
		break;
	case 3: // west
		player_move_by(p, -steps, 0);
		break;
	}
}

static void player_turn(Player *p, const int direction) {
	p->bearing = (p->bearing + direction + 4) % 4;
}

void player_move(Player *p, const char cmd) {
	switch (cmd) {
	case 'f':
		if (config.can_shoot) {
			player_shoot(p);
		}
		break;
	case '^':
		player_step(p, 1);
		break;
	case '<':
		player_turn(p, -1);
		break;
	case '>':
		player_turn(p, 1);
		break;
	case 'v':
		player_step(p, -1);
		break;
	}
}

void player_shoot(Player *p) {
	int vx = 0;
	int vy = 0;
	switch (p->bearing % 4) {
	case 0:
		vy = -1;
		break;
	case 1:
		vx = 1;
		break;
	case 2:
		vy = 1;
		break;
	case 3:
		vx = -1;
		break;
	}
	int range = config.view_radius;
	p->attack_x = p->x;
	p->attack_y = p->y;
	while (range-- > 0) {
		p->attack_x = (p->attack_x + vx + game.map.width) % game.map.width;
		p->attack_y = (p->attack_y + vy + game.map.height) % game.map.height;
		Player *enemy = NULL;
		while ((enemy = player_at(p->attack_x, p->attack_y, enemy))) {
			if (--enemy->life < 1) {
				++p->score;
				enemy->killed_by = p->name;
				game_remove_player(enemy);
			}
		}
	}
}
