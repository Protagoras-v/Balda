#include <stdio.h>
#include <stdlib.h>

#include "game_logic.h"
#include "dict.h"
#include "common.h"

#define FIELD_SIZE 5
#define MAX_WORD_LEN 100

typedef struct WordCell {
	int y, x;
	char letter;
} WordCell;

struct Cell {
	char letter;
	unsigned int player_id : 2; // 0 - клетка пустая, 1 - игрок, 2 - компьютер
	unsigned int new : 1;
};

struct GameField {
	Cell** grid;
	int width, height;
	int center_x, center_y;
};

struct Move {
	int y, x; //координаты вставляемой буквы
	char letter;
	WordCell word[MAX_WORD_LEN];
	int word_len;
	int score;
};

struct GameSettings {
	int time_limit; //ms
	unsigned int difficulty : 2;
	unsigned int first_player : 1;
	char start_word[32];
};

struct Game {
	GameField* field;
	Move current_move;
	int max_wait_time;
	unsigned int difficulty : 2;
	unsigned int current_player : 2; //0 isnt used, because 0 is an empty cell
	int scores[2]; 
	unsigned int game_finished : 1;
};


static GameField* field_create(Dictionary* dict) {
	GameField* field = malloc(sizeof(GameField));
	if (field == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для GameField\n");
		return NULL;
	}
	field->grid = malloc(FIELD_SIZE * sizeof(Cell*));
	if (field->grid == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для field->grid\n");
		free(field);
		return NULL;
	}

	for (int i = 0; i < FIELD_SIZE; i++) {
		field->grid[i] = malloc(FIELD_SIZE * sizeof(Cell));

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
		for (int j = 0; j < FIELD_SIZE; j++) {
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

	field->height = FIELD_SIZE;
	field->width = FIELD_SIZE;
	field->center_y = FIELD_SIZE / 2;
	field->center_x = FIELD_SIZE / 2;

	return field;
}

static void field_destroy(GameField* field) {
	for (int i = 0; i < field->height; i++) {
		free(field->grid[i]);
	}
	free(field->grid);
	free(field);
}

static void field_set_letter(GameField* field, int y, int x, unsigned char letter, int player_id) {
	field->grid[y][x].letter = letter;
	field->grid[y][x].player_id = player_id;
	field->grid[y][x].new = 1;
}

static void confirm_letter(GameField* field, int y, int x) {
	field->grid[y][x].new = 0;
}

static void remove_letter(GameField* field, int y, int x) {
	field->grid[y][x].letter = '\0';
	field->grid[y][x].player_id = 0;
	field->grid[y][x].new = 0;
}


static bool is_cell_empty(GameField* field, int y, int x) {
	if (field->grid[y][x].letter == '\0') {
		return 1;
	}
	return 0;
}

static bool is_cell_coordinates_valid(GameField* field, int y, int x) {
	if (x >= field->width || y >= field->height || x < 0 || y < 0) {
		return 0;
	}
	return 1;
}


//Проверяет, есть ли рядом клетки с буквами
static bool is_letter_near(GameField* field, int y, int x) {
	//слева 
	if (x - 1 >= 0 && !is_cell_empty(field, y, x - 1)) {
		return 1;
	}
	//справа
	if (x + 1 < field->width && !is_cell_empty(field, y, x + 1)) {
		return 1;
	}
	//снизу
	if (y - 1 >= 0 && !is_cell_empty(field, y - 1, x)) {
		return 1;
	}
	//сверху
	if (y + 1 < field->height && !is_cell_empty(field, y + 1, x)) {
		return 1;
	}
	return 0;
}

//Проверяет, граничит ли новая Cell с последней вставленной в current_move.word 
static bool is_cell_next_to_previous(int y, int x, int prev_y, int prev_x) {
	//если клетка выше или ниже 
	if (x == prev_x) {
		if (y - 1 == prev_y || y + 1 == prev_y) {
			return 1;
		}
		return 0;
	}
	//если клетка справа или слева
	if (y == prev_y) {
		if (x - 1 == prev_x || x + 1 == prev_x) {
			return 1;
		}
		return 0;
	}
	//если по диагонали
	return 0;
}

static bool is_cell_in_word(WordCell word[MAX_WORD_LEN], int word_len, int y, int x) {
	for (int i = 0; i < word_len; i++) {
		if (word[i].y == y && word[i].x == x) {
			return 1;
		}
	}
	return 0;
}

static bool is_word_exists(Dictionary* dict, WordCell word[MAX_WORD_LEN], int word_len) {
	char word_str[MAX_WORD_LEN];
	int i = 0;
	while (i < word_len) {
		word_str[i] = word[i++].letter;
	}
	word_str[i] = '\0';
	
	return dict_word_exists(dict, word_str);
}

static StatusCode clear_word_selection(Move* move) {
	if (move->word_len == 0) {
		return FIELD_WORD_ALREADY_EMPTY;
	}
	for (int i = 0; i < move->word_len; i++) {
		move->word[i].y = -1;
		move->word[i].x = -1;
		move->word[i].letter = '\0';
		move->score--;
	}
	move->word_len = 0;
	return SUCCESS;
}


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
	//это должно браться из настроек.
	game->current_player = 1;
	//это должно браться из настроек.
	game->difficulty = 0;
	//это должно браться из настроек.
	game->max_wait_time = 10000;
	game->game_finished = 0;
	game->scores[0] = 0;
	game->scores[1] = 0;

	//Move
	game->current_move.letter = 0;
	game->current_move.score = 0;
	game->current_move.x = -1;
	game->current_move.y = -1;
	//инициализируем массив с координатами клеток, составляющих слово
	for (int i = 0; i < MAX_WORD_LEN; i++) {
		game->current_move.word[i].y = -1;
		game->current_move.word[i].x = -1;
		game->current_move.word[i].letter = '\0';
	}
	game->current_move.word_len = 0;
}

void game_destroy(Game* game);


//В SDL CP1251 Будет преобразовываться в UTF8, можно будет написать простую функцию конвертер, даже отдельно 33 случая рассмотреть if`ами
StatusCode game_get_cell(Game* game, int x, int y, unsigned char* res) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;

	return game->field->grid[y][x].letter;
}

