#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <limits.h>

#include "ai.h"
#include "game_logic.h"
#include "common.h"


#define TOP_MOVES_COUNT 20 //TOP_MOVES_COUNT the most longest words in mid and high algorithms will be saved

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

typedef struct Iteration {
	Path path;
	char letter;
	unsigned char y;
	unsigned char x;
} Iteration;



//set current score (at the beginning of the turn) for all moves
static void set_move_score(Game* game_copy, Move top_moves[]) {
	int ai_score = 0;
	if (game_get_score(game_copy, 2, &ai_score) == SUCCESS) {
		for (int i = 0; i < TOP_MOVES_COUNT; i++) {
			top_moves[i].score = ai_score;
		}
	}
}

//copy iteration into top_moves[i]
static void save_move(Iteration iteration, Move top_moves[], int i) {
	top_moves[i].letter = iteration.letter;
	top_moves[i].y = iteration.y;
	top_moves[i].x = iteration.x;
	top_moves[i].word_len = iteration.path.len;
	for (int j = 0; j < top_moves[i].word_len; j++) {
		top_moves[i].word[j] = iteration.path.cells[j];
	}
	top_moves[i].score += top_moves[i].word_len;
}


static void state_set(AIState* state, int score, unsigned long long time_limit) {
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
			state->end_time = GetTickCount64() + state->time_limit;
		}
		else {
			return -1;
		}
		*counter = 0;
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
			state->end_time = GetTickCount64() + state->time_limit;
		}
		else {
			return -1;
		}
		*counter = 0;
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


static int easy_search(AIState* state, Dictionary* dict, GameField* field, Game* game_copy, int y, int x, Path* path, int* counter) {
	bool visited[FIELD_SIZE][FIELD_SIZE] = { 0 };
	return dfs_easy_rev(state, dict, field, game_copy, visited, path, y, x, counter);
}

