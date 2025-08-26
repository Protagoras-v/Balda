#pragma once
#include <stdbool.h>

typedef struct WordCell {
	unsigned char y : 4;
	unsigned char x : 4;
	char letter;
} WordCell;


bool is_it_ru_letter(const unsigned char c);

bool is_word_valid(const unsigned char* word);

unsigned char to_lower(unsigned char c);

void reverse_word(char* word);

void WordCell_to_char(WordCell source[], char dest[], int word_len);