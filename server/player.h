#ifndef _player_h_
#define _player_h_

struct Player;

void players_set_remaining_scores(int);
Player *player_at(int, int, Player *);
Player *player_near(int, int, int, Player *, double *);
int player_cannot_move_to(int, int);
char player_bearing(const int);
int player_send_view(Player *);
void player_move(Player *, char);
void player_shoot(Player *);

#endif
