#pragma once
#include "status_codes.h"
#include "common.h"

//typedef struct TrieNode TrieNode;
typedef struct Dictionary Dictionary;

Dictionary* dict_init(bool alerts);

void dict_destroy(Dictionary* dict);

bool dict_word_exists(Dictionary* dict, const char* word);

StatusCode dict_get_starting_word(Dictionary* dict, char st_word[]);

StatusCode dict_add_word(Dictionary* dict, const char* word);

bool dict_prefix_exists(Dictionary* dict, char* prefix);

bool dict_reverse_prefix_exists(Dictionary* dict, char* prefix);

bool dict_reverse_word_exists(Dictionary* dict, char* word);

//int dict_save(Dictionary* dict, const char* filename);

//void dict_print(Dictionary* dict);

