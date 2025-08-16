#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "dict.h"
#include "common.h"

typedef struct TrieNode {
	struct TrieNode* children[33];
	bool is_end_of_the_word;
} TrieNode;

struct Dictionary {
	TrieNode* root;
	int total_count;

	//кандидаты на начальное слово
	char** candidates;
	int candidates_count;
	int candidates_capacity;
};


//возвращает позицию русской буквы в алфавите(1-33)
static int char_to_index(unsigned char letter) {
	letter = tolower(letter);
	if (letter >= 224 && letter <= 255) { //а-я
		return letter - 224;
	}
	else if (letter == 184) { //ё
		return 6;
	}
}

static TrieNode* create_node() {
	TrieNode* node = calloc(1, sizeof(TrieNode));
	if (node == NULL) {
		fprintf(stderr, "Ошибка при выделении памяти в функции init_trie()\n");
		return NULL;
	}
	return node;
}

static void insert_word_into_trie(TrieNode* root, const char* word) {
	TrieNode* current = root;

	for (int i = 0; word[i]; i++) {
		int index = char_to_index(word[i]);
		if (current->children[index] == NULL) {
			current->children[index] = create_node();
		}
		current = current->children[index];
	}
	current->is_end_of_the_word = 1;
}

static TrieNode* clear_trie(TrieNode* node) {
	for (int i = 0; i < 33; i++) {
		if (node->children[i] != NULL) {
			node->children[i] = clear_trie(node->children[i]);
		}
	}
	free(node);
	return NULL;
}

static bool is_word_in_trie(TrieNode* root, const char* word) {
	TrieNode* current = root;
	for (int i = 0; word[i]; i++) {
		char letter = word[i];
		int index = char_to_index(letter);
		if (current->children[index] == NULL) {
			return 0;
		}
		current = current->children[index];
	}
	if (current->is_end_of_the_word) {
		return 1;
	}
	return 0;
}


Dictionary* dict_init(const char* filename) {
	FILE* file = fopen(filename, "r+");
	if (file == NULL) {
		fprintf(stderr, "Не удалось открыть файл %s\n", filename);
		return NULL;
	}

	Dictionary* dict = malloc(sizeof(Dictionary));
	if (dict == NULL) {
		fprintf(stderr, "Ошибка выделения памяти для Dictionary\n");
		return NULL;
	}
	dict->total_count = 0;
	dict->candidates_capacity = 100;
	dict->candidates_count = 0;
	dict->root = create_node();
	if (dict->root == NULL) {
		return NULL;
	}
	dict->candidates = malloc(dict->candidates_capacity * sizeof(char*));
	if (dict->candidates == NULL) {
		fprintf(stderr, "Ошибка выделения памяти для dict.candidates\n");
		return NULL;
	}

	char buffer[255] = { 0 };
	//считываем файл построчно
	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		size_t len = strlen(buffer) - 1;
		//удаляем '\n'
		char* n = strchr(buffer, '\n');
		if (n != NULL) *n = '\0';

		if (is_word_valid(buffer)) {
			insert_word_into_trie(dict->root, buffer);
			dict->total_count++;
		}
		else {
			printf("Слово %s было пропущено при инициализации словаря, т.к. содержит недопустимые символы\n", buffer);
			continue;
		}

		//если слово имеет длину 5, делаем ее кандидатом для стартового слова
		if (len == 5) {
			char* word = malloc(len * sizeof(char) + 1);
			if (word == NULL) {
				fprintf(stderr, "Ошибка при выделении памяти в dict_init() для word\n");
				return NULL;
			}
			strncpy(word, buffer, len + 1);
			dict->candidates[dict->candidates_count++] = word;

			//расширяем массив при необходимости
			if (dict->candidates_count == dict->candidates_capacity) {
				char** new_candidates = realloc(dict->candidates, dict->candidates_capacity * 2 * sizeof(char*));
				if (new_candidates == NULL) {
					fprintf(stderr, "Ошибка при перераспределении памяти для new_candidates\n");
					return NULL;
				}
				dict->candidates = new_candidates;
				dict->candidates_capacity *= 2;
			}
		}
	}
	fclose(file);
	return dict;
}


void dict_destroy(Dictionary* dict) {
	clear_trie(dict->root);
	free(dict->candidates);
	free(dict);
}

bool dict_word_exists(Dictionary* dict, const char* word) {
	if (!is_word_valid(word)) {
		printf("Слово %s содержит недопустимые символы!\n", word);
		return 0;
	}
	return is_word_in_trie(dict->root, word);
}

StatusCode dict_add_word(Dictionary* dict, const char* word) {
	if (!is_word_valid(word)) {
		printf("Слово %s содержит недопустимые символы!\n", word);
		return DICT_ERROR_INVALID_WORD;
	}
	insert_word_into_trie(dict->root, word);
	dict->total_count++;

	//a - append
	FILE* file = fopen("dictionary.txt", "a");
	if (file == NULL) {
		fprintf(stderr, "Не удалось открыть файл dictionary.txt\n");
		return ERROR_FILE_NO_FOUND;
	}
	fprintf(file, "%s\n", word);
	fclose(file);

	//если len = 5, добавляем в кандидаты на стартовое слово
	size_t len = strlen(word) - 1;
	if (len == 5) {
		dict->candidates[dict->candidates_count++] = word;

		//расширяем массив при необходимости
		if (dict->candidates_count == dict->candidates_capacity) {
			char** new_candidates = realloc(dict->candidates, dict->candidates_capacity * 2 * sizeof(char*));
			if (new_candidates == NULL) {
				fprintf(stderr, "Ошибка при перераспределении памяти для new_candidates\n");
				return ERROR_OUT_OF_MEMORY;
			}
			dict->candidates = new_candidates;
			dict->candidates_capacity *= 2;
		}
	}

	return SUCCESS;
}


char* dict_get_starting_word(Dictionary* dict) {
	if (dict->candidates_count == 0) {
		fprintf(stderr, "Нет кандидатов для выбора стартового слова!\n");
		return NULL;
	}
	int i = rand() % dict->candidates_count;
	return dict->candidates[i];
}

//Функция принимает указатель, а не структуру, потому что при попытке передать неполную структуру (объявленную в dict.h) компилятор выдаст ошибку (т.е. при вызове dict_print() из других файлов)
//void dict_print(Dictionary *dict) {
//	for (int i = 0; i < dict->st_words_count; i++) {
//		printf("%s\n", dict->starting_words[i]);
//	}
//}



