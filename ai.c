#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#include "ai.h"
#include "game_logic.h"
#include "common.h"

#define MAX_WORD_LEN 26
#define MAX_PATH_COUNT 1000 //it should be dynamics

#define FIELD_SIZE 5

struct AIState {
	Move best_move;
	unsigned long long end_time; //ms
	volatile unsigned char is_move_found;

	volatile unsigned char is_additional_time;
	volatile unsigned int time_limit; //volatile because if user gives an additional time, this field will contain this new value

	volatile unsigned char is_computation_complete;
	volatile unsigned char is_started ; // has the move already started 
	volatile unsigned char gave_up;
	volatile unsigned char percentage;

	HANDLE additional_time_event;
};


typedef struct ThreadArgs {
	Game* game;
	Dictionary* dict;
	AIState* state;
} ThreadArgs;

typedef struct Path {
	WordCell cells[MAX_WORD_LEN];
	int len;
} Path;

//we need additional structure for keeping not only paths, but placed letters too in mid/hard algorithms



static void state_set(AIState* state, int score, int time_limit) {
	state->is_computation_complete = 0;
	state->is_move_found = 0;
	state->percentage = 0;
	state->time_limit = time_limit;
	state->gave_up = 0;
	state->is_additional_time = 0;
	state->is_started = 1;

	state->best_move.letter = '\0';
	state->best_move.y = -1;
	state->best_move.x = -1;
	state->best_move.word_len = 0;
	state->best_move.score = score;
}

