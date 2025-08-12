#pragma once
#include <stdbool.h>
#include "status_codes.h"

typedef struct Cell Cell;

typedef struct GameField GameField;

typedef struct Move Move;

typedef struct GameSettings GameSettings;

typedef struct Game Game;

//еще нужно добавить сюда аргумент настроек
Game* game_create(Dictionary* dict);

void game_destroy(Game* game);

StatusCode game_get_cell(GameField* field, int x, int y, unsigned char* res);

StatusCode game_try_place_letter(Game* game, int x, int y, char letter);

StatusCode game_add_cell_into_word(Game* game, int x, int y);

StatusCode game_confirm_move(Game* game);

StatusCode game_cancel_move(Game* game);