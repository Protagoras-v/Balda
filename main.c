#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <time.h>

#include "dict.h"
#include "game_logic.h"
#include "status_codes.h"
#include "common.h"
#include "ai.h"


#define LEADERBOARD_SIZE 50
#define MAX_WORD_LEN 100
#define MIN_WORD_LEN 3


void check_code(StatusCode code) {
	switch (code) {
	case SUCCESS:
		printf("Успешно\n");
		break;
		// Общие ошибки
	case ERROR_NULL_POINTER:
		printf("Нулевой указатель\n");
		break;
	case ERROR_INVALID_ARGUMENT:
		printf("Неверный аргумент\n");
		break;
	case ERROR_OUT_OF_MEMORY:
		printf("Недостаточно памяти\n");
		break;
	case ERROR_FILE_NOT__FOUND:
		printf("Файл не найден\n");
		break;
		// Ошибки словаря
	case DICT_ERROR_INVALID_WORD:
		printf("Недопустимое слово\n");
		break;
	case DICT_ERROR_DUPLICATE_WORD:
		printf("Слово уже существует в словаре\n");
		break;
	case DICT_ERROR_WORD_TOO_SHORT:
		printf("Слово слишком короткое\n");
		break;
	case DICT_ERROR_WORD_TOO_LONG:
		printf("Слово слишком длинное\n");
		break;
		// Ошибки игрового поля
	case FIELD_INVALID_COORDINATES:
		printf("Неправильные координаты\n");
		break;
	case FIELD_INVALID_LETTER:
		printf("Символ должен быть русской буквой\n");
		break;
	case FIELD_INVALID_CELL:
		printf("Нельзя добавить клетку в слово\n");
		break;
	case FIELD_CELL_OCCUPIED:
		printf("Клетка занята\n");
		break;
	case FIELD_CELL_EMPTY:
		printf("Эта клетка пустая\n");
		break;
	case FIELD_CELL_NOT_CONNECTED:
		printf("Слово должно состоять из смежных букв\n");
		break;
	case FIELD_WORD_ALREADY_EMPTY:
		printf("Слово уже удалено\n");
		break;
	case GAME_INVALID_WORD:
		printf("Недопустимое слово в игре\n");
		break;
	case GAME_WORD_EMPTY:
		printf("Пустое слово\n");
		break;
	case GAME_WORD_DOESNT_CONTAIN_LETTER:
		printf("Слово не содержит указанную букву\n");
		break;
	case GAME_WORD_USED:
		printf("Слово уже использовано\n");
		break;
	case GAME_INVALID_ID:
		printf("Неверный идентификатор игрока\n");
		break;
	case AI_NO_MOVES_FOUND:
		printf("ИИ не нашел доступных ходов\n");
		break;
	case AI_TIMEOUT:
		printf("Время на ход ИИ истекло\n");
		break;
	default:
		printf("Неизвестная ошибка (код: %d)\n", code);
		break;
	}
}

void clear_input_buffer() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}


