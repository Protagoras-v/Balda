#include "common.h"

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