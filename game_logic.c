#define _CRT_SECURE_NO_WARNINGS

#include "game_logic.h"
#include "dict.h"
#include "common.h"



typedef struct User {
	char* username;
	int username_len;
	int score;
} User;

typedef struct PlayerWords {
	char** words;
	int capacity;
	int count;
} PlayerWords;

struct Leaderboard {
	int count;
	User users[LEADERBOARD_SIZE];
};

struct GameSettings {
	unsigned int time_limit; //ms
	unsigned int difficulty : 2;
	unsigned int first_player : 2; // 0 isn`t used, because 0 is an empty cell
};

struct Game {
	GameSettings* settings;
	GameField* field;
	Move current_move;
	unsigned int current_player : 2; //0 isnt used, because 0 is an empty cell
	int scores[2]; 
	unsigned char game_finished : 1;

	PlayerWords player1_words;
	PlayerWords player2_words;
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
	char word[6];
	if (dict_get_starting_word(dict, word) == ERROR_OUT_OF_MEMORY) {
		fprintf(stderr, "Ошибка при формировании стартового слова!\n");
		return NULL;
	}
	for (int i = 0; i < 5; i++) {
		field->grid[2][i].letter = word[i];
	}

	field->height = FIELD_SIZE;
	field->width = FIELD_SIZE;

	return field;
}

static void field_destroy(GameField* field) {
	if (field == NULL) return;
	for (int i = 0; i < field->height; i++) {
		free(field->grid[i]);
	}
	free(field->grid);
	free(field);
}

static GameField* field_make_copy(GameField* field) {
	GameField* copy = malloc(sizeof(GameField));
	if (copy == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для GameField\n");
		return NULL;
	}
	copy->grid = malloc(field->height * sizeof(Cell*));
	if (copy->grid == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для field->grid\n");
		free(copy);
		return NULL;
	}

	for (int i = 0; i < field->height; i++) {
		copy->grid[i] = malloc(field->width * sizeof(Cell));

		if (copy->grid[i] == NULL) {
			fprintf(stderr, "Ошибка при выделении памяти для field->grid[%d]\n", i);
			for (int j = 0; j < i; j++) {
				free(copy->grid[j]);
			}
			free(copy->grid);
			free(copy);
			return NULL;
		}

		for (int j = 0; j < field->width; j++) {
			copy->grid[i][j].letter = field->grid[i][j].letter;
			copy->grid[i][j].player_id = field->grid[i][j].player_id;
		}
	}

	copy->height = field->height;
	copy->width = field->width;


	return copy;
}

static void field_set_letter(GameField* field, int y, int x, unsigned char letter, int player_id) {
	field->grid[y][x].letter = letter;
	field->grid[y][x].player_id = player_id;
	field->grid[y][x].new = 1;
}

static void field_confirm_letter(GameField* field, int y, int x) {
	field->grid[y][x].new = 0;
}

static void field_remove_letter(GameField* field, int y, int x) {
	field->grid[y][x].letter = '\0';
	field->grid[y][x].player_id = 0;
	field->grid[y][x].new = 0;
}

//Is the selected cell connected to the previous one in current_move.word 
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

//add word to the list of player`s words
static StatusCode add_player_word(PlayerWords* p, char* word) {
	size_t size = strlen(word) + 1;
	p->words[p->count] = malloc(sizeof(char) * size);
	if (p->words[p->count] == NULL) {
		return ERROR_OUT_OF_MEMORY;
	}
	else {
		memcpy(p->words[p->count], word, sizeof(char) * size);
		p->count++;

		if (p->count == p->capacity) {
			char** new_words = realloc(p->words, sizeof(char*) * p->capacity * 2);
			if (new_words == NULL) return ERROR_OUT_OF_MEMORY;
			p->words = new_words;
			p->capacity *= 2;
		}
		
		return SUCCESS;
	}
}

//remove last word from player`s words list, its used for minimax
static void remove_from_player_words(PlayerWords* p) {
	p->count--;
	free(p->words[p->count]);
}


static int part_quick_sort(User* arr, int l, int r) {
	int pivot = arr[(l + r) / 2].score;
	while (l <= r) {
		while (arr[l].score > pivot) l++;
		while (arr[r].score < pivot) r--;
		if (l <= r) {
			int temp = arr[l].score;
			arr[l].score = arr[r].score;
			arr[r].score = temp;

			char buffer[MAX_WORD_LEN + 1];
			strcpy(buffer, arr[l].username);
			strcpy(arr[l].username, arr[r].username);
			strcpy(arr[r].username, buffer);

			l++;
			r--;
		}
	}

	return l;
}

