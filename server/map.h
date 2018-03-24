#ifndef _map_h_
#define _map_h_

#define TILE_FLATLAND '.'
#define TILE_WOOD '#'
#define TILE_WATER '~'
#define TILE_GONE 'X'

struct Map {
	char *data;
	unsigned int width;
	unsigned int height;
	size_t size;
};

void map_write(int, char *, unsigned int, unsigned int);
void map_free(struct Map *);
void map_create(struct Map *, unsigned int, unsigned int);
void map_init_random(struct Map *, char *, size_t);
int map_wrap(int, unsigned int);
char map_get(struct Map *, int, int);
void map_set(struct Map *, int, int, char);
int map_impassable(struct Map *, int, int);

#endif
