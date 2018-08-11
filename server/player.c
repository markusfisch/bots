#include <unistd.h>
#include <string.h>

#include "websocket.h"
#include "game.h"
#include "player.h"

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

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

int player_near(const int x, const int y, const int size) {
	int offset = size / 2;
	int v;
	int u;
	for (v = -offset; v < size; ++v) {
		for (u = -offset; u < size; ++u) {
			if (player_at(map_wrap(x + v, game.map.width),
					map_wrap(y + u, game.map.height), NULL)) {
				return 1;
			}
		}
	}
	return 0;
}

int player_cannot_move_to(const int x, const int y) {
	return config.impassable(&game.map, x, y) ||
		(!config.non_exclusive && player_at(x, y, NULL));
}

char player_bearing(const int bearing) {
	switch (bearing % 4) {
	default:
	case NORTH:
		return '^';
	case EAST:
		return '>';
	case SOUTH:
		return 'v';
	case WEST:
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
	} else if (config.translate_walls &&
			(p->bearing == EAST || p->bearing == WEST)) {
		if (tile == '-') {
			tile = '|';
		} else if (tile == '|') {
			tile = '-';
		}
	}
	return tile;
}

static int player_send_view_websocket(WebSocket *ws, const char *view,
		size_t width, size_t height) {
	size_t size = (width + 1) * height;
	char buf[size];
	char *e = buf + size;
	char *p = buf;
	while (p < e) {
		strncpy(p, view, width);
		p += width;
		*p++ = '\n';
		view += width;
	}
	return websocket_send_text_message(ws, buf, size);
}

int player_send_view(Player *player) {
	int radius = config.view_radius;
	size_t len = radius * 2 + 1;
	size_t size = len * len;
	char view[size];
	int left;
	int top;
	int xx;
	int xy;
	int yx;
	int yy;
	switch (player->bearing) {
	default:
	case NORTH:
		left = -radius;
		top = -radius;
		xx = 1;
		xy = 0;
		yx = 0;
		yy = 1;
		break;
	case EAST:
		left = radius;
		top = -radius;
		xx = 0;
		xy = 1;
		yx = -1;
		yy = 0;
		break;
	case SOUTH:
		left = radius;
		top = radius;
		xx = -1;
		xy = 0;
		yx = 0;
		yy = -1;
		break;
	case WEST:
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
	return player->ws.fd > 0 ?
		player_send_view_websocket(&player->ws, view, len, len) :
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
	case NORTH:
		player_move_by(p, 0, -steps);
		break;
	case EAST:
		player_move_by(p, steps, 0);
		break;
	case SOUTH:
		player_move_by(p, 0, steps);
		break;
	case WEST:
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
	case NORTH:
		vy = -1;
		break;
	case EAST:
		vx = 1;
		break;
	case SOUTH:
		vy = 1;
		break;
	case WEST:
		vx = -1;
		break;
	}
	int range = config.view_radius;
	p->attack_x = p->x;
	p->attack_y = p->y;
	while (range-- > 0) {
		p->attack_x = map_wrap(p->attack_x + vx, game.map.width);
		p->attack_y = map_wrap(p->attack_y + vy, game.map.height);
		if (config.impassable(&game.map, p->attack_x, p->attack_y)) {
			break;
		}
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
