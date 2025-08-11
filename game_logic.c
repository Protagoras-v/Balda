#include <stdio.h>
#include <stdlib.h>

#include "game_logic.h"
#include "dict.h"

#define FIELD_WIDTH 5
#define FIELD_HEIGHT 5


struct Cell {
	char letter;
	int player_id : 2; // 0 - клетка пустая, 1 - игрок, 2 - компьютер
};

struct GameField {
	Cell** grid;
	int width, height;
	int center_x, center_y;
};

struct Move {
	int x, y; //координаты вставляемой буквы
	char letter;
	Cell* word[100];
	int score;
	int player_id : 1;
};

struct GameSettings {
	int time_limit; //ms
	int difficulty : 2;
	int first_player : 1;
	char start_word[32];
};

struct Game {
	GameField* field;
	int current_player : 1;
	int scores[2]; 
	int game_finished : 1;
};


static GameField* field_create(Dictionary* dict) {
	GameField* field = malloc(sizeof(GameField));
	if (field == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для GameField\n");
		return NULL;
	}
	field->grid = malloc(FIELD_HEIGHT * sizeof(Cell*));
	if (field->grid == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для field->grid\n");
		free(field);
		return NULL;
	}

	for (int i = 0; i < FIELD_HEIGHT; i++) {
		field->grid[i] = malloc(FIELD_WIDTH * sizeof(Cell));

		//Если произошла ошибка при алокации, ощичаем все выделеное ранее
		if (field->grid[i] == NULL) {
			fprintf(stderr, "Ошибка при выделении памяти для field->grid[%d]\n", i);
			for (int j = 0; j < i; j++) {
				free(field->grid[j]);
			}
			free(field->grid);
			free(field);
			return NULL;
		}

		//инициализация 0-ми
		for (int j = 0; j < FIELD_WIDTH; j++) {
			field->grid[i][j].letter = 0;
			field->grid[i][j].player_id = 0;
		}
	}

	//вставляем стартовое слово
	char* word = dict_get_starting_word(dict);
	if (word == NULL) {
		return NULL;
	}
	for (int i = 0; i < 5; i++) {
		field->grid[2][i].letter = word[i];
	}

	field->height = FIELD_HEIGHT;
	field->width = FIELD_WIDTH;
	field->center_y = FIELD_HEIGHT / 2;
	field->center_x = FIELD_WIDTH / 2;

	return field;
}

static void field_destroy(GameField* field) {
	for (int i = 0; i < field->height; i++) {
		free(field->grid[i]);
	}
	free(field->grid);
	free(field);
}

//функция сделать_ход() принимает вставляемую букву, слово, которое игрок хочет выделить. Если все валидно, вызывается статичная функция move_apply(), 
// которая применяет Move к Game и возвращает какой-то код. В случае какой-либо ошибки в ходе возвращается другой код из enum

//!!!нужно также подумать о предсталвении word в виде клеток с координатами

Game* game_create(Dictionary* dict) {
	Game* game = malloc(sizeof(Game));
	if (game == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для game\n");
		return NULL;
	}
	game->field = field_create(dict);
	if (game->field == NULL) {
		fprintf(stderr, "Ошибка при создании поля\n");
		return NULL;
	}
	game->current_player = 0;
	game->game_finished = 0;
	game->scores[0] = 0;
	game->scores[1] = 0;
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
int game_get_cell(GameField* field, int x, int y, unsigned char* res) {
	if (!is_cell_coordinates_valid(field, x, y)) {
		return 0;
	}
	return field->grid[y][x].letter;
}


int game_insert_letter_into_cell(GameField* field, int x, int y, unsigned char letter, int player_id) {
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
