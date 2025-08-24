#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#include "ai.h"
#include "game_logic.h"

#define MAX_WORD_LEN 26
#define MAX_PATH_COUNT 1000 //it should be dynamics

#define FIELD_SIZE 5

struct WordCell {
	unsigned char y : 4;
	unsigned char x : 4;
	char letter;
};

struct Cell {
	char letter;
	unsigned int player_id : 2; // 0 - клетка пуста€, 1 - игрок, 2 - компьютер
	unsigned int new : 1;
};

struct GameField {
	Cell** grid;
	int width;
	int height;
};

struct Move {
	int y : 5;
	int x : 5;
	char letter;
	WordCell word[MAX_WORD_LEN];
	int word_len;
	int score;
};

struct AIState {
	Move best_move;
	unsigned long long end_time; //ms
	unsigned int is_move_found : 1;

	volatile unsigned int is_additional_time : 1;
	volatile unsigned int time_limit; //volatile because if user gives an additional time, this field will contain this new value

	volatile unsigned int is_computation_complete : 1;
	volatile int percentage;

	HANDLE additional_time_event;
};


typedef struct Path {
	WordCell cells[MAX_WORD_LEN];
	int len;
} Path;

//we need additional structure for keeping not only paths, but placed letters too in mid/hard algorithms



static void state_reset(AIState* state, int score) {
	state->is_computation_complete = 0;
	state->is_move_found = 0;
	state->percentage = 0;

	state->best_move.letter = '\0';
	state->best_move.y = -1;
	state->best_move.x = -1;
	state->best_move.word_len = 0;
	state->best_move.score = score;
}

static void rotate_path(Path* path) {
	for (int i = 0; i < path->len / 2; i++) {
		path->cells[i] = path->cells[path->len - 1 - i];
	}
}

static void add_path(Path* paths, int* path_count, Path current_path) {
	for (int i = 0; i < current_path.len; i++) {
		paths[*path_count].cells[i] = current_path.cells[i];
	}
	paths[*path_count].len = current_path.len;
	(*path_count)++;
}

static void path_to_char(Path current_path, char* res, int wLen) {
	int i = 0;
	while (i < current_path.len && i < wLen - 1) {
		res[i] = current_path.cells[i++].letter;
	}
	res[i] = '\0';
}


static int dfs_easy_direct(AIState* state, Dictionary* dict, GameField* field, bool** visited, Path* path, int y, int x, int* counter) {
	if (*counter % 256 == 0 && GetTickCount64() >= state->end_time) {
		state->is_computation_complete = 1;
		WaitForSingleObject(state->additional_time_event, INFINITE);
		if (state->is_additional_time) {
			state->end_time = GetTickCount64() + state->time_limit;
		}
		else {
			return -1;
		}
	}
	(*counter)++;

	path->cells[path->len++] = (WordCell){ y, x, field->grid[y][x].letter };
	visited[y][x] = 1;

	char prefix[MAX_WORD_LEN];
	path_to_char(*path, prefix, MAX_WORD_LEN);
	if (!dict_prefix_exists(dict, prefix)) {
		path->len--;
		visited[y][x] = 0;
		return 0;
	}

	//Maybe this first letter is the beginning of some word
	if (dict_word_exists(dict, prefix)) {
		return 1;
	}

	//up, down, left, right
	int dy[] = { -1, 1, 0, 0 };
	int dx[] = { 0, 0, -1, 1 };

	for (int i = 0; i < 4; i++) {
		int newY = y + dy[i];
		int newX = x + dx[i];

		if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
			int res = dfs_easy_direct(state, dict, field, visited, path, newY, newX, counter);
			if (res == 1 || res == -1) return res; // -1 - time_out, 1 - word have been founded
		}
	}
	path->len--;
	visited[y][x] = 0;
}


// bool because an easy algorithm returns the first word found
static int dfs_easy_rev(AIState* state, Dictionary* dict, GameField* field, bool** visited, Path* path, int y, int x, int* counter) {
	//check time limit
	if (*counter % 256 == 0 && GetTickCount64() >= state->end_time) {
		state->is_computation_complete = 1;
		WaitForSingleObject(state->additional_time_event, INFINITE);
		if (state->is_additional_time) {
			state->end_time = GetTickCount64() + state->time_limit;
		}
		else {
			return -1;
		}
	}
	(*counter)++;

	//add new letter to path, check if this prefix is in trie, if its not - return, otherwise we have to check whether this new letter has the is_end_of_the_word flag
	path->cells[path->len++] = (WordCell){ y, x, field->grid[y][x].letter };
	visited[y][x] = 1;

	char prefix[MAX_WORD_LEN];
	path_to_char(*path, prefix, MAX_WORD_LEN);
	if (!dict_reverse_prefix_exists(dict, prefix)) {
		path->len--;
		visited[y][x] = 0;
		return 0;
	}

	//if we found reverse prefix, we immediately try to find the whole word 
	if (dict_reverse_word_exists(dict, prefix)) {
		rotate_path(path);
		int res = dfs_easy_direct(state, dict, field, visited, path, y, x, counter);
		if (res == 1 || res == -1) return res; // -1 - time_out, 1 - word have been founded
		else rotate_path(path); //if its not then continue searching
	}

	//up, down, left, right
	int dy[] = { -1, 1, 0, 0 };
	int dx[] = { 0, 0, -1, 1 };

	for (int i = 0; i < 4; i++) {
		int newY = y + dy[i];
		int newX = x + dx[i];

		if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
			int res = dfs_easy_rev(state, dict, field, visited, path, newY, newX, counter);
			if (res == 1 || res == -1) return res; // -1 - time_out, 1 - word have been founded
		}
	}

	path->len--;
	visited[y][x] = 0;
}


