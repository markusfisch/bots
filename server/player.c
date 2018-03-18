#include <unistd.h>

#include "game.h"
#include "player.h"

static char player_bearing(int bearing) {
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

static struct Player *player_at(struct Game *game, int x, int y) {
	struct Player *p = game->players, *e = p + game->nplayers;
	for (; p < e; ++p) {
		if (p->fd > 0 && p->x == x && p->y == y) {
			return p;
		}
	}
	return NULL;
}

static char player_view_at(struct Game *game, struct Player *p, int x, int y) {
	char tile = map_get(&game->map, x, y);
	struct Player *enemy = player_at(game,
		map_wrap(x, game->map.width),
		map_wrap(y, game->map.height));
	if (enemy) {
		tile = player_bearing(p->bearing + enemy->bearing);
	}
	return tile;
}

void player_send_view(struct Game *game, struct Player *player) {
	int radius = game->view_radius;
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
	left = map_wrap(player->x + left, game->map.width);
	top = map_wrap(player->y + top, game->map.height);
	char *p = view;
	size_t y;
	size_t x;
	for (y = 0; y < len; ++y) {
		int l = left;
		int t = top;
		for (x = 0; x < len; ++x, ++p) {
			*p = player_view_at(game, player, l, t);
			l += xx;
			t += xy;
		}
		left += yx;
		top += yy;
	}
	view[radius * len + radius] = player->name;
	map_write(player->fd, view, len, len);
}

static void player_move_by(struct Game *game, struct Player *p, int x, int y) {
	x = map_wrap(p->x + x, game->map.width);
	y = map_wrap(p->y + y, game->map.height);
	if (game->impassable(&game->map, x, y) || player_at(game, x, y)) {
		return;
	}
	p->x = x;
	p->y = y;
}

static void player_move(struct Game *game, struct Player *p, int step) {
	switch (p->bearing % 4) {
	case 0: // north
		player_move_by(game, p, 0, -step);
		break;
	case 1: // east
		player_move_by(game, p, step, 0);
		break;
	case 2: // south
		player_move_by(game, p, 0, step);
		break;
	case 3: // west
		player_move_by(game, p, -step, 0);
		break;
	}
}

static void player_turn(struct Player *p, int direction) {
	p->bearing = (p->bearing + direction + 4) % 4;
}

void player_do(struct Game *game, struct Player *p, char cmd) {
	if (p == NULL || !p->can_move) {
		return;
	}
	p->can_move = 0;

	switch (cmd) {
	case '^':
		player_move(game, p, 1);
		break;
	case '<':
		player_turn(p, -1);
		break;
	case '>':
		player_turn(p, 1);
		break;
	case 'v':
		player_move(game, p, -1);
		break;
	}

	game->moved(game, p);
}
