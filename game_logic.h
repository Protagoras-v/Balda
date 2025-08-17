#pragma once
#include <stdbool.h>
#include "status_codes.h"

#include "dict.h"

typedef struct Cell Cell;

typedef struct GameField GameField;

typedef struct Move Move;

typedef struct GameSettings GameSettings;

typedef struct Game Game;

//еще нужно добавить сюда аргумент настроек
Game* game_create(Dictionary* dict);

void game_destroy(Game* game);

StatusCode game_try_place_letter(Game* game, int y, int x, char letter);

StatusCode game_add_cell_into_word(Game* game, int y, int x);

StatusCode game_confirm_move(Game* game, Dictionary* dict);

StatusCode game_cancel_word_selection(Game* game);

StatusCode game_clear_move(Game* game);

void print_field(Game* game);

//get
StatusCode game_get_cell(GameField* field, int y, int x, unsigned char* res);

int game_get_player_id(Game* game);

StatusCode game_get_player_words(Game* game, int player_id, char*** words, int* count);

//также нужны функции, которые проверяют, есть ли клетка в выделенном слове для UI

//нужна функция и поле в структуре, которое будет хранить все поставленные слова игроков

//Нужна функция, которая будет проверять, поставлена ли буква на этом ходу (для того, чтобы разделить цикл с формированием слова и саму постановку буквы)

//!!!НУЖНО ПРОВЕРЯТЬ, ВКЛЮЧЕНА ЛИ БУКВА В СЛОВО