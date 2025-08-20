#pragma once
#include <stdbool.h>

bool is_it_ru_letter(const unsigned char c);

bool is_word_valid(const unsigned char* word);

unsigned char to_lower(unsigned char c);