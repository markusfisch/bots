#ifndef _player_h_
#define _player_h_

struct Player;

const char *player_long_name(Player *);
void players_set_remaining_scores(int);
char player_marker_show_life(Player *);
Player *player_at(int, int, Player *);
Player *player_near(int, int, int, Player *, double *);
int player_cannot_move_to(int, int);
char player_bearing(const int);
int player_send_view(Player *);
void player_move(Player *, char);
void player_shoot(Player *);

#endif
