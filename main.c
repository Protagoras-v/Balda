#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <time.h>

#include "dict.h"


int main() {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "Russian");

	//����� ��� ��������� ��������� �����
	srand(time(NULL));

	Dictionary* dict = dict_init("dictionary.txt");

	return 0;
}