static void quick_sort(User* arr, int l, int r) {
	if (l >= r) return;
	int rStart = part_quick_sort(arr, l, r);
	quick_sort(arr, l, rStart - 1);
	quick_sort(arr, rStart, r);
}

static void leaderboard_sort(Leaderboard* lb) {
	User* users = lb->users;
	int c = lb->count;

	if (c > 2) {
		quick_sort(users, 0, c - 1);
	}
}

static void leaderboard_add_new(Leaderboard* lb, const char* user, int score) {
	if (lb->count == LEADERBOARD_SIZE) {
		//rewrite 50th place
		memcpy(lb->users[lb->count - 1].username, user, strlen(user) + 1);
		lb->users[lb->count - 1].score = score;
	}
	else {
		size_t len = strlen(user) + 1;
		lb->users[lb->count].username = malloc(len);
		if (lb->users[lb->count].username == NULL) {
			fprintf(stderr, "Ошибка при выделении памяти в leaderboard_add_new()\n");
			return;
		}
		memcpy(lb->users[lb->count].username, user, len);
		lb->users[lb->count].score = score;
		lb->count++;
	}
}


//Each new letter must be connected to others
bool is_letter_near(GameField* field, int y, int x) {
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

bool is_cell_empty(GameField* field, int y, int x) {
	if (field->grid[y][x].letter == '\0') {
		return 1;
	}
	return 0;
}

bool is_cell_coordinates_valid(GameField* field, int y, int x) {
	if (x >= field->width || y >= field->height || x < 0 || y < 0) {
		return 0;
	}
	return 1;
}

bool is_word_used(Game* game, char* buffer) {
	PlayerWords p1 = game->player1_words;
	PlayerWords p2 = game->player2_words;

	for (int i = 0; i < p1.count; i++) {
		if (strcmp(p1.words[i], buffer) == 0) {
			return true;
		}
	}
	for (int i = 0; i < p2.count; i++) {
		if (strcmp(p2.words[i], buffer) == 0) {
			return true;
		}
	}

	if (game->field->width >= MAX_WORD_LEN) {
		fprintf(stderr, "Длина поля больше, чем максимальный размер слова!\n");
		return false;
	}
	//also check the initial word
	char st_word[MAX_WORD_LEN + 1];
	int midY = game->field->height / 2;
	int x = 0;
	while (x < game->field->width) {
		st_word[x] = game->field->grid[midY][x++].letter;
	}
	st_word[x] = '\0';
	if (strcmp(st_word, buffer) == 0) {
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------------------------------
//---------------------------------------------INTERFACES----------------------------------------------
//-----------------------------------------------------------------------------------------------------



Game* game_create(GameSettings* settings, Dictionary* dict) {
	Game* game = malloc(sizeof(Game));
	if (game == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для game\n");
		return NULL;
	}
	game->field = field_create(dict);
	if (game->field == NULL) {
		fprintf(stderr, "Ошибка при создании поля\n");
		free(game);
		return NULL;
	}
	game->settings = settings;
	game->game_finished = 0;
	game->scores[0] = 0;
	game->scores[1] = 0;

	//players words
	game->player1_words.words = malloc(sizeof(char*) * STARTING_PLAYER_WORDS_CAPACITY);
	if (game->player1_words.words == NULL) {
		printf("Ошибка при выделении памяти для player_words!\n");
		free(game->field);
		free(game);
		return NULL;
	}

	game->player2_words.words = malloc(sizeof(char*) * STARTING_PLAYER_WORDS_CAPACITY);
	if (game->player2_words.words == NULL) {
		printf("Ошибка при выделении памяти для player_words!\n");
		free(game->player1_words.words);
		free(game->field);
		free(game);
		return NULL;
	}

	game->player1_words.capacity = STARTING_PLAYER_WORDS_CAPACITY;
	game->player2_words.capacity = STARTING_PLAYER_WORDS_CAPACITY;
	game->player1_words.count = 0;
	game->player2_words.count = 0;

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

	return game;
}

void game_destroy(Game** game_) {
	Game* game = *game_;
	if (game == NULL) return;
	field_destroy(game->field);
	game->settings = NULL;

	//p words
	for (int i = 0; i < game->player1_words.count; i++) {
		free(game->player1_words.words[i]);
	}
	free(game->player1_words.words);

	for (int i = 0; i < game->player2_words.count; i++) {
		free(game->player2_words.words[i]);
	}
	free(game->player2_words.words);

	free(game);
	*game_ = NULL;
}


StatusCode game_try_place_letter(Game* game, int y,int x, char letter) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (!is_it_ru_letter(letter)) return FIELD_INVALID_LETTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;
	if (!is_cell_empty(game->field, y, x)) return FIELD_CELL_OCCUPIED;
	if (!is_letter_near(game->field, y, x)) return FIELD_CELL_NOT_CONNECTED;

	else {
		Move* move = &game->current_move;
		move->letter = to_lower(letter);
		move->x = x;
		move->y = y;
		field_set_letter(game->field, move->y, move->x, move->letter, game->current_player);
		return SUCCESS;
	}
}


StatusCode game_add_cell_into_word(Game* game, int y, int x) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;
	if (is_cell_empty(game->field, y, x)) return FIELD_CELL_EMPTY;

	Move* move = &game->current_move;

	//if letter is not placed yet
	if (move->letter == '\0') {
		return GAME_LETTER_IS_MISSING;
	}

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
		//!!!!!!!!!!!!!!!!вот это было изменено!!!!!!!!
		if (is_cell_in_word(move->word, move->word_len, y, x)) {
			return FIELD_CELL_IS_USED;
		}
		if (is_cell_next_to_previous(y, x, prev_y, prev_x)) {
			move->word[move->word_len].y = y;
			move->word[move->word_len].x = x;
			move->word[move->word_len].letter = game->field->grid[y][x].letter;
			move->word_len++;
			move->score++;
			return SUCCESS;
		}
		else {
			return FIELD_CELL_NOT_CONNECTED;
		}
	}
}

