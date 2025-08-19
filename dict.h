#pragma once
#include <stdbool.h>
#include "status_codes.h"

//typedef struct TrieNode TrieNode;
typedef struct Dictionary Dictionary;

Dictionary* dict_init();

void dict_destroy(Dictionary* dict);

bool dict_word_exists(Dictionary* dict, const char* word);

StatusCode dict_get_starting_word(Dictionary* dict, char st_word[]);

StatusCode dict_add_word(Dictionary* dict, const char* word);

//int dict_save(Dictionary* dict, const char* filename);

//void dict_print(Dictionary* dict);

