#ifndef __player_h__
#define __player_h__

struct Game;
struct Player;

struct Player *player_at(struct Game *, int, int);
void player_send_view(struct Game *, struct Player *);
void player_act(struct Game *, struct Player *, char);

#endif