int parse_command(Dictionary* dict, GameSettings* settings, Game** game, Leaderboard* lb, char* cmd) {
	char* ptr = cmd;
	char* s = cmd;
	bool f = 0;
	StatusCode code;

	//скип пробелов
	while (*s == ' ') s++;

	ptr = s;
	while (*s != '\0' && *s != ' ') s++;
	if (s == ptr) {
		printf("ошибка команды\n");
		return 1;
	}
	if (*s == ' ')
		f = 1;
	*s++ = '\0';
	if (strcmp(ptr, "print_leaderboard") == 0) {
		int scores[LEADERBOARD_SIZE];
		char usernames[LEADERBOARD_SIZE];
		int size = 0;
		game_get_leaderboard(lb, usernames, scores, &size);

		for (int i = 0; i < size; i++) {
			printf("%s %d\n", usernames[i], scores[i]);
		}
	}
	else if (strcmp(ptr, "score") == 0) {
		int s1, s2;
		game_get_score(*game, 1, &s1);
		game_get_score(*game, 2, &s2);
		printf("%d %d\n", s1, s2);
	}
	else if (strcmp(ptr, "add_into_lb") == 0) {
		char name[MAX_WORD_LEN];
		int score;

		printf("Введите имя:\n");
		scanf("%s", &name);
		clear_input_buffer();
		char* n = strchr(name, '\n');
		if (n != NULL) *n = '\0';

		code = game_add_into_leaderboard(lb, *game, name);
		check_code(code);
	}
	else if (strcmp(ptr, "new_game") == 0) {
		if (*game == NULL) {
			*game = game_create(settings, dict);
			if (*game == NULL) {
				fprintf(stderr, "Ошибка при выделении памяти для *game\n");
				return 0;
			}
		}
		else {
			printf("Игра уже запущена\n");
		}
	}
	else if (strcmp(ptr, "load_game") == 0) {
		if (*game != NULL) {
			printf("Игра уже запущена!\n");
		}
		else {
			*game = game_create(settings, dict);
			if (*game == NULL) {
				fprintf(stderr, "Ошибка при выделении памяти для *game\n");
				return 0;
			}
			char filename[100];

			printf("Введите имя файла:\n");
			scanf("%s", filename);

			char* n = strchr(filename, '\n');
			if (n != NULL) *n = '\0';

			clear_input_buffer();

			code = game_load(*game, filename);
			if (code == ERROR_FILE_NOT__FOUND) {
				printf("Такого файла нет!\n");
			}
			else check_code(code);
		}
	}
	else if (strcmp(ptr, "save_game") == 0) {
		if (*game != NULL) {
			char filename[100];
			printf("Введите имя файла:\n");
			scanf("%s", filename);

			char* n = strchr(filename, '\n');
			if (n != NULL) *n = '\0';

			clear_input_buffer();
			game_save(*game, filename);
		}
		else {
			printf("Нельзя сохранить то, что еще не создано\n");
		}
	}
	else if (strcmp(ptr, "end_game") == 0) {
		if (*game != NULL) {
			//only player_1 can end game (because 2 is an AI)
			int id = 0;
			game_get_player_id(*game, &id);

			if (id == 1) {
				int d = 0;
				printf("Хотите сохранить игру? 1 - да, 0 - нет\n");
				scanf("%d", &d);
				clear_input_buffer();

				if (d == 0) {
					id = 0;
					game_get_winner(*game, &id);

					//ask player`s name for leaderbord
					if (id == 1 && game_is_enough_score_for_lb(*game, lb)) {
						char name[255];

						printf("Введите свое имя\n");
						scanf("%s", name);
						clear_input_buffer();
						char* t = strchr(name, '\n');
						if (t != NULL) *t = '\0';

						code = game_add_into_leaderboard(lb, *game, name);
						check_code(code);
					}

					game_destroy(game);
				}
				else if (d != 0) {
					char filename[100];
					printf("Введите имя файла:\n");
					scanf("%s", filename);

					char* n = strchr(filename, '\n');
					if (n != NULL) *n = '\0';

					clear_input_buffer();
					game_save(*game, filename);
				}
			}
			else {
				printf("Закончить игру может только player_1 (человек)\n");
			}
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "confirm_move") == 0) {
		if (*game != NULL) {
			if (!f) {
				code = game_confirm_move(*game, dict);
				if (code == GAME_INVALID_WORD) {
					int r = -1;

					char* word[MAX_WORD_LEN];
					game_get_word(*game, word);

					while (r != 1 && r != 0) {
						printf("Слово '%s' отсутствует в словаре, хотите добавить его ? 1 - да, 0 - нет\n", word);
						scanf("%d", &r);
						if (r == 1) {
							dict_add_word(dict, word);
							code = game_confirm_move(*game, dict);
							check_code(code);
							clear_input_buffer();
						}
						else if (r == 0) {
							game_cancel_word_selection(*game);
						}
						else {
							printf("Невалидный ввод\n");
						}
					}
				}
				else {
					check_code(code);
				}
			}
			else
				printf("ошибка команды\n");
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "letter") == 0) {
		if (*game != NULL) {
			int y = 0, x = 0;
			char letter = '\0';
			char* e1, * e2;

			ptr = s;
			while (*s != ' ' && *s != '\0') s++;
			if (*s == '\0' || s == ptr) {
				printf("ошибка команды\n");
				return 1;
			}
			*s++ = '\0';
			y = (int)strtol(ptr, &e1, 10);
			if (*e1 != '\0') {
				printf("ошибка команды\n");
				return 1;
			}

			ptr = s;
			while (*s != ' ' && *s != '\0') s++;
			if (s == ptr) {
				printf("ошибка команды\n");
				return 1;
			}
			*s++ = '\0';
			x = (int)strtol(ptr, &e2, 10);
			if (*e2 != '\0') {
				printf("ошибка команды\n");
				return 1;
			}

			ptr = s;
			while (*s != ' ' && *s != '\0') s++;
			if (s == ptr) {
				printf("ошибка команды\n");
				return 1;
			}

			letter = *(s - 1);

			code = game_try_place_letter(*game, y, x, letter);
			check_code(code);
			print_field(*game);
		}	
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "add") == 0) {
		if (*game != NULL) {
			int y = 0, x = 0;
			char* e1, * e2, * e3;

			ptr = s;
			while (*s != ' ' && *s != '\0') s++;
			if (*s == '\0' || s == ptr) {
				printf("ошибка команды\n");
				return 1;
			}
			*s++ = '\0';
			y = (int)strtol(ptr, &e1, 10);
			if (*e1 != '\0') {
				printf("ошибка команды\n");
				return 1;
			}

			ptr = s;
			while (*s != ' ' && *s != '\0') s++;
			if (s == ptr) {
				printf("ошибка команды\n");
				return 1;
			}

			*s++ = '\0';
			x = (int)strtol(ptr, &e2, 10);
			if (*e2 != '\0') {
				printf("ошибка команды\n");
				return 1;
			}

			code = game_add_cell_into_word(*game, y, x);
			if (code == GAME_LETTER_IS_MISSING) {
				printf("Сначала нужно разместить букву!\n");
			}
			check_code(code);

			char* buffer[MAX_WORD_LEN];
			code = game_get_word(*game, buffer);
			if (code == GAME_WORD_EMPTY) {
				printf("Вы еще не доавили ни одной буквы в слово!\n");
			}
			else if (code == SUCCESS) {
				printf("Текущее составленное слово: %s\n", buffer);
			}
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "print") == 0) {
		if (*game != NULL)
			print_field(*game);
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "cancel_selection") == 0) {
		if (*game != NULL) {
			game_cancel_word_selection(*game);
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "remove_letter") == 0) {
		if (*game != NULL) {
			game_clear_move(*game);
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "print_word") == 0) {
		if (*game != NULL) {
			char* buffer[MAX_WORD_LEN];
			code = game_get_word(*game, buffer);
			if (code == GAME_WORD_EMPTY) {
				printf("Вы еще не доавили ни одной буквы в слово!\n");
			}
			else if (code == SUCCESS) {
				printf("Текущее составленное слово: %s\n", buffer);
			}
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "print_place_words") == 0) {
		if (*game != NULL) {
			char* e1;
			int id = 0;

			ptr = s;
			while (*s != ' ' && *s != '\0') s++;
			if (*s != '\0') {
				printf("ошибка команды\n");
				return 1;
			}
			*s++ = '\0';
			id = (int)strtol(ptr, &e1, 10);
			if (*e1 != '\0') {
				printf("ошибка команды\n");
				return 1;
			}

			char** words;
			int count;
			code = game_get_player_words(*game, id, &words, &count);
			if (code == GAME_INVALID_ID) {
				printf("Неверный id\n");
			}
			else if (code == SUCCESS) {
				printf("Слова, поставленные пользователем с id %d: ", id);
				for (int i = 0; i < count; i++) {
					printf("%s ", words[i]);
				}
				printf("\n");
			}
		}
		else printf("Игра не запущена!\n");
	}
	else if (strcmp(ptr, "help") == 0) {
		printf("Доступные команды:\n"
			"  print_leaderboard             - таблица лидеров\n"
			"  add_into_lb                   - добавить в таблицу лидеров\n"
			"  new_game                      - новая игра\n"
			"  load_game                     - загрузить игру\n"
			"  save_game                     - сохранить игру\n"
			"  end_game                      - закончить игру\n"
			"  letter <y> <x> <буква>        - поставить букву на поле\n"
			"  add <y> <x>                   - добавить клетку в составляемое слово\n"
			"  confirm_move                  - подтвердить ход\n"
			"  cancel_selection              - отменить выбор слова\n"
			"  remove_letter                 - убрать поставленную букву\n"
			"  print                         - показать поле\n"
			"  print_word                    - показать составляемое слово\n"
			"  print_place_words <id>        - показать слова игрока (0-игрок, 1-ИИ)\n"
			"  help                          - показать эту справку\n"
			"  quit                          - выход из игры\n");
	}
	else if (strcmp(ptr, "quit") == 0) {
		return 0;
	}
	else {
		printf("ошибка команды\n");
	}
	return 1;
}


