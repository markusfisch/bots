#ifndef __player_h__
#define __player_h__

struct Game;
struct Player;

void player_send_view(struct Game *, struct Player *);
void player_do(struct Game *, struct Player *, char);

#endif
