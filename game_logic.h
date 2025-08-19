#pragma once
#include <stdbool.h>
#include "status_codes.h"

#include "dict.h"

typedef struct Cell Cell;

typedef struct GameField GameField;

typedef struct Move Move;

typedef struct GameSettings GameSettings;

typedef struct Game Game;


Game* game_create(GameSettings* settings, Dictionary* dict);

void game_destroy(Game* game);

StatusCode game_try_place_letter(Game* game, int y, int x, char letter);

StatusCode game_add_cell_into_word(Game* game, int y, int x);

StatusCode game_confirm_move(Game* game, Dictionary* dict);

StatusCode game_cancel_word_selection(Game* game);

StatusCode game_clear_move(Game* game);

void print_field(Game* game);


//-------------------------------------------
//--------------------get--------------------
//-------------------------------------------
StatusCode game_get_cell(Game* game, int x, int y, unsigned char* res);

int game_get_player_id(Game* game);

StatusCode game_get_player_words(Game* game, int player_id, char*** words, int* count);

//returns current word
StatusCode game_get_word(Game* game, char* word);


//------------------------------------------------
//--------------------settings--------------------
// -----------------------------------------------
GameSettings* game_init_settings();

StatusCode game_set_max_time_waiting(GameSettings* settings, int time);

StatusCode game_set_difficulty(GameSettings* settings, int time);

StatusCode game_set_first_player(GameSettings* settings, int time);

void print_settings(GameSettings* settings);



//также нужны функции, которые проверяют, есть ли клетка в выделенном слове для UI

//нужна функция и поле в структуре, которое будет хранить все поставленные слова игроков

//Нужна функция, которая будет проверять, поставлена ли буква на этом ходу (для того, чтобы разделить цикл с формированием слова и саму постановку буквы)

//!!!НУЖНО ПРОВЕРЯТЬ, ВКЛЮЧЕНА ЛИ БУКВА В СЛОВО