StatusCode game_cancel_word_selection(Game* game) {
	if (game == NULL) return ERROR_NULL_POINTER;
	printf("yes\n");
	return clear_word_selection(&game->current_move);
}

//clear letter and word selection if it not cleared already
StatusCode game_clear_move(Game* game) {
	if (game == NULL) return ERROR_NULL_POINTER;

	Move* move = &game->current_move;
	clear_word_selection(move);
	field_remove_letter(game->field, move->y, move->x);
	move->letter = '\0';
	move->y = -1;
	move->x = -1;

	return SUCCESS;
}


StatusCode game_confirm_move(Game* game, Dictionary* dict) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (dict == NULL) return ERROR_NULL_POINTER;

	Move* move = &game->current_move;

	if (move->word_len < MIN_WORD_LEN) {
		clear_word_selection(move);
		return GAME_WORD_EMPTY;
	}
	char buffer[MAX_WORD_LEN + 1];
	WordCell_to_char(move->word, buffer, move->word_len);

	if (!is_cell_in_word(move->word, move->word_len, move->y, move->x)) {
		clear_word_selection(move);
		return GAME_WORD_DOESNT_CONTAIN_LETTER;
	}
	if (is_word_used(game, buffer)) {
		game_clear_move(game);
		return GAME_WORD_USED;
	}

	if (dict_word_exists(dict, buffer)) {
		//apply changes
		game->scores[game->current_player - 1] = move->score;
		field_confirm_letter(game->field, move->y, move->x);

		clear_word_selection(move);
		move->letter = '\0';
		move->y = -1;
		move->x = -1;

		//add player`s word
		if (game->current_player == 1) {
			StatusCode code = add_player_word(&game->player1_words, buffer);
			if (code != SUCCESS) return code;
		}
		else if (game->current_player == 2) {
			StatusCode code = add_player_word(&game->player2_words, buffer);
			if (code != SUCCESS) return code;
		}
		
		//pass the turn
		move->score = (game->current_player == 1) ? game->scores[1] : game->scores[0];
		game->current_player = (game->current_player == 1) ? 2 : 1;

		return SUCCESS;
	}
	else {
		return GAME_INVALID_WORD;
	}
}


