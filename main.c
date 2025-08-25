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
	case ERROR_FILE_NOT__FOUND:
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
	case AI_NO_MOVES_FOUND:
		printf("�� �� ����� ��������� �����\n");
		break;
	case AI_TIMEOUT:
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


int parse_command(Dictionary* dict, GameSettings* settings, Game** game, Leaderboard* lb, char* cmd) {
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

		printf("������� ���:\n");
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
				fprintf(stderr, "������ ��� ��������� ������ ��� *game\n");
				return 0;
			}
		}
		else {
			printf("���� ��� ��������\n");
		}
	}
	else if (strcmp(ptr, "load_game") == 0) {
		if (*game != NULL) {
			printf("���� ��� ��������!\n");
		}
		else {
			*game = game_create(settings, dict);
			if (*game == NULL) {
				fprintf(stderr, "������ ��� ��������� ������ ��� *game\n");
				return 0;
			}
			char filename[100];

			printf("������� ��� �����:\n");
			scanf("%s", filename);

			char* n = strchr(filename, '\n');
			if (n != NULL) *n = '\0';

			clear_input_buffer();

			code = game_load(*game, filename);
			if (code == ERROR_FILE_NOT__FOUND) {
				printf("������ ����� ���!\n");
			}
			else check_code(code);
		}
	}
	else if (strcmp(ptr, "save_game") == 0) {
		if (*game != NULL) {
			char filename[100];
			printf("������� ��� �����:\n");
			scanf("%s", filename);

			char* n = strchr(filename, '\n');
			if (n != NULL) *n = '\0';

			clear_input_buffer();
			game_save(*game, filename);
		}
		else {
			printf("������ ��������� ��, ��� ��� �� �������\n");
		}
	}
	else if (strcmp(ptr, "end_game") == 0) {
		if (*game != NULL) {
			//only player_1 can end game (because 2 is an AI)
			int id = 0;
			game_get_player_id(*game, &id);

			if (id == 1) {
				int d = 0;
				printf("������ ��������� ����? 1 - ��, 0 - ���\n");
				scanf("%d", &d);
				clear_input_buffer();

				if (d == 0) {
					id = 0;
					game_get_winner(*game, &id);

					//ask player`s name for leaderbord
					if (id == 1 && game_is_enough_score_for_lb(*game, lb)) {
						char name[255];

						printf("������� ���� ���\n");
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
					printf("������� ��� �����:\n");
					scanf("%s", filename);

					char* n = strchr(filename, '\n');
					if (n != NULL) *n = '\0';

					clear_input_buffer();
					game_save(*game, filename);
				}
			}
			else {
				printf("��������� ���� ����� ������ player_1 (�������)\n");
			}
		}
		else printf("���� �� ��������!\n");
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
						printf("����� '%s' ����������� � �������, ������ �������� ��� ? 1 - ��, 0 - ���\n", word);
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
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "letter") == 0) {
		if (*game != NULL) {
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

			code = game_try_place_letter(*game, y, x, letter);
			check_code(code);
			print_field(*game);
		}	
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "add") == 0) {
		if (*game != NULL) {
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

			code = game_add_cell_into_word(*game, y, x);
			if (code == GAME_LETTER_IS_MISSING) {
				printf("������� ����� ���������� �����!\n");
			}
			check_code(code);

			char* buffer[MAX_WORD_LEN];
			code = game_get_word(*game, buffer);
			if (code == GAME_WORD_EMPTY) {
				printf("�� ��� �� ������� �� ����� ����� � �����!\n");
			}
			else if (code == SUCCESS) {
				printf("������� ������������ �����: %s\n", buffer);
			}
		}
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "print") == 0) {
		if (*game != NULL)
			print_field(*game);
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "cancel_selection") == 0) {
		if (*game != NULL) {
			game_cancel_word_selection(*game);
		}
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "remove_letter") == 0) {
		if (*game != NULL) {
			game_clear_move(*game);
		}
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "print_word") == 0) {
		if (*game != NULL) {
			char* buffer[MAX_WORD_LEN];
			code = game_get_word(*game, buffer);
			if (code == GAME_WORD_EMPTY) {
				printf("�� ��� �� ������� �� ����� ����� � �����!\n");
			}
			else if (code == SUCCESS) {
				printf("������� ������������ �����: %s\n", buffer);
			}
		}
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "print_place_words") == 0) {
		if (*game != NULL) {
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
			code = game_get_player_words(*game, id, &words, &count);
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
		else printf("���� �� ��������!\n");
	}
	else if (strcmp(ptr, "help") == 0) {
		printf("��������� �������:\n"
			"  print_leaderboard             - ������� �������\n"
			"  add_into_lb                   - �������� � ������� �������\n"
			"  new_game                      - ����� ����\n"
			"  load_game                     - ��������� ����\n"
			"  save_game                     - ��������� ����\n"
			"  end_game                      - ��������� ����\n"
			"  letter <y> <x> <�����>        - ��������� ����� �� ����\n"
			"  add <y> <x>                   - �������� ������ � ������������ �����\n"
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
					printf("��������� �� ����� ����� �����, ������ ���� ��� ��� �������?\n1-��, 0 - ���\n");
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

							printf("������� ���� ���\n");
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
					printf("������� ���������� - %d\n", ai_get_percentage(state));
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