//!!!!!!!! its better to pick a random i instead of a for loop, because in general we will find the word in the first iteration (i guess) !!!!!!!!!!!!!!!!
static void ai_easy(Dictionary* dict, Game* game_copy, AIState* state) {
	state->end_time = GetTickCount64() + state->time_limit;
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
			if (counter % 256 == 0) {
				if (GetTickCount64() >= state->end_time) {
					if (state->is_move_found == 0) {
						//ask more time
						state->is_computation_complete = 1; //but is_move_found is still 0, so we can check this situation in main loop and request additional time

						WaitForSingleObject(state->additional_time_event, INFINITE);
						state->is_computation_complete = 0;

						if (state->is_additional_time == 1) {
							state->end_time = GetTickCount64() + state->time_limit;
							state->is_computation_complete = 0;
							state->is_additional_time = 0;
						}
						else if (state->is_additional_time == 0) {
							return;
						}
					}
				}
				counter = 0;
			} 
			counter++;

			int y = candidates[i][0]; 
			int x = candidates[i][1];
			field->grid[y][x].letter = alphabet[let];

			int res = easy_search(state, dict, field, game_copy, y, x, &path, &counter);
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




//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------MIDDLE------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static SearchCode dfs_greedy_direct(AIState* state, Dictionary* dict, GameField* field, Game* game_copy, bool visited[][FIELD_SIZE], Iteration* iteration, Move top_moves[], int y, int x, int* counter) {
	if (*counter % 256 == 0) {
		unsigned long long now = GetTickCount64();
		//fprintf(stderr, "now=%llu end_time=%llu diff=%lld if (now >= state->end_time) : %d\n",
			//now, state->end_time, (long long)(state->end_time - now), now >= state->end_time);
		if (now >= state->end_time) {
			//fprintf(stderr, "iteration time: %llu\n", GetTickCount64());
			if (top_moves[0].word_len == 0 && state->is_move_found == 0) {
				fprintf(stderr, "TIME CHECKER-------\n");
				state->is_computation_complete = 1; //but is_move_found is still 0, so we can check this situation in main loop and request additional time

				WaitForSingleObject(state->additional_time_event, INFINITE);
				state->is_computation_complete = 0;

				if (state->is_additional_time == 1) {
					state->end_time = GetTickCount64() + state->time_limit ;
					state->is_computation_complete = 0;
					state->is_additional_time = 0;
				}
				else if (state->is_additional_time == 0) {
					return TIME_OUT_AND_MOVE_NOT_FOUND;
				}

			}
			else {
				fprintf(stderr, "TIME CHECKER ELSE-------\n");
				state->percentage = 100;
				state->is_computation_complete = 1;

				return TIME_OUT_AND_MOVE_FOUND;
			}
		}
		*counter = 0;
	}
	(*counter)++;
	iteration->path.cells[iteration->path.len++] = (WordCell){ y, x, field->grid[y][x].letter };
	visited[y][x] = 1;


	char prefix[MAX_WORD_LEN];
	path_to_char(iteration->path, prefix, MAX_WORD_LEN);
	if (!dict_prefix_exists(dict, prefix)) {
		iteration->path.len--;
		visited[y][x] = 0;
		return MOVE_NOT_FOUND;
	}

	if (dict_word_exists(dict, prefix)) {
		if (!is_word_used(game_copy, prefix)) {
			//find place for this word in top_moves[] by it`s len (we need it in middle algorithm for compatibilities with hard)
			for (int i = 0; i < TOP_MOVES_COUNT; i++) {
				if (iteration->path.len > top_moves[i].word_len) {
					if (top_moves[i].word_len == 0) { //this position is empty, so we just pass new value into it
						save_move(*iteration, top_moves, i);
					}
					else { // in case where insertion occurs in the middle of top_moves[], we have to shift all elems to right.
						for (int j = i; j < TOP_MOVES_COUNT - 1; j++) {
							top_moves[j + 1] = top_moves[j];
						}
						save_move(*iteration, top_moves, i);
					}
					//state->is_move_found = 1; //this is not necessery, because we should to check whether the move is found or not by checking top_moves[0].path.len 
					break;
				}
			}
		}	
	}

	//up, down, left, right
	int dy[] = { -1, 1, 0, 0 };
	int dx[] = { 0, 0, -1, 1 };

	for (int i = 0; i < 4; i++) {
		int newY = y + dy[i];
		int newX = x + dx[i];

		if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
			SearchCode res = dfs_greedy_direct(state, dict, field, game_copy, visited, iteration, top_moves, newY, newX, counter);
			if (res == TIME_OUT_AND_MOVE_FOUND || res == TIME_OUT_AND_MOVE_NOT_FOUND) return res; 
		}
	}
	iteration->path.len--;
	visited[y][x] = 0;
	return MOVE_NOT_FOUND;
}


