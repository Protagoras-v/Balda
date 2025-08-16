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

	//зерно для генерации случайных чисел
	srand(time(NULL));

	Dictionary* dict = dict_init("dictionary.txt");
	Game* game = game_create(dict);

	bool f = 1;

	while (f) {
		printf("Ход игрока с id: %d\n", game_get_player_id(game));
		print_field(game);
		int x, y, len;
		char c;
		printf("Введите y: ");
		scanf("%d", &y);
		printf("Введите x: ");
		scanf("%d", &x);
		printf("Введите букву: ");
		scanf(" %c", &c);

		StatusCode code = game_try_place_letter(game, y, x, c);
		switch (code) {
		case SUCCESS:
			printf("Введите длину слова: \n");
			scanf("%d", &len);
			int i = 0;
			while (i < len) {
				printf("Введите y: ");
				scanf("%d", &y);
				printf("Введите x: ");
				scanf("%d", &x);

				code = game_add_cell_into_word(game, y, x);
				if (code == FIELD_INVALID_COORDINATES) {
					printf("Неправильные координаты!\n");
				}
				else if (code == FIELD_CELL_EMPTY) {
					printf("Выбранна клетка не содержит буквы!\n");
				}
				else if (code == FIELD_INVALID_CELL) {
					printf("Выбранна клетка не граничит с предыдущей!\n");
				}
				else if (code == SUCCESS) {
					i++;
				}
			}
			/*code = game_confirm_move(game, dict);
			if (code == GAME_INVALID_WORD) {
				printf("Такого слова нет в словаре!\n");
			}
			else if (code == SUCCESS) {
				printf("УСПЕХ!\n");
			}*/
			code = game_clear_move(game);

			break;
		case FIELD_INVALID_COORDINATES:
			printf("Неправильные координаты!\n");
			scanf(" ");
			break;
		case FIELD_CELL_OCCUPIED:
			printf("Клетка занята!\n");
			scanf(" ");
			break;
		case FIELD_CELL_NOT_CONNECTED:
			printf("Буква должна быть рядом с предыдущей!\n");
			scanf(" ");
			break;
		case FIELD_INVALID_LETTER:
			printf("Буква должна быть в диапазоне русских букв!\n");
			scanf(" ");
			break;
		}
	}

	return 0;
}