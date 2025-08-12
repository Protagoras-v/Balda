#include <stdio.h>
#include <stdlib.h>

#include "game_logic.h"
#include "dict.h"

#define FIELD_WIDTH 5
#define FIELD_HEIGHT 5
#define MAX_WORD_LEN 100

struct Cell {
	char letter;
	int player_id : 2; // 0 - ������ ������, 1 - �����, 2 - ���������
};

struct GameField {
	Cell** grid;
	int width, height;
	int center_x, center_y;
};

struct Move {
	int x, y; //���������� ����������� �����
	char letter;
	//���������� �����, ������������ �����
	int word[MAX_WORD_LEN][2]; // [0] - y, [1] - x
	int word_len;
	int score;
};

struct GameSettings {
	int time_limit; //ms
	int difficulty : 2;
	int first_player : 1;
	char start_word[32];
};

struct Game {
	GameField* field;
	Move current_move;
	int max_wait_time;
	int difficulty : 2;
	int current_player : 1;
	int scores[2]; 
	int game_finished : 1;
};


static GameField* field_create(Dictionary* dict) {
	GameField* field = malloc(sizeof(GameField));
	if (field == NULL) {
		fprintf(stderr, "������ ��� ��������� ������ ��� GameField\n");
		return NULL;
	}
	field->grid = malloc(FIELD_HEIGHT * sizeof(Cell*));
	if (field->grid == NULL) {
		fprintf(stderr, "������ ��� ��������� ������ ��� field->grid\n");
		free(field);
		return NULL;
	}

	for (int i = 0; i < FIELD_HEIGHT; i++) {
		field->grid[i] = malloc(FIELD_WIDTH * sizeof(Cell));

		//���� ��������� ������ ��� ��������, ������� ��� ��������� �����
		if (field->grid[i] == NULL) {
			fprintf(stderr, "������ ��� ��������� ������ ��� field->grid[%d]\n", i);
			for (int j = 0; j < i; j++) {
				free(field->grid[j]);
			}
			free(field->grid);
			free(field);
			return NULL;
		}

		//������������� 0-��
		for (int j = 0; j < FIELD_WIDTH; j++) {
			field->grid[i][j].letter = 0;
			field->grid[i][j].player_id = 0;
		}
	}

	//��������� ��������� �����
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

static bool is_letter_valid(unsigned char c) {
	if ((c >= '�' && c <= '�') || (c >= '�' && c <= '�') || c == '�' || c == '�') {
		return 1;
	}
	return 0;
}

static bool is_player_id_valid(int id) {
	if (id == 1 || id == 2) {
		return 1;
	}
	return 0;
}

//���������, ���� �� ����� ������ � �������
static bool is_letter_near(GameField* field, int y, int x) {
	//����� 
	if (x - 1 >= 0 && !is_cell_empty(field, y, x - 1)) {
		return 1;
	}
	//������
	if (x + 1 < field->width && !is_cell_empty(field, y, x + 1)) {
		return 1;
	}
	//�����
	if (y - 1 >= 0 && !is_cell_empty(field, y - 1, x)) {
		return 1;
	}
	//������
	if (y + 1 < field->height && !is_cell_empty(field, y + 1, x)) {
		return 1;
	}
	return 0;
}

//���������, �������� �� ����� Cell � ��������� ����������� � current_move.word 
static bool is_cell_next_to_previous(int y, int x, int prev_y, int prev_x) {
	//���� ������ ���� ��� ���� 
	if (x == prev_x) {
		if (y - 1 == prev_y || y + 1 == prev_y) {
			return 1;
		}
		return 0;
	}
	//���� ������ ������ ��� �����
	if (y == prev_y) {
		if (x - 1 == prev_x || x + 1 == prev_x) {
			return 1;
		}
		return 0;
	}
	//���� �� ���������
	return 0;
}

static bool is_cell_in_word(int word[MAX_WORD_LEN][2], int word_len, int x, int y) {
	for (int i = 0; i < word_len; i++) {
		if (word[i][0] == y && word[i][1] == x) {
			return 1;
		}
	}
	return 0;
}

static StatusCode field_set_letter(GameField* field, int y, int x, unsigned char letter, int player_id) {
	field->grid[y][x].letter = letter;
	field->grid[y][x].player_id = player_id;
	return SUCCESS;
}


Game* game_create(Dictionary* dict) {
	Game* game = malloc(sizeof(Game));
	if (game == NULL) {
		fprintf(stderr, "������ ��� ��������� ������ ��� game\n");
		return NULL;
	}
	game->field = field_create(dict);
	if (game->field == NULL) {
		fprintf(stderr, "������ ��� �������� ����\n");
		return NULL;
	}
	//��� ������ ������� �� ��������.
	game->current_player = 0;
	//��� ������ ������� �� ��������.
	game->difficulty = 0;
	//��� ������ ������� �� ��������.
	game->max_wait_time = 10000;
	game->game_finished = 0;
	game->scores[0] = 0;
	game->scores[1] = 0;

	//Move
	game->current_move.letter = 0;
	game->current_move.score = 0;
	game->current_move.x = -1;
	game->current_move.y = -1;
	//�������������� 0-��� ������ � ������������ ������, ������������ �����
	for (int i = 0; i < MAX_WORD_LEN; i++) {
		game->current_move.word[i][0] = 0;
		game->current_move.word[i][1] = 0;
	}
	game->current_move.word_len = 0;
}


//� SDL CP1251 ����� ����������������� � UTF8, ����� ����� �������� ������� ������� ���������, ���� �������� 33 ������ ����������� if`���
StatusCode game_get_cell(GameField* field, int x, int y, unsigned char* res) {
	if (!is_cell_coordinates_valid(field, y, x)) return FIELD_INVALID_COORDINATES;

	return field->grid[y][x].letter;
}

StatusCode game_try_place_letter(Game* game, int y,int x, char letter) {
	if (!is_letter_valid(letter)) return FIELD_INVALID_LETTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;
	if (!is_cell_empty(game->field, y, x)) return FIELD_CELL_OCCUPIEDL;
	if (!is_letter_near(game->field, y, x)) return FIELD_CELL_NOT_CONNECTED;

	else {
		Move* move = &game->current_move;
		move->letter = letter;
		move->x = x;
		move->y = y;
		return SUCCESS;
	}
}

StatusCode game_add_cell_into_word(Game* game, int x, int y) {
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;
	if (is_cell_empty(game->field, y, x)) return FIELD_CELL_EMPTY;

	Move* move = &game->current_move;

	//���� ������ ����� � �����
	if (move->word_len == 0) {
		move->word[move->word_len][0] = y;
		move->word[move->word_len][1] = x;
		move->word_len++;
		return SUCCESS;
	}
	//���� ��� �� ������ ����� � �����, �� ����� ���������, ��� �� �� ��� � ����� � ��������� �� ��� � ���������� ������
	else {
		int prev_y = move->word[move->word_len - 1][0];
		int prev_x = move->word[move->word_len - 1][1];
		if (is_cell_next_to_previous(y, x, prev_y, prev_x) && !is_cell_in_word(move->word, move->word_len, x, y)) {
			move->word[move->word_len][0] = y;
			move->word[move->word_len][1] = x;
			move->word_len++;
			return SUCCESS;
		}
		else {
			printf("��� ������ �� �������� � ����������\n");
			return FIELD_INVALID_CELL;
		}
	}
}

bool game_confirm_move(Game* game) {

}

bool game_cancel_move(Game* game);