static SearchCode dfs_greedy_rev(AIState* state, Dictionary* dict, GameField* field, Game* game_copy, bool visited[][FIELD_SIZE], Iteration* iteration, Move top_moves[], int y, int x, int* counter) {
	if (*counter % 256 == 0) {
		unsigned long long now = GetTickCount64();
		if (now >= state->end_time) {
			if (top_moves[0].word_len == 0 && state->is_move_found == 0) { 
				//УБРАТЬ ИЗМЕНЕНИЕ is_move_found В DFS, СДЕЛАТЬ ПРОВЕРКУ top_moves[0].path.len != 0. ДЛЯ ТОГО, ЧТОБЫ МИНИМАКС НЕ ПРОСИЛ ДОП ВРЕМЯ КОГДА ХОД БЫЛ НАЙДЕН В ДРУГИХ ВЕТКАХ, МЫ ДОПОЛНИТЕЛЬНО ПОВЕРЯЕМ is_move_found == 0 
				//(Для жадного алгоритма эта просто лишняя проверка, т.к. условие is_move_found будет всегда совпадать с тем, есть ли что-то в top_moves[0], но для совместимости с минимаксом лучше сделать так).
				fprintf(stderr, "TIME CHECKER-------\n");
				state->is_computation_complete = 1; //but is_move_found is still 0, so we can check this situation in main loop and request additional time

				WaitForSingleObject(state->additional_time_event, INFINITE);
				state->is_computation_complete = 0;

				if (state->is_additional_time == 1) {
					state->end_time = GetTickCount64() + state->time_limit;
					state->is_additional_time = 0;
				}
				else if (state->is_additional_time == 0) {
					return TIME_OUT_AND_MOVE_NOT_FOUND;
				}
			
			}
			else {
				fprintf(stderr, "TIME CHECKER ELSE-------\n");
				state->percentage = 100;
				state->is_computation_complete = 1;

				return TIME_OUT_AND_MOVE_FOUND;
			}
		}
		*counter = 0;
	}
	(*counter)++;
	//add new letter to path, check if this prefix is in trie, if its not - return, otherwise we have to check whether this new letter has the is_end_of_the_word flag
	iteration->path.cells[iteration->path.len++] = (WordCell){ y, x, field->grid[y][x].letter }; 
	visited[y][x] = 1;

	char prefix[MAX_WORD_LEN];
	path_to_char(iteration->path, prefix, MAX_WORD_LEN);
	if (!dict_reverse_prefix_exists(dict, prefix)) {
		//fprintf(stderr, "Denied rev prefix: %s\n", prefix);
		iteration->path.len--;
		visited[y][x] = 0;
		return MOVE_NOT_FOUND;
	}

	//if we found reverse prefix, try to find all word 

	if (dict_reverse_word_exists(dict, prefix)) {
		//fprintf(stderr, "prefix: %s\n", prefix);
		rotate_path(&iteration->path);
		//int dy[] = { -1, 1, 0, 0 };
		//int dx[] = { 0, 0, -1, 1 };
		//for (int i = 0; i < 4; i++) {
		//	//starting points for dfs_direct() - cells around placed letter
		//	int newY = iteration->y + dy[i]; //КАКОГО ХЕРА ОН ЭТО ПРОПУСТИ?????????? (;)
		//	int newX = iteration->x + dx[i];
		//	//fprintf(stderr, "(STARTING %d %d)    NEW -%d %d\n", iteration->y, iteration->x, newY, newX);
		//	if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
		//		SearchCode res = dfs_greedy_direct(state, dict, field, game_copy, visited, iteration, top_moves, newY, newX, counter);
		//		if (res == TIME_OUT_AND_MOVE_FOUND || res == TIME_OUT_AND_MOVE_NOT_FOUND) return res;
		//	}
		//}
		
		//starting point for dfs_direct() - the placed letter, candidate for which the algorithm tries to find words (For example, if we have found "tnomer" reversed prefix, after rotation wo-
		//rd part "remont" will be already in this word (to be precise in the iteration->path), so we try to continue it from 't' letter)
		//but we need to "remove" (by simply decreasing path.len) this 't' from the word and then add it again in the first call of dfs_DIRECT
		iteration->path.len--;
		SearchCode res = dfs_greedy_direct(state, dict, field, game_copy, visited, iteration, top_moves, iteration->y, iteration->x, counter);
		if (res == TIME_OUT_AND_MOVE_FOUND || res == TIME_OUT_AND_MOVE_NOT_FOUND) return res;
		iteration->path.len++;
		rotate_path(&iteration->path); //if its not then continue searching
	}

	//up, down, left, right
	int dy[] = { -1, 1, 0, 0 };
	int dx[] = { 0, 0, -1, 1 };
	for (int i = 0; i < 4; i++) {
		int newY = y + dy[i];
		int newX = x + dx[i];
		if (is_cell_coordinates_valid(field, newY, newX) && !visited[newY][newX] && !is_cell_empty(field, newY, newX)) {
			SearchCode res = dfs_greedy_rev(state, dict, field, game_copy, visited, iteration, top_moves, newY, newX, counter);
			if (res == TIME_OUT_AND_MOVE_FOUND || res == TIME_OUT_AND_MOVE_NOT_FOUND) return res;
		}
	}

	iteration->path.len--;
	visited[y][x] = 0;
	return MOVE_NOT_FOUND;
}


