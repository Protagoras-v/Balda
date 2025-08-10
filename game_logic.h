#pragma once

typedef struct Cell Cell;

typedef struct GameField GameField;

typedef struct Move Move;

typedef struct GameSettings GameSettings;

typedef struct GameState GameState;


GameField* field_create(int width, int height);

void field_destroy(GameField* field);

int get_cell(GameField* field, int x, int y, unsigned char* res);

int insert_letter_in_cell(GameField* field, int x, int y, unsigned char letter, int player_id);