StatusCode game_try_place_letter(Game* game, int y,int x, char letter) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (!is_it_ru_letter(letter)) return FIELD_INVALID_LETTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;
	if (!is_cell_empty(game->field, y, x)) return FIELD_CELL_OCCUPIED;
	if (!is_letter_near(game->field, y, x)) return FIELD_CELL_NOT_CONNECTED;

	else {
		Move* move = &game->current_move;
		move->letter = letter;
		move->x = x;
		move->y = y;
		field_set_letter(game->field, y, x, letter, game->current_player);
		return SUCCESS;
	}
}

StatusCode game_add_cell_into_word(Game* game, int y, int x) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;
	if (is_cell_empty(game->field, y, x)) return FIELD_CELL_EMPTY;

	Move* move = &game->current_move;

	//если первая буква в слове
	if (move->word_len == 0) {
		move->word[move->word_len].y = y;
		move->word[move->word_len].x = x;
		move->word[move->word_len].letter = game->field->grid[y][x].letter;
		move->word_len++;
		move->score++;
		return SUCCESS;
	}
	//если это не первая буква в слове, то нужно проверить, нет ли ее уже в слове и соединена ли она с предыдущей буквой
	else {
		int prev_y = move->word[move->word_len - 1].y;
		int prev_x = move->word[move->word_len - 1].x;
		if (is_cell_next_to_previous(y, x, prev_y, prev_x) && !is_cell_in_word(move->word, move->word_len, y, x)) {
			move->word[move->word_len].y = y;
			move->word[move->word_len].x = x;
			move->word[move->word_len].letter = game->field->grid[y][x].letter;
			move->word_len++;
			move->score++;
			return SUCCESS;
		}
		else {
			return FIELD_INVALID_CELL;
		}
	}
}

StatusCode game_cancel_word_selection(Game* game) {
	if (game == NULL) return ERROR_NULL_POINTER;
	return clear_word_selection(&game->current_move);
}

//clear letter and word selection if it not cleared already
StatusCode game_clear_move(Game* game) {
	if (game == NULL) return ERROR_NULL_POINTER;

	Move* move = &game->current_move;
	clear_word_selection(move);
	remove_letter(game->field, move->y, move->x);
	move->letter = '\0';
	move->y = -1;
	move->x = -1;

	return SUCCESS;
}

StatusCode game_confirm_move(Game* game, Dictionary* dict) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (dict == NULL) return ERROR_NULL_POINTER;

	Move* move = &game->current_move;
	if (is_word_exists(dict, move->word, move->word_len)
		&& is_cell_in_word(move->word, move->word_len, move->y, move->x)) { //is new letter in the word
		//apply changes
		game->scores[game->current_player] = move->score;
		confirm_letter(game->field, move->y, move->x);
		game_clear_move(game);
		//pass the turn
		game->current_player = (game->current_player == 1) ? 2 : 1;
	}
	else {
		return GAME_INVALID_WORD;
	}
}


int game_get_player_id(Game* game) {
	if (game == NULL) return 0;
	return game->current_player;
}



void print_field(Game* game) {
	for (int i = 0; i < FIELD_SIZE; i++) {
		for (int j = 0; j < FIELD_SIZE; j++) {
			char c = game->field->grid[i][j].letter;
			if (c == 0) {
				printf("[ ]");
			}
			else{
				printf("[%c]", c);
			}	
		}
		printf("\n");
	}
}


//добавить поля с поставленными словами и соответствующие проверки
//изменить то, как выбирается начальное слово (повторное чтение файла)