static void rotate_path(Path* path) {
	for (int i = 0; i < path->len / 2; i++) {
		WordCell temp = path->cells[i];
		path->cells[i] = path->cells[path->len - 1 - i];
		path->cells[path->len - 1 - i] = temp;
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


static int dfs_easy_direct(AIState* state, Dictionary* dict, GameField* field, Game* game_copy, bool visited[][FIELD_SIZE], Path* path, int y, int x, int* counter) {
	//printf("direct\n");
	if (*counter % 256 == 0 && GetTickCount64() >= state->end_time) {
		state->is_computation_complete = 1;
		WaitForSingleObject(state->additional_time_event, INFINITE);
		state->is_computation_complete = 0;
		if (state->is_additional_time) {
			state->end_time = GetTickCount64() + state->time_limit * 1000;
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
	//fprintf(stderr, "ЭТОТ ПРЕФИКС DIRECT - %s\n", prefix);
	//Maybe this first letter is the beginning of some word
	if (dict_word_exists(dict, prefix)) {
		if (!is_word_used(game_copy, prefix))
			return 1;
		else {
			path->len--;
			visited[y][x] = 0;
			return 0;
		}
			
	}

	//up, down, left, right
	int dy[] = { -1, 1, 0, 0 };
	int dx[] = { 0, 0, -1, 1 };

	for (int i = 0; i < 4; i++) {
		int newY = y + dy[i];
		int newX = x + dx[i];

		if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
			int res = dfs_easy_direct(state, dict, field, game_copy, visited, path, newY, newX, counter);
			if (res == 1 || res == -1) return res; // -1 - time_out, 1 - word have been founded, 0 - dead end
		}
	}
	path->len--;
	visited[y][x] = 0;
	return 0;
}


// bool because an easy algorithm returns the first word found
static int dfs_easy_rev(AIState* state, Dictionary* dict, GameField* field, Game* game_copy, bool visited[][FIELD_SIZE], Path* path, int y, int x, int* counter) {
	//check time limit
	if (*counter % 256 == 0 && GetTickCount64() >= state->end_time) {
		state->is_computation_complete = 1;
		WaitForSingleObject(state->additional_time_event, INFINITE);
		state->is_computation_complete = 0;
		if (state->is_additional_time) {
			state->end_time = GetTickCount64() + state->time_limit * 1000;
		}
		else {
			return -1;
		}
	}
	(*counter)++;

	//add new letter to path, check if this prefix is in trie, if its not - return, otherwise we have to check whether this new letter has the is_end_of_the_word flag
	path->cells[path->len++] = (WordCell){ y, x, field->grid[y][x].letter }; //заменить на простое добавление значений полям
	visited[y][x] = 1;

	char prefix[MAX_WORD_LEN];
	path_to_char(*path, prefix, MAX_WORD_LEN);
	if (!dict_reverse_prefix_exists(dict, prefix)) {
		//fprintf(stderr, "Denied rev prefix: %s\n", prefix);
		path->len--;
		visited[y][x] = 0;
		return 0;
	}
	
	//if we found reverse prefix, try to find all word 

	if (dict_reverse_word_exists(dict, prefix)) {
		//fprintf(stderr, "FIND\n");
		int letterY = path->cells[0].y;
		int letterX = path->cells[0].x;
		rotate_path(path);
		//starting points for dfs_direct() - cells around placed letter
		int dy[] = { -1, 1, 0, 0 };
		int dx[] = { 0, 0, -1, 1 };
		for (int i = 0; i < 4; i++) {
			int newY = letterY + dy[i];
			int newX = letterX + dx[i];
			if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
				int res = dfs_easy_direct(state, dict, field, game_copy, visited, path, newY, newX, counter);
				if (res == 1 || res == -1) return res; // -1 - time_out, 1 - word have been founded, 0 - dead end
			}
		}
		rotate_path(path); //if its not then continue searching
	}

	//up, down, left, right
	int dy[] = { -1, 1, 0, 0 };
	int dx[] = { 0, 0, -1, 1 };
	for (int i = 0; i < 4; i++) {
		int newY = y + dy[i];
		int newX = x + dx[i];
		if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
			int res = dfs_easy_rev(state, dict, field, game_copy, visited, path, newY, newX, counter);
			if (res == 1 || res == -1) return res; // -1 - time_out, 1 - word have been founded, 0 - dead end
		}
	}

	path->len--;
	visited[y][x] = 0;
	return 0;
}


static int easy_found(AIState* state, Dictionary* dict, GameField* field, Game* game_copy, int y, int x, Path* path, int* counter) {
	bool visited[FIELD_SIZE][FIELD_SIZE] = { 0 };
	return dfs_easy_rev(state, dict, field, game_copy, visited, path, y, x, counter);
}



//!!!!!!!! its better to pick a random i instead of a for loop, because in general we will find the word in the first iteration (i guess) !!!!!!!!!!!!!!!!
static void ai_easy(Dictionary* dict, Game* game_copy, AIState* state) {
	state->end_time = GetTickCount64() + state->time_limit * 1000;
	int counter = 0;

	GameField* field = game_get_field(game_copy);
	if (field == NULL) {
		fprintf(stderr, "Ошибка при получении поля game_get_field()\n");
		return;
	}

	char alphabet[34] = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

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
	int total_combinations = count * 33;
	int operations = 0;
	//try all candidates
	for (int i = 0; i < count; i++) {
		for (int let = 0; let < 33; let++) {
			//progress
			state->percentage = ((i * 33 + let) * 100) / total_combinations;

			//check time limit, value should be divisible by 2, since in this case the compiler optimizes it to bit mask
			if (counter % 128 == 0) {
				if (GetTickCount64() >= state->end_time) {
					if (state->is_move_found == 0) {
						//ask more time
						state->is_computation_complete = 1; //but is_move_found is still 0, so we can check this situation in main loop and request additional time

						WaitForSingleObject(state->additional_time_event, INFINITE);
						state->is_computation_complete = 0;

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
			} 
			counter++;

			int y = candidates[i][0]; 
			int x = candidates[i][1];
			field->grid[y][x].letter = alphabet[let];

			int res = easy_found(state, dict, field, game_copy, y, x, &path, &counter);
			if (res) {
				state->best_move.letter = alphabet[let];
				state->best_move.y = y;
				state->best_move.x = x;
				state->best_move.word_len = path.len;
				for (int j = 0; j < path.len; j++) {
					state->best_move.word[j] = path.cells[j];
					//fprintf(stderr, "%c ", path.cells[j].letter);
				}
				//fprintf(stderr, "\n");
				state->best_move.score += path.len;

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
				printf("time_out\n");
				state->gave_up = 1;
				return;
			}
		}
	}
	//if there are no words
	state->gave_up = 1;
}


static void ai_thread(void* param) {
	//Sleep(3000);
	ThreadArgs* args = (ThreadArgs*)param;
	Dictionary* dict = args->dict;
	Game* game = game_make_copy(args->game);
	AIState* state = args->state;

	if (game == NULL || state == NULL) {
		return;
	}

	int score = 0;
	StatusCode code = game_get_score(game, 2, &score);
	if (code != SUCCESS) {
		fprintf(stderr, "Ошибка при получении счета в ai_thread()\n");
		return;
	}

	state_set(state, score, game_get_time_limit(game));

	//different algorithms for different difficulties
	if (game_get_difficulty(game) == 0) {
		ai_easy(dict, game, state);
	}

	//applying to AIState

	free(args);
	game_destroy(&game);
	_endthread();
}


StatusCode ai_start_turn(Game* game, AIState* state, Dictionary* dict) {
	if (game == NULL) return ERROR_NULL_POINTER;
	if (state == NULL) return ERROR_NULL_POINTER;
	if (dict == NULL) return ERROR_NULL_POINTER;

	state->is_computation_complete = 0;
	state->is_started = 1;

	ThreadArgs* args = malloc(sizeof(ThreadArgs));
	if (args == NULL) {
		fprintf(stderr, "ERROR_OUT_OF_MEMORY ai_start_turn()\n");
		return ERROR_OUT_OF_MEMORY;
	}
	args->game = game;
	args->dict = dict;
	args->state = state;

	//8-byte (in 64-bit system) value, thread handle (NOT AN ID!)
	uintptr_t thread_handle = _beginthread(
		ai_thread,
		0,
		(void*) args
	);

	if (thread_handle == -1) {
		fprintf(stderr, "Ошибка при создании потока!\n");
		return AI_THREAD_ERROR;
	}
	return SUCCESS;
}


AIState* ai_state_init() {
	AIState* state = malloc(sizeof(AIState));
	if (state == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти в ai_state_init()\n");
		return NULL;
	}

	fprintf(stderr, "INIT STATE\n");

	state->additional_time_event = CreateEvent(NULL, FALSE, FALSE, NULL); // event for thread waiting
	state->percentage = 0;
	state->is_move_found = 0;
	state->is_computation_complete = 1;
	state->is_additional_time = 0;
	state->is_started = 0;
	state->gave_up = 0;

	state->best_move.letter = '\0';
	state->best_move.y = -1;
	state->best_move.x = -1;
	state->best_move.word_len = 0;
	state->best_move.score = 0;

	return state;
}



// 1 - start, 0 - stop
StatusCode ai_set_stop(AIState* state) {
	if (state == NULL) return ERROR_NULL_POINTER;
	state->is_started = 0;
	return SUCCESS;
}

StatusCode ai_give_additional_time(AIState* state, bool n, int additional_time) {
	if (state == NULL) return ERROR_NULL_POINTER;
	if (!n) state->is_additional_time = 0;
	else {
		state->is_additional_time = 1;
		state->time_limit = additional_time;
	}
	return SUCCESS;
}

//true - started, false - stoped
bool ai_status(AIState* state) {
	if (state == NULL) {
		fprintf(stderr, "ERROR state NULL\n");
		return false;
	}
	return state->is_started;
}

bool ai_need_additional_time(AIState* state) {
	if (state == NULL) {
		fprintf(stderr, "ERROR state NULL\n");
		return false;
	}
	return state->is_computation_complete && !state->is_move_found;
}

bool ai_word_founded(AIState* state) {
	if (state == NULL) {
		fprintf(stderr, "ERROR state NULL\n");
		return false;
	}
	return state->is_computation_complete && state->is_move_found;
}

bool ai_gave_up(AIState* state) {
	if (state == NULL) {
		fprintf(stderr, "ERROR state NULL\n");
		return false;
	}
	return state->gave_up;
}

unsigned char ai_get_percentage(AIState* state) {
	if (state == NULL) return -1;
	return state->percentage;
}

HANDLE ai_get_handle(AIState* state) {
	return state->additional_time_event;
}

Move* ai_get_move(AIState* state) {
	return &state->best_move;
}