//serch all possible moves 
static SearchCode greedy_algorithm(Dictionary* dict, Game* game_copy, AIState* state, Move top_moves[], int* counter) {
	GameField* field = game_get_field(game_copy);
	if (field == NULL) {
		fprintf(stderr, "Ошибка при получении поля game_get_field()\n");
		return -2;
	}

	Iteration iteration = { 0 };

	char alphabet[34] = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";
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
			if (*counter % 256 == 0) {
				unsigned long long now = GetTickCount64();
				/*fprintf(stderr, "now=%llu end_time=%llu diff=%lld if (now >= state->end_time) : %d\n",
					now, state->end_time, (long long)(state->end_time - now), now >= state->end_time);*/
				if (now >= state->end_time) {
					fprintf(stderr, "iteration time: %llu\n", now);
					if (top_moves[0].word_len == 0 && state->is_move_found == 0) {
						state->is_computation_complete = 1; //but is_move_found is still 0, so we can check this situation in main loop and request additional time
						fprintf(stderr, "Wait for SO\n");
						WaitForSingleObject(state->additional_time_event, INFINITE);

						if (state->is_additional_time == 1) {
							state->end_time = GetTickCount64() + state->time_limit;
							state->is_computation_complete = 0;
							state->is_additional_time = 0;
						}
						else if (state->is_additional_time == 0) {
							return TIME_OUT_AND_MOVE_NOT_FOUND;
						}

					}
					else {
						fprintf(stderr, "TIME CHECKER ELSE-------\n");
						state->percentage = 100;
						return TIME_OUT_AND_MOVE_FOUND;
					}
				}
				*counter = 0;
			}
			(*counter)++;

			int y = candidates[i][0];
			int x = candidates[i][1];
			field->grid[y][x].letter = alphabet[let];

			//save letter and y/x for access in recursion
			iteration.letter = alphabet[let];
			iteration.y = y;
			iteration.x = x;

			bool visited[FIELD_SIZE][FIELD_SIZE] = { 0 };
			SearchCode res = dfs_greedy_rev(state, dict, field, game_copy, visited, &iteration, top_moves, y, x, counter);
			if (res == TIME_OUT_AND_MOVE_NOT_FOUND) {
				fprintf(stderr, "TIME_OUT_AND_MOVE_NOT_FOUND\n");
				return res;
			}
			else if (res == TIME_OUT_AND_MOVE_FOUND) {
				fprintf(stderr, "TIME_OUT_AND_MOVE_FOUND\n");
				return res;
			}
			field->grid[y][x].letter = '\0'; //unlike easy level, we continue until all possible cells have been checked or timeout has expired (and move has been found)
		}
	}
	//try all candidates
	
	
	//state->percentage = 5;

	//int y = 3;
	//int x = 0;
	//field->grid[y][x].letter = 'т';

	////save letter and y/x for access in recursion
	//iteration.letter = 'т';
	//iteration.y = y;
	//iteration.x = x;

	//bool visited[FIELD_SIZE][FIELD_SIZE] = { 0 };
	//SearchCode res = dfs_greedy_rev(state, dict, field, game_copy, visited, &iteration, top_moves, y, x, counter);
	//if (res == TIME_OUT_AND_MOVE_NOT_FOUND) {
	//	fprintf(stderr, "TIME_OUT_AND_MOVE_NOT_FOUND\n");
	//	return res;
	//}
	//else if (res == TIME_OUT_AND_MOVE_FOUND) {
	//	fprintf(stderr, "TIME_OUT_AND_MOVE_FOUND\n");
	//	return res;
	//}
	//field->grid[y][x].letter = '\0'; //unlike easy level, we continue until all possible cells have been checked or timeout has expired (and move has been found)
	
	if (top_moves[0].word_len != 0) {
		return MOVE_FOUND;
	}
	return MOVE_NOT_FOUND;
}