Leaderboard* game_leaderboard_init() {
	FILE* file = fopen("leaderboard.txt", "r");
	if (file == NULL) {
		file = fopen("leaderboard.txt", "w");
		if (file != NULL) {
			fclose(file);
		}
		file = fopen("leaderboard.txt", "r");
	}

	Leaderboard* lb = malloc(sizeof(Leaderboard));
	if (lb == NULL) {
		fclose(file);
		fprintf(stderr, "Ошибка при выделении памяти для leaderboard\n");
		return NULL;
	}
	lb->count = 0;

	char buffer[255];
	while (fgets(buffer, sizeof(buffer), file) != NULL && lb->count < LEADERBOARD_SIZE) {
		bool f = 0;
		char* t;
		t = strchr(buffer, '\n');
		if (t != NULL) *t = '\0';
		buffer[244] = '\0';

		for (int i = 0; buffer[i]; i++) {
			if (buffer[i] == ' ' && !f) {
				buffer[i] = '\0';

				t = &buffer[i + 1];

				//space skip
				while (*t == ' ') t++;

				//situation wherer a spaces is followed by '\0', so there isn`t score part and this strings is invalid
				if (*t == '\0') {
					f = 0;
				}
				else {
					f = 1;
				}
				break;
			}
		}

		//Skip an invalid line
		if (!f) {
			continue;
		}
		else {
			char* e;
			int score = (int)strtol(t, &e, 10);
			//if there is any non-digit symbols after score, the line will be considered invalid
			if (*e != ' ' && *e != '\0') {
				continue;
			}

			size_t username_len = strlen(buffer);
			lb->users[lb->count].username = malloc(username_len + 1); //the buffer pointer contains only the first part (username), because gap was replaced by '\0'
			if (lb->users[lb->count].username == NULL) {
				fclose(file);
				free(lb);
				fprintf(stderr, "Ошибка при выделении памяти в game_leaderboard_init()\n");
				return NULL;
			}

			memcpy(lb->users[lb->count].username, buffer, username_len + 1);
			lb->users[lb->count].score = score;
			lb->users[lb->count].username_len = username_len;
			lb->count++;
		}
	}

	fclose(file);

	return lb;
}

void game_leaderboard_destroy(Leaderboard* lb) {
	if (lb == NULL) return;
	for (int i = 0; i < lb->count; i++) {
		free(lb->users[i].username);
	}
	free(lb);
}


StatusCode game_add_into_leaderboard(Leaderboard* lb, Game* game, const char* username) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (lb == NULL) return ERROR_NULL_POINTER;

	int score = game->scores[0]; //0 because its a human and 1 is ai (obviously only human can be added into lb)
	if (!game_is_enough_score_for_lb(game, lb)) return GAME_LOW_SCORE;
	leaderboard_add_new(lb, username, score);
	leaderboard_sort(lb);

	//rewrite entire file with a sorted leaderboard
	FILE* file = fopen("leaderboard.txt", "w");
	if (file == NULL) return ERROR_FILE_NOT__FOUND;
	for (int i = 0; i < lb->count; i++) {
		fprintf(file, "%s %d\n", lb->users[i].username, lb->users[i].score);
	}
	fclose(file);
	return SUCCESS;
}


bool game_is_enough_score_for_lb(Game* game, Leaderboard* lb) {
	if (lb->count < LEADERBOARD_SIZE)
		return true;
	else {
		//if its higher than last place
		if (game->scores[0] > lb->users[lb->count - 1].score) {
			return true;
		}
	}
	return false;
}