static int easy_found(AIState* state, Dictionary* dict, GameField* field, int y, int x, Path* path, int* counter) {
	bool visited[FIELD_SIZE][FIELD_SIZE] = { 0 };
	return dfs_easy_rev(state, dict, field, visited, path, y, x, counter);
}



//!!!!!!!! its better to pick a random i instead of a for loop, because in general we will find the word in the first iteration (i guess) !!!!!!!!!!!!!!!!
static void ai_easy(Dictionary* dict, Game* game, AIState* state) {
	state->end_time = GetTickCount64() + game_get_time_limit(game) * 1000;
	int counter = 0;

	GameField* field = game_get_field(game);
	char alphabet[34] = "абвгдеЄжзийклмнопрстуфхцчшщъыьэю€";

	Path path = { 0 };
	path.len = 0;

	int candidates[20][2]; //candidates for letter placing, 20 because there are 25 - 5 cells for 5x5
	int count = 0;
	for (int y = 0; y < field->height; y++) {
		for (int x = 0; x < field->width; x++) {
			if (is_cell_empty(field, y, x) && is_letter_near(field, y, x)) {
				candidates[count][0] = y;
				candidates[count++][1] = x;
			}
		}
	}
	//try all candidates
	for (int i = 0; i < count; i++) {
		for (int let = 0; let < 33; let++) {
			//check time limit, value should be divisible by 2, since in this case the compiler optimizes it to bit mask
			if (counter % 128 == 0 && GetTickCount() >= state->end_time) {
				if (state->is_move_found == 0) {
					//ask more time
					state->is_computation_complete = 1; //but is_move_found is still 0, so we can check this situation in main loop and request additional time

					WaitForSingleObject(state->additional_time_event, INFINITE);

					if (state->is_additional_time == 1) {
						state->end_time = GetTickCount64() + state->time_limit * 1000;
						state->is_computation_complete = 0;
						state->is_additional_time = 0;
					}
					else if (state->is_additional_time == 0) {
						return;
					}
				}
			} 
			counter++;

			int y = candidates[i][0]; 
			int x = candidates[i][1];
			field->grid[y][x].letter = alphabet[let];

			int res = easy_found(state, dict, field, y, x, &path, &counter);
			if (res) {
				state->best_move.letter = alphabet[let];
				state->best_move.y = y;
				state->best_move.x = x;
				state->best_move.word_len = path.len;
				for (int i = 0; i < path.len; i++)
					state->best_move.word[i] = path.cells[i];

				state->is_move_found = 1;
				state->percentage = 100;
				state->is_computation_complete = 1;	

				return;
			}
			else if (res == 0) {
				//word not found
				field->grid[y][x].letter = '\0';
			}
			else if (res == -1) {
				//timeout and denied by user
				return;
			}
		}
	}
}


void ai_thread(void* param1, void* param2, void* param3) {
	Dictionary* dict = (Dictionary*)param1;
	Game* game = game_make_copy((Game*)param2);
	AIState* state = (AIState*)param3;

	if (game == NULL || state == NULL) {
		return;
	}

	int score = 0;
	StatusCode code = game_get_score(game, 2, &score);
	if (code != SUCCESS) {
		fprintf(stderr, "ќшибка при получении счета в ai_thread()\n");
		return;
	}
	
	// Ёто нужно вынести в отдельную функции AIState_init(), чтобы создать Event один раз
	state_reset(state, score);
	state->additional_time_event = CreateEvent(NULL, FALSE, FALSE, NULL);

	state->time_limit = game_get_time_limit(game);
	//different algorithms for different difficulties
	if (game_get_difficulty(game) == 0) {
		ai_easy(dict, game, state);
	}

	//applying to AIState

	_endthread();
}

StatusCode ai_start_turn(Game* game, AIState* state, Dictionary* dict) {
	state->is_computation_complete = 0;

	//8-byte (in 64-bit system) value, thread handle (NOT AN ID!)
	uintptr_t thread_handle = _beginthread(
		ai_thread,
		0,
		game, state, dict
	);

	if (thread_handle == -1) {
		fprintf(stderr, "ќшибка при создании потока!\n");
		return AI_THREAD_ERROR;
	}
	return SUCCESS;
}