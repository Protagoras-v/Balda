#pragma once

typedef struct Cell Cell;

typedef struct GameField GameField;

typedef struct Move Move;

typedef struct GameSettings GameSettings;

typedef struct Game Game;

//еще нужно добавить сюда аргумент настроек
Game* game_create(Dictionary* dict);

void game_destroy(Game* game);

int game_get_cell(GameField* field, int x, int y, unsigned char* res);

int game_insert_letter_into_cell(GameField* field, int x, int y, unsigned char letter, int player_id);