#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <time.h>

#include "dict.h"
#include "game_logic.h"



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
	case ERROR_FILE_NO_FOUND:
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
		// Ошибки игровой логики
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
		// Ошибки ИИ
	case AI_ERROR_NO_MOVES_FOUND:
		printf("ИИ не нашел доступных ходов\n");
		break;
	case AI_ERROR_TIMEOUT:
		printf("Время на ход ИИ истекло\n");
		break;
	default:
		printf("Неизвестная ошибка (код: %d)\n", code);
		break;
	}
}



int parse_command(Dictionary* dict, Game* game, char* cmd) {
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

	if (strcmp(ptr, "confirm_move") == 0) {
		if (!f) {
			code = game_confirm_move(game, dict);
			check_code(code);
		}
		else
			printf("ошибка команды\n");
	}
	else if (strcmp(ptr, "place_letter") == 0) {
		int y = 0, x = 0;
		char letter = '\0';
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

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (s == ptr) {
			printf("ошибка команды\n");
			return 1;
		}

		letter = *(s - 1);
		
		code = game_try_place_letter(game, y, x, letter);
		check_code(code);
		print_field(game);
	}
	else if (strcmp(ptr, "add_into_word") == 0) {
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

		code = game_add_cell_into_word(game, y, x);
		check_code(code);
	}
	else if (strcmp(ptr, "print") == 0) {
		print_field(game);
	}
	else if (strcmp(ptr, "cancel_selection") == 0) {
		game_cancel_word_selection(game);
	}
	else if (strcmp(ptr, "remove_letter") == 0) {
		game_clear_move(game);
	}
	else if (strcmp(ptr, "print_place_words") == 0) {
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
		code = game_get_player_words(game, id, &words, &count);
		if (code == GAME_INVALID_ID) {
			printf("Неверный id");
		}
		else if (code == SUCCESS) {
			printf("Слова, поставленные пользователем с id %d: ", id);
			for (int i = 0; i < count; i++) {
				printf("%s ", words[i]);
			}
			printf("\n");
		}
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

	Dictionary* dict = dict_init("dictionary.txt");
	Game* game = game_create(dict);

	bool f = 1;
	while (f) {
		char cmd[100] = { 0 };
		char* n;

		fgets(cmd, sizeof(cmd), stdin);
		n = strchr(cmd, '\n');
		if (n != NULL) *n = '\0';

		f = parse_command(dict, game, cmd);
	}

	return 0;
}
