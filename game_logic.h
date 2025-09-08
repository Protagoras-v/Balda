#pragma once
#include "status_codes.h"
#include "common.h"
#include "dict.h"


typedef struct Cell {
	char letter;
	unsigned char player_id : 2; // 0 - клетка пустая, 1 - игрок, 2 - компьютер
	unsigned char new : 1;
} Cell;

typedef struct GameField {
	Cell** grid;
	unsigned char height : 4;
	unsigned char width : 4;
} GameField;

typedef struct Move {
	unsigned char y : 4;
	unsigned char x : 4;
	char letter;
	WordCell word[MAX_WORD_LEN];
	unsigned char word_len;
	int score;
} Move;


typedef struct Leaderboard Leaderboard;

typedef struct GameSettings GameSettings;

typedef struct Game Game;


Game* game_create(GameSettings* settings, Dictionary* dict);

void game_destroy(Game** game_);

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

StatusCode game_apply_generated_move(Game* game, Move move);

StatusCode game_undo_generated_move(Game* game, Move move);


//-------------------------------------------
//--------------------get--------------------
//-------------------------------------------
int game_get_difficulty(Game* game);
unsigned long long game_get_time_limit(Game* game);
int game_get_player_id(Game* game);

GameField* game_get_field(Game* game);

StatusCode game_get_cell(Game* game, int x, int y, unsigned char* res);

StatusCode game_get_score(Game* game, int id, int* score);

StatusCode game_get_player_words(Game* game, int player_id, char*** words, int* count);

StatusCode game_get_winner(Game* game, int* winner_id);
//returns current word
StatusCode game_get_word(Game* game, char* word);

StatusCode game_get_word_from_move(Move move, char* word);

StatusCode game_get_leaderboard(Leaderboard* lb, char usernames[][MAX_WORD_LEN + 1], int scores[], int* size);

bool game_is_enough_score_for_lb(Game* game, Leaderboard* lb);

//------------------------------------------------
//--------------------settings--------------------
// -----------------------------------------------
GameSettings* game_init_settings();

StatusCode game_set_max_time_waiting(GameSettings* settings, int time);

StatusCode game_set_difficulty(GameSettings* settings, int difficulty);

StatusCode game_set_first_player(GameSettings* settings, int first_player);

void print_settings(GameSettings* settings);


//additional
bool is_letter_near(GameField* field, int y, int x);

bool is_cell_empty(GameField* field, int y, int x);

bool is_cell_coordinates_valid(GameField* field, int y, int x);

bool is_word_used(Game* game, char* buffer);