int main() {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "Russian");

	//зерно для генерации случайных чисел
	srand(time(NULL));

	StatusCode code;

	Game* game = NULL;

	GameSettings* settings = game_init_settings();
	if (settings == NULL) {
		return 1;
	}
	Dictionary* dict = dict_init("dictionary.txt");
	if (dict == NULL) {
		return 1;
	}

	Leaderboard* lb = game_leaderboard_init();
	if (lb == NULL) {
		return 1;
	}

	AIState* state = ai_state_init();
	if (state == NULL) {
		return 1;
	}

	//game_set_max_time_waiting(settings, 100);
	//game_set_difficulty(settings, 2);
	//game_set_first_player(settings, 2);
	//
	//print_settings(settings);

	bool f = 1;
	while (f) {
		//AI
		if (game != NULL && game_get_player_id(game) == 2) {
			if (!ai_status(state)) {
				ai_start_turn(game, state, dict);
			}
			//if already started
			else {
				if (ai_need_additional_time(state)) {
					int reply = 0;
					printf("Компьютер не успел найти слово, хотите дать ему еще времени?\n1-да, 0 - нет\n");
					scanf("%d", &reply);
					clear_input_buffer();
					if (reply == 1) {
						//let it be default time_limit
						SetEvent(ai_get_handle(state));
					}
					else {
						//ask name if player wins and close game
						int id = 0;
						game_get_winner(game, &id);

						//ask player`s name for leaderboard
						if (id == 1 && game_is_enough_score_for_lb(game, lb)) {
							char name[255];

							printf("Введите свое имя\n");
							scanf("%s", name);
							clear_input_buffer();
							char* t = strchr(name, '\n');
							if (t != NULL) *t = '\0';

							code = game_add_into_leaderboard(lb, game, name);
							check_code(code);
						}

						game_destroy(game);
					}
				}
				else if (ai_word_founded(state)) {
					game_set_move(game, ai_get_move(state));
					code = game_confirm_move(game, dict);
				}
				else {
					printf("Процент выполнения - %d\n", ai_get_percentage(state));
				}
			}
		}
		//PLAYER
		else {
			char cmd[100] = { 0 };
			char* n;

			fgets(cmd, sizeof(cmd), stdin);
			n = strchr(cmd, '\n');
			if (n != NULL) *n = '\0';

			f = parse_command(dict, settings, &game, lb, cmd);
		}
	}

	dict_destroy(dict);
	game_destroy(game);
	game_leaderboard_destroy(lb);
	free(settings);
	return 0;
}
