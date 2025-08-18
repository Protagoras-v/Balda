#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <time.h>

#include "dict.h"
#include "game_logic.h"

#define MAX_WORD_LEN 100

void check_code(StatusCode code) {
	switch (code) {
	case SUCCESS:
		printf("�������\n");
		break;
		// ����� ������
	case ERROR_NULL_POINTER:
		printf("������� ���������\n");
		break;
	case ERROR_INVALID_ARGUMENT:
		printf("�������� ��������\n");
		break;
	case ERROR_OUT_OF_MEMORY:
		printf("������������ ������\n");
		break;
	case ERROR_FILE_NO_FOUND:
		printf("���� �� ������\n");
		break;
		// ������ �������
	case DICT_ERROR_INVALID_WORD:
		printf("������������ �����\n");
		break;
	case DICT_ERROR_DUPLICATE_WORD:
		printf("����� ��� ���������� � �������\n");
		break;
	case DICT_ERROR_WORD_TOO_SHORT:
		printf("����� ������� ��������\n");
		break;
	case DICT_ERROR_WORD_TOO_LONG:
		printf("����� ������� �������\n");
		break;
		// ������ �������� ����
	case FIELD_INVALID_COORDINATES:
		printf("������������ ����������\n");
		break;
	case FIELD_INVALID_LETTER:
		printf("������ ������ ���� ������� ������\n");
		break;
	case FIELD_INVALID_CELL:
		printf("������ �������� ������ � �����\n");
		break;
	case FIELD_CELL_OCCUPIED:
		printf("������ ������\n");
		break;
	case FIELD_CELL_EMPTY:
		printf("��� ������ ������\n");
		break;
	case FIELD_CELL_NOT_CONNECTED:
		printf("����� ������ �������� �� ������� ����\n");
		break;
	case FIELD_WORD_ALREADY_EMPTY:
		printf("����� ��� �������\n");
		break;
	case GAME_INVALID_WORD:
		printf("������������ ����� � ����\n");
		break;
	case GAME_WORD_EMPTY:
		printf("������ �����\n");
		break;
	case GAME_WORD_DOESNT_CONTAIN_LETTER:
		printf("����� �� �������� ��������� �����\n");
		break;
	case GAME_WORD_USED:
		printf("����� ��� ������������\n");
		break;
	case GAME_INVALID_ID:
		printf("�������� ������������� ������\n");
		break;
	case AI_ERROR_NO_MOVES_FOUND:
		printf("�� �� ����� ��������� �����\n");
		break;
	case AI_ERROR_TIMEOUT:
		printf("����� �� ��� �� �������\n");
		break;
	default:
		printf("����������� ������ (���: %d)\n", code);
		break;
	}
}

void clear_input_buffer() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}


