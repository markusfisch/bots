#ifndef _map_h_
#define _map_h_

#define TILE_FLATLAND '.'
#define TILE_DIRT ':'
#define TILE_WOOD '#'
#define TILE_WATER '~'
#define TILE_GONE 'X'

typedef struct {
	char *data;
	unsigned int width;
	unsigned int height;
	size_t size;
} Map;

void map_write(int, char *, unsigned int, unsigned int);
void map_free(Map *);
void map_create(Map *, unsigned int, unsigned int);
void map_init_chess(Map *);
void map_init_random(Map *, char *, size_t);
int map_wrap(int, unsigned int);
char map_get(Map *, int, int);
void map_set(Map *, int, int, char);
int map_impassable(Map *, int, int);

#endif
