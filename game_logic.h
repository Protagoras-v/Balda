#pragma once
#include <stdbool.h>
#include "status_codes.h"

#include "dict.h"

#define MAX_WORD_LEN 100

typedef struct WordCell WordCell;

typedef struct Move Move;

typedef struct Leaderboard Leaderboard;

typedef struct Cell Cell;

typedef struct GameField GameField;

typedef struct GameSettings GameSettings;

typedef struct Game Game;


Game* game_create(GameSettings* settings, Dictionary* dict);

void game_destroy(Game* game);

StatusCode game_load(Game* game, const char* filename);

StatusCode game_save(Game* game, const char* filename);

StatusCode game_try_place_letter(Game* game, int y, int x, char letter);

StatusCode game_add_cell_into_word(Game* game, int y, int x);

StatusCode game_confirm_move(Game* game, Dictionary* dict);

StatusCode game_cancel_word_selection(Game* game);

StatusCode game_clear_move(Game* game);

void print_field(Game* game);

Leaderboard* game_leaderboard_init();

void game_leaderboard_destroy(Leaderboard* lb);

StatusCode game_add_into_leaderboard(Leaderboard* lb, Game* game, const char* username);

Game* game_make_copy(Game* game);


//-------------------------------------------
//--------------------get--------------------
//-------------------------------------------
int game_get_difficulty(Game* game);
int game_get_time_limit(Game* game);
GameField* game_get_field(Game* game);

StatusCode game_get_cell(Game* game, int x, int y, unsigned char* res);

int game_get_player_id(Game* game, int* id);

StatusCode game_get_score(Game* game, int id, int* score);

StatusCode game_get_player_words(Game* game, int player_id, char*** words, int* count);

StatusCode game_get_winner(Game* game, int* winner_id);
//returns current word
StatusCode game_get_word(Game* game, char* word);

StatusCode game_get_leaderboard(Leaderboard* lb, char usernames[], int scores[], int *size);

bool game_is_enough_score_for_lb(Game* game, Leaderboard* lb);

//------------------------------------------------
//--------------------settings--------------------
// -----------------------------------------------
GameSettings* game_init_settings();

StatusCode game_set_max_time_waiting(GameSettings* settings, int time);

StatusCode game_set_difficulty(GameSettings* settings, int time);

StatusCode game_set_first_player(GameSettings* settings, int time);

void print_settings(GameSettings* settings);


//additional
bool is_letter_near(GameField* field, int y, int x);

bool is_cell_empty(GameField* field, int y, int x);

bool is_cell_coordinates_valid(GameField* field, int y, int x);