Game* game_make_copy(Game* game) {
	Game* copy = malloc(sizeof(Game));
	if (copy == NULL) {
		fprintf(stderr, "Не удалось создать копию Game (ошибка при выделении памяти)\n");
		return NULL;
	}
	copy->current_move = game->current_move;
	copy->current_player = game->current_player;
	copy->scores[0] = game->scores[0];
	copy->scores[1] = game->scores[1];

	copy->settings = game_init_settings();
	if (copy->settings == NULL) {
		fprintf(stderr, "Не удалось создать копию Game (ошибка при выделении памяти)\n");
		free(copy);
		return NULL;
	}
	copy->settings->difficulty = game->settings->difficulty;
	copy->settings->first_player = game->settings->first_player;
	copy->settings->time_limit = game->settings->time_limit;

	copy->field = field_make_copy(game->field);
	if (copy->field == NULL) {
		fprintf(stderr, "Не удалось создать копию Game (ошибка при выделении памяти)\n");
		free(copy->settings);
		free(copy);
		return NULL;
	}

	copy->player1_words.capacity = game->player1_words.capacity;
	copy->player1_words.count = game->player1_words.count;
	copy->player1_words.words = malloc(sizeof(char*) * copy->player1_words.capacity);
	for (int i = 0; i < copy->player1_words.count; i++) {
		size_t len = strlen(game->player1_words.words[i]) + 1;
		copy->player1_words.words[i] = malloc(sizeof(char) * len);
		//free all words which were already allocated 
		if (copy->player1_words.words[i] == NULL) {
			while (i > 0) {
				i--;
				free(copy->player1_words.words[i]);
			}
			free(copy->player1_words.words);
			free(copy->settings);
			free(copy->field);
			free(copy);
			fprintf(stderr, "Не удалось создать копию Game (ошибка при выделении памяти)\n");
			return NULL;
		}
		memcpy(copy->player1_words.words[i], game->player1_words.words[i], len * sizeof(char));
	}

	copy->player2_words.capacity = game->player2_words.capacity;
	copy->player2_words.count = game->player2_words.count;
	copy->player2_words.words = malloc(sizeof(char*) * copy->player2_words.capacity);
	if (copy->player2_words.words == NULL) {
		size_t i = copy->player1_words.count;
		while (i > 0) {
			i--;
			free(copy->player1_words.words[i]);
		}
		free(copy->player1_words.words);
		free(copy->settings);
		free(copy->field);
		free(copy);
		fprintf(stderr, "Не удалось создать копию Game (ошибка при выделении памяти)\n");
		return NULL;
	}
	for (int i = 0; i < copy->player2_words.count; i++) {
		int len = strlen(game->player2_words.words[i]) + 1;
		copy->player2_words.words[i] = malloc(sizeof(char) * len);
		//free all words which were already allocated 
		if (copy->player2_words.words[i] == NULL) {
			while (i > 0) {
				i--;
				free(copy->player2_words.words[i]);
			}
			i = copy->player1_words.count;
			while (i > 0) {
				i--;
				free(copy->player1_words.words[i]);
			}
			free(copy->player1_words.words);
			free(copy->player2_words.words);
			free(copy->settings);
			free(copy);
			fprintf(stderr, "Не удалось создать копию Game (ошибка при выделении памяти)\n");
			return NULL;
		}
		memcpy(copy->player2_words.words[i], game->player2_words.words[i], len * sizeof(char));
	}

	return copy;
}


//for ai moves, apply move, switch player, increase score
StatusCode game_apply_generated_move(Game* game, Move move) {
	if (game == NULL) return ERROR_NULL_POINTER;

	//we need to add the word to player_words, because otherwise the greedy algorithm might use the same word twice
	char word[MAX_WORD_LEN];
	WordCell_to_char(move.word, word, move.word_len);
	if (game->current_player == 1) {
		StatusCode code = add_player_word(&game->player1_words, word);
		if (code != SUCCESS) return code;
	}
	else if (game->current_player == 2) {
		StatusCode code = add_player_word(&game->player2_words, word);
		if (code != SUCCESS) return code;
	}

	field_set_letter(game->field, move.y, move.x, move.letter, game_get_player_id(game));
	game->scores[game->current_player - 1] += move.word_len;
	game->current_player = (game->current_player == 1) ? 2 : 1;

	//move
	game->current_move.letter = '\0';
	game->current_move.y = -1;
	game->current_move.x = -1;
	game->current_move.word_len = 0;
	game->current_move.score = game->current_player == 1 ? game->scores[1] : game->scores[0]; //set next player`s starting score

	return SUCCESS;
}

//for ai moves, undo move, switch player back and decrease score
StatusCode game_undo_generated_move(Game* game, Move move) {
	if (game == NULL) return ERROR_NULL_POINTER;

	game->current_player = (game->current_player == 1) ? 2 : 1;

	//remove word from player_words
	game->current_player == 1 ? remove_from_player_words(&game->player1_words) : remove_from_player_words(&game->player2_words);
	game->scores[game->current_player - 1] -= move.word_len;
	field_remove_letter(game->field, move.y, move.x);

	game->current_move.letter = '\0';
	game->current_move.y = -1;
	game->current_move.x = -1;
	game->current_move.word_len = 0;
	game->current_move.score -= move.word_len;
	game->current_move.word_len = 0;

	return SUCCESS;
}

