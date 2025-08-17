#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <time.h>

#include "dict.h"
#include "game_logic.h"

int main() {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "Russian");

	//����� ��� ��������� ��������� �����
	srand(time(NULL));

	Dictionary* dict = dict_init("dictionary.txt");
	Game* game = game_create(dict);

	unsigned char u = '�';
	printf("%u\n", u);
	bool f = 1;

	while (f) {
		char **p1_words, **p2_words;
		int p1w_count, p2w_count;
		StatusCode code;
		code = game_get_player_words(game, 1, &p1_words, &p1w_count);
		if (code != SUCCESS) {
			printf("������ ��������� ����\n");
			return 1;
		}
		code = game_get_player_words(game, 2, &p2_words, &p2w_count);
		if (code != SUCCESS) {
			printf("������ ��������� ����\n");
			return 1;
		}

		printf("�����, ������������ ������� 1: ");
		for (int i = 0; i < p1w_count; i++) {
			printf("%s ", p1_words[i]);
		}
		printf("\n");

		printf("�����, ������������ ������� 2: ");
		for (int i = 0; i < p2w_count; i++) {
			printf("%s ", p2_words[i]);
		}
		printf("\n");

		printf("��� ������ � id: %d\n", game_get_player_id(game));
		print_field(game);
		int x, y, len;
		char c;
		printf("������� y: ");
		scanf("%d", &y);
		printf("������� x: ");
		scanf("%d", &x);
		printf("������� �����: ");
		scanf(" %c", &c);

		code = game_try_place_letter(game, y, x, c);
		switch (code) {
		case SUCCESS:
			print_field(game);
			printf("������� ����� �����: \n");
			scanf("%d", &len);
			int i = 0;
			while (i < len) {
				printf("������� y: ");
				scanf("%d", &y);
				printf("������� x: ");
				scanf("%d", &x);

				code = game_add_cell_into_word(game, y, x);
				if (code == FIELD_INVALID_COORDINATES) {
					printf("������������ ����������!\n");
				}
				else if (code == FIELD_CELL_EMPTY) {
					printf("�������� ������ �� �������� �����!\n");
				}
				else if (code == FIELD_INVALID_CELL) {
					printf("�������� ������ �� �������� � ����������!\n");
				}
				else if (code == SUCCESS) {
					i++;
				}
			}
			code = game_confirm_move(game, dict);
			if (code == SUCCESS) printf("�����!\n");
			else if (code == GAME_WORD_USED) printf("����� ��� ������������\n");
			else if (code == GAME_WORD_DOESNT_CONTAIN_LETTER) printf("����� ������ ��������� ����������� �����\n");

			break;
		case FIELD_INVALID_COORDINATES:
			printf("������������ ����������!\n");
			scanf(" ");
			break;
		case FIELD_CELL_OCCUPIED:
			printf("������ ������!\n");
			scanf(" ");
			break;
		case FIELD_CELL_NOT_CONNECTED:
			printf("����� ������ ���� ����� � ����������!\n");
			scanf(" ");
			break;
		case FIELD_INVALID_LETTER:
			printf("����� ������ ���� � ��������� ������� ����!\n");
			scanf(" ");
			break;
		}
	}

	return 0;
}