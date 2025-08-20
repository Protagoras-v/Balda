#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "dict.h"
#include "common.h"

#define FILE_NAME "dictionary.txt"

typedef struct TrieNode {
	struct TrieNode* children[33];
	bool is_end_of_the_word;
} TrieNode;

struct Dictionary {
	TrieNode* root;
	int count;
};


//возвращает позицию русской буквы в алфавите(1-33)
static int char_to_index(unsigned char letter) {
	letter = to_lower(letter);
	if (letter >= 224 && letter <= 229) { //а-е
		return letter - 224;
	}
	else if (letter >= 230 && letter <= 255) {//ж-я (т.к ё не по порядку в CP1251) 
		return letter - 223;
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
		unsigned char letter = word[i];
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


Dictionary* dict_init() {
	FILE* file = fopen(FILE_NAME, "r+");
	if (file == NULL) {
		fprintf(stderr, "Не удалось открыть файл %s\n", FILE_NAME);
		return NULL;
	}

	Dictionary* dict = malloc(sizeof(Dictionary));
	if (dict == NULL) {
		fprintf(stderr, "Ошибка выделения памяти для Dictionary\n");
		fclose(file);
		return NULL;
	}
	dict->count = 0;
	dict->root = create_node();
	if (dict->root == NULL) {
		free(dict);
		fclose(file);
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
			dict->count++;
		}
		else {
			printf("Слово %s было пропущено при инициализации словаря, т.к. содержит недопустимые символы\n", buffer);
			continue;
		}
	}
	fclose(file);
	return dict;
}


void dict_destroy(Dictionary* dict) {
	if (dict == NULL) return;
	clear_trie(dict->root);
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
	dict->count++;

	//a - append
	FILE* file = fopen("dictionary.txt", "a");
	if (file == NULL) {
		fprintf(stderr, "Не удалось открыть файл dictionary.txt\n");
		return ERROR_FILE_NOT__FOUND;
	}
	fprintf(file, "%s\n", word);
	fclose(file);

	return SUCCESS;
}

//since this operation executes once when the game is started, there is no problem reading file again
StatusCode dict_get_starting_word(Dictionary* dict, char st_word[]) {
	FILE* file = fopen(FILE_NAME, "r");
	if (file == NULL) {
		fprintf(stderr, "Не удалось открыть файл %s для выбора начального слова\n", FILE_NAME);
		return ERROR_FILE_NOT__FOUND;
	}

	int count = 0;
	char** candidates = malloc(sizeof(char*) * dict->count);
	if (candidates == NULL) return ERROR_OUT_OF_MEMORY;

	char buffer[255] = { 0 };
	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		// only 5 letter words
		if ((buffer[5] == '\n' || buffer[5] == '\0') && buffer[4] != '\n' && buffer[4] != '\0') {
			buffer[5] = '\0';
			candidates[count] = malloc(sizeof(char) * 6);
			strncpy(candidates[count++], buffer, 6);
		}	
	}

	strncpy(st_word, candidates[rand() % count], 6);

	for (int i = 0; i < count; i++) {
		free(candidates[i]);
	}
	free(candidates);

	return SUCCESS;
}

//Функция принимает указатель, а не структуру, потому что при попытке передать неполную структуру (объявленную в dict.h) компилятор выдаст ошибку (т.е. при вызове dict_print() из других файлов)
//void dict_print(Dictionary *dict) {
//	for (int i = 0; i < dict->st_words_count; i++) {
//		printf("%s\n", dict->starting_words[i]);
//	}
//}