StatusCode game_confirm_generated_move(Game* game) {
	Move* move = &game->current_move;

	field_confirm_letter(game->field, move->y, move->x);
	clear_word_selection(move);
	move->letter = '\0';
	move->y = -1;
	move->x = -1;

	//pass the turn
	move->score = (game->current_player == 1) ? game->scores[1] : game->scores[0];
	game->current_player = (game->current_player == 1) ? 2 : 1;

	return SUCCESS;
}


//---------------------------------
//---------------GET---------------
//---------------------------------

//В SDL CP1251 Будет преобразовываться в UTF8, можно будет написать простую функцию конвертер, даже отдельно 33 случая рассмотреть if`ами
StatusCode game_get_cell_letter(Game* game, int y, int x, unsigned char* res) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (!is_cell_coordinates_valid(game->field, y, x)) return FIELD_INVALID_COORDINATES;

	*res = game->field->grid[y][x].letter;
	return SUCCESS;
}

StatusCode game_get_field_size(Game* game, int* h, int* w) {
	if (game == NULL) {
		fprintf(stderr, "Game = NULL!!!\n");
		return ERROR_NULL_POINTER;
	}
	*h = game->field->height;
	*w = game->field->width;
	return SUCCESS;
}

GameField* game_get_field(Game* game) {
	if (game == NULL) return NULL;
	return game->field;
}

StatusCode game_get_score(Game* game, int id, int* score) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (id < 1 || id > 2) return GAME_INVALID_ID;
	*score = game->scores[id - 1];
	return SUCCESS;
}

StatusCode game_get_player_words(Game* game, int player_id, char*** words, int* count) {
	if (game == NULL) return ERROR_NULL_POINTER;

	if (player_id == 1) {
		*words = game->player1_words.words;
		*count = game->player1_words.count;
		return SUCCESS;
	}
	else if (player_id == 2) {
		*words = game->player2_words.words;
		*count = game->player2_words.count;
		return SUCCESS;
	}

	return GAME_INVALID_ID;
}

StatusCode game_get_word(Game* game, char word[]) {
	if (game == NULL) return ERROR_NULL_POINTER;

	Move* move = &game->current_move;

	if (move->word_len < 1) {
		clear_word_selection(move);
		return GAME_WORD_EMPTY;
	}
	char buffer[MAX_WORD_LEN + 1];
	WordCell_to_char(move->word, buffer, move->word_len);
	strncpy(word, buffer, strlen(buffer) + 1);

	return SUCCESS;
}

StatusCode game_get_word_from_move(Move move, char* word) {
	if (move.word_len < 1) {
		clear_word_selection(&move);
		return GAME_WORD_EMPTY;
	}
	char buffer[MAX_WORD_LEN + 1];
	WordCell_to_char(move.word, buffer, move.word_len);
	strncpy(word, buffer, strlen(buffer) + 1);

	return SUCCESS;
}


StatusCode game_get_winner(Game* game, int* winner_id) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (game->scores[0] > game->scores[1]) *winner_id = 1;
	if (game->scores[0] < game->scores[1])  *winner_id = 2;
	if (game->scores[0] == game->scores[1])  *winner_id = 0; //draw
	return SUCCESS;
}

StatusCode game_get_leaderboard(Leaderboard* lb, char usernames[][LEADERBOARD_MAX_NAME_LEN], int scores[], int* size) {
	if (lb == NULL) return ERROR_NULL_POINTER;
	for (int i = 0; i < lb->count && i < LEADERBOARD_SIZE; i++) {
		strcpy(usernames[i], lb->users[i].username);
		scores[i] = lb->users[i].score;
	}
	*size = lb->count;
	return SUCCESS;
}

int game_get_player_id(Game* game) {
	return game->current_player;
}

int game_get_difficulty(Game* game) {
	return game->settings->difficulty;
}
unsigned long long game_get_time_limit(Game* game) {
	return (unsigned long long) game->settings->time_limit;
}

//--------------------------------------
//---------------SETTINGS---------------
//--------------------------------------

GameSettings* game_init_settings() {
	GameSettings* settings = malloc(sizeof(GameSettings));
	if (settings == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти для GameSettings\n");
		return NULL;
	}
	settings->difficulty = DEFAULT_DIFFICULTY;
	settings->time_limit = DEFAULT_MAX_TIME;
	settings->first_player = DEFAULT_FIRST_PLAYER;

	return settings;
}