static void ai_mid(Dictionary* dict, Game* game_copy, AIState* state) {
	//fprintf(stderr, "time_limit=%d\n", state->time_limit);
	state->end_time = GetTickCount64() + (unsigned long long)state->time_limit;
	//fprintf(stderr, "End time: %llu\n", state->end_time);
	int counter = 0;


	Move top_moves[TOP_MOVES_COUNT] = { 0 }; //top TOP_MOVES_COUNT words (with info about letter), we need it for compatibilities with hard algorithm
	//set score
	set_move_score(game_copy, top_moves);

	SearchCode res = greedy_algorithm(dict, game_copy, state, top_moves, &counter);

	if (res == TIME_OUT_AND_MOVE_FOUND || res == MOVE_FOUND) {
		state->best_move = top_moves[0];
		state->percentage = 100;
		state->is_move_found = 1;
	}
	else {
		state->gave_up = 1;
	}
	state->is_computation_complete = 1;
}





//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------HARD------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int evaluate(Game* game_copy) {
	int p2 = 0;
	int p1 = 0;
	game_get_score(game_copy, 2, &p2);
	game_get_score(game_copy, 1, &p1);
	//fprintf(stderr, "%d %d\n", p2, p1);
	return p2 - p1;
}


int minimax(Game* game_copy, Dictionary* dict, AIState* state, int depth, int alpha, int beta, bool max, bool* timeout, int* counter) {
	if (depth == 0) return evaluate(game_copy);

	//ПОТОМ НУЖНО БУДЕТ УБРАТЬ ЛИШНИЙ КОД, заменив на best_eval и на конструкции (max ? a : b), НО СНАЧАЛА СДЕЛАЮ КАК ЕСТЬ
	
	//computer 
	if (max) {
		int max_eval = INT_MIN;

		Move top_moves[TOP_MOVES_COUNT] = { 0 };
		set_move_score(game_copy, top_moves);
		SearchCode res = greedy_algorithm(dict, game_copy, state, top_moves, counter);

		//stop recursion 
		if ((res == TIME_OUT_AND_MOVE_FOUND || res == TIME_OUT_AND_MOVE_NOT_FOUND) && depth > 1) {
			*timeout = true;
			return max_eval;
		}
		//in this case we don`t stop right now because we just need to place 20 word, call the evaluation function and return the result, it won`t take long
		else if (res == TIME_OUT_AND_MOVE_FOUND && depth == 1) {
			*timeout = true;
		}

		int moves_count = 0;
		while (moves_count < TOP_MOVES_COUNT && top_moves[moves_count].word_len != 0) moves_count++;
		if (moves_count == 0) return max_eval;

		for (int i = 0; i < moves_count; i++) {
			game_apply_generated_move(game_copy, top_moves[i]);
			int eval = minimax(game_copy, dict, state, depth - 1, alpha, beta, 0, timeout, counter);
			game_undo_generated_move(game_copy, top_moves[i]);

			max_eval = MAX(max_eval, eval);
			alpha = MAX(alpha, eval);

			if (beta <= alpha) {
				break;
			}
			//if a timeout occurs in minimax above
			if (*timeout) {
				break;
			}
		}
		return max_eval;
	}
	//player`s optimal request
	else {
		int min_eval = INT_MAX;

		Move top_moves[TOP_MOVES_COUNT] = { 0 };
		set_move_score(game_copy, top_moves);
		SearchCode res = greedy_algorithm(dict, game_copy, state, top_moves, counter);

		//stop recursion 
		if ((res == TIME_OUT_AND_MOVE_FOUND || res == TIME_OUT_AND_MOVE_NOT_FOUND) && depth > 1) {
			*timeout = true;
			return min_eval;
		}
		//in this case we don`t stop right now because we just need to place 20 word, call the evaluation function and return the result, it won`t take long
		else if (res == TIME_OUT_AND_MOVE_FOUND && depth == 1) {
			*timeout = true;
		}

		int moves_count = 0;
		while (moves_count < TOP_MOVES_COUNT && top_moves[moves_count].word_len != 0) moves_count++;
		if (moves_count == 0) return min_eval;

		for (int i = 0; i < moves_count; i++) {
			game_apply_generated_move(game_copy, top_moves[i]);
			int eval = minimax(game_copy, dict, state, depth - 1, alpha, beta, 1, timeout, counter);
			game_undo_generated_move(game_copy, top_moves[i]);

			min_eval = MIN(min_eval, eval);
			beta = MIN(beta, eval);

			if (beta <= alpha) {
				//in this case min_eval will be return (which is lover than alpha (guarantee for computer))
				//so the condition 
				//int score = minimax();
				//if (score > best_score)
				// will not be met, so there is no sense to continue for loop, because this branch will be discarded by higher level of recurtion 
				break;
			}
			if (*timeout) {
				break;
			}
		}
		return min_eval;
	}
}

