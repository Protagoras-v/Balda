#include <stdio.h>
#include <stdlib.h>

#include "game_logic.h"

struct Cell {
	char letter;
	int player_id : 2; // 0 - cell is empty, 1 - player, 2 - computer
};

struct GameField {
	Cell** grid;
	int width, height;
	int center_x, center_y;
};

struct Move {
	int x, y; //letter coordinates
	char letter;
	char* word;
	int score;
	int player_id : 1;
};

struct GameSettings {
	int time_limit; //ms
	int difficulty : 2;
	int first_player : 1;
	char start_word[32];
};

struct GameState {
	GameField field;
	//Move* moves_history;
	int moves_count;
	int current_player : 1;
	int scores[3]; // 0 is not used
	int game_finished : 1;
};


GameField* field_create(int width, int height) {
	GameField* field = malloc(sizeof(GameField));
	if (field == NULL)
		return NULL;
		field->grid = malloc(height * sizeof(Cell*));
	if (field->grid == NULL) {
		free(field);
		return NULL;
	}

	for (int i = 0; i < height; i++) {
		field->grid[i] = malloc(width * sizeof(Cell));

		//If allocation fails we`ll have to clear all cells that were previously allocated
		if (field->grid[i] == NULL) {
			for (int j = 0; j < i; j++) {
				free(field->grid[j]);
			}
			free(field->grid);
			free(field);
			return NULL;
		}

		//initialization
		for (int j = 0; j < width; j++) {
			field->grid[i][j].letter = 0;
			field->grid[i][j].player_id = 0;
		}
	}

	field->height = height;
	field->width = width;
	field->center_y = height / 2;
	field->center_x = width / 2;

	return field;
}

void field_destroy(GameField* field) {
	for (int i = 0; i < field->height; i++) {
		free(field->grid[i]);
	}
	free(field->grid);
	free(field);
}


//получить содержимое, вставить букву, проверить, есть ли буква

static int is_cell_coordinates_valid(GameField* field, int x, int y) {
	if (x > field->width || y > field->height || x < 0 || y < 0) {
		return 0;
	}

	return 1;
}

static int is_cell_empty(GameField* field, int x, int y) {
	if (field->grid[y][x].letter == '\0') {
		return 1;
	}
		
	return 0;
}

static int is_letter_valid(unsigned char c) {
	if ((c >= 'А' && c <= 'Я') || (c >= 'а' && c <= 'я') || c == 'Ё' || c == 'ё') {
		return 1;
	}
	
	return 0;
}

static int is_player_id_valid(int id) {
	if (id == 1 || id == 2)
		return 1;

	return 0;
}


//В SDL CP1251 Будет преобразовываться в UTF8, можно будет написать простую функцию конвертер, даже отдельно 33 случая рассмотреть if`ами
int get_cell(GameField* field, int x, int y, unsigned char* res) {
	if (!is_cell_coordinates_valid(field, x, y)) {
		return 0;
	}
	return field->grid[y][x].letter;
}


int insert_letter_in_cell(GameField* field, int x, int y, unsigned char letter, int player_id) {
	if (!is_cell_coordinates_valid(field, x, y)) {
		return 0;
	}

	if (is_letter_valid(letter) && is_player_id_valid(player_id)) {
		field->grid[y][x].letter = letter;
		field->grid[y][x].player_id = player_id;
		return 1;
	}
	else {
		return 0;
	}
	
	//field extension !!!!НЕТ, ЭТУ ЧАСТЬ НУЖНО ПЕРЕНЕСТИ В БОЛЕЕ ОБЩУЮ ФУНКЦИЮ
	//if (x == field->width || y == field->height) {
	//	extend_field();
	//}
}

