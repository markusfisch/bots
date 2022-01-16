#include <unistd.h>
#include <string.h>
#include <math.h>

#include "websocket.h"
#include "game.h"
#include "player.h"

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

const char *player_long_name(struct Player *p) {
	return p->long_name && *p->long_name ? p->long_name : p->addr;
}

void players_set_remaining_scores(int score) {
	Player *p = game.players, *e = p + game.nplayers;
	for (; p < e; ++p) {
		if (!p->score) {
			p->score = score;
		}
	}
}

char player_marker_show_life(struct Player *p) {
	return p->life < config.player_life ? (char) (48 + p->life) : p->name;
}

Player *player_at(int x, int y, Player *last) {
	Player *p = last ? ++last : game.players,
		*e = game.players + game.nplayers;
	x = map_wrap(x, game.map.width);
	y = map_wrap(y, game.map.height);
	for (; p < e; ++p) {
		if (p->fd > 0 && p->x == x && p->y == y) {
			return p;
		}
	}
	return NULL;
}

Player *player_near(int x, int y, const int radius, Player *last,
		double *dist) {
	Player *p = last ? ++last : game.players,
		*e = game.players + game.nplayers;
	x = map_wrap(x, game.map.width);
	y = map_wrap(y, game.map.height);
	int r2 = radius * radius;
	for (; p < e; ++p) {
		if (p->fd < 1) {
			continue;
		}
		int dx = x - p->x;
		int dy = y - p->y;
		int d = dx*dx + dy*dy;
		if (d < r2) {
			if (dist) {
				*dist = sqrt(d);
			}
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
	Map *map = p->map;
	if (!map) {
		map = &game.map;
	}
	char tile = map_get(map, x, y);
	Player *enemy = player_at(x, y, NULL);
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
	player->map = NULL;
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
		if (config.move_blocked_at) {
			config.move_blocked_at(p, x, y);
		}
		return;
	}
	p->x = x;
	p->y = y;
}

static void player_step(Player *p, const int steps, const int side) {
	switch (p->bearing % 4) {
	case NORTH:
		player_move_by(p, side, -steps);
		break;
	case EAST:
		player_move_by(p, steps, side);
		break;
	case SOUTH:
		player_move_by(p, -side, steps);
		break;
	case WEST:
		player_move_by(p, -steps, -side);
		break;
	}
}

static void player_turn(Player *p, const int direction) {
	p->bearing = (p->bearing + direction + 4) % 4;
}

static void player_move_diagonal(Player *p, const char cmd) {
	if (config.diagonal_interval < 1 || (p->last_diagonal > 0 &&
			game.turn - p->last_diagonal < config.diagonal_interval)) {
		return;
	}
	p->last_diagonal = game.turn;
	switch (cmd) {
	case '{':
		player_step(p, 1, -1);
		break;
	case '}':
		player_step(p, 1, 1);
		break;
	case '(':
		player_step(p, 0, -1);
		break;
	case ')':
		player_step(p, 0, 1);
		break;
	case '[':
		player_step(p, -1, -1);
		break;
	case ']':
		player_step(p, -1, 1);
		break;
	}
}

void player_move(Player *p, const char cmd) {
	switch (cmd) {
	case 'f':
		if (config.can_shoot) {
			player_shoot(p);
		}
		break;
	case '(':
	case ')':
	case '{':
	case '}':
	case '[':
	case ']':
		player_move_diagonal(p, cmd);
		break;
	case '^':
		player_step(p, 1, 0);
		break;
	case '<':
		player_turn(p, -1);
		break;
	case '>':
		player_turn(p, 1);
		break;
	case 'v':
		player_step(p, -1, 0);
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
		if (config.attacking && config.attacking(p)) {
			break;
		}
		if (config.impassable(&game.map, p->attack_x, p->attack_y)) {
			break;
		}
		Player *enemy = NULL;
		while ((enemy = player_at(p->attack_x, p->attack_y, enemy))) {
			++p->score;
			if (--enemy->life < 1) {
				enemy->killed_by = p->name;
				game_remove_player(enemy);
			}
		}
	}
}