static void ai_high(Dictionary* dict, Game* game_copy, AIState* state) {
	unsigned long long start_time = GetTickCount64();
	state->end_time = start_time + (unsigned long long)state->time_limit;
	fprintf(stderr, "Start time: %llu\nEnd time; %llu\n", start_time, state->end_time);
	int counter = 0;

	Move top_moves[TOP_MOVES_COUNT] = { 0 };
	set_move_score(game_copy, top_moves);
	SearchCode res = greedy_algorithm(dict, game_copy, state, top_moves, &counter);
	int moves_count = 0;
	while (moves_count < TOP_MOVES_COUNT && top_moves[moves_count].word_len != 0) moves_count++;

	int best_score = INT_MIN;
	bool timeout = false;
	int alpha = INT_MIN;
	//minimax
	for (int i = 0; i < moves_count; i++) {
		game_apply_generated_move(game_copy, top_moves[i]);
		int score = minimax(game_copy, dict, state, MINIMAX_DEPTH, alpha, INT_MAX, false, &timeout, &counter);
		fprintf(stderr, "score: %d\n", score);
		if (score > best_score) {
			best_score = score;
			state->best_move = top_moves[i];
			fprintf(stderr, "New best word: ");
			for (int i = 0; i < state->best_move.word_len; i++) {
				fprintf(stderr, "%c", state->best_move.word[i].letter);
			}
			fprintf(stderr, "\n");
			state->is_move_found = 1;
		}
		game_undo_generated_move(game_copy, top_moves[i]);

		//this condition only occurs in two cases: 
		// 1) if timeout occurs and user declines  the request for additional time 
		// 2) if there is already best_move (thanks for state.is_move_found condition in greedy_algorithm())
		if (timeout) {
			if (state->is_move_found) {
				fprintf(stderr, "ai_hard() timeout and move is found\n");
				break;
			}
			else {
				fprintf(stderr, "ai_hard() timeout and move is not found\n");
				state->gave_up = 1;
				break;
			}
		}
	}
	fprintf(stderr, "END\n");
	state->is_computation_complete = 1;
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
	else if (game_get_difficulty(game) == 1) {
		ai_mid(dict, game, state);
	}
	else if (game_get_difficulty(game) == 2) {
		ai_high(dict, game, state);
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

	state->additional_time_event = CreateEvent(NULL, FALSE, FALSE, NULL); // event for thread waiting
	state->percentage = 0;
	state->is_move_found = 0;
	fprintf(stderr, "STATE INIT-----\n");
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
		state->is_computation_complete = 0;
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

bool ai_word_found(AIState* state) {
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