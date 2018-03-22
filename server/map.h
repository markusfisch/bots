#ifndef _map_h_
#define _map_h_

#define TILE_FLATLAND '.'
#define TILE_WOOD '#'
#define TILE_WATER '~'
#define TILE_GONE 'X'

struct Map {
	char *data;
	size_t width;
	size_t height;
	size_t size;
};

void map_write(int, char *, size_t, size_t);
void map_free(struct Map *);
void map_create(struct Map *, size_t, size_t);
void map_init_random(struct Map *, char *, size_t);
int map_wrap(int, size_t);
char map_get(struct Map *, int, int);
void map_set(struct Map *, int, int, char);
int map_impassable(struct Map *, int, int);

#endif