//ms
StatusCode game_set_timelimit(GameSettings* settings, int time) {
	if (settings == NULL) return ERROR_NULL_POINTER;

	if (time < 1) return GAME_INVALID_TIME;
	else settings->time_limit = time;
	return SUCCESS;
}

StatusCode game_set_difficulty(GameSettings* settings, int difficulty) {
	if (settings == NULL) return ERROR_NULL_POINTER;

	if (difficulty < 0 || difficulty > 2) return GAME_INVALID_DIFFICULTY;
	else settings->difficulty = difficulty;
	return SUCCESS;
}

StatusCode game_set_first_player(GameSettings* settings, int first_player) {
	if (settings == NULL) return ERROR_NULL_POINTER;

	if (first_player < 1 || first_player > 2) return GAME_INVALID_FIRST_PLAYER;
	else settings->first_player = first_player;
	return SUCCESS;
}

unsigned int game_get_settings_difficulty(GameSettings* settings) {
	if (settings == NULL) {
		fprintf(stderr, "NULL в game_get_settings_difficulty()\n");
		return -1;
	}
	return settings->difficulty;
}
unsigned int game_get_settings_timelimit(GameSettings* settings) {
	if (settings == NULL) {
		fprintf(stderr, "NULL в game_get_settings_timelimit()\n");
		return -1;
	}
	return settings->time_limit;
}
unsigned int game_get_settings_first_player(GameSettings* settings) {
	if (settings == NULL) {
		fprintf(stderr, "NULL в game_get_settings_first_player()\n");
		return -1;
	}
	return settings->first_player;
}



//----------------------------------------
//---------------SAVE FILES---------------
//----------------------------------------

// 
// 'BALD' - 4 bytes (magic)

// GAME STATE  
// [UNSIGNED CHAR current_player] - 1 byte (0 или 1)
// [UNSIGNED SHORT score_player] - 2 bytes  
// [UNSIGNED SHORT score_computer] - 2 bytes

// SETTINGS
// [UNSIGNED SHORT time_limit] - 2 bytes (SEC)
// [UNSIGNED CHAR difficulty] - 1 byte (0-2)
// [UNSIGNED CHAR first_player] - 1 byte

// FIELD
// [UNSIGNED CHAR width] - 1 byte (max 255)
// [UNSIGNED CHAR height] - 1 byte
// ([CHAR letter] [CHAR is_new]) * width * height - только буквы, 0 если пусто

// MOVE WILL NOT BE SAVED BECAUSE ITS NOT NECESSERY (there is no problem to place letter and select word again)

// WORDS HISTORY
// [UNSIGNED SHORT player_words_count] - 2 bytes
// For every player1`s word:
//   [UNSIGNED CHAR len] - 1 byte
//   [CHAR * len] - только нужные байты
// [UNSIGNED SHORT computer_words_count] - 2 bytes  
// For every player2`s (computer) word:
//   [UNSIGNED CHAR len] - 1 byte
//   [CHAR * len] - только нужные байты


StatusCode game_save(Game* game, const char* filename) {
	FILE* file = fopen(filename, "wb+");
	if (file == NULL) {
		return ERROR_FILE_NOT__FOUND;
	}

	unsigned char c;
	unsigned short sh;

	//magic
	unsigned char MAGIC[4] = "BALD";
	fwrite(MAGIC, sizeof(MAGIC), 1, file);

	// GAME STATE 
	c = game->current_player;
	fwrite(&c, 1, 1, file);
	sh = game->scores[0];
	fwrite(&sh, 2, 1, file);
	sh = game->scores[1];
	fwrite(&sh, 2, 1, file);

	// SETTINGS
	sh = game->settings->time_limit;
	fwrite(&sh, 2, 1, file);
	c = game->settings->difficulty;
	fwrite(&c, 1, 1, file);
	c = game->settings->first_player;
	fwrite(&c, 1, 1, file);

	// FIELD
	c = game->field->height;
	fwrite(&c, 1, 1, file);
	c = game->field->width;
	fwrite(&c, 1, 1, file);
	// cells
	for (int i = 0; i < game->field->height; i++) {
		for (int j = 0; j < game->field->width; j++) {
			c = game->field->grid[i][j].letter;
			fwrite(&c, 1, 1, file);
			c = game->field->grid[i][j].player_id;;
			fwrite(&c, 1, 1, file);
			c = game->field->grid[i][j].new;;
			fwrite(&c, 1, 1, file);
		}
	}
	
	// WORDS HISTORY
	sh = game->player1_words.count;
	fwrite(&sh, 2, 1, file);
	for (int i = 0; i < game->player1_words.count; i ++) {
		//word len + 1 '\0' byte
		unsigned char len = strlen(game->player1_words.words[i]) + 1;
		fwrite(&len, sizeof(len), 1, file);
		//word
		fwrite(game->player1_words.words[i], 1, len, file);
	}

	sh = game->player2_words.count;
	fwrite(&sh, 2, 1, file);
	for (int i = 0; i < game->player2_words.count; i++) {
		//word len + 1 '\0' byte
		unsigned char len = strlen(game->player2_words.words[i]) + 1;
		fwrite(&len, sizeof(len), 1, file);
		//word
		fwrite(game->player2_words.words[i], 1, len, file);
	}

	fclose(file);
	return SUCCESS;
}


