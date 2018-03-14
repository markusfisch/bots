#ifndef __player_h__
#define __player_h__

struct Game;
struct Player;

struct Player *player_get(struct Game *, int);
void player_remove(struct Game *, int);
int player_add(struct Game *, int);
void player_view_write(struct Game *, struct Player *);
void player_do(struct Game *, struct Player *, char);

#endif
