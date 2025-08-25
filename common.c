#include <string.h>
#include "common.h"
#include "game_logic.h"

struct WordCell {
	int y : 5;
	int x : 5;
	char letter;
};

bool is_it_ru_letter(const unsigned char c) {
	if ((c >= 192 && c <= 223) || //À-ß
		(c >= 224 && c <= 255) || //à-ÿ
		(c == 168) || //¨
		(c == 184)) { //¸ 
		return 1;
	}
	return 0;
}

bool is_word_valid(const unsigned char* word) {
	char* s = word;
	while (*s != '\0') {
		if (!is_it_ru_letter(*s)) {
			return 0;
		}
		s++;
	}
	return 1;
}

//cp1251
unsigned char to_lower(unsigned char c) {
	if (c >= 192 && c <= 223) {  //À-ß
		return c + 32;
	}
	if (c == 168) { //¨
		return 184; //¸
	}
	return c;
}

void reverse_word(char* word) {
	int len = strlen(word);
	
	for (int i = 0; i < len / 2; i++) {
		char temp = word[len - i];
		word[len - 1 - i] = word[i];
		word[i] = temp;
	}
}

void WordCell_to_char(WordCell source[], char dest[], int word_len) {
	int i = 0;
	while (i < word_len) {
		dest[i] = source[i++].letter;
	}
	dest[i] = '\0';
}