StatusCode game_load(Game* game, const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		return ERROR_FILE_NOT__FOUND;
	}

	unsigned char c = 0;
	unsigned short sh = 0;

	//MAGIC
	unsigned char MAGIC[4];
	fread(MAGIC, sizeof(MAGIC), 1, file);
	if (!(MAGIC[0] == 'B' && MAGIC[1] == 'A' && MAGIC[2] == 'L' && MAGIC[3] == 'D')) {
		fclose(file);
		return ERROR_INVALID_FILE_FORMAT;
	}

	// GAME STATE 
	fread(&c, 1, 1, file);
	game->current_player = c;
	fread(&sh, 2, 1, file);
	game->scores[0] = sh;
	fread(&sh, 2, 1, file);
	game->scores[1] = sh;

	// SETTINGS
	fread(&sh, 2, 1, file);
	game->settings->time_limit = sh;
	fread(&c, 1, 1, file);
	game->settings->difficulty = c;
	fread(&c, 1, 1, file);
	game->settings->first_player = c;

	// FIELD
	fread(&c, 1, 1, file);
	game->field->height = c;
	fread(&c, 1, 1, file);
	game->field->width = c;
	// cells
	for (int i = 0; i < game->field->height; i++) {
		for (int j = 0; j < game->field->width; j++) {
			fread(&c, 1, 1, file);
			game->field->grid[i][j].letter = c;
			fread(&c, 1, 1, file);
			game->field->grid[i][j].player_id = c;
			fread(&c, 1, 1, file);
			game->field->grid[i][j].new = c;
		}
	}

	// WORDS HISTORY
	fread(&sh, 2, 1, file);
	game->player1_words.count = sh;
	for (int i = 0; i < game->player1_words.count; i++) {
		unsigned char len; // word len + '\0'
		fread(&len, 1, 1, file);

		game->player1_words.words[i] = malloc(len);
		if (game->player1_words.words[i] == NULL) {
			//free all words which have been allocated
			while (i >= 0) {
				free(game->player1_words.words[i--]);
			}
			fclose(file);
			return ERROR_OUT_OF_MEMORY;
		}
		fread(game->player1_words.words[i], 1, len, file);
	}

	fread(&sh, 2, 1, file);
	game->player2_words.count = sh;
	for (int i = 0; i < game->player2_words.count; i++) {
		unsigned char len; // word len + '\0'
		fread(&len, 1, 1, file);

		game->player2_words.words[i] = malloc(len);
		if (game->player2_words.words[i] == NULL) {
			//free all words which have been allocated
			while (i >= 0) {
				free(game->player2_words.words[i--]);
			}
			fclose(file);
			return ERROR_OUT_OF_MEMORY;
		}
		fread(game->player2_words.words[i], 1, len, file);
	}

	fclose(file);
	return SUCCESS;
}


void print_field(Game* game) {
	for (int i = 0; i < FIELD_SIZE; i++) {
		for (int j = 0; j < FIELD_SIZE; j++) {
			char c = game->field->grid[i][j].letter;
			if (c == 0) {
				fprintf(stderr, "[ ]");
			}
			else {
				fprintf(stderr, "[%c]", c);
			}
		}
		printf("\n");
	}
}


void print_settings(GameSettings* settings) {
	printf("time: %d\n difficulty: %d\n first_player: %d\n", settings->time_limit, settings->difficulty, settings->first_player);
}