int parse_command(Dictionary* dict, Game* game, char* cmd) {
	char* ptr = cmd;
	char* s = cmd;
	bool f = 0;
	StatusCode code;

	//���� ��������
	while (*s == ' ') s++;

	ptr = s;
	while (*s != '\0' && *s != ' ') s++;
	if (s == ptr) {
		printf("������ �������\n");
		return 1;
	}
	if (*s == ' ')
		f = 1;
	*s++ = '\0';

	if (strcmp(ptr, "confirm_move") == 0) {
		if (!f) {
			code = game_confirm_move(game, dict);
			if (code == GAME_INVALID_WORD) {
				int r = -1;

				char* word[MAX_WORD_LEN];
				game_get_word(game, word);

				while (r != 1 && r != 0) {
					printf("����� '%s' ����������� � �������, ������ �������� ��� ? 1 - ��, 0 - ���\n", word);
					scanf("%d", &r);

					if (r == 1) {
						dict_add_word(dict, word);
						code = game_confirm_move(game, dict);
						check_code(code);
						clear_input_buffer();
					}
					else if (r == 0) {
						game_cancel_word_selection(game);
					}
					else {
						printf("���������� ����\n");
					}
				}
			}
			else { 
				check_code(code); 
			}
		}
		else
			printf("������ �������\n");
	}
	else if (strcmp(ptr, "letter") == 0) {
		int y = 0, x = 0;
		char letter = '\0';
		char* e1, * e2;

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (*s == '\0' || s == ptr) {
			printf("������ �������\n");
			return 1;
		}
		*s++ = '\0';
		y = (int)strtol(ptr, &e1, 10);
		if (*e1 != '\0') {
			printf("������ �������\n");
			return 1;
		}

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (s == ptr) {
			printf("������ �������\n");
			return 1;
		}
		*s++ = '\0';
		x = (int)strtol(ptr, &e2, 10);
		if (*e2 != '\0') {
			printf("������ �������\n");
			return 1;
		}

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (s == ptr) {
			printf("������ �������\n");
			return 1;
		}

		letter = *(s - 1);
		
		code = game_try_place_letter(game, y, x, letter);
		check_code(code);
		print_field(game);
	}
	else if (strcmp(ptr, "add") == 0) {
		int y = 0, x = 0;
		char* e1, * e2, * e3;

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (*s == '\0' || s == ptr) {
			printf("������ �������\n");
			return 1;
		}
		*s++ = '\0';
		y = (int)strtol(ptr, &e1, 10);
		if (*e1 != '\0') {
			printf("������ �������\n");
			return 1;
		}

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (s == ptr) {
			printf("������ �������\n");
			return 1;
		}

		*s++ = '\0';
		x = (int)strtol(ptr, &e2, 10);
		if (*e2 != '\0') {
			printf("������ �������\n");
			return 1;
		}

		code = game_add_cell_into_word(game, y, x);
		if (code == GAME_LETTER_IS_MISSING) {
			printf("������� ����� ���������� �����!\n");
		}
		else check_code(code);
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
	else if (strcmp(ptr, "print_word") == 0) {
		char* buffer[MAX_WORD_LEN];
		code = game_get_word(game, buffer);
		if (code == GAME_WORD_EMPTY) {
			printf("�� ��� �� ������� �� ����� ����� � �����!\n");
		}
		else if (code == SUCCESS) {
			printf("������� ������������ �����: %s\n", buffer);
		}
	}
	else if (strcmp(ptr, "print_place_words") == 0) {
		char* e1;
		int id = 0;

		ptr = s;
		while (*s != ' ' && *s != '\0') s++;
		if (*s != '\0') {
			printf("������ �������\n");
			return 1;
		}
		*s++ = '\0';
		id = (int)strtol(ptr, &e1, 10);
		if (*e1 != '\0') {
			printf("������ �������\n");
			return 1;
		}

		char** words;
		int count;
		code = game_get_player_words(game, id, &words, &count);
		if (code == GAME_INVALID_ID) {
			printf("�������� id\n");
		}
		else if (code == SUCCESS) {
			printf("�����, ������������ ������������� � id %d: ", id);
			for (int i = 0; i < count; i++) {
				printf("%s ", words[i]);
			}
			printf("\n");
		}
	}
	else if (strcmp(ptr, "help") == 0) {
		printf("��������� �������:\n"
			"  place_letter <y> <x> <�����>  - ��������� ����� �� ����\n"
			"  add_into_word <y> <x>         - �������� ������ � ������������ �����\n"
			"  confirm_move                  - ����������� ���\n"
			"  cancel_selection              - �������� ����� �����\n"
			"  remove_letter                 - ������ ������������ �����\n"
			"  print                         - �������� ����\n"
			"  print_word                    - �������� ������������ �����\n"
			"  print_place_words <id>        - �������� ����� ������ (0-�����, 1-��)\n"
			"  help                          - �������� ��� �������\n"
			"  quit                          - ����� �� ����\n");
	}
	else if (strcmp(ptr, "quit") == 0) {
		return 0;
	}
	else {
		printf("������ �������\n");
	}
	return 1;
}


int main() {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "Russian");

	//����� ��� ��������� ��������� �����
	srand(time(NULL));

	StatusCode code;

	GameSettings* settings = game_init_settings();
	if (settings == NULL) {
		return 1;
	}
	Dictionary* dict = dict_init("dictionary.txt");
	if (dict == NULL) {
		return 1;
	}
	Game* game = game_create(settings, dict);
	if (game == NULL) {
		return 1;
	}

	//game_set_max_time_waiting(settings, 100);
	//game_set_difficulty(settings, 2);
	//game_set_first_player(settings, 2);
	//
	//print_settings(settings);

	bool f = 1;
	while (f) {
		char cmd[100] = { 0 };
		char* n;

		fgets(cmd, sizeof(cmd), stdin);
		n = strchr(cmd, '\n');
		if (n != NULL) *n = '\0';

		f = parse_command(dict, game, cmd);
	}

	dict_destroy(dict);
	game_destroy(game);
	free(settings);
	return 0;
}
